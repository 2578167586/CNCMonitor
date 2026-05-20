#ifndef DNC_SCADA_MONITOR_WINDOW_H
#define DNC_SCADA_MONITOR_WINDOW_H

#include "MonitorEngine.h"
#include "NumericDelegate.h"
#include "ParameterTableModel.h"
#include "RealTimePlotWidget.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTableWidget>

namespace DncScada {

class MonitorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MonitorWindow(QWidget *parent = nullptr);

private slots:
    void startMonitor();
    void stopMonitor();
    void onStatusUpdated(DncScada::DeviceStatus status);
    void onConnectionStateChanged(quint8 deviceId, DncScada::ConnectionState state);
    void onAlarmRaised(DncScada::AlarmEvent alarm);
    void sendSelectedParameter();

private:
    void buildUi();
    void ensureStatusRow(quint8 deviceId);

    MonitorEngine *engine = nullptr;
    QLineEdit *hostEdit = nullptr;
    QSpinBox *firstPortSpin = nullptr;
    QSpinBox *deviceCountSpin = nullptr;
    QTableWidget *connectionTable = nullptr;
    QTableWidget *statusTable = nullptr;
    QListWidget *alarmList = nullptr;
    RealTimePlotWidget *plot = nullptr;
    ParameterTableModel *parameterModel = nullptr;
    QTableView *parameterView = nullptr;
    QLabel *messageLabel = nullptr;
};

} // namespace DncScada

#endif // DNC_SCADA_MONITOR_WINDOW_H
