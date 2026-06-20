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
#include <QTimer>
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
    void flushStatusUpdates();

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
    QHash<quint8, DeviceStatus> pendingStatuses;
    QTimer *flushTimer = nullptr;
    int flushCounter = 0;
};

} // namespace DncScada

#endif // DNC_SCADA_MONITOR_PAGE_H
