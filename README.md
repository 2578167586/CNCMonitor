# CNC Studio

基于 Qt 5.12.8 的 CNC（计算机数控）设备统一监控与仿真仪表盘。将原来两个独立窗口（CNC_Simulator + CNC_Monitor）合并为单一深色主题可执行文件，左侧可折叠导航栏切换页面。

## 功能概览

- **设备监控（Monitor）** — 实时接收设备状态数据，展示动态曲线图、状态表、参数面板和报警列表
- **设备仿真（Simulator）** — 模拟多台 CNC 设备，生成正弦波运动数据并通过 TCP 广播
- **自定义二进制协议** — 帧头 `0xAA55` + CRC-16/MODBUS 校验，适配工业场景低带宽高实时性需求
- **异步日志系统** — 生产-消费者模式，后台线程批量写盘，按天 + 10MB 轮转
- **SQLite 持久化存储** — 双缓冲 + 事务批量写入，不阻塞主线程
- **深色主题 UI** — 可折叠侧边栏（56px / 200px），QSS 自定义样式

## 截图

> 运行构建产物 `bin/CNC_Studio.exe` 体验完整界面。

## 构建

### 环境要求

- **Qt 5.12.8**（MinGW 7.3.0 32-bit）
- **编译器**: MinGW 7.3.0 32-bit（随 Qt 5.12.8 发行）
- **操作系统**: Windows

> ⚠️ **注意**: 如果本机同时安装了多个 Qt 版本，构建前必须显式将 Qt 5.12.8 的 `bin` 放在 `PATH` 最前面，否则链接时会报 `File format not recognized`。

### 构建命令

在项目根目录下执行：

```powershell
# 1. 设置环境变量（关键步骤！）
$env:PATH = "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin;C:\Qt\Qt5.12.8\Tools\mingw730_32\bin;$env:PATH"

# 2. 完整重编译
& "C:\Qt\Qt5.12.8\5.12.8\mingw73_32\bin\qmake.exe" "D:\qt project\DNC_SCADA.pro" -r -spec win32-g++
mingw32-make

# 3. 增量编译（日常修改代码后）
mingw32-make
```

### 构建产物

- `bin/CNC_Studio.exe` — 主程序
- `bin/Qt5PrintSupport.dll` — QCustomPlot 运行时依赖，须与 EXE 同目录

## 项目结构

```
D:\qt project\
├── Common/                  # 共享基础模块
│   ├── Types.h / .cpp       # 数据类型定义（DeviceStatus、ProtocolFrame、AlarmEvent 等）
│   └── Logger.h / .cpp      # 异步日志系统（生产-消费者模式 + 文件轮转）
├── NetworkModule/           # 网络通信模块
│   ├── Protocol.h / .cpp    # 二进制协议（帧编解码 + CRC-16/MODBUS + 拆包状态机）
│   ├── TcpClientWorker.h/.cpp  # 监控端 TCP 客户端（每设备一个线程）
│   └── TcpDeviceServer.h/.cpp  # 仿真端 TCP 服务器（每设备一个线程）
├── StorageModule/           # 数据持久化模块
│   └── StorageWorker.h/.cpp # SQLite 异步批量写入（双缓冲 + 事务）
├── CoreEngine/              # 核心引擎
│   ├── MonitorEngine.h/.cpp    # 监控协调器（整合网络 + 存储）
│   └── DeviceStatePool.h/.cpp  # 线程安全设备状态缓存
├── AppUI/                   # UI 组件库
│   ├── PlotWidget.h / .cpp     # QCustomPlot 封装（深色画布 + 三曲线 + 悬停提示）
│   ├── ParameterTableModel.h/.cpp  # 参数表 Model（5 列：Name/Value/Min/Max/Type）
│   ├── NumericDelegate.h/.cpp     # 数值输入限制委托
│   └── StatusColors.h / .cpp      # 状态颜色辅助函数
├── ThirdParty/              # 第三方库
│   └── qcustomplot.h / .cpp # QCustomPlot 2.1.1（GPL）
├── apps/
│   └── CNC_Studio/          # 主应用程序
│       ├── main.cpp         # 入口
│       ├── MainWindow.h/.cpp   # 主窗口（可折叠侧边栏 + QStackedWidget）
│       ├── MonitorPage.h/.cpp  # 监控页面
│       ├── SimulatorPage.h/.cpp# 仿真页面
│       └── theme/style.qss     # 深色主题样式表
└── DNC_SCADA.pro            # 顶层 SUBDIRS 工程文件
```

## 架构

### 模块依赖顺序

```
Common → NetworkModule, StorageModule
AppUI 依赖 Common
CoreEngine 依赖 Common, NetworkModule, StorageModule
CNC_Studio（app）依赖以上全部
```

### 线程模型

```
GUI 主线程 ─── MainWindow + MonitorPage + SimulatorPage
工作线程 ×N ─── TcpDeviceServer（仿真端，每设备一个）
工作线程 ×N ─── TcpClientWorker（监控端，每设备一个）
存储线程 ×1 ─── StorageWorker（SQLite 异步写入）
日志线程 ×1 ─── AsyncLogger::Impl（文件异步写入）
```

跨线程通信：Qt 信号槽 + `Qt::QueuedConnection` + `qRegisterMetaType`

### 数据流

```
仿真端 generateTick()
  → Protocol::encodeStatusPayload()
  → Protocol::encodeFrame()
  → TcpDeviceServer::broadcastFrame() → TCP 写出

监控端 TcpClientWorker::onReadyRead()
  → ProtocolParser::feed() 拆包
  → emit frameReceived (跨线程)
  → MonitorEngine::onFrameReceived()
  → Protocol::decodeStatusPayload()
  → emit statusUpdated
  → MonitorPage 更新 UI + StorageWorker 异步写库
```

## 自定义协议帧结构

```
┌──────────┬──────────┬──────────┬───────────┬──────────────────┬────────────┐
│  Header  │ DeviceId │ Function │ PayloadLen│     Payload      │   CRC-16   │
│  2 bytes │  1 byte  │  1 byte  │  2 bytes  │   0~4096 bytes   │  2 bytes   │
│  0xAA55  │          │          │           │                  │            │
└──────────┴──────────┴──────────┴───────────┴──────────────────┴────────────┘
│←─────────────── FixedHeaderSize = 6 bytes ──────────────→│                │
│←─────────────────── CRC 校验覆盖范围 ─────────────────────→│
```

### 功能码

| 值 | 含义 |
|----|------|
| `0x01` | 状态上报（StatusReport） |
| `0x02` | 参数下发（ParameterWrite） |
| `0x03` | 心跳（Heartbeat） |

### 协议特性

- **帧同步**: 通过 `0xAA55` 帧头逐字节扫描定位帧边界
- **变长帧**: PayloadLen 字段支持 0~4096 字节负载
- **差错校验**: CRC-16/MODBUS（多项式 `0xA001`）
- **防粘包/半包**: `ProtocolParser` 状态机 + 持久化缓冲区

## 数据库

SQLite 数据库文件位于 `bin/data/monitor.sqlite`，包含三张表：

| 表名 | 说明 |
|------|------|
| `status_history` | 设备状态历史（device_id, x, y, z, spindle_rpm, servo_load, temperature, run_state, alarm_code, created_at） |
| `alarm_log` | 报警日志（device_id, code, message, created_at） |
| `parameter_log` | 参数变更记录（device_id, name, value, minimum, maximum, core_parameter, created_at） |

写入策略：双缓冲无锁 swap + SQL 事务批量 INSERT + 100 条满 / 500ms 定时双触发。

## 使用方式

### 启动应用

运行 `bin/CNC_Studio.exe`。

### 功能测试（本机自环）

1. **左侧导航栏** 切换到 **Simulator** 页面
2. 设置设备数量（默认 10）、起始端口（默认 8001）、发送频率（默认 50Hz）
3. 点击 **▶ Start** 启动仿真设备
4. 切换到 **Monitor** 页面
5. 保持 Host=`127.0.0.1`、Port=`8001`、Devices=`10`
6. 点击 **Connect** 开始监控
7. 观察实时图表、状态表变化，可下发参数、注入报警测试

### 连接真实设备

监控端不需要改动任何代码。将 Host 改为真实设备的 IP 地址，确保设备端使用相同协议（`0xAA55` 帧头 + CRC-16/MODBUS + 小端序）即可。

## 许可证

本项目代码采用 MIT 许可证。

第三方依赖：

- [QCustomPlot](https://www.qcustomplot.com/) 2.1.1 — GPL 许可

## 作者

CNC Studio — 统一 CNC 设备监控与仿真仪表盘。
