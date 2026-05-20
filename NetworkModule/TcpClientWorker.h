#ifndef DNC_SCADA_TCP_CLIENT_WORKER_H
#define DNC_SCADA_TCP_CLIENT_WORKER_H

#include "Protocol.h"
#include "Types.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace DncScada {

class TcpClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit TcpClientWorker(quint8 deviceId, QObject *parent = nullptr);

public slots:
    void start(const QString &host, quint16 port);
    void stop();
    void sendFrame(const DncScada::ProtocolFrame &frame);

signals:
    void frameReceived(DncScada::ProtocolFrame frame);
    void rawBytesReceived(quint8 deviceId, QByteArray bytes);
    void connectionStateChanged(quint8 deviceId, DncScada::ConnectionState state);

private slots:
    void connectToHost();
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    void scheduleReconnect();

    quint8 deviceId;
    QString host;
    quint16 port = 0;
    QTcpSocket *socket = nullptr;
    QTimer *reconnectTimer = nullptr;
    ProtocolParser parser;
    int reconnectDelayMs = 1000;
    bool requestedStop = false;
};

} // namespace DncScada

#endif // DNC_SCADA_TCP_CLIENT_WORKER_H
