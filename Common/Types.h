#ifndef DNC_SCADA_TYPES_H
#define DNC_SCADA_TYPES_H

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace DncScada {

enum class MachineRunState : quint8 {
    Stop = 0,
    Run = 1,
    Alarm = 2
};

enum class FunctionCode : quint8 {
    StatusReport = 0x01,
    ParameterWrite = 0x02,
    Heartbeat = 0x03
};

enum class ConnectionState : quint8 {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Reconnecting = 3
};

struct DeviceStatus {
    quint8 deviceId = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int spindleRpm = 0;
    double servoLoad = 0.0;
    double temperature = 0.0;
    MachineRunState runState = MachineRunState::Stop;
    quint16 alarmCode = 0;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
};

struct AlarmEvent {
    quint8 deviceId = 0;
    quint16 code = 0;
    QString message;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
};

struct ProcessParameter {
    quint8 deviceId = 0;
    QString name;
    double value = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    bool coreParameter = true;
};

struct ProtocolFrame {
    quint8 deviceId = 0;
    FunctionCode function = FunctionCode::Heartbeat;
    QByteArray payload;
};

QString runStateToString(MachineRunState state);
QString connectionStateToString(ConnectionState state);
void registerMetaTypes();

} // namespace DncScada

Q_DECLARE_METATYPE(DncScada::MachineRunState)
Q_DECLARE_METATYPE(DncScada::FunctionCode)
Q_DECLARE_METATYPE(DncScada::ConnectionState)
Q_DECLARE_METATYPE(DncScada::DeviceStatus)
Q_DECLARE_METATYPE(DncScada::AlarmEvent)
Q_DECLARE_METATYPE(DncScada::ProcessParameter)
Q_DECLARE_METATYPE(DncScada::ProtocolFrame)

#endif // DNC_SCADA_TYPES_H
