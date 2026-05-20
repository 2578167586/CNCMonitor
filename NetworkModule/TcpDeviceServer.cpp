#include "TcpDeviceServer.h"
#include "Logger.h"

namespace DncScada {

TcpDeviceServer::TcpDeviceServer(quint8 id, QObject *parent)
    : QObject(parent)
    , deviceId(id)
{
}

void TcpDeviceServer::start(quint16 port)
{
    if (!server) {
        server = new QTcpServer(this);
        connect(server, &QTcpServer::newConnection, this, &TcpDeviceServer::onNewConnection);
    }

    if (server->isListening()) {
        server->close();
    }

    if (server->listen(QHostAddress::Any, port)) {
        emit started(deviceId, port);
    }
}

void TcpDeviceServer::stop()
{
    for (QTcpSocket *client : clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    clients.clear();
    qDeleteAll(parsers);
    parsers.clear();

    if (server) {
        server->close();
    }
    emit clientCountChanged(deviceId, 0);
    emit stopped(deviceId);
}

void TcpDeviceServer::broadcastFrame(const ProtocolFrame &frame)
{
    const QByteArray bytes = Protocol::encodeFrame(frame);
    int sentCount = 0;
    for (QTcpSocket *client : clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(bytes);
            sentCount++;
        }
    }
    static int bcCount = 0;
    bcCount++;
    if (bcCount % 50 == 1) {
        AsyncLogger::instance().log(QStringLiteral("INFO"),
            QStringLiteral("TcpDeviceServer dev=%1 broadcastFrame #%2 clients=%3 sent=%4")
                .arg(deviceId).arg(bcCount).arg(clients.size()).arg(sentCount),
            QStringLiteral(__FILE__), __LINE__);
    }
}

void TcpDeviceServer::onNewConnection()
{
    AsyncLogger::instance().log(QStringLiteral("INFO"),
        QStringLiteral("TcpDeviceServer dev=%1 onNewConnection").arg(deviceId),
        QStringLiteral(__FILE__), __LINE__);
    while (server->hasPendingConnections()) {
        QTcpSocket *client = server->nextPendingConnection();
        clients.append(client);
        parsers.insert(client, new ProtocolParser);

        connect(client, &QTcpSocket::readyRead, this, [this, client]() {
            ProtocolParser *parser = parsers.value(client, nullptr);
            if (!parser) {
                return;
            }
            const QVector<ProtocolFrame> frames = parser->feed(client->readAll());
            for (const ProtocolFrame &frame : frames) {
                if (frame.function == FunctionCode::ParameterWrite) {
                    ProcessParameter parameter;
                    if (Protocol::decodeParameterPayload(frame.payload, &parameter)) {
                        emit parameterReceived(parameter);
                    }
                }
            }
        });
        connect(client, &QTcpSocket::disconnected, this, [this, client]() {
            clients.removeAll(client);
            delete parsers.take(client);
            client->deleteLater();
            emit clientCountChanged(deviceId, clients.size());
        });

        emit clientCountChanged(deviceId, clients.size());
    }
}

} // namespace DncScada
