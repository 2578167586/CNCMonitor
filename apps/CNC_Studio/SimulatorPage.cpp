#include "SimulatorPage.h"
#include "Logger.h"
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
    static int tickCount = 0;
    tickCount++;
    if (tickCount % 50 == 1) {
        AsyncLogger::instance().log(QStringLiteral("INFO"),
            QStringLiteral("Simulator generateTick #%1 running").arg(tickCount),
            QStringLiteral(__FILE__), __LINE__);
    }
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
