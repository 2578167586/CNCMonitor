#ifndef DNC_SCADA_SIMULATOR_WINDOW_H
#define DNC_SCADA_SIMULATOR_WINDOW_H

#include "TcpDeviceServer.h"
#include "Types.h"

#include <QDoubleSpinBox>
#include <QHash>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QThread>
#include <QTimer>

namespace DncScada {

class SimulatorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimulatorWindow(QWidget *parent = nullptr);
    ~SimulatorWindow() override;

private slots:
    void startServers();
    void stopServers();
    void generateTick();
    void injectAlarm();
    void onParameterReceived(DncScada::ProcessParameter parameter);

private:
    struct ServerHandle {
        QThread *thread = nullptr;
        TcpDeviceServer *server = nullptr;
    };

    void buildUi();
    void updateStatusTable(const DeviceStatus &status);

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

#endif // DNC_SCADA_SIMULATOR_WINDOW_H
