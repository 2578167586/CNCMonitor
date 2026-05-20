**# DNC/SCADA Qt 项目任务划分**



**## Summary**

**基于 `agent.md`，项目拆成两个独立应用：`CNC\_Simulator` 仿真下位机与 `CNC\_Monitor` 监控上位机。构建体系采用 `qmake`，同时保留模块化目录与库边界：`NetworkModule`、`StorageModule`、`CoreEngine`、`AppUI`。**



**## Key Tasks**

**1. \*\*工程骨架\*\***

&#x20;  **- 建立 qmake `SUBDIRS` 工程：公共模块库 + 两个可执行程序。**

&#x20;  **- 定义公共数据结构：设备状态、报警、参数配置、协议帧、连接状态。**

&#x20;  **- 注册跨线程 Qt 元类型，统一信号槽数据传输格式。**



**2. \*\*NetworkModule\*\***

&#x20;  **- 实现二进制协议编解码：`0xAA55` 帧头、设备 ID、功能码、长度、数据体、CRC-16/MODBUS。**

&#x20;  **- 实现 FSM 拆包器，覆盖粘包、半包、错帧、CRC 错误恢复。**

&#x20;  **- 实现 TCP 客户端/服务端基础类，支持多线程 socket 运行。**



**3. \*\*CNC\_Simulator\*\***

&#x20;  **- 实现 10 路 TCP 服务端端口，默认 `8001\~8010`。**

&#x20;  **- 每台设备以默认 50Hz 生成 X/Y/Z、主轴转速、伺服负载、温度、RUN/STOP/ALARM 状态。**

&#x20;  **- 提供 QtWidgets 界面：启动/停止端口、频率调整、指定设备报警注入。**



**4. \*\*CoreEngine\*\***

&#x20;  **- 建立线程安全设备状态池，统一管理 10 台设备实时状态。**

&#x20;  **- 实现网络数据进入解析队列、解析结果更新状态池、状态变化通知 UI。**

&#x20;  **- 实现参数下发规则：参数范围校验；设备处于 RUN 时拒绝核心参数下发。**



**5. \*\*CNC\_Monitor\*\***

&#x20;  **- 实现 10 路并发 TCP 客户端连接。**

&#x20;  **- 实现断线自动识别与指数退避重连：1s、2s、4s、8s，最大 15s。**

&#x20;  **- 实现主界面：设备列表、连接状态、实时状态表、报警列表。**

&#x20;  **- 实现实时曲线绘制：至少 3 通道，优先用 QCustomPlot；不可用时用 `QPainter` 双缓冲。**

&#x20;  **- 实现工艺参数表：`QTableView` + `QStyledItemDelegate` + Validator。**



**6. \*\*StorageModule\*\***

&#x20;  **- 设计 SQLite 表：实时历史数据、报警日志、参数变更日志。**

&#x20;  **- 实现独立数据库线程。**

&#x20;  **- 实现双缓冲批量写入：满 100 条或 500ms 触发一次事务写入。**

&#x20;  **- 保证 UI 与网络线程不直接执行数据库写入。**



**7. \*\*异步日志系统\*\***

&#x20;  **- 实现线程安全单例日志器。**

&#x20;  **- 提供 `LOG\_INFO`、`LOG\_WARN`、`LOG\_ERROR` 宏。**

&#x20;  **- 日志投递到专用线程，定时写文件。**

&#x20;  **- 支持按天或 10MB 大小分卷归档。**



**## Interfaces**

**- 公共协议类型：**

&#x20; **- `DeviceStatus`**

&#x20; **- `AlarmEvent`**

&#x20; **- `ProcessParameter`**

&#x20; **- `ProtocolFrame`**

&#x20; **- `ConnectionState`**

**- 核心信号：**

&#x20; **- `frameReceived(deviceId, rawBytes)`**

&#x20; **- `statusUpdated(DeviceStatus)`**

&#x20; **- `alarmRaised(AlarmEvent)`**

&#x20; **- `connectionStateChanged(deviceId, state)`**

&#x20; **- `storageBatchReady(records)`**

**- 线程规则：**

&#x20; **- GUI 线程只做交互与绘制。**

&#x20; **- 网络、解析、数据库分别运行在线程中。**

&#x20; **- 跨线程信号默认使用 `Qt::QueuedConnection`。**



**## Test Plan**

**- 协议测试：CRC 正确/错误、粘包、半包、乱序垃圾字节恢复。**

**- Simulator 测试：10 个端口同时启动，50Hz 连续发送。**

**- Monitor 测试：10 路连接、断线重连、5 秒内恢复数据。**

**- UI 测试：3 路曲线同时刷新，界面拖动不卡顿。**

**- 存储测试：10 台设备 20ms 周期写入，验证无明显队列堆积。**

**- 稳定性测试：长时间运行，观察内存曲线、日志归档、数据库增长。**



**## Assumptions**

**- 构建工具采用 `qmake`，不再按 CMake 拆工程。**

**- Qt 版本固定为 Qt 5.12.8。**

**- 数据库使用 SQLite 3。**

**- 初版目标是完整可运行的本机/局域网仿真系统，不接入真实 CNC 设备。**



