# CNC Studio — UI 现代化重设计规范

## 1. 概述

将现有的两个独立窗口程序 `CNC_Simulator.exe` + `CNC_Monitor.exe` 合并为一个统一的可执行程序 `CNC_Studio.exe`，采用深色主题、左侧可折叠导航栏、卡片式布局、QCustomPlot 实时图表、彩色状态指示器。

### 核心理念

- 工业感深色主题，适合控制室暗光环境长时间盯屏
- 卡片式信息分区，明确视觉边界
- 彩色状态圆点替代纯文本状态指示
- 统一单窗口操作，消除双窗口切换

## 2. 架构变更

### 2.1 文件结构

```
apps/CNC_Studio/
├── main.cpp                   # 统一入口
├── MainWindow.h/.cpp          # 外壳：侧边栏导航 + QStackedWidget
├── MonitorPage.h/.cpp         # Monitor 仪表盘页面
├── SimulatorPage.h/.cpp       # Simulator 控制页面
└── theme/
    └── style.qss              # 全局深色主题样式表

AppUI/（新增）
├── PlotWidget.h/.cpp          # QCustomPlot 封装，替代原 RealTimePlotWidget

AppUI/（已有，可能微调）
├── RealTimePlotWidget.h/.cpp  # 保留不删，PlotWidget 替代使用
├── ParameterTableModel.h/.cpp # 保留，Device 列移除（改为下拉框选择）
├── NumericDelegate.h/.cpp     # 保留，去除 Device 列的 SpinBox 编辑器
```

### 2.2 Pro 文件变更

`DNC_SCADA.pro`：
- 移除 `CNC_Simulator` 和 `CNC_Monitor` 子项目
- 添加 `CNC_Studio` 子项目（`apps/CNC_Studio`）
- `CNC_Studio` 依赖：`Common NetworkModule StorageModule CoreEngine AppUI`

### 2.3 保留不动的模块

以下模块零改动：
- `Common/` — 类型、枚举、元类型注册、日志
- `NetworkModule/` — 协议、TCP 客户端/服务端、解析器
- `StorageModule/` — SQLite 异步写入
- `CoreEngine/` — MonitorEngine、DeviceStatePool

只在新 UI 层重新组合调用。

### 2.4 旧代码处理

`apps/CNC_Simulator/` 和 `apps/CNC_Monitor/` 目录保留不删除，方便回退和对比。`.pro` 中不再引用。

## 3. MainWindow 外壳设计

### 3.1 整体布局

```
┌──────────────────────────────────────────────────┐
│  [Logo] CNC Studio                ⬤ Sim Running  │  顶部标题栏 40px
├────────┬─────────────────────────────────────────┤
│  📊    │                                         │
│ Monitor│     ┌───────────────────────────────┐    │
│        │     │       QStackedWidget           │    │
│  ⚙     │     │   (MonitorPage /              │    │
│ Simulat│     │    SimulatorPage)              │    │
│        │     │                               │    │
│        │     └───────────────────────────────┘    │
├────────┴─────────────────────────────────────────┤
│  Ready.  D1 connected.                  ⬤ DB OK  │  底部状态栏 28px
└──────────────────────────────────────────────────┘
```

- 左侧导航栏：折叠态 56px / 展开态 200px
- 顶部标题栏：高度 40px，深色底色
- 底部状态栏：高度 28px
- 窗口默认尺寸：1280×820

### 3.2 左侧导航栏

两个导航按钮（QPushButton + QSS 样式）：

| 图标 | 文字 | 对应页面 |
|------|------|----------|
| 📊  | Monitor | MonitorPage (index 0，默认) |
| ⚙   | Simulator | SimulatorPage (index 1) |

行为：
- 默认折叠，只显示图标列 56px
- 悬停 300ms 后滑出完整文字面板 (QPropertyAnimation, 200px)
- 点击导航项切换 QStackedWidget 页码，选中项高亮蓝色左边框
- 可用顶部汉堡按钮锁定展开

### 3.3 顶部标题栏

- 左侧：应用图标 + "CNC Studio"（14pt 粗体）
- 右侧：Simulator 状态指示灯
  - 绿色圆点 + "Sim Running" — 仿真运行中
  - 灰色圆点 + "Sim Idle" — 仿真未启动
- 背景色：`#1a1a2e`

### 3.4 底部状态栏

- 左侧：操作消息文本（同现有 messageLabel）
- 右侧：数据库连接状态指示
- 字体 11px，颜色 `#8899aa`

## 4. MonitorPage 设计

### 4.1 布局

```
┌──────────────────────────────────────────────────────┐
│  ╔══════════════════════════════════════════════════╗ │
│  ║ DEVICE CARDS (横向排列)       HOST/PORT/CONNECT  ║ │  ← 顶部工具栏卡片
│  ╚══════════════════════════════════════════════════╝ │
│                                                      │
│  ┌────────────────────────────┐  ┌─────────────────┐ │
│  │   REAL-TIME CHART           │  │   ALARMS         │ │  ← 图表 60% + 报警 40%
│  │   (QCustomPlot)            │  │   (QListWidget)  │ │
│  └────────────────────────────┘  └─────────────────┘ │
│                                                      │
│  ┌────────────────────────────┐  ┌─────────────────┐ │
│  │   LIVE STATUS TABLE         │  │   PARAMETERS     │ │  ← 状态表 60% + 参数 40%
│  │   (QTableWidget)           │  │   (QTableView)   │ │
│  └────────────────────────────┘  └─────────────────┘ │
└──────────────────────────────────────────────────────┘
```

使用 `QGridLayout` 或嵌套 `QHBoxLayout`/`QVBoxLayout` 实现。

### 4.2 顶部工具栏卡片

- 左侧：设备状态快速概览 — 横向排列的设备小卡片
  - 每个卡片：状态圆点 + Device ID + 端口号
  - 绿色圆点 = Connected，红色圆点 = Disconnected/Error，黄色圆点 = Connecting
  - 圆点直径 8px，与外圈 12px 的同心圆环（半透明）组装
- 右侧：Host QLineEdit + First Port QSpinBox + Devices QSpinBox + Connect / Stop 按钮
- 整行包在圆角卡片（`#16213e` 背景，`border-radius: 8px`）中

### 4.3 实时图表 (QCustomPlot)

替换原有 `RealTimePlotWidget`（QPainter 自绘）。

- 深色画布背景 `#0d1117`，浅灰网格线 `#21262d`
- 三条曲线：
  - Servo Load — `#3b82f6`（亮蓝），实线 2px
  - Temperature — `#22c55e`（亮绿），实线 2px
  - RPM/60 — `#ef4444`（亮红），实线 2px
- X 轴：采样序号（0–300），Y 轴：0–100 归一化值
- 右上角图例，半透明背景
- 鼠标悬停显示 tooltip（QCPItemTracer + QToolTip）
- `setInteractions` 启用拖拽缩放和滚轮缩放
- 新数据到达时平滑滚动 X 轴

**PlotWidget 公开接口：**

```cpp
class PlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);
public slots:
    void appendStatus(DeviceStatus status);  // 添加采样点并刷新曲线
    void clear();                            // 清空全部曲线数据
};
```

与旧 `RealTimePlotWidget` 保持相同接口，MonitorPage 无需修改调用方式。内部使用 QCustomPlot 的三个 `QCPGraph` 实现。

### 4.4 报警日志卡片

- 使用 `QListWidget`，自定义 item 样式
- 每条报警：`⚠` 图标 + 时间戳 + 设备 ID + 报警码 + 报警信息
- 时间戳格式 `HH:mm:ss.zzz`
- 最新报警在顶部
- 空状态时显示 "No active alarms" 灰色居中文字
- 超过 100 条后移除最旧项

### 4.5 实时状态表

- `QTableWidget`，8 列：Device / X / Y / Z / RPM / Load / Temp / State
- State 列：彩色圆点 + 文字（`● RUN` 绿色 / `● ALARM` 红色）
- 数值列右对齐
- 行高 28px，行间隔色（`#1a1a2e` / `#16213e`）
- 表头背景 `#0d1117`

### 4.6 参数控制卡片

- 顶部：目标设备下拉框 `QComboBox`（1–10），替换原表格中的 Device 列
- 参数表格：Name / Value / Min / Max / Type（5 列）
- 下拉框切换设备时，ParameterTableModel 批量更新 deviceId
- "Send Parameter" 按钮使用蓝色强调色

## 5. SimulatorPage 设计

### 5.1 布局

```
┌──────────────────────────────────────────────────────┐
│  ╔══════════════════════════════════════════════════╗ │
│  ║  SIMULATOR CONTROL CARD                          ║ │  ← 控制面板
│  ╚══════════════════════════════════════════════════╝ │
│                                                      │
│  ┌──────────────────────────────────────────────────┐ │
│  │   DEVICE STATUS TABLE                            │ │  ← 状态表
│  └──────────────────────────────────────────────────┘ │
│                                                      │
│  ┌──────────────────────────────────────────────────┐ │
│  │   EVENT LOG                                      │ │  ← 日志
│  └──────────────────────────────────────────────────┘ │
```

### 5.2 控制面板卡片

单行排列，水平等间距：

```
First Port    Devices    Frequency    Alarm Dev    [▶ Start] [■ Stop]
[8001      ]  [10     ]  [50      ]  [1       ]    [⚠ Inject Alarm]
```

- 每个 SpinBox 上方有小标签（11px，`#8899aa`）
- SpinBox 自定义样式（深色背景、圆角边框）
- Start 按钮：绿色强调（`#22c55e`）
- Stop 按钮：红色强调（`#ef4444`）
- Inject Alarm 按钮：红色边框幽灵按钮（透明背景，红色边框和文字）

### 5.3 设备状态表

- `QTableWidget`，9 列：Device / Port / State / X / Y / Z / RPM / Load / Temp
- State 列：彩色圆点 + 文字
- 数值列右对齐
- 行高 32px

### 5.4 事件日志

- `QTextEdit`，只读模式
- 等宽字体 `Consolas` / `SF Mono`，11px
- 深底 `#0d1117`，浅字 `#c9d1d9`
- 自动滚动到底部
- 最大高度 140px

## 6. 样式系统

### 6.1 配色方案

| 用途 | 颜色代码 | 说明 |
|------|----------|------|
| 页面背景 | `#0d1117` | 最深底色 |
| 卡片背景 | `#161b22` | 卡片/面板 |
| 卡片边框 | `#30363d` | 细微分隔 |
| 标题栏背景 | `#1a1a2e` | 比页面略亮 |
| 主文字 | `#e6edf3` | 正文 |
| 次要文字 | `#8b949e` | 标签/辅助 |
| 强调蓝色 | `#3b82f6` | 按钮/选中/曲线1 |
| 状态绿色 | `#22c55e` | RUN/Connected/成功 |
| 状态红色 | `#ef4444` | ALARM/Error/断开 |
| 状态黄色 | `#eab308` | Connecting/警告 |
| 表格行交替 | `#1a1a2e` / `#161b22` | 行条纹 |

### 6.2 字体

- 中文/英文标题：系统默认 sans-serif（Windows 上为 Segoe UI）
- 等宽字体（日志）：`Consolas, "SF Mono", monospace`
- 标题大小：14pt 粗体
- 正文大小：12pt
- 标签大小：11pt

### 6.3 圆角规范

- 大卡片：`border-radius: 8px`
- 按钮：`border-radius: 6px`
- 输入框/SpinBox：`border-radius: 4px`
- 小状态卡片：`border-radius: 6px`

### 6.4 阴影

- 卡片使用 `QGraphicsDropShadowEffect`，颜色 `rgba(0,0,0,0.4)`，blur 12px，offset (0, 4)

### 6.5 QSS 文件组织

`apps/CNC_Studio/theme/style.qss` 统一管理所有样式：
- 全局默认（QMainWindow, QWidget）
- 按钮（QPushButton 默认、主要、危险、幽灵）
- 输入控件（QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox）
- 表格（QTableWidget, QTableView, QHeaderView）
- 列表（QListWidget）
- 滚动条（QScrollBar）
- 标签（QLabel）

## 7. 通用 UI 组件说明

### 7.1 状态圆点指示器

表格和卡片中的彩色状态圆点统一用以下方式实现，无需新建 widget 类：

- **表格中**：通过 `QTableWidgetItem` 设置 `Qt::DecorationRole` 为一个小的 `QPixmap`（12×12 圆形图标，QPainter 在内存中绘制）
- **设备卡片中**：使用 `QLabel` + QSS `background-color` + `border-radius: 50%` 实现 8px 圆点
- 统一辅助函数 `QColor runStateColor(MachineRunState)` 和 `QColor connectionStateColor(ConnectionState)` 放在 `AppUI/` 或页面中

### 7.2 卡片阴影

对需要阴影的卡片容器 QWidget，在代码中创建 `QGraphicsDropShadowEffect` 并 `widget->setGraphicsEffect(effect)`。不在 QSS 中处理（Qt 5 不支持 CSS `box-shadow`）。

## 8. 数据流（不变）

Monitor 和 Simulator 的数据流保持现有架构不变：

- Simulator 生成 → Protocol 编码 → TcpDeviceServer 广播 → TcpClientWorker 接收 → ProtocolParser 拆包 → MonitorEngine 分发 → UI 更新 + 存储
- 参数下发流程不变

## 9. 实施边界

### 包含
- 新建 `CNC_Studio` 统一应用
- 深色主题 QSS 样式表
- 左侧可折叠导航栏
- MonitorPage 卡片式仪表盘
- SimulatorPage 控制面板
- QCustomPlot 替代 QPainter 自绘图表
- 彩色状态圆点指示器
- 控件圆角 + 阴影
- 设备下拉框替换参数表 Device 列

### 不包含
- 动画过渡（页面切换动画、悬停动画等超出 QSS 能力的部分）
- 可切换浅色/深色主题
- 设置页面（Settings 导航项留空或移除）
- 国际化/多语言
- QCustomPlot 源码（作为第三方库引入，不修改其源码）
- 单元测试（本 spec 仅涉及 UI 层）

## 10. 依赖

- **新增**：QCustomPlot（`qcustomplot.h` / `qcustomplot.cpp`），MIT 许可，放入 `ThirdParty/` 或直接置于 `AppUI/`
- **已有**：Qt 5.12.8（Widgets, Network, Sql, Core, Gui）
- **无变更**：Common, NetworkModule, StorageModule, CoreEngine
