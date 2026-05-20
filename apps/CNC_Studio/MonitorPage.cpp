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
    messageLabel->setText(QStringLiteral("Connecting to %1:%2-%3 ...")
        .arg(hostEdit->text()).arg(firstPortSpin->value())
        .arg(firstPortSpin->value() + count - 1));
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
    messageLabel->setText(QStringLiteral("Recv D%1 X=%2 Y=%3 Z=%4 RPM=%5")
        .arg(status.deviceId).arg(status.x, 0, 'f', 1)
        .arg(status.y, 0, 'f', 1).arg(status.z, 0, 'f', 1)
        .arg(status.spindleRpm));

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
    messageLabel->setText(QStringLiteral("D%1 %2").arg(deviceId).arg(connectionStateToString(state)));
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
