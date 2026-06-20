# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 构建命令

本机同时安装了 Qt 5.12.8 和 Qt 4.8.5，**必须将 Qt 5.12.8 的 bin 放在 PATH 最前面**，否则链接时报 `File format not recognized`。

### PowerShell

```powershell
$env:PATH = "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin;C:\Qt\Qt5.12.8\Tools\mingw730_32\bin;$env:PATH"

# 完整重编译
& "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\qmake.exe" "D:\qt project\DNC_SCADA.pro" -r -spec win32-g++
mingw32-make

# 增量编译（日常修改代码后）
mingw32-make
```

### Git Bash

```bash
export PATH="/c/Qt/Qt5.12.8/5.12.8/mingw73_32/bin:/c/Qt/Qt5.12.8/Tools/mingw730_32/bin:$PATH"
qmake "D:/qt project/DNC_SCADA.pro" -r -spec win32-g++
mingw32-make     # 全量
mingw32-make     # 增量
```

产物：`bin/CNC_Studio.exe`，中间文件在 `build/`（`bin/` 和 `build/` 均被 `.gitignore` 忽略）。

> ⚠️ **Tests 构建失败**：`Tests/` 已加入顶层 SUBDIRS，但 `TestStorage`、`TestNetwork`、`TestEngine`、`TestIntegration` 缺少对应的 `tst_*.cpp` 源文件，构建末尾会报 `No rule to make target`。这是已知问题，不影响主程序构建。

> ⚠️ **遗留陷阱**：`apps/CNC_Simulator/` 和 `apps/CNC_Monitor/` 仍保留旧的 `.pro` 文件和源码，但它们已**不在**顶层 `DNC_SCADA.pro` 的 SUBDIRS 中，不要尝试单独编译它们 —— 功能已被 CNC_Studio 统一。

### 部署 DLL

除 Qt5PrintSupport.dll 外，运行时还需要 Qt 核心 DLL（`Qt5Core`、`Qt5Gui`、`Qt5Widgets`、`Qt5Network`、`Qt5Sql`）。可使用 windeployqt 自动收集：

```powershell
& "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\windeployqt.exe" "D:\qt project\bin\CNC_Studio.exe"
```

> `C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\Qt5PrintSupport.dll` 需手动复制到 `bin/` —— QCustomPlot 依赖它但 windeployqt 可能不会自动包含。

## 架构概览

Qt 5.12.8 / C++14 / qmake SUBDIRS 项目（`CONFIG += ordered` 强制按列表顺序构建）。**所有类型定义在 `DncScada` 命名空间下**。

所有子模块编译为静态库（`staticlib`），仅 `CNC_Studio` 为可执行文件（`app`），链接所有静态库。单一可执行文件 `CNC_Studio.exe`，将原来两个独立窗口合并为统一深色主题仪表盘，左侧可折叠导航栏切换页面。

**本项目无自动化测试**（无单元测试框架，无 CI 配置）。

### 模块依赖顺序

```
Common → NetworkModule, StorageModule
AppUI 依赖 Common
CoreEngine 依赖 Common, NetworkModule, StorageModule
CNC_Studio（app）依赖以上全部
```

### 各模块职责

- **Common** — 共享类型（`DeviceStatus`、`ProtocolFrame`、`AlarmEvent`、枚举）、跨线程元类型注册、异步日志（`LOG_INFO`/`LOG_WARN`/`LOG_ERROR` 宏写入 `bin/logs/`，按天 + 10MB 轮转）
- **NetworkModule** — 二进制协议（帧头 `0xAA55`、CRC-16/MODBUS、**小端字节序**）、`ProtocolParser` 状态机拆包处理 TCP 粘包半包、`TcpClientWorker`（监控端，每设备一个线程）、`TcpDeviceServer`（仿真端，每设备一个线程）
- **StorageModule** — `StorageWorker` 独立线程、SQLite 异步批量写入（`status_history`、`alarm_log`、`parameter_log` 三张表）、双缓冲每 500ms 或 100 条刷盘
- **CoreEngine** — `MonitorEngine` 协调整合网络 worker 与存储、`DeviceStatePool` 线程安全设备状态缓存、参数下发防呆（核心参数在设备 RUN 状态时拒绝下发）
- **AppUI** — `PlotWidget`（QCustomPlot 封装，深色画布+三条曲线+悬停提示）、`ParameterTableModel`（5 列：Name/Value/Min/Max/Type，Device 改为下拉框选择）、`NumericDelegate`、`StatusColors` 辅助函数
- **ThirdParty** — `qcustomplot.h/.cpp`（2.1.1，GPL 许可）。注意：`ThirdParty/qcustomplot-source/` 是参考副本，实际编译使用的是 `ThirdParty/qcustomplot.h`

### 线程模型

```
GUI 主线程 ─── MainWindow + MonitorPage + SimulatorPage
工作线程 ×N ─── TcpDeviceServer（仿真端，每设备一个）
工作线程 ×N ─── TcpClientWorker（监控端，每设备一个）
存储线程 ×1 ─── StorageWorker（SQLite 异步写入）
日志线程 ×1 ─── AsyncLogger::Impl（生产者-消费者，后台写盘）
```

跨线程通信：Qt 信号槽 + `Qt::QueuedConnection` + `qRegisterMetaType`

### 数据流

1. 仿真端 `generateTick()` → `Protocol::encodeStatusPayload()` → `TcpDeviceServer::broadcastFrame()` → TCP 写出
2. 监控端 `TcpClientWorker::onReadyRead()` → `ProtocolParser::feed()` → 发出 `frameReceived` 信号
3. `MonitorEngine::onFrameReceived()` → 解码 → 发出 `statusUpdated` 信号
4. `MonitorPage::onStatusUpdated()` → 缓存到 `pendingStatuses` → 定时器批量刷新 UI + 图表

### UI 刷新节流（重要）

默认设置（10 设备 × 50 Hz）每秒产生 **500 次状态更新**。为防止主线程饱和导致界面卡顿，UI 更新已做解耦：

- **PlotWidget** — 内置 50ms QTimer（20 FPS），`appendStatus()` 仅增量 `addData()` + 标记 dirty，`replot()` 由定时器触发，不在每次数据到达时立即重绘
- **MonitorPage** — `onStatusUpdated()` 仅将最新状态缓存到 `QHash<quint8, DeviceStatus> pendingStatuses`，60ms 定时器 `flushStatusUpdates()` 批量更新表格和图表（~16 FPS）
- **MonitorEngine** — 不打印每帧日志（500 条/秒的日志会阻塞事件循环）

修改前（旧代码）`PlotWidget::appendStatus()` 每次调用都 clear 全部数据 → 逐点重建 300 个采样点 × 3 条曲线 → `replot()`，即 `replot()` 被调用 500 次/秒，这是原始卡顿的根因。

### 自定义协议帧结构（小端字节序）

```
┌──────────┬──────────┬──────────┬───────────┬──────────────────┬────────────┐
│  Header  │ DeviceId │ Function │ PayloadLen│     Payload      │   CRC-16   │
│  2 bytes │  1 byte  │  1 byte  │  2 bytes  │   0~4096 bytes   │  2 bytes   │
│  0xAA55  │          │          │           │                  │            │
└──────────┴──────────┴──────────┴───────────┴──────────────────┴────────────┘
│←─────────────── FixedHeaderSize = 6 bytes ──────────────→│                │
│←─────────────────── CRC 校验覆盖范围 ─────────────────────→│
```

| 功能码 | 含义 |
|--------|------|
| `0x01` | 状态上报（StatusReport） |
| `0x02` | 参数下发（ParameterWrite） |
| `0x03` | 心跳（Heartbeat） |

- 帧同步：通过 `0xAA55` 帧头逐字节扫描定位帧边界
- CRC：CRC-16/MODBUS（多项式 `0xA001`）
- `ProtocolParser` 状态机处理 TCP 粘包/半包，内部维护持久化缓冲区

## 常见陷阱

- **Q_ARG 命名空间**：`Q_ARG(DncScada::ProtocolFrame, frame)` 必须带完整命名空间前缀。缺少 `DncScada::` 导致 `#type` 字符串化后变成 `"ProtocolFrame"` 而非注册名 `"DncScada::ProtocolFrame"`，`QMetaObject::invokeMethod` 静默失败，数据不会发送。
- **双 Qt 版本**：本机同时安装了 Qt 5.12.8 和 Qt 4.8.5。构建前必须显式将 Qt 5.12.8 的 bin 放在 PATH 最前面，否则部分 .o 文件会被 Qt 4.8.5 编译器生成，链接时报 `File format not recognized`。
- **Qt5PrintSupport.dll**：QCustomPlot 运行时依赖此 DLL，必须放在 `bin/` 目录下与其他 Qt DLL 同级。源码位于 `C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\Qt5PrintSupport.dll`。windeployqt 可能遗漏此 DLL，需手动复制。
- **LOG_INFO 宏限制**：`LOG_INFO(message)` 内部通过 `QStringLiteral(message)` 展开，参数必须是字符串字面量。动态拼接消息需直接调用 `AsyncLogger::instance().log()`。
- **旧 .o 清理**：切换 Qt 版本或大改后，删除 `build/` 目录避免格式不兼容导致链接失败。
- **手动删除 CNC_Studio.exe 后重编**：`mingw32-make` 可能不检测到库变更而跳过链接。删除 `bin/CNC_Studio.exe` 再 `mingw32-make` 可强制重新链接。

## UI 导航

CNC_Studio 左侧导航栏（折叠 56px / 展开 200px）两个页面：
- **Monitor**（index 0，默认页）— 设备连接卡片、实时图表、状态表、参数面板、报警列表
- **Simulator**（index 1）— 仿真控制面板、设备状态表、事件日志

深色主题 QSS：`apps/CNC_Studio/theme/style.qss`，由 `MainWindow::buildUi()` 加载。

### 本机自环测试步骤

1. 切换到 **Simulator** 页面，点击 **▶ Start**（默认 10 设备 / 8001 端口起 / 50 Hz）
2. 切换到 **Monitor** 页面，Host=`127.0.0.1`，点击 **Connect**
3. 观察实时图表、状态表变化 — 界面应流畅不卡顿
