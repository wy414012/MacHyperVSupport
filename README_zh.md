MacHyperVSupport
================

[![Build Status](https://github.com/acidanthera/MacHyperVSupport/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/MacHyperVSupport/actions) [![Scan Status](https://scan.coverity.com/projects/23212/badge.svg?flat=1)](https://scan.coverity.com/projects/23212)

MacHyperVSupport是适用于macOS的Hyper-V集成服务。需要在Windows Server 2012 R2 / Windows 8.1或更高版本的第二代虚拟机上运行。目前不支持Windows Server 2016。

所有Intel macOS版本均受支持。

### 支持的Hyper-V设备和服务
- 心跳
- 客户机关机（使用守护程序）
- 时间同步（使用守护程序）
- 主机到客户机文件复制（使用守护程序）
- PCI直通（部分支持）
- 综合图形（部分支持）
- 综合键盘
- 综合鼠标
- 综合网络控制器
- 综合SCSI控制器

### 二进制文件
- MacHyperVSupport.kext：macOS 10.4到11.0的核心Hyper-V支持kext。
- MacHyperVSupportMonterey.kext：macOS 12.0及更高版本的核心Hyper-V支持kext。
- hvfilecopyd：文件复制用户空间守护程序。
- hvshutdownd：关机用户空间守护程序。
- hvtimesyncd：时间同步用户空间守护程序。

### OpenCore配置
#### ACPI
- [SSDT-HV-VMBUS](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/AcpiSamples/Source/SSDT-HV-VMBUS.dsl)：启用正确的启动磁盘操作，请确保也配置了其中描述的补丁。
- [SSDT-HV-DEV](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/AcpiSamples/Source/SSDT-HV-DEV.dsl)：在Windows Server 2019 / Windows 10及更高版本上需要，在macOS下提供正确的处理器对象并禁用不兼容的虚拟设备。
- [SSDT-HV-DEV-WS2022](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/AcpiSamples/Source/SSDT-HV-DEV-WS2022.dsl)：在Windows Server 2022 / Windows 11及更高版本上需要，在macOS下禁用额外的不兼容的虚拟设备。
- [SSDT-HV-PLUG](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/AcpiSamples/Source/SSDT-HV-PLUG.dsl)：确保在Big Sur及更高版本中加载VMPlatformPlugin，避免使用默认PlatformPlugin时出现冻结。
* 确保所有上述SSDT中描述的补丁都在`ACPI->Patch`中。

#### 引导程序特殊点
- `AllowRelocationBlock` - 对于macOS 10.7及更早版本需要
- `AvoidRuntimeDefrag` - 需要
- `ForceExitBootServices` - 对于macOS 10.7及更早版本需要
- `ProvideCustomSlide` - 需要
- `RebuildAppleMemoryMap` - 对于macOS 10.6及更早版本需要

#### 内核
- 特殊点
  - `ProvideCurrentCpuInfo` - 为了正确的TSC/FSB值和CPU拓扑值，需要。
- 还需要以下其他内核扩展：
  - [Lilu](https://github.com/acidanthera/Lilu) - 修补和库函数
  - [VirtualSMC](https://github.com/acidanthera/VirtualSMC) - SMC模拟器
- 阻止
  - com.apple.driver.AppleEFIRuntime
    - 对于32位版本的macOS（10.4和10.5以及32位模式下的10.6），由于与Hyper-V UEFI不兼容性而无法使用EFI运行时服务和NVRAM。
- 强制
  - 在较旧版本的macOS上，可能需要强制注入以下内核扩展。有关详细信息，请参阅OpenCore配置手册。
  - IONetworkingFamily（`com.apple.iokit.IONetworkingFamily`）
  - IOSCSIParallelFamily（`com.apple.iokit.IOSCSIParallelFamily`）
- 补丁
  - 禁用_hpet_init
    - Arch = `i386`
    - Base = `_hpet_init`
    - Comment = `由于没有HPET硬件可用，禁用_hpet_init`
    - Count = `1`
    - Identifier = `kernel`
    - MaxKernel = `9.5.99`
    - Replace = `C3`
  - 禁用IOHIDDeviceShim ::newTransportString()
    - Arch = `i386`
    - Base = `__ZNK15IOHIDDeviceShim18newTransportStringEv`
    - Comment = `修复IOHIDDeviceShim ::newTransportString()中由_null deviceType_引起的崩溃`
    - Count = `1`
    - Identifier = `com.apple.iokit.IOHIDFamily`
    - MaxKernel = `9.6.99`
    - MinKernel = `9.5.0`
    - Replace = `31C0C3`
  - 禁用X/Y鼠标移动的缩放因子
    - Arch = `i386`
    - Base = `__ZN16IOHIDEventDriver21handleInterruptReportE12UnsignedWideP18IOMemoryDescriptor15IOHIDReportTypem`
    - Comment = `10.4中没有AbsoluteAxisBoundsRemovalPercentage的解决方法`
    - Identifier = `com.apple.iokit.IOHIDFamily`
    - Find = `BA1F85EB51`
    - MaxKernel = `8.11.99`
    - MinKernel = `8.0.0`
    - Replace = `BA00000000`
- 模拟
  - 对于较旧版本的macOS，可能需要虚拟电源管理和CPU欺骗，具体取决于主机CPU。

#### NVRAM
- 引导参数
  - 对于运行32位版本的macOS（10.4 - 10.5，以及在32位模式下运行的10.6），需要`-legacy`。这些版本中无法使用64位应用程序和NVRAM支持。

#### UEFI
- 特殊点
  - `DisableSecurityPolicy` - 在Windows Server 2019 / Windows 10及更高版本上需要

### 安装程序镜像创建
- 可以通过从USB硬盘传递安装程序映像或使用`qemu-img`将DMG转换为VHDX映像来创建安装程序映像：
  - DMG需要先以读/写格式存在。
  - `qemu-img convert -f raw -O vhdx Installer.dmg Installer.vhdx`

### 引导参数
有关每个模块的引导参数，请参见[模块列表](Docs/modules_zh.md)。

### 学分
- [Apple](https://www.apple.com) 提供macOS
- [Goldfish64](https://github.com/Goldfish64) 提供此软件
- [vit9696](https://github.com/vit9696) 提供[Lilu.kext](https://github.com/acidanthera/Lilu)和提供帮助
- [flagers](https://github.com/flagersgit) 提供文件复制实现并提供帮助
- [Microsoft Hypervisor顶级功能规范](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/tlfs)
- [Linux](https://github.com/torvalds/linux/tree/master/drivers/hv)和[FreeBSD](https://github.com/freebsd/freebsd-src/tree/main/sys/dev/hyperv)的Hyper-V集成服务
