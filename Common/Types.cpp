#include "Types.h"

namespace DncScada {

QString runStateToString(MachineRunState state)
{
    switch (state) {
    case MachineRunState::Stop:
        return QStringLiteral("STOP");
    case MachineRunState::Run:
        return QStringLiteral("RUN");
    case MachineRunState::Alarm:
        return QStringLiteral("ALARM");
    }
    return QStringLiteral("UNKNOWN");
}

QString connectionStateToString(ConnectionState state)
{
    switch (state) {
    case ConnectionState::Disconnected:
        return QStringLiteral("Disconnected");
    case ConnectionState::Connecting:
        return QStringLiteral("Connecting");
    case ConnectionState::Connected:
        return QStringLiteral("Connected");
    case ConnectionState::Reconnecting:
        return QStringLiteral("Reconnecting");
    }
    return QStringLiteral("Unknown");
}

void registerMetaTypes()
{
    qRegisterMetaType<DncScada::MachineRunState>("DncScada::MachineRunState");
    qRegisterMetaType<DncScada::FunctionCode>("DncScada::FunctionCode");
    qRegisterMetaType<DncScada::ConnectionState>("DncScada::ConnectionState");
    qRegisterMetaType<DncScada::DeviceStatus>("DncScada::DeviceStatus");
    qRegisterMetaType<DncScada::AlarmEvent>("DncScada::AlarmEvent");
    qRegisterMetaType<DncScada::ProcessParameter>("DncScada::ProcessParameter");
    qRegisterMetaType<DncScada::ProtocolFrame>("DncScada::ProtocolFrame");
}

} // namespace DncScada
