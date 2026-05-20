#include "DeviceStatePool.h"

namespace DncScada {

void DeviceStatePool::updateStatus(const DeviceStatus &status)
{
    QMutexLocker locker(&mutex);
    statuses.insert(status.deviceId, status);
}

bool DeviceStatePool::status(quint8 deviceId, DeviceStatus *out) const
{
    QMutexLocker locker(&mutex);
    if (!statuses.contains(deviceId)) {
        return false;
    }
    if (out) {
        *out = statuses.value(deviceId);
    }
    return true;
}

QVector<DeviceStatus> DeviceStatePool::allStatuses() const
{
    QMutexLocker locker(&mutex);
    return statuses.values().toVector();
}

} // namespace DncScada
