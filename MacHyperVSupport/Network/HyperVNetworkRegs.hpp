//
//  HyperVNetworkRegs.cpp
//  Hyper-V network driver
//
//  Copyright © 2021 Goldfish64. All rights reserved.
//

#ifndef HyperVNetworkRegs_h
#define HyperVNetworkRegs_h

#define kHyperVNetworkRingBufferSize (16 * PAGE_SIZE)

#define kHyperVNetworkNDISVersion60001    0x00060001
#define kHyperVNetworkNDISVersion6001E    0x0006001E

#define kHyperVNetworkReceiveBufferSize         (1024 * 1024 * 16)
#define kHyperVNetworkReceiveBufferSizeLegacy   (1024 * 1024 * 15)
#define kHyperVNetworkSendBufferSize            (1024 * 1024 * 15)

//
// Protocol versions.
//
typedef enum : UInt32 {
  kHyperVNetworkProtocolVersion1  = 0x00002,
  kHyperVNetworkProtocolVersion2  = 0x30002,
  kHyperVNetworkProtocolVersion4  = 0x40000,
  kHyperVNetworkProtocolVersion5  = 0x50000,
  kHyperVNetworkProtocolVersion6  = 0x60000,
  kHyperVNetworkProtocolVersion61 = 0x60001
} HyperVNetworkProtocolVersion;

//
// Network message types.
//
typedef enum : UInt32 {
  kHyperVNetworkMessageTypeNone           = 0,
  
  // Initialization.
  kHyperVNetworkMessageTypeInit           = 1,
  kHyperVNetworkMessageTypeInitComplete   = 2,
  
  // Protocol version 1.
  kHyperVNetworkMessageTypeV1SendNDISVersion              = 100,
  kHyperVNetworkMessageTypeV1SendReceiveBuffer,
  kHyperVNetworkMessageTypeV1SendReceiveBufferComplete,
  kHyperVNetworkMessageTypeV1RevokeReceiveBuffer,
  kHyperVNetworkMessageTypeV1SendSendBuffer,
  kHyperVNetworkMessageTypeV1SendSendBufferComplete,
  kHyperVNetworkMessageTypeV1RevokeSendBuffer,
  kHyperVNetworkMessageTypeV1SendRNDISPacket,
  kHyperVNetworkMessageTypeV1SendRNDISPacketComplete
} HyperVNetworkMessageType;

//
// Network status codes.
//
typedef enum : UInt32 {
  kHyperVNetworkMessageStatusNone = 0,
  kHyperVNetworkMessageStatusSuccess,
  kHyperVNetworkMessageStatusFailure,
  kHyperVNetworkMessageStatusProtocolTooNew,
  kHyperVNetworkMessageStatusProtocolTooOld,
  kHyperVNetworkMessageStatusInvalidRNDISPacket,
  kHyperVNetworkMessageStatusBusy,
  kHyperVNetworkMessageStatusProtocolUnsupported,
  kHyperVNetworkMessageStatusMaximum
} HyperVNetworkMessageStatus;

//
// Initialization (all versions)
//

//
// Initialization version message.
// This message is used during intial setup right after opening the VMBus channel.
//
typedef struct __attribute__((packed)) {
  HyperVNetworkProtocolVersion  minProtocolVersion;
  HyperVNetworkProtocolVersion  maxProtocolVersion;
} HyperVNetworkMessageInitVersion;

//
// Initialization complete message.
// This is the response message from Hyper-V used during intial setup right
// after opening the VMBus channel.
//
typedef struct __attribute__((packed)) {
  HyperVNetworkProtocolVersion  negotiatedProtocolVersion;
  UInt32                        maxMdlChainLength;
  HyperVNetworkMessageStatus    status;
} HyperVNetworkMessageInitComplete;

//
// Initialization message.
//
typedef union __attribute__((packed)) {
  HyperVNetworkMessageInitVersion   initVersion;
  HyperVNetworkMessageInitComplete  initComplete;
} HyperVNetworkMessageInit;

//
// Protocol version 1
//

//
// Send NDIS version to Hyper-V.
//
typedef struct __attribute__((packed)) {
  UInt32 major;
  UInt32 minor;
} HyperVNetworkV1MessageSendNDISVersion;

//
// Send receive buffer to Hyper-V using an allocated GPADL.
//
typedef struct __attribute__((packed)) {
  UInt32 gpadlHandle;
  UInt16 id;
} HyperVNetworkV1MessageSendReceiveBuffer;

typedef struct __attribute__((packed)) {
  UInt32 offset;
  UInt32 subAllocSize;
  UInt32 numSubAllocs;
  UInt32 endOffset;
} HyperVNetworkV1MessageReceiveBufferSection;

//
// Completion response message from Hyper-V after sending a receive buffer.
// This message will be sent before Hyper-V begins using the receive buffer.
//
typedef struct __attribute__((packed)) {
  HyperVNetworkMessageStatus  status;
  UInt32                      numSections;

  //
  // A receive buffer is split into two portions, a large section and a small section.
  // These are then suballocated by a certain size.
  //
  // For example, the following break up of the receive buffer has 6
  // large suballocations and 10 small suballocations.
  //
  //
  //
  // |            Large Section          |  |   Small Section   |
  // ------------------------------------------------------------
  // |     |     |     |     |     |     |  | | | | | | | | | | |
  // |                                      |
  // LargeOffset                            SmallOffset
  HyperVNetworkV1MessageReceiveBufferSection sections[1];
} HyperVNetworkV1MessageSendReceiveBufferComplete;

//
// Message sent to Hyper-V revoking the receive buffer.
//
typedef struct {
  UInt16 id;
} HyperVNetworkV1MessageRevokeReceiveBuffer;

//
// Send send buffer to Hyper-V using an allocated GPADL.
//
typedef struct __attribute__((packed)) {
  UInt32 gpadlHandle;
  UInt16 id;
} HyperVNetworkV1MessageSendSendBuffer;

//
// Completion response message from Hyper-V after sending a send buffer.
// This message will be sent before Hyper-V begins using the send buffer.
//
typedef struct __attribute__((packed)) {
  HyperVNetworkMessageStatus  status;
  
  // VM can select the size of the send buffer.
  UInt32                      sectionSize;
} HyperVNetworkV1MessageSendSendBufferComplete;

//
// Message sent to Hyper-V revoking the send buffer.
//
typedef struct {
  UInt16 id;
} HyperVNetworkV1MessageRevokeSendBuffer;

typedef enum : UInt32 {
  kHyperVNetworkRNDISChannelTypeData      = 0,
  kHyperVNetworkRNDISChannelTypeControl   = 1
} HyperVNetworkRNDISChannelType;

//
// Message used to send an RNDIS packet to the other end of the channel.
// This message is used by both Hyper-V and the VM.
//
typedef struct __attribute__((packed)) {
  //
  // Specified by RNDIS. RNDIS uses 0 for DATA, and 1 for CONTROL.
  // Ethernet packets are typically data, all other non-Ethernet packets are control.
  //
  HyperVNetworkRNDISChannelType channelType;

  //
  // Used to specify what data is to be sent from the send buffer.
  // 0xFFFFFFFF means the send buffer is not being used and data was sent via another method.
  //
  UInt32 sendBufferSectionIndex;
  UInt32 sendBufferSectionSize;
} HyperVNetworkV1MessageSendRNDISPacket;

//
// Response message used after an RNDIS packet is received at the other end of the channel.
// This message is used by both Hyper-V and the VM.
//
typedef struct __attribute__((packed)) {
  HyperVNetworkMessageStatus  status;
} HyperVNetworkV1MessageSendRNDISPacketComplete;

//
// Protocol version 1 messages.
//
typedef union __attribute__((packed)) {
  HyperVNetworkV1MessageSendNDISVersion             sendNDISVersion;
  HyperVNetworkV1MessageSendReceiveBuffer           sendReceiveBuffer;
  HyperVNetworkV1MessageSendReceiveBufferComplete   sendReceiveBufferComplete;
  HyperVNetworkV1MessageRevokeReceiveBuffer         revokeReceiveBuffer;
  HyperVNetworkV1MessageSendSendBuffer              sendSendBuffer;
  HyperVNetworkV1MessageSendSendBufferComplete      sendSendBufferComplete;
  HyperVNetworkV1MessageSendRNDISPacket             sendRNDISPacket;
  HyperVNetworkV1MessageSendRNDISPacketComplete     sendRNDISPacketComplete;
} HyperVNetworkV1Message;

//
// Main message structure.
//
typedef struct __attribute__((packed)) {
  HyperVNetworkMessageType messageType;
  union {
    HyperVNetworkMessageInit    init;
    HyperVNetworkV1Message      v1;
  } __attribute__((packed));
  UInt8 padd[sizeof (HyperVNetworkMessageInit)]; // TODO: required for now for some reason, otherwise Hyper-V rejects message
} HyperVNetworkMessage;

//
// RNDIS messages.
//

#define kHyperVNetworkReceiveBufferID           0xCAFE
#define kHyperVNetworkSendBufferID              0x0000

#define kHyperVNetworkRNDISVersionMajor         0x0001
#define kHyperVNetworkRNDISVersionMinor         0x0000

#define kHyperVNetworkRNDISMessageTypeCompletion  0x80000000

typedef enum : UInt32 {
  kHyperVNetworkRNDISMessageTypePacket          = 0x1,
  kHyperVNetworkRNDISMessageTypeInit            = 0x2,
  kHyperVNetworkRNDISMessageTypeInitComplete    = (kHyperVNetworkRNDISMessageTypeInit | kHyperVNetworkRNDISMessageTypeCompletion),
  
  kHyperVNetworkRNDISMessageTypeSet             = 0x5,
  kHyperVNetworkRNDISMessageTypeSetComplete     = (kHyperVNetworkRNDISMessageTypeSet | kHyperVNetworkRNDISMessageTypeCompletion)
  
} HyperVNetworkRNDISMessageType;

//
// Initialization message.
//
typedef struct {
  UInt32 requestId;
  UInt32 majorVersion;
  UInt32 minorVersion;
  UInt32 maxTransferSize;
} HyperVNetworkRNDISMessageInitializeRequest;

//
// Initialization complete message.
//
typedef struct {
  UInt32 requestId;
  UInt32 status;
  UInt32 majorVersion;
  UInt32 minorVersion;
  UInt32 devFlags;
  UInt32 medium;
  UInt32 maxPacketsPerMessage;
  UInt32 maxTransferSize;
  UInt32 packetAlignmentFactor;
  UInt32 afListOffset;
  UInt32 afListSize;
} HyperVNetworkRNDISMessageInitializeComplete;

//
// Set request message.
//
typedef struct {
  UInt32 requestId;
  UInt32 oid;
  UInt32 infoBufferLength;
  UInt32 infoBufferOffset;
  UInt32 deviceVcHandle;
} HyperVNetworkRNDISMessageSetRequest;

//
// Set complete message.
//
typedef struct {
  UInt32 requestId;
  UInt32 status;
} HyperVNetworkRNDISMessageSetComplete;

//
// Main message structure.
//
typedef struct {
  HyperVNetworkRNDISMessageType msgType;
  UInt32                        msgLength;
  
  union {
    UInt32                                        requestId;
    HyperVNetworkRNDISMessageInitializeRequest    initRequest;
    HyperVNetworkRNDISMessageInitializeComplete   initComplete;
    HyperVNetworkRNDISMessageSetRequest           setRequest;
    HyperVNetworkRNDISMessageSetComplete          setComplete;
  };
} HyperVNetworkRNDISMessage;

#endif /* HyperVNetworkRegs_h */
