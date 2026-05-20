# CNC Studio UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Merge CNC_Simulator and CNC_Monitor into a single CNC_Studio.exe with dark theme, sidebar navigation, card layout, QCustomPlot charts, and color-coded status indicators.

**Architecture:** New `apps/CNC_Studio/` app subdirectory reuses existing `CoreEngine`, `NetworkModule`, `StorageModule`, and updated `AppUI` modules. MainWindow hosts a collapsible sidebar (QStackedWidget switching between MonitorPage and SimulatorPage). PlotWidget wraps QCustomPlot with the same API as the old RealTimePlotWidget.

**Tech Stack:** Qt 5.12.8, C++14, qmake, QCustomPlot 2.x (MIT), Qt Widgets

---

### Task 1: Add QCustomPlot and build PlotWidget in AppUI

**Files:**
- Create: `ThirdParty/qcustomplot.h` (download)
- Create: `ThirdParty/qcustomplot.cpp` (download)
- Create: `AppUI/PlotWidget.h`
- Create: `AppUI/PlotWidget.cpp`
- Create: `AppUI/StatusColors.h`
- Modify: `AppUI/AppUI.pro`

- [ ] **Step 1: Download QCustomPlot source files**

Download `qcustomplot.h` and `qcustomplot.cpp` from https://www.qcustomplot.com/release/2.1.1fixed/QCustomPlot-source.tar.gz and place them in `ThirdParty/`.

```powershell
mkdir -p "D:\qt project\ThirdParty"
# Manual: download and extract to D:\qt project\ThirdParty\qcustomplot.h and qcustomplot.cpp
```

- [ ] **Step 2: Create StatusColors.h helper**

Write `D:\qt project\AppUI\StatusColors.h`:

```cpp
#ifndef DNC_SCADA_STATUS_COLORS_H
#define DNC_SCADA_STATUS_COLORS_H

#include "Types.h"
#include <QColor>
#include <QPainter>
#include <QPixmap>

namespace DncScada {

inline QColor runStateColor(MachineRunState state) {
    switch (state) {
    case MachineRunState::Run:  return QColor("#22c55e");
    case MachineRunState::Alarm: return QColor("#ef4444");
    case MachineRunState::Stop:  return QColor("#8b949e");
    }
    return QColor("#8b949e");
}

inline QColor connectionStateColor(ConnectionState state) {
    switch (state) {
    case ConnectionState::Connected:    return QColor("#22c55e");
    case ConnectionState::Connecting:   return QColor("#eab308");
    case ConnectionState::Reconnecting: return QColor("#eab308");
    case ConnectionState::Disconnected: return QColor("#8b949e");
    }
    return QColor("#8b949e");
}

inline QPixmap statusDotPixmap(const QColor &color, int size = 12) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(1, 1, size - 2, size - 2);
    painter.end();
    return pix;
}

} // namespace DncScada

#endif // DNC_SCADA_STATUS_COLORS_H
```

- [ ] **Step 3: Create PlotWidget.h**

Write `D:\qt project\AppUI\PlotWidget.h`:

```cpp
#ifndef DNC_SCADA_PLOT_WIDGET_H
#define DNC_SCADA_PLOT_WIDGET_H

#include "Types.h"
#include <QWidget>

class QCustomPlot;
class QCPGraph;
class QCPItemTracer;

namespace DncScada {

class PlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlotWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;

public slots:
    void appendStatus(DeviceStatus status);
    void clear();

private:
    struct Sample {
        double load = 0.0;
        double temperature = 0.0;
        double rpm = 0.0;
    };

    QCustomPlot *plot = nullptr;
    QCPGraph *loadGraph = nullptr;
    QCPGraph *tempGraph = nullptr;
    QCPGraph *rpmGraph = nullptr;
    QCPItemTracer *tracer = nullptr;
    QVector<Sample> samples;
    int maxSamples = 300;
    double xIndex = 0.0;
};

} // namespace DncScada

#endif // DNC_SCADA_PLOT_WIDGET_H
```

- [ ] **Step 4: Create PlotWidget.cpp**

Write `D:\qt project\AppUI\PlotWidget.cpp`:

```cpp
#include "PlotWidget.h"
#include "qcustomplot.h"

namespace DncScada {

PlotWidget::PlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(220);

    plot = new QCustomPlot(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot);

    plot->setBackground(QColor("#0d1117"));
    plot->xAxis->setLabelColor(QColor("#8b949e"));
    plot->yAxis->setLabelColor(QColor("#8b949e"));
    plot->xAxis->setTickLabelColor(QColor("#8b949e"));
    plot->yAxis->setTickLabelColor(QColor("#8b949e"));
    plot->xAxis->setBasePen(QPen(QColor("#30363d")));
    plot->yAxis->setBasePen(QPen(QColor("#30363d")));
    plot->xAxis->setTickPen(QPen(QColor("#30363d")));
    plot->yAxis->setTickPen(QPen(QColor("#30363d")));
    plot->xAxis->grid()->setPen(QPen(QColor("#21262d"), 1, Qt::DotLine));
    plot->yAxis->grid()->setPen(QPen(QColor("#21262d"), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridVisible(false);
    plot->yAxis->grid()->setSubGridVisible(false);

    plot->xAxis->setRange(0, maxSamples);
    plot->yAxis->setRange(0, 100);
    plot->xAxis->setLabel(QStringLiteral("Samples"));
    plot->yAxis->setLabel(QStringLiteral("Value"));

    loadGraph = plot->addGraph();
    loadGraph->setPen(QPen(QColor("#3b82f6"), 2));
    loadGraph->setName(QStringLiteral("Load"));

    tempGraph = plot->addGraph();
    tempGraph->setPen(QPen(QColor("#22c55e"), 2));
    tempGraph->setName(QStringLiteral("Temp"));

    rpmGraph = plot->addGraph();
    rpmGraph->setPen(QPen(QColor("#ef4444"), 2));
    rpmGraph->setName(QStringLiteral("RPM/60"));

    plot->legend->setVisible(true);
    plot->legend->setBrush(QBrush(QColor(22, 27, 34, 200)));
    plot->legend->setTextColor(QColor("#e6edf3"));
    plot->legend->setBorderPen(QPen(QColor("#30363d")));
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    tracer = new QCPItemTracer(plot);
    tracer->setVisible(false);
    tracer->setStyle(QCPItemTracer::tsCircle);
    tracer->setPen(QPen(QColor("#e6edf3")));
    tracer->setBrush(QColor("#3b82f6"));
    tracer->setSize(7);

    connect(plot, &QCustomPlot::mouseMove, this, [this](QMouseEvent *) {
        double x = plot->xAxis->pixelToCoord(plot->mapFromGlobal(QCursor::pos()).x());
        int idx = qRound(x);
        if (idx >= 0 && idx < samples.size()) {
            const Sample &s = samples.at(idx);
            tracer->setVisible(true);
            tracer->setGraph(loadGraph);
            tracer->setGraphKey(idx);
            tracer->updatePosition();
            QToolTip::showText(QCursor::pos(),
                QStringLiteral("Load: %1  Temp: %2  RPM/60: %3")
                    .arg(s.load, 0, 'f', 1)
                    .arg(s.temperature, 0, 'f', 1)
                    .arg(s.rpm, 0, 'f', 1),
                this);
        } else {
            tracer->setVisible(false);
            QToolTip::hideText();
        }
    });
}

QSize PlotWidget::minimumSizeHint() const {
    return QSize(480, 220);
}

void PlotWidget::appendStatus(DeviceStatus status) {
    Sample sample;
    sample.load = status.servoLoad;
    sample.temperature = status.temperature;
    sample.rpm = status.spindleRpm / 60.0;

    samples.append(sample);
    while (samples.size() > maxSamples) {
        samples.removeFirst();
    }

    loadGraph->clearData();
    tempGraph->clearData();
    rpmGraph->clearData();

    for (int i = 0; i < samples.size(); ++i) {
        const Sample &s = samples.at(i);
        loadGraph->addData(i, qBound(0.0, s.load, 100.0));
        tempGraph->addData(i, qBound(0.0, s.temperature, 100.0));
        rpmGraph->addData(i, qBound(0.0, s.rpm, 100.0));
    }

    if (samples.size() > 1) {
        plot->xAxis->setRange(qMax(0.0, samples.size() - maxSamples), samples.size() + 10);
    }

    plot->replot();
}

void PlotWidget::clear() {
    samples.clear();
    loadGraph->clearData();
    tempGraph->clearData();
    rpmGraph->clearData();
    plot->replot();
}

} // namespace DncScada
```

- [ ] **Step 5: Update AppUI.pro**

Modify `D:\qt project\AppUI\AppUI.pro` to add PlotWidget and QCustomPlot:

```
QT += core widgets printsupport
CONFIG += staticlib c++14 warn_on
TEMPLATE = lib
TARGET = AppUI

DESTDIR = ../bin
OBJECTS_DIR = ../build/AppUI/obj
MOC_DIR = ../build/AppUI/moc
RCC_DIR = ../build/AppUI/rcc

INCLUDEPATH += $$PWD ../Common ../ThirdParty
LIBS += -L../bin -lCommon

HEADERS += \
    RealTimePlotWidget.h \
    ParameterTableModel.h \
    NumericDelegate.h \
    PlotWidget.h \
    StatusColors.h

SOURCES += \
    RealTimePlotWidget.cpp \
    ParameterTableModel.cpp \
    NumericDelegate.cpp \
    PlotWidget.cpp \
    ../ThirdParty/qcustomplot.cpp
```

- [ ] **Step 6: Commit**

```bash
git add ThirdParty/ AppUI/PlotWidget.h AppUI/PlotWidget.cpp AppUI/StatusColors.h AppUI/AppUI.pro
git commit -m "feat: add QCustomPlot, PlotWidget, and StatusColors to AppUI"
```

---

### Task 2: Update ParameterTableModel and NumericDelegate

**Files:**
- Modify: `AppUI/ParameterTableModel.h`
- Modify: `AppUI/ParameterTableModel.cpp`
- Modify: `AppUI/NumericDelegate.cpp`

- [ ] **Step 1: Update ParameterTableModel.h — remove Device column**

Modify the `columnCount` from 6 to 5 and adjust column indices. All changes are in the .cpp — the header stays the same except for a comment change.

- [ ] **Step 2: Update ParameterTableModel.cpp — shift column indices**

Modify `D:\qt project\AppUI\ParameterTableModel.cpp`:

Change `headerData` — remove "Device" from headers, keep 5 columns:

```cpp
QVariant ParameterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    static const QStringList headers = {
        QStringLiteral("Name"),
        QStringLiteral("Value"),
        QStringLiteral("Min"),
        QStringLiteral("Max"),
        QStringLiteral("Type")
    };
    return headers.value(section);
}
```

Change `columnCount` return value:

```cpp
int ParameterTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 5;
}
```

Change `data()` — remap columns, remove Device column:

```cpp
QVariant ParameterTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= parameters.size()) {
        return QVariant();
    }

    const ProcessParameter &parameter = parameters.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return parameter.name;
        case 1: return parameter.value;
        case 2: return parameter.minimum;
        case 3: return parameter.maximum;
        case 4: return parameter.coreParameter ? QStringLiteral("Core") : QStringLiteral("Runtime");
        default: return QVariant();
        }
    }
    return QVariant();
}
```

Change `flags()` — only Value (column 1) is editable now:

```cpp
Qt::ItemFlags ParameterTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == 1) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}
```

Change `setData()` — remove Device column case:

```cpp
bool ParameterTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.row() >= parameters.size()) {
        return false;
    }

    ProcessParameter &parameter = parameters[index.row()];
    if (index.column() == 1) {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || number < parameter.minimum || number > parameter.maximum) {
            return false;
        }
        parameter.value = number;
    } else {
        return false;
    }

    emit dataChanged(index, index, {role, Qt::DisplayRole});
    return true;
}
```

Change `setDeviceId()` — dataChanged signal uses 5 columns now:

```cpp
void ParameterTableModel::setDeviceId(quint8 deviceId)
{
    for (ProcessParameter &parameter : parameters) {
        parameter.deviceId = deviceId;
    }
    // No need to emit dataChanged since Device is no longer a visible column
}
```

- [ ] **Step 3: Update NumericDelegate.cpp — remove device column editor**

Modify `D:\qt project\AppUI\NumericDelegate.cpp`:

```cpp
#include "NumericDelegate.h"
#include <QDoubleSpinBox>

namespace DncScada {

NumericDelegate::NumericDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *NumericDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(option)
    if (index.column() == 1) {
        QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
        editor->setDecimals(2);
        editor->setRange(0.0, 12000.0);
        editor->setSingleStep(10.0);
        return editor;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

} // namespace DncScada
```

- [ ] **Step 4: Commit**

```bash
git add AppUI/ParameterTableModel.h AppUI/ParameterTableModel.cpp AppUI/NumericDelegate.cpp
git commit -m "feat: remove Device column from ParameterTableModel, update NumericDelegate"
```

---

### Task 3: Create CNC_Studio project scaffolding

**Files:**
- Create: `apps/CNC_Studio/CNC_Studio.pro`
- Create: `apps/CNC_Studio/main.cpp`
- Create: `apps/CNC_Studio/theme/style.qss`

- [ ] **Step 1: Create directory and CNC_Studio.pro**

```powershell
mkdir -p "D:\qt project\apps\CNC_Studio\theme"
```

Write `D:\qt project\apps\CNC_Studio\CNC_Studio.pro`:

```
QT += core widgets network sql printsupport
CONFIG += c++14 warn_on
TEMPLATE = app
TARGET = CNC_Studio

DESTDIR = ../../bin
OBJECTS_DIR = ../../build/CNC_Studio/obj
MOC_DIR = ../../build/CNC_Studio/moc
RCC_DIR = ../../build/CNC_Studio/rcc

INCLUDEPATH += $$PWD ../../Common ../../NetworkModule ../../StorageModule ../../CoreEngine ../../AppUI ../../ThirdParty
LIBS += -L../../bin -lCoreEngine -lStorageModule -lNetworkModule -lAppUI -lCommon

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    MonitorPage.cpp \
    SimulatorPage.cpp \
    ../../ThirdParty/qcustomplot.cpp

HEADERS += \
    MainWindow.h \
    MonitorPage.h \
    SimulatorPage.h
```

- [ ] **Step 2: Create main.cpp**

Write `D:\qt project\apps\CNC_Studio\main.cpp`:

```cpp
#include "MainWindow.h"
#include "Types.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DncScada::registerMetaTypes();

    DncScada::MainWindow window;
    window.resize(1280, 820);
    window.show();

    return app.exec();
}
```

- [ ] **Step 3: Create style.qss**

Write `D:\qt project\apps\CNC_Studio\theme\style.qss`:

```css
/* ===== Global ===== */
QMainWindow {
    background-color: #0d1117;
}
QWidget {
    background-color: #0d1117;
    color: #e6edf3;
    font-family: "Segoe UI", sans-serif;
    font-size: 12px;
}

/* ===== Buttons ===== */
QPushButton {
    background-color: #21262d;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px 16px;
    min-height: 20px;
}
QPushButton:hover {
    background-color: #30363d;
}
QPushButton:pressed {
    background-color: #161b22;
}
QPushButton:disabled {
    color: #484f58;
    background-color: #161b22;
}

/* Primary button (blue) */
QPushButton[cssClass="primary"] {
    background-color: #3b82f6;
    color: #ffffff;
    border: 1px solid #3b82f6;
}
QPushButton[cssClass="primary"]:hover {
    background-color: #2563eb;
}

/* Success button (green) */
QPushButton[cssClass="success"] {
    background-color: #22c55e;
    color: #ffffff;
    border: 1px solid #22c55e;
}
QPushButton[cssClass="success"]:hover {
    background-color: #16a34a;
}

/* Danger button (red) */
QPushButton[cssClass="danger"] {
    background-color: #ef4444;
    color: #ffffff;
    border: 1px solid #ef4444;
}
QPushButton[cssClass="danger"]:hover {
    background-color: #dc2626;
}

/* Ghost danger (transparent bg, red border) */
QPushButton[cssClass="ghost-danger"] {
    background-color: transparent;
    color: #ef4444;
    border: 1px solid #ef4444;
}
QPushButton[cssClass="ghost-danger"]:hover {
    background-color: rgba(239, 68, 68, 0.15);
}

/* ===== Input Controls ===== */
QLineEdit {
    background-color: #0d1117;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 4px 8px;
    selection-background-color: #3b82f6;
}
QLineEdit:focus {
    border-color: #3b82f6;
}

QSpinBox, QDoubleSpinBox {
    background-color: #0d1117;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 4px 8px;
}
QSpinBox:focus, QDoubleSpinBox:focus {
    border-color: #3b82f6;
}
QSpinBox::up-button, QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right;
    width: 20px;
    border-left: 1px solid #30363d;
    border-bottom: 1px solid #30363d;
    border-top-right-radius: 4px;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right;
    width: 20px;
    border-left: 1px solid #30363d;
    border-bottom-right-radius: 4px;
}

QComboBox {
    background-color: #0d1117;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 4px 8px;
}
QComboBox:focus {
    border-color: #3b82f6;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: 1px solid #30363d;
}
QComboBox QAbstractItemView {
    background-color: #161b22;
    color: #e6edf3;
    border: 1px solid #30363d;
    selection-background-color: #3b82f6;
}

/* ===== Tables ===== */
QTableWidget, QTableView {
    background-color: #161b22;
    color: #e6edf3;
    border: 1px solid #30363d;
    gridline-color: #30363d;
    selection-background-color: rgba(59, 130, 246, 0.25);
    selection-color: #e6edf3;
    alternate-background-color: #1a1a2e;
}
QTableWidget::item, QTableView::item {
    padding: 4px 8px;
}
QHeaderView::section {
    background-color: #0d1117;
    color: #8b949e;
    border: none;
    border-bottom: 1px solid #30363d;
    border-right: 1px solid #30363d;
    padding: 6px 8px;
    font-weight: bold;
}

/* ===== List Widget ===== */
QListWidget {
    background-color: #161b22;
    color: #e6edf3;
    border: 1px solid #30363d;
    outline: none;
}
QListWidget::item {
    padding: 6px 8px;
    border-bottom: 1px solid #30363d;
}
QListWidget::item:selected {
    background-color: rgba(59, 130, 246, 0.25);
}

/* ===== Text Edit ===== */
QTextEdit {
    background-color: #0d1117;
    color: #c9d1d9;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 8px;
    font-family: "Consolas", "SF Mono", monospace;
    font-size: 11px;
}

/* ===== Scrollbars ===== */
QScrollBar:vertical {
    background: #0d1117;
    width: 10px;
    border-radius: 5px;
}
QScrollBar::handle:vertical {
    background: #30363d;
    border-radius: 5px;
    min-height: 30px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: #0d1117;
    height: 10px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background: #30363d;
    border-radius: 5px;
    min-width: 30px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

/* ===== Labels ===== */
QLabel {
    background: transparent;
    color: #e6edf3;
}
QLabel[cssClass="subtitle"] {
    color: #8b949e;
    font-size: 11px;
}

/* ===== Splitters ===== */
QSplitter::handle {
    background-color: #30363d;
}
QSplitter::handle:vertical {
    height: 1px;
}
QSplitter::handle:horizontal {
    width: 1px;
}

/* ===== Card container (set via property) ===== */
QWidget[cssClass="card"] {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 8px;
}

/* ===== Title bar ===== */
QWidget[cssClass="titleBar"] {
    background-color: #1a1a2e;
}

/* ===== Status bar ===== */
QWidget[cssClass="statusBar"] {
    background-color: #161b22;
    border-top: 1px solid #30363d;
}

/* ===== Sidebar ===== */
QWidget[cssClass="sidebar"] {
    background-color: #161b22;
    border-right: 1px solid #30363d;
}

QPushButton[cssClass="navBtn"] {
    background-color: transparent;
    color: #8b949e;
    border: none;
    border-radius: 0;
    padding: 12px 8px;
    text-align: left;
}
QPushButton[cssClass="navBtn"]:hover {
    background-color: #21262d;
    color: #e6edf3;
}
QPushButton[cssClass="navBtn"][selected="true"] {
    background-color: rgba(59, 130, 246, 0.15);
    color: #3b82f6;
    border-left: 3px solid #3b82f6;
}
```

- [ ] **Step 4: Commit**

```bash
git add apps/CNC_Studio/CNC_Studio.pro apps/CNC_Studio/main.cpp apps/CNC_Studio/theme/style.qss
git commit -m "feat: add CNC_Studio project scaffolding with dark theme QSS"
```

---

### Task 4: Create MainWindow shell

**Files:**
- Create: `apps/CNC_Studio/MainWindow.h`
- Create: `apps/CNC_Studio/MainWindow.cpp`

- [ ] **Step 1: Create MainWindow.h**

Write `D:\qt project\apps\CNC_Studio\MainWindow.h`:

```cpp
#ifndef DNC_SCADA_MAIN_WINDOW_H
#define DNC_SCADA_MAIN_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QPropertyAnimation>

namespace DncScada {

class MonitorPage;
class SimulatorPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void setSimulatorRunning(bool running);
    void setStatusMessage(const QString &message);

private:
    void buildUi();
    void switchPage(int index);

    QStackedWidget *stack = nullptr;
    MonitorPage *monitorPage = nullptr;
    SimulatorPage *simulatorPage = nullptr;

    QWidget *sidebar = nullptr;
    QPushButton *monitorNavBtn = nullptr;
    QPushButton *simulatorNavBtn = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *simStatusLabel = nullptr;
    QLabel *statusLabel = nullptr;

    QPropertyAnimation *sidebarAnim = nullptr;
    int sidebarCollapsedWidth = 56;
    int sidebarExpandedWidth = 200;
    bool sidebarExpanded = false;
};

} // namespace DncScada

#endif // DNC_SCADA_MAIN_WINDOW_H
```

- [ ] **Step 2: Create MainWindow.cpp**

Write `D:\qt project\apps\CNC_Studio\MainWindow.cpp`:

```cpp
#include "MainWindow.h"
#include "MonitorPage.h"
#include "SimulatorPage.h"

#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace DncScada {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    switchPage(0);
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("CNC Studio"));
    setMinimumSize(960, 600);

    // Apply style.qss
    QFile styleFile(QApplication::applicationDirPath() + QStringLiteral("/../apps/CNC_Studio/theme/style.qss"));
    if (!styleFile.exists()) {
        styleFile.setFileName(QStringLiteral("apps/CNC_Studio/theme/style.qss"));
    }
    if (styleFile.open(QFile::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    }

    QWidget *root = new QWidget(this);
    setCentralWidget(root);
    QVBoxLayout *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // === Title Bar ===
    QWidget *titleBar = new QWidget(root);
    titleBar->setProperty("cssClass", "titleBar");
    titleBar->setFixedHeight(40);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(16, 0, 16, 0);

    QPushButton *hamburgerBtn = new QPushButton(QStringLiteral("☰"), titleBar);
    hamburgerBtn->setProperty("cssClass", "navBtn");
    hamburgerBtn->setFixedSize(32, 32);
    hamburgerBtn->setCursor(Qt::PointingHandCursor);
    connect(hamburgerBtn, &QPushButton::clicked, this, [this]() {
        sidebarExpanded = !sidebarExpanded;
        int targetWidth = sidebarExpanded ? sidebarExpandedWidth : sidebarCollapsedWidth;
        sidebar->setMaximumWidth(targetWidth);
        sidebar->setMinimumWidth(targetWidth);
    });
    titleLayout->addWidget(hamburgerBtn);

    titleLabel = new QLabel(QStringLiteral("CNC Studio"), titleBar);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #e6edf3; background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    simStatusLabel = new QLabel(QStringLiteral("● Sim Idle"), titleBar);
    simStatusLabel->setStyleSheet("color: #8b949e; background: transparent; font-size: 11px;");
    titleLayout->addWidget(simStatusLabel);

    rootLayout->addWidget(titleBar);

    // === Body (sidebar + content) ===
    QHBoxLayout *bodyLayout = new QHBoxLayout;
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Sidebar
    sidebar = new QWidget(root);
    sidebar->setProperty("cssClass", "sidebar");
    sidebar->setFixedWidth(sidebarCollapsedWidth);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 8, 0, 8);
    sidebarLayout->setSpacing(2);

    monitorNavBtn = new QPushButton(QStringLiteral("  ■  Monitor"), sidebar);
    monitorNavBtn->setProperty("cssClass", "navBtn");
    monitorNavBtn->setCursor(Qt::PointingHandCursor);
    monitorNavBtn->setToolTip(QStringLiteral("Monitor"));
    connect(monitorNavBtn, &QPushButton::clicked, this, [this]() { switchPage(0); });
    sidebarLayout->addWidget(monitorNavBtn);

    simulatorNavBtn = new QPushButton(QStringLiteral("  ⚙  Simulator"), sidebar);
    simulatorNavBtn->setProperty("cssClass", "navBtn");
    simulatorNavBtn->setCursor(Qt::PointingHandCursor);
    simulatorNavBtn->setToolTip(QStringLiteral("Simulator"));
    connect(simulatorNavBtn, &QPushButton::clicked, this, [this]() { switchPage(1); });
    sidebarLayout->addWidget(simulatorNavBtn);

    sidebarLayout->addStretch();
    bodyLayout->addWidget(sidebar);

    // Separator line
    QFrame *sep = new QFrame(root);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #30363d;");
    bodyLayout->addWidget(sep);

    // Content stack
    stack = new QStackedWidget(root);
    monitorPage = new MonitorPage(stack);
    simulatorPage = new SimulatorPage(stack);

    // Forward simulator running state to title bar
    connect(simulatorPage, &SimulatorPage::simulatorRunningChanged, this, &MainWindow::setSimulatorRunning);

    stack->addWidget(monitorPage);
    stack->addWidget(simulatorPage);
    bodyLayout->addWidget(stack, 1);

    rootLayout->addLayout(bodyLayout, 1);

    // === Status Bar ===
    QWidget *statusBarWidget = new QWidget(root);
    statusBarWidget->setProperty("cssClass", "statusBar");
    statusBarWidget->setFixedHeight(28);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusBarWidget);
    statusLayout->setContentsMargins(16, 2, 16, 2);

    statusLabel = new QLabel(QStringLiteral("Ready."), statusBarWidget);
    statusLabel->setStyleSheet("color: #8899aa; background: transparent; font-size: 11px;");
    statusLayout->addWidget(statusLabel);

    rootLayout->addWidget(statusBarWidget);
}

void MainWindow::switchPage(int index)
{
    stack->setCurrentIndex(index);
    monitorNavBtn->setProperty("selected", index == 0);
    simulatorNavBtn->setProperty("selected", index == 1);
    // Force style refresh
    monitorNavBtn->style()->unpolish(monitorNavBtn);
    monitorNavBtn->style()->polish(monitorNavBtn);
    simulatorNavBtn->style()->unpolish(simulatorNavBtn);
    simulatorNavBtn->style()->polish(simulatorNavBtn);
}

void MainWindow::setSimulatorRunning(bool running)
{
    if (running) {
        simStatusLabel->setText(QStringLiteral("● Sim Running"));
        simStatusLabel->setStyleSheet("color: #22c55e; background: transparent; font-size: 11px;");
    } else {
        simStatusLabel->setText(QStringLiteral("● Sim Idle"));
        simStatusLabel->setStyleSheet("color: #8b949e; background: transparent; font-size: 11px;");
    }
}

void MainWindow::setStatusMessage(const QString &message)
{
    statusLabel->setText(message);
}

} // namespace DncScada
```

- [ ] **Step 3: Commit**

```bash
git add apps/CNC_Studio/MainWindow.h apps/CNC_Studio/MainWindow.cpp
git commit -m "feat: add MainWindow shell with sidebar navigation and title bar"
```

---

### Task 5: Create MonitorPage

**Files:**
- Create: `apps/CNC_Studio/MonitorPage.h`
- Create: `apps/CNC_Studio/MonitorPage.cpp`

- [ ] **Step 1: Create MonitorPage.h**

Write `D:\qt project\apps\CNC_Studio\MonitorPage.h`:

```cpp
#ifndef DNC_SCADA_MONITOR_PAGE_H
#define DNC_SCADA_MONITOR_PAGE_H

#include "MonitorEngine.h"
#include "ParameterTableModel.h"
#include "PlotWidget.h"
#include "StatusColors.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTableWidget>
#include <QWidget>

namespace DncScada {

class MonitorPage : public QWidget {
    Q_OBJECT

public:
    explicit MonitorPage(QWidget *parent = nullptr);

private slots:
    void startMonitor();
    void stopMonitor();
    void onStatusUpdated(DeviceStatus status);
    void onConnectionStateChanged(quint8 deviceId, ConnectionState state);
    void onAlarmRaised(AlarmEvent alarm);
    void sendSelectedParameter();
    void onDeviceComboChanged(int index);

private:
    void buildUi();
    void ensureStatusRow(quint8 deviceId);
    QWidget *createDeviceCard(quint8 deviceId);
    void updateDeviceCard(quint8 deviceId, ConnectionState state);
    QWidget *createCard(const QString &title, QWidget *content);

    MonitorEngine *engine = nullptr;
    QLineEdit *hostEdit = nullptr;
    QSpinBox *firstPortSpin = nullptr;
    QSpinBox *deviceCountSpin = nullptr;
    QTableWidget *statusTable = nullptr;
    QListWidget *alarmList = nullptr;
    PlotWidget *plot = nullptr;
    ParameterTableModel *parameterModel = nullptr;
    QTableView *parameterView = nullptr;
    QComboBox *deviceCombo = nullptr;
    QLabel *messageLabel = nullptr;
    QLabel *alarmPlaceholder = nullptr;
    QVector<QWidget*> deviceCards;
    QWidget *deviceCardsContainer = nullptr;
};

} // namespace DncScada

#endif // DNC_SCADA_MONITOR_PAGE_H
```

- [ ] **Step 2: Create MonitorPage.cpp**

Write `D:\qt project\apps\CNC_Studio\MonitorPage.cpp`:

```cpp
#include "MonitorPage.h"
#include "NumericDelegate.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>

namespace DncScada {

MonitorPage::MonitorPage(QWidget *parent)
    : QWidget(parent)
    , engine(new MonitorEngine(this))
{
    buildUi();
    connect(engine, &MonitorEngine::statusUpdated, this, &MonitorPage::onStatusUpdated);
    connect(engine, &MonitorEngine::connectionStateChanged, this, &MonitorPage::onConnectionStateChanged);
    connect(engine, &MonitorEngine::alarmRaised, this, &MonitorPage::onAlarmRaised);
    connect(engine, &MonitorEngine::parameterRejected, this, [this](quint8 deviceId, const QString &reason) {
        messageLabel->setText(QStringLiteral("D%1 rejected: %2").arg(deviceId).arg(reason));
    });
    connect(engine, &MonitorEngine::parameterAccepted, this, [this](const ProcessParameter &parameter) {
        messageLabel->setText(QStringLiteral("Sent D%1 %2=%3")
            .arg(parameter.deviceId).arg(parameter.name).arg(parameter.value));
    });
    connect(engine, &MonitorEngine::storageError, this, [this](const QString &msg) {
        messageLabel->setText(QStringLiteral("Storage error: %1").arg(msg));
    });
}

QWidget *MonitorPage::createCard(const QString &title, QWidget *content)
{
    QWidget *card = new QWidget(this);
    card->setProperty("cssClass", "card");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(8);

    if (!title.isEmpty()) {
        QLabel *titleLabel = new QLabel(title, card);
        QFont f = titleLabel->font();
        f.setBold(true);
        f.setPointSize(11);
        titleLabel->setFont(f);
        titleLabel->setStyleSheet("color: #8b949e; background: transparent;");
        cardLayout->addWidget(titleLabel);
    }

    if (content) {
        cardLayout->addWidget(content, 1);
    }
    return card;
}

void MonitorPage::buildUi()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 12, 16, 12);
    rootLayout->setSpacing(12);

    // === Top toolbar card ===
    QWidget *toolbarCard = new QWidget(this);
    toolbarCard->setProperty("cssClass", "card");
    QGraphicsDropShadowEffect *tbShadow = new QGraphicsDropShadowEffect(toolbarCard);
    tbShadow->setBlurRadius(12);
    tbShadow->setColor(QColor(0, 0, 0, 100));
    tbShadow->setOffset(0, 4);
    toolbarCard->setGraphicsEffect(tbShadow);
    QHBoxLayout *tbLayout = new QHBoxLayout(toolbarCard);
    tbLayout->setContentsMargins(16, 10, 16, 10);
    tbLayout->setSpacing(10);

    // Device cards container (left side)
    deviceCardsContainer = new QWidget(toolbarCard);
    deviceCardsContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *cardsLayout = new QHBoxLayout(deviceCardsContainer);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(6);
    tbLayout->addWidget(deviceCardsContainer);
    tbLayout->addStretch();

    // Controls (right side)
    hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), toolbarCard);
    hostEdit->setFixedWidth(120);
    tbLayout->addWidget(new QLabel(QStringLiteral("Host"), toolbarCard));
    tbLayout->addWidget(hostEdit);

    firstPortSpin = new QSpinBox(toolbarCard);
    firstPortSpin->setRange(1, 65500);
    firstPortSpin->setValue(8001);
    firstPortSpin->setFixedWidth(80);
    tbLayout->addWidget(new QLabel(QStringLiteral("Port"), toolbarCard));
    tbLayout->addWidget(firstPortSpin);

    deviceCountSpin = new QSpinBox(toolbarCard);
    deviceCountSpin->setRange(1, 10);
    deviceCountSpin->setValue(10);
    deviceCountSpin->setFixedWidth(60);
    tbLayout->addWidget(new QLabel(QStringLiteral("Devices"), toolbarCard));
    tbLayout->addWidget(deviceCountSpin);

    QPushButton *connectBtn = new QPushButton(QStringLiteral("Connect"), toolbarCard);
    connectBtn->setProperty("cssClass", "primary");
    connect(connectBtn, &QPushButton::clicked, this, &MonitorPage::startMonitor);
    tbLayout->addWidget(connectBtn);

    QPushButton *stopBtn = new QPushButton(QStringLiteral("Stop"), toolbarCard);
    stopBtn->setProperty("cssClass", "danger");
    connect(stopBtn, &QPushButton::clicked, this, &MonitorPage::stopMonitor);
    tbLayout->addWidget(stopBtn);

    rootLayout->addWidget(toolbarCard);

    // === Middle row: Chart (60%) + Alarms (40%) ===
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->setSpacing(12);

    plot = new PlotWidget(this);
    QWidget *chartCard = createCard(QStringLiteral("REAL-TIME CHART"), plot);
    midLayout->addWidget(chartCard, 3);

    alarmList = new QListWidget(this);
    alarmList->setAlternatingRowColors(false);
    alarmPlaceholder = new QLabel(QStringLiteral("No active alarms"), this);
    alarmPlaceholder->setStyleSheet("color: #8b949e; background: transparent;");
    alarmPlaceholder->setAlignment(Qt::AlignCenter);
    QVBoxLayout *alarmLayout = new QVBoxLayout;
    alarmLayout->addWidget(alarmPlaceholder);
    alarmLayout->addWidget(alarmList);
    QWidget *alarmContent = new QWidget(this);
    alarmContent->setLayout(alarmLayout);
    alarmContent->setStyleSheet("background: transparent;");
    QWidget *alarmCard = createCard(QStringLiteral("ALARMS"), alarmContent);
    midLayout->addWidget(alarmCard, 2);

    rootLayout->addLayout(midLayout, 1);

    // === Bottom row: Status Table (60%) + Parameters (40%) ===
    QHBoxLayout *botLayout = new QHBoxLayout;
    botLayout->setSpacing(12);

    statusTable = new QTableWidget(this);
    statusTable->setColumnCount(8);
    statusTable->setHorizontalHeaderLabels({
        QStringLiteral("Device"), QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z"),
        QStringLiteral("RPM"), QStringLiteral("Load"), QStringLiteral("Temp"), QStringLiteral("State")
    });
    statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    statusTable->setAlternatingRowColors(true);
    statusTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    statusTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QWidget *statusCard = createCard(QStringLiteral("LIVE STATUS"), statusTable);
    botLayout->addWidget(statusCard, 3);

    QWidget *paramContent = new QWidget(this);
    paramContent->setStyleSheet("background: transparent;");
    QVBoxLayout *paramLayout = new QVBoxLayout(paramContent);
    paramLayout->setContentsMargins(0, 0, 0, 0);
    paramLayout->setSpacing(8);

    QHBoxLayout *deviceSelectLayout = new QHBoxLayout;
    deviceSelectLayout->addWidget(new QLabel(QStringLiteral("Target Device"), paramContent));
    deviceCombo = new QComboBox(paramContent);
    for (int i = 1; i <= 10; ++i) {
        deviceCombo->addItem(QStringLiteral("Device %1").arg(i), i);
    }
    connect(deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MonitorPage::onDeviceComboChanged);
    deviceSelectLayout->addWidget(deviceCombo);
    deviceSelectLayout->addStretch();
    paramLayout->addLayout(deviceSelectLayout);

    parameterModel = new ParameterTableModel(this);
    parameterView = new QTableView(paramContent);
    parameterView->setModel(parameterModel);
    parameterView->setItemDelegate(new NumericDelegate(parameterView));
    parameterView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    parameterView->setAlternatingRowColors(true);
    parameterView->setSelectionBehavior(QAbstractItemView::SelectRows);
    paramLayout->addWidget(parameterView, 1);

    QPushButton *sendBtn = new QPushButton(QStringLiteral("Send Parameter"), paramContent);
    sendBtn->setProperty("cssClass", "primary");
    connect(sendBtn, &QPushButton::clicked, this, &MonitorPage::sendSelectedParameter);
    paramLayout->addWidget(sendBtn);

    QWidget *paramCard = createCard(QStringLiteral("PARAMETERS"), paramContent);
    botLayout->addWidget(paramCard, 2);

    rootLayout->addLayout(botLayout, 1);

    // Status message
    messageLabel = new QLabel(this);
    messageLabel->setStyleSheet("color: #8b949e; background: transparent; font-size: 11px;");
    rootLayout->addWidget(messageLabel);
}

QWidget *MonitorPage::createDeviceCard(quint8 deviceId)
{
    QWidget *card = new QWidget(deviceCardsContainer);
    card->setFixedSize(80, 48);
    card->setStyleSheet(QStringLiteral(
        "background-color: #0d1117; border: 1px solid #30363d; border-radius: 6px;"));

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(2);

    QLabel *dot = new QLabel(card);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QStringLiteral("background-color: #8b949e; border-radius: 4px;"));
    dot->setObjectName(QStringLiteral("dot_%1").arg(deviceId));
    layout->addWidget(dot);

    QLabel *label = new QLabel(QStringLiteral("D%1").arg(deviceId), card);
    label->setStyleSheet("color: #e6edf3; background: transparent; font-size: 11px;");
    layout->addWidget(label);

    return card;
}

void MonitorPage::updateDeviceCard(quint8 deviceId, ConnectionState state)
{
    if (deviceId < 1 || deviceId > deviceCards.size()) return;
    QWidget *card = deviceCards.at(deviceId - 1);
    QLabel *dot = card->findChild<QLabel*>(QStringLiteral("dot_%1").arg(deviceId));
    if (dot) {
        QColor color = connectionStateColor(state);
        dot->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 4px;").arg(color.name()));
    }
}

void MonitorPage::startMonitor()
{
    const int count = deviceCountSpin->value();
    statusTable->setRowCount(0);
    alarmList->clear();
    alarmPlaceholder->setVisible(true);
    plot->clear();

    // Clear old device cards
    for (QWidget *card : deviceCards) {
        card->deleteLater();
    }
    deviceCards.clear();

    // Create new device cards
    for (int i = 0; i < count; ++i) {
        QWidget *card = createDeviceCard(static_cast<quint8>(i + 1));
        deviceCards.append(card);
        deviceCardsContainer->layout()->addWidget(card);
    }

    QDir dataDir(QApplication::applicationDirPath());
    dataDir.mkpath(QStringLiteral("data"));
    const QString dbPath = dataDir.filePath(QStringLiteral("data/monitor.sqlite"));
    engine->startMonitoring(hostEdit->text(), static_cast<quint16>(firstPortSpin->value()),
                            count, dbPath);
}

void MonitorPage::stopMonitor()
{
    engine->stopMonitoring();
    messageLabel->setText(QStringLiteral("Stopped."));
}

void MonitorPage::onStatusUpdated(DeviceStatus status)
{
    ensureStatusRow(status.deviceId);
    const int row = status.deviceId - 1;

    // Device column with colored dot
    QTableWidgetItem *devItem = statusTable->item(row, 0);
    if (!devItem) {
        devItem = new QTableWidgetItem;
        statusTable->setItem(row, 0, devItem);
    }
    devItem->setIcon(QIcon(statusDotPixmap(runStateColor(status.runState))));
    devItem->setText(QString::number(status.deviceId));

    // Data columns
    auto setCell = [&](int col, const QString &text) {
        QTableWidgetItem *item = statusTable->item(row, col);
        if (!item) {
            item = new QTableWidgetItem;
            statusTable->setItem(row, col, item);
        }
        item->setText(text);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    };

    setCell(1, QString::number(status.x, 'f', 2));
    setCell(2, QString::number(status.y, 'f', 2));
    setCell(3, QString::number(status.z, 'f', 2));
    setCell(4, QString::number(status.spindleRpm));
    setCell(5, QString::number(status.servoLoad, 'f', 1));
    setCell(6, QString::number(status.temperature, 'f', 1));

    // State column with colored dot
    QTableWidgetItem *stateItem = statusTable->item(row, 7);
    if (!stateItem) {
        stateItem = new QTableWidgetItem;
        statusTable->setItem(row, 7, stateItem);
    }
    stateItem->setIcon(QIcon(statusDotPixmap(runStateColor(status.runState))));
    stateItem->setText(runStateToString(status.runState));

    plot->appendStatus(status);
}

void MonitorPage::onConnectionStateChanged(quint8 deviceId, ConnectionState state)
{
    updateDeviceCard(deviceId, state);
}

void MonitorPage::onAlarmRaised(AlarmEvent alarm)
{
    alarmPlaceholder->setVisible(false);
    alarmList->insertItem(0, QStringLiteral("⚠ %1  D%2  code=%3  %4")
        .arg(alarm.timestamp.toLocalTime().toString(QStringLiteral("hh:mm:ss.zzz")))
        .arg(alarm.deviceId)
        .arg(alarm.code)
        .arg(alarm.message));

    // Keep max 100 items
    while (alarmList->count() > 100) {
        delete alarmList->takeItem(alarmList->count() - 1);
    }
}

void MonitorPage::sendSelectedParameter()
{
    const QModelIndex index = parameterView->currentIndex();
    const int row = index.isValid() ? index.row() : 0;
    engine->sendParameter(parameterModel->parameterAt(row));
}

void MonitorPage::onDeviceComboChanged(int index)
{
    const int deviceId = deviceCombo->itemData(index).toInt();
    parameterModel->setDeviceId(static_cast<quint8>(deviceId));
}

void MonitorPage::ensureStatusRow(quint8 deviceId)
{
    while (statusTable->rowCount() < deviceId) {
        statusTable->insertRow(statusTable->rowCount());
    }
}

} // namespace DncScada
```

- [ ] **Step 3: Commit**

```bash
git add apps/CNC_Studio/MonitorPage.h apps/CNC_Studio/MonitorPage.cpp
git commit -m "feat: add MonitorPage with card layout, device cards, QCustomPlot chart, and alarms"
```

---

### Task 6: Create SimulatorPage

**Files:**
- Create: `apps/CNC_Studio/SimulatorPage.h`
- Create: `apps/CNC_Studio/SimulatorPage.cpp`

- [ ] **Step 1: Create SimulatorPage.h**

Write `D:\qt project\apps\CNC_Studio\SimulatorPage.h`:

```cpp
#ifndef DNC_SCADA_SIMULATOR_PAGE_H
#define DNC_SCADA_SIMULATOR_PAGE_H

#include "TcpDeviceServer.h"
#include "Types.h"
#include "StatusColors.h"

#include <QDoubleSpinBox>
#include <QHash>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QWidget>

namespace DncScada {

class SimulatorPage : public QWidget {
    Q_OBJECT

public:
    explicit SimulatorPage(QWidget *parent = nullptr);
    ~SimulatorPage() override;

signals:
    void simulatorRunningChanged(bool running);

private slots:
    void startServers();
    void stopServers();
    void generateTick();
    void injectAlarm();
    void onParameterReceived(ProcessParameter parameter);

private:
    struct ServerHandle {
        QThread *thread = nullptr;
        TcpDeviceServer *server = nullptr;
    };

    void buildUi();
    void updateStatusTable(const DeviceStatus &status);
    QWidget *createCard(const QString &title, QWidget *content);

    QSpinBox *firstPortSpin = nullptr;
    QSpinBox *deviceCountSpin = nullptr;
    QSpinBox *frequencySpin = nullptr;
    QSpinBox *alarmDeviceSpin = nullptr;
    QTableWidget *statusTable = nullptr;
    QTextEdit *eventLog = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *stopButton = nullptr;
    QTimer *generationTimer = nullptr;
    QHash<quint8, ServerHandle> servers;
    QHash<quint8, DeviceStatus> statuses;
    double phase = 0.0;
};

} // namespace DncScada

#endif // DNC_SCADA_SIMULATOR_PAGE_H
```

- [ ] **Step 2: Create SimulatorPage.cpp**

Write `D:\qt project\apps\CNC_Studio\SimulatorPage.cpp`:

```cpp
#include "SimulatorPage.h"
#include "Protocol.h"

#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QtMath>

namespace DncScada {

SimulatorPage::SimulatorPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    generationTimer = new QTimer(this);
    connect(generationTimer, &QTimer::timeout, this, &SimulatorPage::generateTick);
}

SimulatorPage::~SimulatorPage()
{
    stopServers();
}

QWidget *SimulatorPage::createCard(const QString &title, QWidget *content)
{
    QWidget *card = new QWidget(this);
    card->setProperty("cssClass", "card");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 10, 16, 10);
    cardLayout->setSpacing(8);

    if (!title.isEmpty()) {
        QLabel *titleLabel = new QLabel(title, card);
        QFont f = titleLabel->font();
        f.setBold(true);
        f.setPointSize(11);
        titleLabel->setFont(f);
        titleLabel->setStyleSheet("color: #8b949e; background: transparent;");
        cardLayout->addWidget(titleLabel);
    }

    if (content) {
        cardLayout->addWidget(content, 1);
    }
    return card;
}

void SimulatorPage::buildUi()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 12, 16, 12);
    rootLayout->setSpacing(12);

    // === Control Panel Card ===
    QWidget *controlContent = new QWidget(this);
    controlContent->setStyleSheet("background: transparent;");
    QHBoxLayout *controlLayout = new QHBoxLayout(controlContent);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(20);

    auto addSpinGroup = [&](const QString &label, QSpinBox *spin) {
        QVBoxLayout *group = new QVBoxLayout;
        group->setSpacing(2);
        QLabel *lbl = new QLabel(label, controlContent);
        lbl->setProperty("cssClass", "subtitle");
        lbl->setStyleSheet("color: #8b949e; background: transparent; font-size: 11px;");
        group->addWidget(lbl);
        spin->setFixedWidth(110);
        group->addWidget(spin);
        controlLayout->addLayout(group);
    };

    firstPortSpin = new QSpinBox(controlContent);
    firstPortSpin->setRange(1, 65500);
    firstPortSpin->setValue(8001);
    addSpinGroup(QStringLiteral("First Port"), firstPortSpin);

    deviceCountSpin = new QSpinBox(controlContent);
    deviceCountSpin->setRange(1, 10);
    deviceCountSpin->setValue(10);
    addSpinGroup(QStringLiteral("Devices"), deviceCountSpin);

    frequencySpin = new QSpinBox(controlContent);
    frequencySpin->setRange(1, 200);
    frequencySpin->setValue(50);
    addSpinGroup(QStringLiteral("Frequency (Hz)"), frequencySpin);

    alarmDeviceSpin = new QSpinBox(controlContent);
    alarmDeviceSpin->setRange(1, 10);
    alarmDeviceSpin->setValue(1);
    addSpinGroup(QStringLiteral("Alarm Device"), alarmDeviceSpin);

    controlLayout->addStretch();

    QVBoxLayout *btnGroup1 = new QVBoxLayout;
    btnGroup1->setSpacing(6);
    startButton = new QPushButton(QStringLiteral("▶  Start"), controlContent);
    startButton->setProperty("cssClass", "success");
    startButton->setFixedWidth(100);
    connect(startButton, &QPushButton::clicked, this, &SimulatorPage::startServers);
    btnGroup1->addWidget(startButton);

    stopButton = new QPushButton(QStringLiteral("■  Stop"), controlContent);
    stopButton->setProperty("cssClass", "danger");
    stopButton->setFixedWidth(100);
    connect(stopButton, &QPushButton::clicked, this, &SimulatorPage::stopServers);
    btnGroup1->addWidget(stopButton);
    controlLayout->addLayout(btnGroup1);

    QVBoxLayout *btnGroup2 = new QVBoxLayout;
    btnGroup2->setSpacing(6);
    QLabel *spacer2 = new QLabel(controlContent);
    spacer2->setFixedHeight(16);
    btnGroup2->addWidget(spacer2);
    QPushButton *alarmButton = new QPushButton(QStringLiteral("⚠ Inject Alarm"), controlContent);
    alarmButton->setProperty("cssClass", "ghost-danger");
    alarmButton->setFixedWidth(130);
    connect(alarmButton, &QPushButton::clicked, this, &SimulatorPage::injectAlarm);
    btnGroup2->addWidget(alarmButton);
    controlLayout->addLayout(btnGroup2);

    QWidget *controlCard = createCard(QStringLiteral("SIMULATOR CONTROL"), controlContent);
    rootLayout->addWidget(controlCard);

    // === Status Table ===
    statusTable = new QTableWidget(this);
    statusTable->setColumnCount(9);
    statusTable->setHorizontalHeaderLabels({
        QStringLiteral("Device"), QStringLiteral("Port"), QStringLiteral("State"),
        QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z"),
        QStringLiteral("RPM"), QStringLiteral("Load"), QStringLiteral("Temp")
    });
    statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    statusTable->setAlternatingRowColors(true);
    statusTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    statusTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QWidget *tableCard = createCard(QStringLiteral("DEVICE STATUS"), statusTable);
    rootLayout->addWidget(tableCard, 1);

    // === Event Log ===
    eventLog = new QTextEdit(this);
    eventLog->setReadOnly(true);
    eventLog->setMaximumHeight(140);
    eventLog->setPlaceholderText(QStringLiteral("Event log..."));
    QWidget *logCard = createCard(QStringLiteral("EVENT LOG"), eventLog);
    rootLayout->addWidget(logCard);
}

void SimulatorPage::startServers()
{
    stopServers();

    const int count = deviceCountSpin->value();
    const int firstPort = firstPortSpin->value();
    statusTable->setRowCount(count);
    statuses.clear();

    for (int index = 0; index < count; ++index) {
        const quint8 deviceId = static_cast<quint8>(index + 1);
        DeviceStatus status;
        status.deviceId = deviceId;
        status.runState = MachineRunState::Run;
        statuses.insert(deviceId, status);
        updateStatusTable(status);

        QThread *thread = new QThread(this);
        TcpDeviceServer *server = new TcpDeviceServer(deviceId);
        server->moveToThread(thread);
        connect(thread, &QThread::finished, server, &QObject::deleteLater);
        connect(server, &TcpDeviceServer::parameterReceived,
                this, &SimulatorPage::onParameterReceived, Qt::QueuedConnection);
        connect(thread, &QThread::started, this, [server, firstPort, index]() {
            QMetaObject::invokeMethod(server, "start", Qt::QueuedConnection,
                                      Q_ARG(quint16, static_cast<quint16>(firstPort + index)));
        });
        servers.insert(deviceId, { thread, server });
        thread->start();
    }

    generationTimer->start(qMax(1, 1000 / frequencySpin->value()));
    eventLog->append(QStringLiteral("%1  Started %2 simulated devices.")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")))
        .arg(count));
    emit simulatorRunningChanged(true);
}

void SimulatorPage::stopServers()
{
    if (generationTimer) {
        generationTimer->stop();
    }
    for (auto it = servers.begin(); it != servers.end(); ++it) {
        if (it.value().server) {
            QMetaObject::invokeMethod(it.value().server, "stop", Qt::BlockingQueuedConnection);
        }
        if (it.value().thread) {
            it.value().thread->quit();
            it.value().thread->wait(3000);
            it.value().thread->deleteLater();
        }
    }
    servers.clear();
    emit simulatorRunningChanged(false);
}

void SimulatorPage::generateTick()
{
    phase += 0.08;
    for (auto it = statuses.begin(); it != statuses.end(); ++it) {
        DeviceStatus status = it.value();
        if (status.runState != MachineRunState::Alarm) {
            const double offset = status.deviceId * 0.35;
            status.x = qSin(phase + offset) * 120.0;
            status.y = qCos(phase * 0.7 + offset) * 80.0;
            status.z = qSin(phase * 0.4 + offset) * 40.0;
            status.spindleRpm = 3000 + static_cast<int>(qSin(phase + offset) * 900.0);
            status.servoLoad = 45.0 + qSin(phase * 1.5 + offset) * 25.0;
            status.temperature = 42.0 + qCos(phase * 0.33 + offset) * 8.0;
            status.runState = MachineRunState::Run;
            status.alarmCode = 0;
        }
        status.timestamp = QDateTime::currentDateTimeUtc();
        it.value() = status;
        updateStatusTable(status);

        ProtocolFrame frame;
        frame.deviceId = status.deviceId;
        frame.function = FunctionCode::StatusReport;
        frame.payload = Protocol::encodeStatusPayload(status);
        const ServerHandle handle = servers.value(status.deviceId);
        if (handle.server) {
            QMetaObject::invokeMethod(handle.server, "broadcastFrame", Qt::QueuedConnection,
                                      Q_ARG(ProtocolFrame, frame));
        }
    }
}

void SimulatorPage::injectAlarm()
{
    const quint8 deviceId = static_cast<quint8>(alarmDeviceSpin->value());
    DeviceStatus status = statuses.value(deviceId);
    status.deviceId = deviceId;
    status.runState = MachineRunState::Alarm;
    status.alarmCode = 1001;
    status.timestamp = QDateTime::currentDateTimeUtc();
    statuses.insert(deviceId, status);
    eventLog->append(QStringLiteral("%1  Injected over-travel alarm into device %2.")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")))
        .arg(deviceId));
}

void SimulatorPage::onParameterReceived(ProcessParameter parameter)
{
    eventLog->append(QStringLiteral("%1  Parameter received: D%2 %3=%4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")))
        .arg(parameter.deviceId)
        .arg(parameter.name)
        .arg(parameter.value));
}

void SimulatorPage::updateStatusTable(const DeviceStatus &status)
{
    const int row = status.deviceId - 1;
    if (row < 0 || row >= statusTable->rowCount()) {
        return;
    }
    const int port = firstPortSpin->value() + row;

    // Device
    QTableWidgetItem *devItem = statusTable->item(row, 0);
    if (!devItem) {
        devItem = new QTableWidgetItem;
        statusTable->setItem(row, 0, devItem);
    }
    devItem->setIcon(QIcon(statusDotPixmap(runStateColor(status.runState))));
    devItem->setText(QString::number(status.deviceId));

    // Port
    QTableWidgetItem *portItem = statusTable->item(row, 1);
    if (!portItem) {
        portItem = new QTableWidgetItem;
        statusTable->setItem(row, 1, portItem);
    }
    portItem->setText(QString::number(port));

    // State
    QTableWidgetItem *stateItem = statusTable->item(row, 2);
    if (!stateItem) {
        stateItem = new QTableWidgetItem;
        statusTable->setItem(row, 2, stateItem);
    }
    stateItem->setIcon(QIcon(statusDotPixmap(runStateColor(status.runState))));
    stateItem->setText(runStateToString(status.runState));

    auto setCell = [&](int col, const QString &text) {
        QTableWidgetItem *item = statusTable->item(row, col);
        if (!item) {
            item = new QTableWidgetItem;
            statusTable->setItem(row, col, item);
        }
        item->setText(text);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    };

    setCell(3, QString::number(status.x, 'f', 2));
    setCell(4, QString::number(status.y, 'f', 2));
    setCell(5, QString::number(status.z, 'f', 2));
    setCell(6, QString::number(status.spindleRpm));
    setCell(7, QString::number(status.servoLoad, 'f', 1));
    setCell(8, QString::number(status.temperature, 'f', 1));
}

} // namespace DncScada
```

- [ ] **Step 3: Commit**

```bash
git add apps/CNC_Studio/SimulatorPage.h apps/CNC_Studio/SimulatorPage.cpp
git commit -m "feat: add SimulatorPage with control panel card, status table, and event log"
```

---

### Task 7: Update root DNC_SCADA.pro

**Files:**
- Modify: `DNC_SCADA.pro`

- [ ] **Step 1: Replace app subdirs in DNC_SCADA.pro**

Read current `D:\qt project\DNC_SCADA.pro`:

```
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    Common \
    NetworkModule \
    StorageModule \
    CoreEngine \
    AppUI \
    CNC_Simulator \
    CNC_Monitor

CNC_Simulator.subdir = apps/CNC_Simulator
CNC_Monitor.subdir = apps/CNC_Monitor

NetworkModule.depends = Common
StorageModule.depends = Common
CoreEngine.depends = Common NetworkModule StorageModule
AppUI.depends = Common
CNC_Simulator.depends = Common NetworkModule AppUI
CNC_Monitor.depends = Common NetworkModule StorageModule CoreEngine AppUI
```

Replace with:

```
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    Common \
    NetworkModule \
    StorageModule \
    CoreEngine \
    AppUI \
    CNC_Studio

CNC_Studio.subdir = apps/CNC_Studio

NetworkModule.depends = Common
StorageModule.depends = Common
CoreEngine.depends = Common NetworkModule StorageModule
AppUI.depends = Common
CNC_Studio.depends = Common NetworkModule StorageModule CoreEngine AppUI
```

- [ ] **Step 2: Commit**

```bash
git add DNC_SCADA.pro
git commit -m "feat: replace CNC_Simulator and CNC_Monitor with CNC_Studio in root .pro"
```

---

### Task 8: Build and verify

- [ ] **Step 1: Clean and rebuild**

```powershell
cd "D:\qt project"
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
qmake DNC_SCADA.pro
mingw32-make
```

Expected: Build succeeds with zero errors. `bin/CNC_Studio.exe` is produced.

- [ ] **Step 2: Launch and inspect**

```powershell
.\bin\CNC_Studio.exe
```

Verify:
- Dark theme applies correctly
- Sidebar shows Monitor and Simulator nav buttons
- Monitor page displays with chart area, status table, parameter panel, alarm panel
- Switch to Simulator page — shows control panel, status table, event log
- Sidebar collapses/expands with hamburger button

- [ ] **Step 3: Functional test**

1. On Simulator page, click Start — verify status table shows 10 devices with green RUN dots
2. Switch to Monitor page, enter 127.0.0.1:8001, click Connect — verify device cards show green dots
3. Verify Monitor status table updates in real-time
4. Verify chart renders three colored curves
5. On Simulator page, click Inject Alarm — verify alarm appears in Monitor alarm list
6. On Monitor page, select a parameter and click Send Parameter — verify message appears
7. Click Stop on both pages — verify clean shutdown

- [ ] **Step 4: Commit if clean**

```bash
git add -A
git commit -m "chore: build verification, CNC_Studio.exe confirmed working"
```
