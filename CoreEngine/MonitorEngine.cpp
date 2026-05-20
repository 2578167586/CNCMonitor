#include "MonitorEngine.h"

#include "Logger.h"
#include "Protocol.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>

namespace DncScada {

MonitorEngine::MonitorEngine(QObject *parent)
    : QObject(parent)
{
}

MonitorEngine::~MonitorEngine()
{
    stopMonitoring();
}

QVector<DeviceStatus> MonitorEngine::currentStatuses() const
{
    return statePool.allStatuses();
}

void MonitorEngine::startMonitoring(const QString &host, quint16 firstPort, int deviceCount, const QString &databasePath)
{
    stopMonitoring();
    startStorage(databasePath);

    for (int index = 0; index < deviceCount; ++index) {
        const quint8 deviceId = static_cast<quint8>(index + 1);
        QThread *thread = new QThread(this);
        TcpClientWorker *worker = new TcpClientWorker(deviceId);
        worker->moveToThread(thread);

        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(worker, &TcpClientWorker::frameReceived, this, &MonitorEngine::onFrameReceived, Qt::QueuedConnection);
        connect(worker, &TcpClientWorker::connectionStateChanged,
                this, &MonitorEngine::connectionStateChanged, Qt::QueuedConnection);

        connect(thread, &QThread::started, this, [worker, host, firstPort, index]() {
            QMetaObject::invokeMethod(worker, "start", Qt::QueuedConnection,
                                      Q_ARG(QString, host),
                                      Q_ARG(quint16, static_cast<quint16>(firstPort + index)));
        });

        clients.insert(deviceId, { thread, worker });
        thread->start();
    }
}

void MonitorEngine::stopMonitoring()
{
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        ClientHandle handle = it.value();
        if (handle.worker) {
            QMetaObject::invokeMethod(handle.worker, "stop", Qt::BlockingQueuedConnection);
        }
        if (handle.thread) {
            handle.thread->quit();
            handle.thread->wait(3000);
            handle.thread->deleteLater();
        }
    }
    clients.clear();
    stopStorage();
}

void MonitorEngine::sendParameter(ProcessParameter parameter)
{
    if (parameter.value < parameter.minimum || parameter.value > parameter.maximum) {
        emit parameterRejected(parameter.deviceId, QStringLiteral("Parameter is outside its valid range."));
        return;
    }

    DeviceStatus status;
    if (statePool.status(parameter.deviceId, &status)
        && parameter.coreParameter
        && status.runState == MachineRunState::Run) {
        emit parameterRejected(parameter.deviceId, QStringLiteral("Core parameters cannot be sent while device is RUN."));
        return;
    }

    ClientHandle handle = clients.value(parameter.deviceId);
    if (!handle.worker) {
        emit parameterRejected(parameter.deviceId, QStringLiteral("Device client is not available."));
        return;
    }

    ProtocolFrame frame;
    frame.deviceId = parameter.deviceId;
    frame.function = FunctionCode::ParameterWrite;
    frame.payload = Protocol::encodeParameterPayload(parameter);
    QMetaObject::invokeMethod(handle.worker, "sendFrame", Qt::QueuedConnection,
                              Q_ARG(DncScada::ProtocolFrame, frame));

    if (storageWorker) {
        QMetaObject::invokeMethod(storageWorker, "enqueueParameterChange", Qt::QueuedConnection,
                                  Q_ARG(DncScada::ProcessParameter, parameter));
    }
    emit parameterAccepted(parameter);
}

void MonitorEngine::onFrameReceived(ProtocolFrame frame)
{
    if (frame.function != FunctionCode::StatusReport) {
        AsyncLogger::instance().log(QStringLiteral("INFO"),
            QStringLiteral("MonitorEngine non-status frame func=%1").arg(static_cast<int>(frame.function)),
            QStringLiteral(__FILE__), __LINE__);
        return;
    }

    DeviceStatus status;
    if (!Protocol::decodeStatusPayload(frame.payload, &status)) {
        AsyncLogger::instance().log(QStringLiteral("INFO"),
            QStringLiteral("MonitorEngine decode FAILED dev=%1 payloadSize=%2").arg(frame.deviceId).arg(frame.payload.size()),
            QStringLiteral(__FILE__), __LINE__);
        return;
    }

    status.deviceId = frame.deviceId;
    statePool.updateStatus(status);
    AsyncLogger::instance().log(QStringLiteral("INFO"),
        QStringLiteral("MonitorEngine emit statusUpdated dev=%1 x=%2 y=%3").arg(status.deviceId).arg(status.x, 0, 'f', 1).arg(status.y, 0, 'f', 1),
        QStringLiteral(__FILE__), __LINE__);
    emit statusUpdated(status);

    if (storageWorker) {
        QMetaObject::invokeMethod(storageWorker, "enqueueStatus", Qt::QueuedConnection,
                                  Q_ARG(DncScada::DeviceStatus, status));
    }

    if (status.runState == MachineRunState::Alarm && status.alarmCode != 0) {
        AlarmEvent alarm;
        alarm.deviceId = status.deviceId;
        alarm.code = status.alarmCode;
        alarm.message = QStringLiteral("Device alarm %1").arg(status.alarmCode);
        alarm.timestamp = status.timestamp;
        emit alarmRaised(alarm);
        if (storageWorker) {
            QMetaObject::invokeMethod(storageWorker, "enqueueAlarm", Qt::QueuedConnection,
                                      Q_ARG(DncScada::AlarmEvent, alarm));
        }
    }
}

void MonitorEngine::startStorage(const QString &databasePath)
{
    storageThread = new QThread(this);
    storageWorker = new StorageWorker;
    storageWorker->moveToThread(storageThread);
    connect(storageThread, &QThread::finished, storageWorker, &QObject::deleteLater);
    connect(storageWorker, &StorageWorker::storageError, this, &MonitorEngine::storageError, Qt::QueuedConnection);
    connect(storageThread, &QThread::started, this, [this, databasePath]() {
        QMetaObject::invokeMethod(storageWorker, "start", Qt::QueuedConnection,
                                  Q_ARG(QString, databasePath));
    });
    storageThread->start();
}

void MonitorEngine::stopStorage()
{
    if (storageWorker) {
        QMetaObject::invokeMethod(storageWorker, "stop", Qt::BlockingQueuedConnection);
    }
    if (storageThread) {
        storageThread->quit();
        storageThread->wait(3000);
        storageThread->deleteLater();
    }
    storageWorker = nullptr;
    storageThread = nullptr;
}

} // namespace DncScada
