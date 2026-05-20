#include "MonitorWindow.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

namespace DncScada {

MonitorWindow::MonitorWindow(QWidget *parent)
    : QMainWindow(parent)
    , engine(new MonitorEngine(this))
{
    buildUi();
    connect(engine, &MonitorEngine::statusUpdated, this, &MonitorWindow::onStatusUpdated);
    connect(engine, &MonitorEngine::connectionStateChanged, this, &MonitorWindow::onConnectionStateChanged);
    connect(engine, &MonitorEngine::alarmRaised, this, &MonitorWindow::onAlarmRaised);
    connect(engine, &MonitorEngine::parameterRejected, this, [this](quint8 deviceId, const QString &reason) {
        messageLabel->setText(QStringLiteral("D%1 rejected: %2").arg(deviceId).arg(reason));
    });
    connect(engine, &MonitorEngine::parameterAccepted, this, [this](const ProcessParameter &parameter) {
        messageLabel->setText(QStringLiteral("Sent D%1 %2=%3")
            .arg(parameter.deviceId)
            .arg(parameter.name)
            .arg(parameter.value));
    });
    connect(engine, &MonitorEngine::storageError, this, [this](const QString &message) {
        messageLabel->setText(QStringLiteral("Storage error: %1").arg(message));
    });
}

void MonitorWindow::buildUi()
{
    setWindowTitle(QStringLiteral("CNC Monitor"));
    QWidget *root = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(root);

    QHBoxLayout *controls = new QHBoxLayout;
    hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), root);
    firstPortSpin = new QSpinBox(root);
    firstPortSpin->setRange(1, 65500);
    firstPortSpin->setValue(8001);
    deviceCountSpin = new QSpinBox(root);
    deviceCountSpin->setRange(1, 10);
    deviceCountSpin->setValue(10);
    QPushButton *startButton = new QPushButton(QStringLiteral("Connect"), root);
    QPushButton *stopButton = new QPushButton(QStringLiteral("Stop"), root);
    connect(startButton, &QPushButton::clicked, this, &MonitorWindow::startMonitor);
    connect(stopButton, &QPushButton::clicked, this, &MonitorWindow::stopMonitor);
    controls->addWidget(new QLabel(QStringLiteral("Host"), root));
    controls->addWidget(hostEdit);
    controls->addWidget(new QLabel(QStringLiteral("First Port"), root));
    controls->addWidget(firstPortSpin);
    controls->addWidget(new QLabel(QStringLiteral("Devices"), root));
    controls->addWidget(deviceCountSpin);
    controls->addWidget(startButton);
    controls->addWidget(stopButton);
    layout->addLayout(controls);

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, root);
    QWidget *top = new QWidget(mainSplitter);
    QHBoxLayout *topLayout = new QHBoxLayout(top);

    connectionTable = new QTableWidget(top);
    connectionTable->setColumnCount(3);
    connectionTable->setHorizontalHeaderLabels({
        QStringLiteral("Device"), QStringLiteral("Endpoint"), QStringLiteral("State")
    });
    connectionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    topLayout->addWidget(connectionTable, 1);

    statusTable = new QTableWidget(top);
    statusTable->setColumnCount(8);
    statusTable->setHorizontalHeaderLabels({
        QStringLiteral("Device"), QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z"),
        QStringLiteral("RPM"), QStringLiteral("Load"), QStringLiteral("Temp"), QStringLiteral("State")
    });
    statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    topLayout->addWidget(statusTable, 2);
    mainSplitter->addWidget(top);

    plot = new RealTimePlotWidget(mainSplitter);
    mainSplitter->addWidget(plot);

    QWidget *bottom = new QWidget(mainSplitter);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottom);
    parameterModel = new ParameterTableModel(this);
    parameterView = new QTableView(bottom);
    parameterView->setModel(parameterModel);
    parameterView->setItemDelegate(new NumericDelegate(parameterView));
    parameterView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QPushButton *sendButton = new QPushButton(QStringLiteral("Send Parameter"), bottom);
    connect(sendButton, &QPushButton::clicked, this, &MonitorWindow::sendSelectedParameter);

    QVBoxLayout *parameterLayout = new QVBoxLayout;
    parameterLayout->addWidget(parameterView);
    parameterLayout->addWidget(sendButton);
    bottomLayout->addLayout(parameterLayout, 2);

    alarmList = new QListWidget(bottom);
    bottomLayout->addWidget(alarmList, 1);
    mainSplitter->addWidget(bottom);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 2);
    mainSplitter->setStretchFactor(2, 2);
    layout->addWidget(mainSplitter, 1);

    messageLabel = new QLabel(root);
    layout->addWidget(messageLabel);
    setCentralWidget(root);
}

void MonitorWindow::startMonitor()
{
    connectionTable->setRowCount(deviceCountSpin->value());
    statusTable->setRowCount(0);
    alarmList->clear();
    plot->clear();

    for (int i = 0; i < deviceCountSpin->value(); ++i) {
        const int deviceId = i + 1;
        const QStringList values = {
            QString::number(deviceId),
            QStringLiteral("%1:%2").arg(hostEdit->text()).arg(firstPortSpin->value() + i),
            connectionStateToString(ConnectionState::Disconnected)
        };
        for (int column = 0; column < values.size(); ++column) {
            connectionTable->setItem(i, column, new QTableWidgetItem(values.at(column)));
        }
    }

    QDir dataDir(QApplication::applicationDirPath());
    dataDir.mkpath(QStringLiteral("data"));
    const QString dbPath = dataDir.filePath(QStringLiteral("data/monitor.sqlite"));
    engine->startMonitoring(hostEdit->text(), static_cast<quint16>(firstPortSpin->value()),
                            deviceCountSpin->value(), dbPath);
}

void MonitorWindow::stopMonitor()
{
    engine->stopMonitoring();
    messageLabel->setText(QStringLiteral("Stopped."));
}

void MonitorWindow::onStatusUpdated(DeviceStatus status)
{
    ensureStatusRow(status.deviceId);
    const int row = status.deviceId - 1;
    const QStringList values = {
        QString::number(status.deviceId),
        QString::number(status.x, 'f', 2),
        QString::number(status.y, 'f', 2),
        QString::number(status.z, 'f', 2),
        QString::number(status.spindleRpm),
        QString::number(status.servoLoad, 'f', 1),
        QString::number(status.temperature, 'f', 1),
        runStateToString(status.runState)
    };
    for (int column = 0; column < values.size(); ++column) {
        QTableWidgetItem *item = statusTable->item(row, column);
        if (!item) {
            item = new QTableWidgetItem;
            statusTable->setItem(row, column, item);
        }
        item->setText(values.at(column));
    }
    plot->appendStatus(status);
}

void MonitorWindow::onConnectionStateChanged(quint8 deviceId, ConnectionState state)
{
    const int row = deviceId - 1;
    if (row < 0 || row >= connectionTable->rowCount()) {
        return;
    }
    QTableWidgetItem *item = connectionTable->item(row, 2);
    if (!item) {
        item = new QTableWidgetItem;
        connectionTable->setItem(row, 2, item);
    }
    item->setText(connectionStateToString(state));
}

void MonitorWindow::onAlarmRaised(AlarmEvent alarm)
{
    alarmList->insertItem(0, QStringLiteral("%1 D%2 code=%3 %4")
        .arg(alarm.timestamp.toLocalTime().toString(QStringLiteral("hh:mm:ss.zzz")))
        .arg(alarm.deviceId)
        .arg(alarm.code)
        .arg(alarm.message));
}

void MonitorWindow::sendSelectedParameter()
{
    const QModelIndex index = parameterView->currentIndex();
    const int row = index.isValid() ? index.row() : 0;
    engine->sendParameter(parameterModel->parameterAt(row));
}

void MonitorWindow::ensureStatusRow(quint8 deviceId)
{
    while (statusTable->rowCount() < deviceId) {
        statusTable->insertRow(statusTable->rowCount());
    }
}

} // namespace DncScada
