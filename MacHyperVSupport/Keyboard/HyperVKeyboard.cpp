//
//  HyperVKeyboard.cpp
//  Hyper-V keyboard driver
//
//  Copyright © 2021 Goldfish64. All rights reserved.
//

#include "HyperVKeyboard.hpp"
#include "HyperVADBMap.hpp"

OSDefineMetaClassAndStructors(HyperVKeyboard, super);

bool HyperVKeyboard::start(IOService *provider) {
  bool      result  = false;
  bool      started = false;
  IOReturn  status;
  
  //
  // Get parent VMBus device object.
  //
  hvDevice = OSDynamicCast(HyperVVMBusDevice, provider);
  if (hvDevice == nullptr) {
    HVSYSLOG("Unable to get parent VMBus device nub");
    return false;
  }
  hvDevice->retain();
  HVCheckDebugArgs();
  
  do {
    HVDBGLOG("Initializing Hyper-V Synthetic Keyboard");
    
    if (HVCheckOffArg()) {
      HVSYSLOG("Disabling Hyper-V Synthetic Keyboard due to boot arg");
      break;
    }
    
    started = super::start(provider);
    if (!started) {
      HVSYSLOG("Superclass start function failed");
      break;
    }
    
    //
    // Configure interrupt.
    //
    interruptSource =
      IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &HyperVKeyboard::handleInterrupt), provider, 0);
    if (interruptSource == nullptr) {
      HVSYSLOG("Unable to initialize interrupt");
      break;
    }
    
    status = getWorkLoop()->addEventSource(interruptSource);
    if (status != kIOReturnSuccess) {
      HVSYSLOG("Unable to add interrupt event source: 0x%X", status);
      break;
    }
    interruptSource->enable();
    
    //
    // Configure the channel.
    //
    if (hvDevice->openVMBusChannel(kHyperVKeyboardRingBufferSize, kHyperVKeyboardRingBufferSize) != kIOReturnSuccess) {
      HVSYSLOG("Unable to configure VMBus channel");
      break;
    }
    
    status = connectKeyboard();
    if (status != kIOReturnSuccess) {
      HVSYSLOG("Unable to connect to keyboard device: 0x%X", status);
      break;
    }
    
    result = true;
    HVDBGLOG("Initialized Hyper-V Synthetic Keyboard");
  } while (false);
  
  if (!result) {
    freeStructures();
    if (started) {
      super::stop(provider);
    }
  }
  
  return result;
}

void HyperVKeyboard::stop(IOService *provider) {
  HVDBGLOG("Stopping Hyper-V Synthetic Keyboard");
  
  freeStructures();
  super::stop(provider);
}

OSReturn HyperVKeyboard::connectKeyboard() {
  HVDBGLOG("Connecting to keyboard interface");
  
  HyperVKeyboardMessageProtocolRequest requestMsg;
  requestMsg.header.type = kHyperVKeyboardMessageTypeProtocolRequest;
  requestMsg.versionRequested = kHyperVKeyboardVersion;
  
  return hvDevice->writeInbandPacket(&requestMsg, sizeof (requestMsg), true);
}

UInt32 HyperVKeyboard::deviceType() {
  return 3;
}

UInt32 HyperVKeyboard::interfaceID() {
  return NX_EVS_DEVICE_INTERFACE_ADB;
}

inline UInt32 getKeyCode(HyperVKeyboardMessageKeystroke *keyEvent) {
  UInt8 keyCode = PS2ToADBMapStock[keyEvent->makeCode + (keyEvent->isE0 ? kADBConverterExStart : 0)];
  
  return keyCode;
}

void HyperVKeyboard::freeStructures() {
  //
  // Release interrupt.
  //
  if (interruptSource != nullptr) {
    interruptSource->disable();
    getWorkLoop()->removeEventSource(interruptSource);
    interruptSource->release();
  }
  
  //
  // Close channel and release parent VMBus device object.
  //
  if (hvDevice != nullptr) {
    hvDevice->closeVMBusChannel();
    hvDevice->release();
  }
}

void HyperVKeyboard::handleInterrupt(OSObject *owner, IOInterruptEventSource *sender, int count) {
  UInt8 data128[128];

  do {
    //
    // Check for available inband packets.
    // Large packets will be allocated as needed.
    //
    HyperVKeyboardMessage *message;
    UInt32 pktDataLength;
    if (!hvDevice->nextInbandPacketAvailable(&pktDataLength)) {
      break;
    }

    if (pktDataLength <= sizeof (data128)) {
      message = (HyperVKeyboardMessage*)data128;
    } else {
      HVDBGLOG("Allocating large packet of %u bytes", pktDataLength);
      message = (HyperVKeyboardMessage*)IOMalloc(pktDataLength);
    }

    //
    // Read next packet.
    //
    if (hvDevice->readInbandCompletionPacket((void *)message, pktDataLength, nullptr) == kIOReturnSuccess) {
      switch (message->header.type) {
        case kHyperVKeyboardMessageTypeProtocolResponse:
          HVDBGLOG("Keyboard protocol status %u %u", message->protocolResponse.header.type, message->protocolResponse.status);
          break;

        case kHyperVKeyboardMessageTypeEvent:
          UInt64 time;
          clock_get_uptime(&time);
          
          HVDBGLOG("Got make code 0x%X (E0: %u, break: %u)", message->keystroke.makeCode, message->keystroke.isE0, message->keystroke.isBreak);
          dispatchKeyboardEvent(getKeyCode(&message->keystroke), !message->keystroke.isBreak, *(AbsoluteTime*)&time);
          break;

        default:
          HVDBGLOG("Unknown message type %u, size %u", message->header.type, pktDataLength);
          break;
      }
    }

    //
    // Free allocated packet if needed.
    //
    if (pktDataLength > sizeof (data128)) {
      IOFree(message, pktDataLength);
    }
  } while (true);
}

const unsigned char* HyperVKeyboard::defaultKeymapOfLength(UInt32 * length)
{
  //
  // Taken from 10.4.9's IOHIDFamily/IOHIDKeyboard.cpp
  //
  static const unsigned char appleUSAKeyMap[] = {
    //
    // Use byte units.
    //
    0x00, 0x00,
          
    //
    // Number of modifier definitions.
    //
    0x0A,
    
    //
    // Modifier definitions.
    // (modifier ID, number of keycodes modifier is tied to, keycodes modifier is tied to)
    //
    NX_MODIFIERKEY_SHIFT,       0x01, 0x38,
    NX_MODIFIERKEY_CONTROL,     0x01, 0x3B,
    NX_MODIFIERKEY_ALTERNATE,   0x01, 0x3A,
    NX_MODIFIERKEY_COMMAND,     0x01, 0x37,
    NX_MODIFIERKEY_NUMERICPAD,  0x15, 0x52, 0x41, 0x4C, 0x53, 0x54, 0x55, 0x45, 0x58, 0x57, 0x56, 0x5B, 0x5C, 0x43, 0x4B, 0x51, 0x7B, 0x7D, 0x7E, 0x7C, 0x4E, 0x59,
    NX_MODIFIERKEY_HELP,        0x01, 0x72,
    NX_MODIFIERKEY_RSHIFT,      0x01, 0x3C,
    NX_MODIFIERKEY_RCONTROL,    0x01, 0x3E,
    NX_MODIFIERKEY_RALTERNATE,  0x01, 0x3D,
    NX_MODIFIERKEY_RCOMMAND,    0x01, 0x36,
    
    //
    // Number of keycode defintions.
    //
    0x7F,
    
    //
    // Keycode definitions.
    // (modifier mask + number of character pairs, character pairs to generate (character set, character code))
    //
    // The modifier mask indicates what modifiers will affect this key and generate the character pairs that follow.
    // 0xFF = key does not generate characters (typically modifier keys)
    //
    0x0D, 0x00, 0x61, 0x00, 0x41, 0x00, 0x01, 0x00, 0x01, 0x00, 0xCA, 0x00, 0xC7, 0x00, 0x01, 0x00, 0x01, // 00 A
    0x0D, 0x00, 0x73, 0x00, 0x53, 0x00, 0x13, 0x00, 0x13, 0x00, 0xFB, 0x00, 0xA7, 0x00, 0x13, 0x00, 0x13, // 01 S
    0x0D, 0x00, 0x64, 0x00, 0x44, 0x00, 0x04, 0x00, 0x04, 0x01, 0x44, 0x01, 0xB6, 0x00, 0x04, 0x00, 0x04, // 02 D
    0x0D, 0x00, 0x66, 0x00, 0x46, 0x00, 0x06, 0x00, 0x06, 0x00, 0xA6, 0x01, 0xAC, 0x00, 0x06, 0x00, 0x06, // 03 F
    0x0D, 0x00, 0x68, 0x00, 0x48, 0x00, 0x08, 0x00, 0x08, 0x00, 0xE3, 0x00, 0xEB, 0x00, 0x00, 0x18, 0x00, // 04 H
    0x0D, 0x00, 0x67, 0x00, 0x47, 0x00, 0x07, 0x00, 0x07, 0x00, 0xF1, 0x00, 0xE1, 0x00, 0x07, 0x00, 0x07, // 05 G
    0x0D, 0x00, 0x7A, 0x00, 0x5A, 0x00, 0x1A, 0x00, 0x1A, 0x00, 0xCF, 0x01, 0x57, 0x00, 0x1A, 0x00, 0x1A, // 06 Z
    0x0D, 0x00, 0x78, 0x00, 0x58, 0x00, 0x18, 0x00, 0x18, 0x01, 0xB4, 0x01, 0xCE, 0x00, 0x18, 0x00, 0x18, // 07 X
    0x0D, 0x00, 0x63, 0x00, 0x43, 0x00, 0x03, 0x00, 0x03, 0x01, 0xE3, 0x01, 0xD3, 0x00, 0x03, 0x00, 0x03, // 08 C
    0x0D, 0x00, 0x76, 0x00, 0x56, 0x00, 0x16, 0x00, 0x16, 0x01, 0xD6, 0x01, 0xE0, 0x00, 0x16, 0x00, 0x16, // 09 V
    0x02, 0x00, 0x3C, 0x00, 0x3E,                                                                         // 0A NON-US-BACKSLASH ON ANSI AND JIS KEYBOARDS GRAVE ON ISO
    0x0D, 0x00, 0x62, 0x00, 0x42, 0x00, 0x02, 0x00, 0x02, 0x01, 0xE5, 0x01, 0xF2, 0x00, 0x02, 0x00, 0x02, // 0B B
    0x0D, 0x00, 0x71, 0x00, 0x51, 0x00, 0x11, 0x00, 0x11, 0x00, 0xFA, 0x00, 0xEA, 0x00, 0x11, 0x00, 0x11, // 0C Q
    0x0D, 0x00, 0x77, 0x00, 0x57, 0x00, 0x17, 0x00, 0x17, 0x01, 0xC8, 0x01, 0xC7, 0x00, 0x17, 0x00, 0x17, // 0D W
    0x0D, 0x00, 0x65, 0x00, 0x45, 0x00, 0x05, 0x00, 0x05, 0x00, 0xC2, 0x00, 0xC5, 0x00, 0x05, 0x00, 0x05, // 0E E
    0x0D, 0x00, 0x72, 0x00, 0x52, 0x00, 0x12, 0x00, 0x12, 0x01, 0xE2, 0x01, 0xD2, 0x00, 0x12, 0x00, 0x12, // 0F R
    0x0D, 0x00, 0x79, 0x00, 0x59, 0x00, 0x19, 0x00, 0x19, 0x00, 0xA5, 0x01, 0xDB, 0x00, 0x19, 0x00, 0x19, // 10 Y
    0x0D, 0x00, 0x74, 0x00, 0x54, 0x00, 0x14, 0x00, 0x14, 0x01, 0xE4, 0x01, 0xD4, 0x00, 0x14, 0x00, 0x14, // 11 T
    0x0A, 0x00, 0x31, 0x00, 0x21, 0x01, 0xAD, 0x00, 0xA1,                                                 // 12 1
    0x0E, 0x00, 0x32, 0x00, 0x40, 0x00, 0x32, 0x00, 0x00, 0x00, 0xB2, 0x00, 0xB3, 0x00, 0x00, 0x00, 0x00, // 13 2
    0x0A, 0x00, 0x33, 0x00, 0x23, 0x00, 0xA3, 0x01, 0xBA,                                                 // 14 3
    0x0A, 0x00, 0x34, 0x00, 0x24, 0x00, 0xA2, 0x00, 0xA8,                                                 // 15 4
    0x0E, 0x00, 0x36, 0x00, 0x5E, 0x00, 0x36, 0x00, 0x1E, 0x00, 0xB6, 0x00, 0xC3, 0x00, 0x1E, 0x00, 0x1E, // 16 6
    0x0A, 0x00, 0x35, 0x00, 0x25, 0x01, 0xA5, 0x00, 0xBD,                                                 // 17 5
    0x0A, 0x00, 0x3D, 0x00, 0x2B, 0x01, 0xB9, 0x01, 0xB1,                                                 // 18 EQUALS
    0x0A, 0x00, 0x39, 0x00, 0x28, 0x00, 0xAC, 0x00, 0xAB,                                                 // 19 9
    0x0A, 0x00, 0x37, 0x00, 0x26, 0x01, 0xB0, 0x01, 0xAB,                                                 // 1A 7
    0x0E, 0x00, 0x2D, 0x00, 0x5F, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0xB1, 0x00, 0xD0, 0x00, 0x1F, 0x00, 0x1F, // 1B MINUS
    0x0A, 0x00, 0x38, 0x00, 0x2A, 0x00, 0xB7, 0x00, 0xB4,                                                 // 1C 8
    0x0A, 0x00, 0x30, 0x00, 0x29, 0x00, 0xAD, 0x00, 0xBB,                                                 // 1D 0
    0x0E, 0x00, 0x5D, 0x00, 0x7D, 0x00, 0x1D, 0x00, 0x1D, 0x00, 0x27, 0x00, 0xBA, 0x00, 0x1D, 0x00, 0x1D, // 1E RIGHT BRACKET
    0x0D, 0x00, 0x6F, 0x00, 0x4F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0xF9, 0x00, 0xE9, 0x00, 0x0F, 0x00, 0x0F, // 1F O
    0x0D, 0x00, 0x75, 0x00, 0x55, 0x00, 0x15, 0x00, 0x15, 0x00, 0xC8, 0x00, 0xCD, 0x00, 0x15, 0x00, 0x15, // 20 U
    0x0E, 0x00, 0x5B, 0x00, 0x7B, 0x00, 0x1B, 0x00, 0x1B, 0x00, 0x60, 0x00, 0xAA, 0x00, 0x1B, 0x00, 0x1B, // 21 LEFT BRACKET
    0x0D, 0x00, 0x69, 0x00, 0x49, 0x00, 0x09, 0x00, 0x09, 0x00, 0xC1, 0x00, 0xF5, 0x00, 0x09, 0x00, 0x09, // 22 I
    0x0D, 0x00, 0x70, 0x00, 0x50, 0x00, 0x10, 0x00, 0x10, 0x01, 0x70, 0x01, 0x50, 0x00, 0x10, 0x00, 0x10, // 23 P
    0x10, 0x00, 0x0D, 0x00, 0x03,                                                                         // 24 ENTER/RETURN
    0x0D, 0x00, 0x6C, 0x00, 0x4C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0xF8, 0x00, 0xE8, 0x00, 0x0C, 0x00, 0x0C, // 25 L
    0x0D, 0x00, 0x6A, 0x00, 0x4A, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0xC6, 0x00, 0xAE, 0x00, 0x0A, 0x00, 0x0A, // 26 J
    0x0A, 0x00, 0x27, 0x00, 0x22, 0x00, 0xA9, 0x01, 0xAE,                                                 // 27 APOSTROPHE
    0x0D, 0x00, 0x6B, 0x00, 0x4B, 0x00, 0x0B, 0x00, 0x0B, 0x00, 0xCE, 0x00, 0xAF, 0x00, 0x0B, 0x00, 0x0B, // 28 K
    0x0A, 0x00, 0x3B, 0x00, 0x3A, 0x01, 0xB2, 0x01, 0xA2,                                                 // 29 SEMICOLON
    0x0E, 0x00, 0x5C, 0x00, 0x7C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0xE3, 0x00, 0xEB, 0x00, 0x1C, 0x00, 0x1C, // 2A BACKSLASH
    0x0A, 0x00, 0x2C, 0x00, 0x3C, 0x00, 0xCB, 0x01, 0xA3,                                                 // 2B COMMA
    0x0A, 0x00, 0x2F, 0x00, 0x3F, 0x01, 0xB8, 0x00, 0xBF,                                                 // 2C SLASH
    0x0D, 0x00, 0x6E, 0x00, 0x4E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0xC4, 0x01, 0xAF, 0x00, 0x0E, 0x00, 0x0E, // 2D N
    0x0D, 0x00, 0x6D, 0x00, 0x4D, 0x00, 0x0D, 0x00, 0x0D, 0x01, 0x6D, 0x01, 0xD8, 0x00, 0x0D, 0x00, 0x0D, // 2E M
    0x0A, 0x00, 0x2E, 0x00, 0x3E, 0x00, 0xBC, 0x01, 0xB3,                                                 // 2F PERIOD
    0x02, 0x00, 0x09, 0x00, 0x19,                                                                         // 30 TAB
    0x0C, 0x00, 0x20, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,                                                 // 31 SPACE
    0x0A, 0x00, 0x60, 0x00, 0x7E, 0x00, 0x60, 0x01, 0xBB,                                                 // 32 GRAVE ON ANSI AND JIS KEYBOARDS, NON-US-BACKSLASH ON ISO
    0x02, 0x00, 0x7F, 0x00, 0x08,                                                                         // 33 BACKSPACE
    0xFF,                                                                                                 // 34 PLAY/PAUSE
    0x02, 0x00, 0x1B, 0x00, 0x7E,                                                                         // 35 ESCAPE
    0xFF,                                                                                                 // 36 RIGHT COMMAND
    0xFF,                                                                                                 // 37 LEFT COMMAND
    0xFF,                                                                                                 // 38 LEFT SHIFT
    0xFF,                                                                                                 // 39 CAPS LOCK
    0xFF,                                                                                                 // 3A LEFT ALT
    0xFF,                                                                                                 // 3B LEFT CONTROL
    0xFF,                                                                                                 // 3C RIGHT SHIFT
    0xFF,                                                                                                 // 3D RIGHT ALT
    0xFF,                                                                                                 // 3E RIGHT CONTROL
    0xFF,                                                                                                 // 3F APPLE FUNCTION
    0xFF,                                                                                                 // 40 F17
    0x00, 0x00, 0x2E,                                                                                     // 41 KEYPAD PERIOD
    0xFF,                                                                                                 // 42 NEXT TRACK
    0x00, 0x00, 0x2A,                                                                                     // 43 KEYPAD MULTIPLY
    0xFF,                                                                                                 // 44 UNUSED
    0x00, 0x00, 0x2B,                                                                                     // 45 KEYPAD PLUS
    0xFF,                                                                                                 // 46 UNUSED
    0x00, 0x00, 0x1B,                                                                                     // 47 CLEAR
    0xFF,                                                                                                 // 48 VOLUME UP
    0xFF,                                                                                                 // 49 VOLUME DOWN
    0xFF,                                                                                                 // 4A MUTE
    0x0E, 0x00, 0x2F, 0x00, 0x5C, 0x00, 0x2F, 0x00, 0x1C, 0x00, 0x2F, 0x00, 0x5C, 0x00, 0x00, 0x0A, 0x00, // 4B KEYPAD DIVIDE
    0x00, 0x00, 0x0D,                                                                                     // 4C APPLE FUNCTION + RETURN = ENTER
    0xFF,                                                                                                 // 4D PREVIOUS TRACK
    0x00, 0x00, 0x2D,                                                                                     // 4E KEYPAD MINUS
    0xFF,                                                                                                 // 4F F18
    0xFF,                                                                                                 // 50 F19
    0x0E, 0x00, 0x3D, 0x00, 0x7C, 0x00, 0x3D, 0x00, 0x1C, 0x00, 0x3D, 0x00, 0x7C, 0x00, 0x00, 0x18, 0x46, // 51 KEYPAD EQUALS
    0x00, 0x00, 0x30,                                                                                     // 52 KEYPAD 0
    0x00, 0x00, 0x31,                                                                                     // 53 KEYPAD 1
    0x00, 0x00, 0x32,                                                                                     // 54 KEYPAD 2
    0x00, 0x00, 0x33,                                                                                     // 55 KEYPAD 3
    0x00, 0x00, 0x34,                                                                                     // 56 KEYPAD 4
    0x00, 0x00, 0x35,                                                                                     // 57 KEYPAD 5
    0x00, 0x00, 0x36,                                                                                     // 58 KEYPAD 6
    0x00, 0x00, 0x37,                                                                                     // 59 KEYPAD 7
    0xFF,                                                                                                 // 5A F20
    0x00, 0x00, 0x38,                                                                                     // 5B KEYPAD 8
    0x00, 0x00, 0x39,                                                                                     // 5C KEYPAD 9
    0xFF,                                                                                                 // 5D
    0xFF,                                                                                                 // 5E
    0xFF,                                                                                                 // 5F
    0x00, 0xFE, 0x24,                                                                                     // 60 F5
    0x00, 0xFE, 0x25,                                                                                     // 61 F6
    0x00, 0xFE, 0x26,                                                                                     // 62 F7
    0x00, 0xFE, 0x22,                                                                                     // 63 F3
    0x00, 0xFE, 0x27,                                                                                     // 64 F8
    0x00, 0xFE, 0x28,                                                                                     // 65 F9
    0xFF,                                                                                                 // 66
    0x00, 0xFE, 0x2A,                                                                                     // 67 F11
    0xFF,                                                                                                 // 68
    0x00, 0xFE, 0x32,                                                                                     // 69 F13
    0x00, 0xFE, 0x35,                                                                                     // 6A F16
    0x00, 0xFE, 0x33,                                                                                     // 6B F14
    0xFF,                                                                                                 // 6C UNUSED
    0x00, 0xFE, 0x29,                                                                                     // 6D F10
    0xFF,                                                                                                 // 6E UNUSED
    0x00, 0xFE, 0x2B,                                                                                     // 6F F12
    0xFF,                                                                                                 // 70 UNUSED
    0x00, 0xFE, 0x34,                                                                                     // 71 F15
    0xFF,                                                                                                 // 72 HELP
    0x00, 0xFE, 0x2E,                                                                                     // 73 HOME
    0x00, 0xFE, 0x30,                                                                                     // 74 PAGE UP
    0x00, 0xFE, 0x2D,                                                                                     // 75 DELETE
    0x00, 0xFE, 0x23,                                                                                     // 76 F4
    0x00, 0xFE, 0x2F,                                                                                     // 77 END
    0x00, 0xFE, 0x21,                                                                                     // 78 F2
    0x00, 0xFE, 0x31,                                                                                     // 79 PAGE DOWN
    0x00, 0xFE, 0x20,                                                                                     // 7A F1
    0x00, 0x01, 0xAC,                                                                                     // 7B LEFT ARROW
    0x00, 0x01, 0xAE,                                                                                     // 7C RIGHT ARROW
    0x00, 0x01, 0xAF,                                                                                     // 7D DOWN ARROW
    0x00, 0x01, 0xAD,                                                                                     // 7E UP ARROW
    
    //
    // Number of key sequence definitions.
    //
    0x0F,
    
    //
    // Key sequence definitions.
    //
    0x02, 0xFF, 0x04, 0x00, 0x31, // Command + '1'
    0x02, 0xFF, 0x04, 0x00, 0x32, // Command + '2'
    0x02, 0xFF, 0x04, 0x00, 0x33, // Command + '3'
    0x02, 0xFF, 0x04, 0x00, 0x34, // Command + '4'
    0x02, 0xFF, 0x04, 0x00, 0x35, // Command + '5'
    0x02, 0xFF, 0x04, 0x00, 0x36, // Command + '6'
    0x02, 0xFF, 0x04, 0x00, 0x37, // Command + '7'
    0x02, 0xFF, 0x04, 0x00, 0x38, // Command + '8'
    0x02, 0xFF, 0x04, 0x00, 0x39, // Command + '9'
    0x02, 0xFF, 0x04, 0x00, 0x30, // Command + '0'
    0x02, 0xFF, 0x04, 0x00, 0x2D, // Command + '-'
    0x02, 0xFF, 0x04, 0x00, 0x3D, // Command + '='
    0x02, 0xFF, 0x04, 0x00, 0x70, // Command + 'p'
    0x02, 0xFF, 0x04, 0x00, 0x5D, // Command + ']'
    0x02, 0xFF, 0x04, 0x00, 0x5B, // Command + '['
    
    //
    // Number of special key definitions.
    //
    0x07,
    
    //
    // Special key definitions.
    //
    NX_KEYTYPE_CAPS_LOCK,   0x39,
    NX_KEYTYPE_HELP,        0x72,
    NX_POWER_KEY,           0x7F,
    NX_KEYTYPE_MUTE,        0x4A,
    NX_KEYTYPE_SOUND_UP,    0x48,
    NX_KEYTYPE_SOUND_DOWN,  0x49,
    NX_KEYTYPE_NUM_LOCK,    0x47,
  };
  
  *length = sizeof(appleUSAKeyMap);
  return appleUSAKeyMap;
}

UInt32 HyperVKeyboard::maxKeyCodes() {
  //
  // Max number of keycodes for 10.4 is 128.
  //
  return 0x80;
}
