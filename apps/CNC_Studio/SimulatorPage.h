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
