#ifndef DNC_SCADA_MONITOR_ENGINE_H
#define DNC_SCADA_MONITOR_ENGINE_H

#include "DeviceStatePool.h"
#include "TcpClientWorker.h"
#include "StorageWorker.h"

#include <QObject>
#include <QHash>
#include <QThread>

namespace DncScada {

class MonitorEngine : public QObject
{
    Q_OBJECT

public:
    explicit MonitorEngine(QObject *parent = nullptr);
    ~MonitorEngine() override;

    QVector<DeviceStatus> currentStatuses() const;

public slots:
    void startMonitoring(const QString &host, quint16 firstPort, int deviceCount, const QString &databasePath);
    void stopMonitoring();
    void sendParameter(DncScada::ProcessParameter parameter);

signals:
    void statusUpdated(DncScada::DeviceStatus status);
    void alarmRaised(DncScada::AlarmEvent alarm);
    void connectionStateChanged(quint8 deviceId, DncScada::ConnectionState state);
    void parameterRejected(quint8 deviceId, QString reason);
    void parameterAccepted(DncScada::ProcessParameter parameter);
    void storageError(QString message);

private slots:
    void onFrameReceived(DncScada::ProtocolFrame frame);

private:
    struct ClientHandle {
        QThread *thread = nullptr;
        TcpClientWorker *worker = nullptr;
    };

    void startStorage(const QString &databasePath);
    void stopStorage();

    DeviceStatePool statePool;
    QHash<quint8, ClientHandle> clients;
    QThread *storageThread = nullptr;
    StorageWorker *storageWorker = nullptr;
};

} // namespace DncScada

#endif // DNC_SCADA_MONITOR_ENGINE_H
