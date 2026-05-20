#include "SimulatorWindow.h"

#include "Protocol.h"

#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMetaObject>
#include <QVBoxLayout>
#include <QtMath>

namespace DncScada {

SimulatorWindow::SimulatorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    generationTimer = new QTimer(this);
    connect(generationTimer, &QTimer::timeout, this, &SimulatorWindow::generateTick);
}

SimulatorWindow::~SimulatorWindow()
{
    stopServers();
}

void SimulatorWindow::buildUi()
{
    setWindowTitle(QStringLiteral("CNC Simulator"));

    QWidget *root = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(root);

    QHBoxLayout *controls = new QHBoxLayout;
    firstPortSpin = new QSpinBox(root);
    firstPortSpin->setRange(1, 65500);
    firstPortSpin->setValue(8001);
    deviceCountSpin = new QSpinBox(root);
    deviceCountSpin->setRange(1, 10);
    deviceCountSpin->setValue(10);
    frequencySpin = new QSpinBox(root);
    frequencySpin->setRange(1, 200);
    frequencySpin->setValue(50);
    alarmDeviceSpin = new QSpinBox(root);
    alarmDeviceSpin->setRange(1, 10);
    alarmDeviceSpin->setValue(1);

    startButton = new QPushButton(QStringLiteral("Start"), root);
    stopButton = new QPushButton(QStringLiteral("Stop"), root);
    QPushButton *alarmButton = new QPushButton(QStringLiteral("Inject Alarm"), root);
    connect(startButton, &QPushButton::clicked, this, &SimulatorWindow::startServers);
    connect(stopButton, &QPushButton::clicked, this, &SimulatorWindow::stopServers);
    connect(alarmButton, &QPushButton::clicked, this, &SimulatorWindow::injectAlarm);

    controls->addWidget(new QLabel(QStringLiteral("First Port"), root));
    controls->addWidget(firstPortSpin);
    controls->addWidget(new QLabel(QStringLiteral("Devices"), root));
    controls->addWidget(deviceCountSpin);
    controls->addWidget(new QLabel(QStringLiteral("Hz"), root));
    controls->addWidget(frequencySpin);
    controls->addWidget(startButton);
    controls->addWidget(stopButton);
    controls->addStretch();
    controls->addWidget(new QLabel(QStringLiteral("Alarm Device"), root));
    controls->addWidget(alarmDeviceSpin);
    controls->addWidget(alarmButton);
    layout->addLayout(controls);

    statusTable = new QTableWidget(root);
    statusTable->setColumnCount(9);
    statusTable->setHorizontalHeaderLabels({
        QStringLiteral("Device"), QStringLiteral("Port"), QStringLiteral("X"),
        QStringLiteral("Y"), QStringLiteral("Z"), QStringLiteral("RPM"),
        QStringLiteral("Load"), QStringLiteral("Temp"), QStringLiteral("State")
    });
    statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(statusTable, 1);

    eventLog = new QTextEdit(root);
    eventLog->setReadOnly(true);
    eventLog->setMaximumHeight(140);
    layout->addWidget(eventLog);

    setCentralWidget(root);
}

void SimulatorWindow::startServers()
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
                this, &SimulatorWindow::onParameterReceived, Qt::QueuedConnection);
        connect(thread, &QThread::started, this, [server, firstPort, index]() {
            QMetaObject::invokeMethod(server, "start", Qt::QueuedConnection,
                                      Q_ARG(quint16, static_cast<quint16>(firstPort + index)));
        });
        servers.insert(deviceId, { thread, server });
        thread->start();
    }

    generationTimer->start(qMax(1, 1000 / frequencySpin->value()));
    eventLog->append(QStringLiteral("Started %1 simulated devices.").arg(count));
}

void SimulatorWindow::stopServers()
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
}

void SimulatorWindow::generateTick()
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
                                      Q_ARG(DncScada::ProtocolFrame, frame));
        }
    }
}

void SimulatorWindow::injectAlarm()
{
    const quint8 deviceId = static_cast<quint8>(alarmDeviceSpin->value());
    DeviceStatus status = statuses.value(deviceId);
    status.deviceId = deviceId;
    status.runState = MachineRunState::Alarm;
    status.alarmCode = 1001;
    status.timestamp = QDateTime::currentDateTimeUtc();
    statuses.insert(deviceId, status);
    eventLog->append(QStringLiteral("Injected over-travel alarm into device %1.").arg(deviceId));
}

void SimulatorWindow::onParameterReceived(ProcessParameter parameter)
{
    eventLog->append(QStringLiteral("Parameter received: D%1 %2=%3")
        .arg(parameter.deviceId)
        .arg(parameter.name)
        .arg(parameter.value));
}

void SimulatorWindow::updateStatusTable(const DeviceStatus &status)
{
    const int row = status.deviceId - 1;
    if (row < 0 || row >= statusTable->rowCount()) {
        return;
    }
    const int port = firstPortSpin->value() + row;
    const QStringList values = {
        QString::number(status.deviceId),
        QString::number(port),
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
}

} // namespace DncScada
