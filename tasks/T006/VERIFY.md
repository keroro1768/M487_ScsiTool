# T006 - Windows C++ Win32 HID Tool

## 工作日誌 (WORKLOG.md)

### 任务描述
Windows 上与 M487 USB HID 沟通的工具，提供 CLI/GUI 界面。

### 状态
⏳ Pending

### 子任务进度

---

## 验证文件 (VERIFY.md)

### 验证项目

| 验证项 | 方法 | 状态 |
|--------|------|------|
| CLI `device list` | 插上 M487，执行 `hidtool.exe device list` | ⏳ |
| CLI `read reg` | 读取 I2C register，对照 datasheet | ⏳ |
| CLI `write reg` | 写入后立即 read 验证 | ⏳ |
| GUI 界面 | 启动 GUI，操作所有按钮 | ⏳ |
| Hotplug 监听 | 插拔设备，观察通知 | ⏳ |
| 跨平台编译 | Windows / Linux / macOS 各编译一次 | ⏳ |

### 验证环境
- Windows 10/11 x64
- Visual Studio 2022
- USB VID=0x04F3 PID=0x0732

### 验证记录
| 日期 | 验证项 | 结果 | 备注 |
|------|--------|------|------|
|      |        |      |      |
