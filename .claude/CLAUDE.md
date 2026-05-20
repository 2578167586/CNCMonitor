# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 构建命令

```powershell
# 必须显式使用 Qt 5.12.8 —— 本机同时安装了 Qt 4.8.5，在 PATH 中会干扰构建
$env:PATH = "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin;C:\Qt\Qt5.12.8\Tools\mingw730_32\bin;$env:PATH"

# 完整重编译
& "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\qmake.exe" "D:\qt project\DNC_SCADA.pro" -r -spec win32-g++
mingw32-make

# 增量编译（日常修改代码后）
mingw32-make
```

产物：`bin/CNC_Studio.exe`

## 架构概览

Qt 5.12.8 / C++14 / qmake SUBDIRS 项目。单一可执行文件 `CNC_Studio.exe`，将原来两个独立窗口（CNC_Simulator + CNC_Monitor）合并为统一深色主题仪表盘，左侧可折叠导航栏切换页面。

### 模块依赖顺序

```
Common → NetworkModule, StorageModule
AppUI 依赖 Common
CoreEngine 依赖 Common, NetworkModule, StorageModule
CNC_Studio（app）依赖以上全部
```

### 各模块职责

- **Common** — 共享类型（`DeviceStatus`、`ProtocolFrame`、`AlarmEvent`、枚举）、跨线程元类型注册、异步日志（`LOG_INFO`/`LOG_WARN`/`LOG_ERROR` 宏写入 `bin/logs/`）
- **NetworkModule** — 二进制协议（帧头 `0xAA55`、CRC-16/MODBUS）、`ProtocolParser` 状态机拆包处理 TCP 粘包半包、`TcpClientWorker`（监控端，每设备一个线程）、`TcpDeviceServer`（仿真端，每设备一个线程）
- **StorageModule** — `StorageWorker` 独立线程、SQLite 异步批量写入（`status_history`、`alarm_log`、`parameter_log` 三张表）、双缓冲每 500ms 或 100 条刷盘
- **CoreEngine** — `MonitorEngine` 协调整合网络 worker 与存储、`DeviceStatePool` 线程安全设备状态缓存、参数下发防呆（核心参数在设备 RUN 状态时拒绝下发）
- **AppUI** — `PlotWidget`（QCustomPlot 封装，深色画布+三条曲线+悬停提示）、`ParameterTableModel`（5 列：Name/Value/Min/Max/Type，Device 改为下拉框选择）、`NumericDelegate`、`StatusColors` 辅助函数
- **ThirdParty** — `qcustomplot.h/.cpp`（2.1.1，GPL 许可）

### 线程模型

- GUI 主线程：MainWindow + 两个 Page
- 仿真端每设备一个 `QThread`（TcpDeviceServer）
- 监控端每设备一个 `QThread`（TcpClientWorker）
- 数据库一个 `QThread`（StorageWorker）
- 跨线程通信：Qt 信号槽 + `Qt::QueuedConnection` + `qRegisterMetaType`

### 数据流

1. 仿真端 `generateTick()` → `Protocol::encodeStatusPayload()` → `TcpDeviceServer::broadcastFrame()` → TCP 写出
2. 监控端 `TcpClientWorker::onReadyRead()` → `ProtocolParser::feed()` → 发出 `frameReceived` 信号
3. `MonitorEngine::onFrameReceived()` → 解码 → 发出 `statusUpdated` 信号
4. `MonitorPage::onStatusUpdated()` → 更新表格 + 追加图表 + 投递存储队列

## 常见陷阱

- **Q_ARG 命名空间**：`Q_ARG(DncScada::ProtocolFrame, frame)` 必须带完整命名空间前缀。缺少 `DncScada::` 导致 `#type` 字符串化后变成 `"ProtocolFrame"` 而非注册名 `"DncScada::ProtocolFrame"`，`QMetaObject::invokeMethod` 静默失败，数据不会发送。
- **双 Qt 版本**：本机同时安装了 Qt 5.12.8 和 Qt 4.8.5。构建前必须显式将 Qt 5.12.8 的 bin 放在 PATH 最前面，否则部分 .o 文件会被 Qt 4.8.5 编译器生成，链接时报 `File format not recognized`。
- **Qt5PrintSupport.dll**：QCustomPlot 运行时依赖此 DLL，必须放在 `bin/` 目录下与其他 Qt DLL 同级。源码位于 `C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\Qt5PrintSupport.dll`。
- **LOG_INFO 宏限制**：`LOG_INFO(message)` 内部通过 `QStringLiteral(message)` 展开，参数必须是字符串字面量。动态拼接消息需直接调用 `AsyncLogger::instance().log()`。
- **旧 .o 清理**：切换 Qt 版本或大改后，删除 `build/` 目录避免格式不兼容导致链接失败。

## UI 导航

CNC_Studio 左侧导航栏（折叠 56px / 展开 200px）两个页面：
- **Monitor**（index 0，默认页）— 设备连接卡片、实时图表、状态表、参数面板、报警列表
- **Simulator**（index 1）— 仿真控制面板、设备状态表、事件日志

深色主题 QSS：`apps/CNC_Studio/theme/style.qss`，由 `MainWindow::buildUi()` 加载。
