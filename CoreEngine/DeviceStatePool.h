#ifndef DNC_SCADA_DEVICE_STATE_POOL_H
#define DNC_SCADA_DEVICE_STATE_POOL_H

#include "Types.h"

#include <QHash>
#include <QMutex>
#include <QVector>

namespace DncScada {

class DeviceStatePool
{
public:
    void updateStatus(const DeviceStatus &status);
    bool status(quint8 deviceId, DeviceStatus *status) const;
    QVector<DeviceStatus> allStatuses() const;

private:
    mutable QMutex mutex;
    QHash<quint8, DeviceStatus> statuses;
};

} // namespace DncScada

#endif // DNC_SCADA_DEVICE_STATE_POOL_H
