#ifndef DNC_SCADA_STORAGE_WORKER_H
#define DNC_SCADA_STORAGE_WORKER_H

#include "Types.h"

#include <QObject>
#include <QSqlDatabase>
#include <QTimer>
#include <QVector>

namespace DncScada {

class StorageWorker : public QObject
{
    Q_OBJECT

public:
    explicit StorageWorker(QObject *parent = nullptr);
    ~StorageWorker() override;

public slots:
    void start(const QString &databasePath);
    void stop();
    void enqueueStatus(DncScada::DeviceStatus status);
    void enqueueAlarm(DncScada::AlarmEvent alarm);
    void enqueueParameterChange(DncScada::ProcessParameter parameter);
    void flush();

signals:
    void storageError(QString message);
    void batchWritten(int statusCount, int alarmCount, int parameterCount);

private:
    bool ensureSchema();
    bool flushBatch(const QVector<DeviceStatus> &statuses,
                    const QVector<AlarmEvent> &alarms,
                    const QVector<ProcessParameter> &parameters);

    QSqlDatabase db;
    QString connectionName;
    QTimer *flushTimer = nullptr;
    QVector<DeviceStatus> activeStatuses;
    QVector<DeviceStatus> standbyStatuses;
    QVector<AlarmEvent> activeAlarms;
    QVector<AlarmEvent> standbyAlarms;
    QVector<ProcessParameter> activeParameters;
    QVector<ProcessParameter> standbyParameters;
    bool opened = false;
};

} // namespace DncScada

#endif // DNC_SCADA_STORAGE_WORKER_H
