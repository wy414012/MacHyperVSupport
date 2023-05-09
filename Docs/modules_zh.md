# 驱动模块列表

## 核心控制器 (HyperVController)
核心 Hyper-V 控制器模块。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvctrldbg     | 在 DEBUG 构建中启用调试打印

## CPU 禁用器 (HyperVCPU)
在 macOS 10.4 下禁用额外的 CPU。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvcpudbg      | 在 DEBUG 构建中启用调试打印

## 文件拷贝 (HyperVFileCopy)
提供从主机到客户端的文件拷贝支持（Guest Services）。需要运行 `hvfilecopyd` 用户空间守护程序。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvfcopydbg    | 在 DEBUG 构建中启用调试打印
| -hvfcopymsgdbg | 在 DEBUG 构建中启用消息数据的调试打印
| -hvfcopyoff    | 禁用此模块

## 图形桥接 (HyperVGraphicsBridge)
为 macOS 提供基本的图形支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvgfxbdbg     | 在 DEBUG 构建中启用调试打印
| -hvgfxbmsgdbg  | 在 DEBUG 构建中启用消息数据的调试打印
| -hvgfxboff     | 禁用此模块

## 心跳 (HyperVHeartbeat)
向 Hyper-V 提供心跳报告。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvheartdbg    | 在 DEBUG 构建中启用调试打印
| -hvheartmsgdbg | 在 DEBUG 构建中启用消息数据的调试打印
| -hvheartoff    | 禁用此模块

## 键盘 (HyperVKeyboard)
提供键盘支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvkbddbg      | 在 DEBUG 构建中启用调试打印
| -hvkbdmsgdbg   | 在 DEBUG 构建中启用消息数据的调试打印
| -hvkbdoff      | 禁用此模块

## 鼠标 (HyperVMouse)
提供鼠标支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvmousdbg     | 在 DEBUG 构建中启用调试打印
| -hvmousmsgdbg  | 在 DEBUG 构建中启用消息数据的调试打印
| -hvmousoff     | 禁用此模块

## 网络 (HyperVNetwork)
提供网络支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvnetdbg      | 在 DEBUG 构建中启用调试打印
| -hvnetmsgdbg   | 在 DEBUG 构建中启用消息数据的调试打印
| -hvnetoff      | 禁用此模块

## PCI 桥接器 (HyperVPCIBridge)
提供 PCI 穿透支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvpcibdbg     | 在 DEBUG 构建中启用调试打印
| -hvpcibmsgdbg  | 在 DEBUG 构建中启用消息数据的调试打印
| -hvpciboff     | 禁用此模块

## PCI 模块设备 (HyperVModuleDevice)
提供 PCI 穿透的 MMIO 分配/释放函数。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvpcimdbg     | 在 DEBUG 构建中启用调试打印

## PCI 提供程序 (HyperVPCIProvider)
在第 2 代 VMS 上提供 IOACPIPlatformDevice nub，用于虚假的 PCI 根桥接器 (HyperVPCIRoot)。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvpcipdbg     | 在 DEBUG 构建中启用调试打印

## PCI 根桥接器 (HyperVPCIRoot)
为第 2 代 VM 提供虚假的 PCI 根桥接器，以实现 macOS 的正常功能，并提供 PCI 穿透支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvpcirdbg     | 在 DEBUG 构建中启用调试打印

## 关机 (HyperVShutdown)
通过 Virtual Machine Connection 和 PowerShell 提供软关机功能。需要运行 `hvshutdownd` 用户空间守护程序。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvshutdbg     | 在 DEBUG 构建中启用调试打印
| -hvshutmsgdbg  | 在 DEBUG 构建中启用消息数据的调试打印
| -hvshutoff     | 禁用此模块

## 存储 (HyperVStorage)
提供 SCSI 存储支持。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvstordbg     | 在 DEBUG 构建中启用调试打印
| -hvstormsgdbg  | 在 DEBUG 构建中启用消息数据的调试打印
| -hvstoroff     | 禁用此模块

## 时间同步 (HyperVTimeSync)
提供主机到客户端的时间同步支持。需要运行 `hvtimesyncd` 用户空间守护程序。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvtimedbg     | 在 DEBUG 构建中启用调试打印
| -hvtimemsgdbg  | 在 DEBUG 构建中启用消息数据的调试打印
| -hvtimeoff     | 禁用此模块

## VMBus 控制器 (HyperVVMBus)
提供 VMBus 设备和服务的根。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvvmbusdbg    | 在 DEBUG 构建中启用调试打印

## VMBus 设备 nub (HyperVVMBusDevice)
为子 VMBus 设备模块提供连接 nub。

| 启动参数 | 描述 |
|----------------|-------------|
| -hvvmbusdebdbg | 在 DEBUG 构建中启用调试打印# Driver Module List

