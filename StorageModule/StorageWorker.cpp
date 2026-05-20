#include "StorageWorker.h"

#include "Logger.h"

#include <QDir>
#include <QFileInfo>
#include <QVariant>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace DncScada {

StorageWorker::StorageWorker(QObject *parent)
    : QObject(parent)
    , connectionName(QStringLiteral("storage_%1").arg(QUuid::createUuid().toString()))
{
}

StorageWorker::~StorageWorker()
{
    stop();
}

void StorageWorker::start(const QString &databasePath)
{
    if (opened) {
        return;
    }

    const QFileInfo info(databasePath);
    if (!info.absoluteDir().exists()) {
        info.absoluteDir().mkpath(QStringLiteral("."));
    }

    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(databasePath);
    if (!db.open()) {
        emit storageError(db.lastError().text());
        return;
    }

    opened = ensureSchema();
    if (!opened) {
        return;
    }

    flushTimer = new QTimer(this);
    flushTimer->setInterval(500);
    connect(flushTimer, &QTimer::timeout, this, &StorageWorker::flush);
    flushTimer->start();
}

void StorageWorker::stop()
{
    if (flushTimer) {
        flushTimer->stop();
    }
    flush();
    if (db.isOpen()) {
        db.close();
    }
    if (!connectionName.isEmpty()) {
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }
    opened = false;
}

void StorageWorker::enqueueStatus(DeviceStatus status)
{
    if (!opened) {
        return;
    }
    activeStatuses.append(status);
    if (activeStatuses.size() >= 100) {
        flush();
    }
}

void StorageWorker::enqueueAlarm(AlarmEvent alarm)
{
    if (!opened) {
        return;
    }
    activeAlarms.append(alarm);
    if (activeAlarms.size() >= 100) {
        flush();
    }
}

void StorageWorker::enqueueParameterChange(ProcessParameter parameter)
{
    if (!opened) {
        return;
    }
    activeParameters.append(parameter);
    if (activeParameters.size() >= 100) {
        flush();
    }
}

void StorageWorker::flush()
{
    if (!opened || (activeStatuses.isEmpty() && activeAlarms.isEmpty() && activeParameters.isEmpty())) {
        return;
    }

    standbyStatuses.swap(activeStatuses);
    standbyAlarms.swap(activeAlarms);
    standbyParameters.swap(activeParameters);

    if (flushBatch(standbyStatuses, standbyAlarms, standbyParameters)) {
        emit batchWritten(standbyStatuses.size(), standbyAlarms.size(), standbyParameters.size());
        standbyStatuses.clear();
        standbyAlarms.clear();
        standbyParameters.clear();
    } else {
        activeStatuses += standbyStatuses;
        activeAlarms += standbyAlarms;
        activeParameters += standbyParameters;
        standbyStatuses.clear();
        standbyAlarms.clear();
        standbyParameters.clear();
    }
}

bool StorageWorker::ensureSchema()
{
    QSqlQuery query(db);
    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS status_history ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "device_id INTEGER NOT NULL,"
                       "x REAL NOT NULL,"
                       "y REAL NOT NULL,"
                       "z REAL NOT NULL,"
                       "spindle_rpm INTEGER NOT NULL,"
                       "servo_load REAL NOT NULL,"
                       "temperature REAL NOT NULL,"
                       "run_state INTEGER NOT NULL,"
                       "alarm_code INTEGER NOT NULL,"
                       "created_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS alarm_log ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "device_id INTEGER NOT NULL,"
                       "code INTEGER NOT NULL,"
                       "message TEXT NOT NULL,"
                       "created_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS parameter_log ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "device_id INTEGER NOT NULL,"
                       "name TEXT NOT NULL,"
                       "value REAL NOT NULL,"
                       "minimum REAL NOT NULL,"
                       "maximum REAL NOT NULL,"
                       "core_parameter INTEGER NOT NULL,"
                       "created_at TEXT NOT NULL)")
    };

    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            emit storageError(query.lastError().text());
            return false;
        }
    }
    return true;
}

bool StorageWorker::flushBatch(const QVector<DeviceStatus> &statuses,
                               const QVector<AlarmEvent> &alarms,
                               const QVector<ProcessParameter> &parameters)
{
    if (!db.transaction()) {
        emit storageError(db.lastError().text());
        return false;
    }

    QSqlQuery statusQuery(db);
    statusQuery.prepare(QStringLiteral("INSERT INTO status_history "
        "(device_id, x, y, z, spindle_rpm, servo_load, temperature, run_state, alarm_code, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    for (const DeviceStatus &status : statuses) {
        statusQuery.addBindValue(QVariant(static_cast<int>(status.deviceId)));
        statusQuery.addBindValue(QVariant(status.x));
        statusQuery.addBindValue(QVariant(status.y));
        statusQuery.addBindValue(QVariant(status.z));
        statusQuery.addBindValue(QVariant(status.spindleRpm));
        statusQuery.addBindValue(QVariant(status.servoLoad));
        statusQuery.addBindValue(QVariant(status.temperature));
        statusQuery.addBindValue(QVariant(static_cast<int>(status.runState)));
        statusQuery.addBindValue(QVariant(static_cast<int>(status.alarmCode)));
        statusQuery.addBindValue(QVariant(status.timestamp.toString(Qt::ISODateWithMs)));
        if (!statusQuery.exec()) {
            emit storageError(statusQuery.lastError().text());
            db.rollback();
            return false;
        }
    }

    QSqlQuery alarmQuery(db);
    alarmQuery.prepare(QStringLiteral("INSERT INTO alarm_log "
        "(device_id, code, message, created_at) VALUES (?, ?, ?, ?)"));
    for (const AlarmEvent &alarm : alarms) {
        alarmQuery.addBindValue(QVariant(static_cast<int>(alarm.deviceId)));
        alarmQuery.addBindValue(QVariant(static_cast<int>(alarm.code)));
        alarmQuery.addBindValue(QVariant(alarm.message));
        alarmQuery.addBindValue(QVariant(alarm.timestamp.toString(Qt::ISODateWithMs)));
        if (!alarmQuery.exec()) {
            emit storageError(alarmQuery.lastError().text());
            db.rollback();
            return false;
        }
    }

    QSqlQuery parameterQuery(db);
    parameterQuery.prepare(QStringLiteral("INSERT INTO parameter_log "
        "(device_id, name, value, minimum, maximum, core_parameter, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    for (const ProcessParameter &parameter : parameters) {
        parameterQuery.addBindValue(QVariant(static_cast<int>(parameter.deviceId)));
        parameterQuery.addBindValue(QVariant(parameter.name));
        parameterQuery.addBindValue(QVariant(parameter.value));
        parameterQuery.addBindValue(QVariant(parameter.minimum));
        parameterQuery.addBindValue(QVariant(parameter.maximum));
        parameterQuery.addBindValue(QVariant(parameter.coreParameter ? 1 : 0));
        parameterQuery.addBindValue(QVariant(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)));
        if (!parameterQuery.exec()) {
            emit storageError(parameterQuery.lastError().text());
            db.rollback();
            return false;
        }
    }

    return db.commit();
}

} // namespace DncScada
