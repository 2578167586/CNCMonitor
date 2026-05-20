#ifndef DNC_SCADA_TCP_DEVICE_SERVER_H
#define DNC_SCADA_TCP_DEVICE_SERVER_H

#include "Protocol.h"

#include <QHash>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

namespace DncScada {

class TcpDeviceServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpDeviceServer(quint8 deviceId, QObject *parent = nullptr);

public slots:
    void start(quint16 port);
    void stop();
    void broadcastFrame(const DncScada::ProtocolFrame &frame);

signals:
    void started(quint8 deviceId, quint16 port);
    void stopped(quint8 deviceId);
    void clientCountChanged(quint8 deviceId, int count);
    void parameterReceived(DncScada::ProcessParameter parameter);

private slots:
    void onNewConnection();

private:
    quint8 deviceId;
    QTcpServer *server = nullptr;
    QList<QTcpSocket *> clients;
    QHash<QTcpSocket *, ProtocolParser *> parsers;
};

} // namespace DncScada

#endif // DNC_SCADA_TCP_DEVICE_SERVER_H
