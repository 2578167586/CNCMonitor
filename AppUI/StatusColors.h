#ifndef DNC_SCADA_STATUS_COLORS_H
#define DNC_SCADA_STATUS_COLORS_H

#include "Types.h"
#include <QColor>
#include <QPainter>
#include <QPixmap>

namespace DncScada {

inline QColor runStateColor(MachineRunState state) {
    switch (state) {
    case MachineRunState::Run:  return QColor("#22c55e");
    case MachineRunState::Alarm: return QColor("#ef4444");
    case MachineRunState::Stop:  return QColor("#8b949e");
    }
    return QColor("#8b949e");
}

inline QColor connectionStateColor(ConnectionState state) {
    switch (state) {
    case ConnectionState::Connected:    return QColor("#22c55e");
    case ConnectionState::Connecting:   return QColor("#eab308");
    case ConnectionState::Reconnecting: return QColor("#eab308");
    case ConnectionState::Disconnected: return QColor("#8b949e");
    }
    return QColor("#8b949e");
}

inline QPixmap statusDotPixmap(const QColor &color, int size = 12) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(1, 1, size - 2, size - 2);
    painter.end();
    return pix;
}

} // namespace DncScada

#endif // DNC_SCADA_STATUS_COLORS_H
