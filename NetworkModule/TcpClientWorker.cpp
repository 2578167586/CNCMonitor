#include "TcpClientWorker.h"

#include "Logger.h"

namespace DncScada {

TcpClientWorker::TcpClientWorker(quint8 id, QObject *parent)
    : QObject(parent)
    , deviceId(id)
{
}

void TcpClientWorker::start(const QString &targetHost, quint16 targetPort)
{
    host = targetHost;
    port = targetPort;
    requestedStop = false;

    if (!socket) {
        socket = new QTcpSocket(this);
        connect(socket, &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
        connect(socket, &QTcpSocket::connected, this, &TcpClientWorker::onConnected);
        connect(socket, &QTcpSocket::disconnected, this, &TcpClientWorker::onDisconnected);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this, &TcpClientWorker::onError);
    }

    if (!reconnectTimer) {
        reconnectTimer = new QTimer(this);
        reconnectTimer->setSingleShot(true);
        connect(reconnectTimer, &QTimer::timeout, this, &TcpClientWorker::connectToHost);
    }

    reconnectDelayMs = 1000;
    connectToHost();
}

void TcpClientWorker::stop()
{
    requestedStop = true;
    if (reconnectTimer) {
        reconnectTimer->stop();
    }
    if (socket) {
        socket->disconnectFromHost();
    }
    emit connectionStateChanged(deviceId, ConnectionState::Disconnected);
}

void TcpClientWorker::sendFrame(const ProtocolFrame &frame)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    socket->write(Protocol::encodeFrame(frame));
}

void TcpClientWorker::connectToHost()
{
    if (requestedStop || !socket) {
        return;
    }
    emit connectionStateChanged(deviceId, socket->state() == QAbstractSocket::UnconnectedState
        ? ConnectionState::Connecting
        : ConnectionState::Reconnecting);
    socket->abort();
    socket->connectToHost(host, port);
}

void TcpClientWorker::onReadyRead()
{
    const QByteArray bytes = socket->readAll();
    emit rawBytesReceived(deviceId, bytes);
    const QVector<ProtocolFrame> frames = parser.feed(bytes);
    AsyncLogger::instance().log(QStringLiteral("INFO"),
        QStringLiteral("TcpClientWorker onReadyRead dev=%1 bytes=%2 frames=%3")
            .arg(deviceId).arg(bytes.size()).arg(frames.size()),
        QStringLiteral(__FILE__), __LINE__);
    for (const ProtocolFrame &frame : frames) {
        emit frameReceived(frame);
    }
}

void TcpClientWorker::onConnected()
{
    reconnectDelayMs = 1000;
    emit connectionStateChanged(deviceId, ConnectionState::Connected);
}

void TcpClientWorker::onDisconnected()
{
    emit connectionStateChanged(deviceId, ConnectionState::Disconnected);
    if (!requestedStop) {
        scheduleReconnect();
    }
}

void TcpClientWorker::onError(QAbstractSocket::SocketError)
{
    if (!requestedStop) {
        scheduleReconnect();
    }
}

void TcpClientWorker::scheduleReconnect()
{
    if (!reconnectTimer || reconnectTimer->isActive()) {
        return;
    }
    emit connectionStateChanged(deviceId, ConnectionState::Reconnecting);
    reconnectTimer->start(reconnectDelayMs);
    reconnectDelayMs = qMin(reconnectDelayMs * 2, 15000);
}

} // namespace DncScada
