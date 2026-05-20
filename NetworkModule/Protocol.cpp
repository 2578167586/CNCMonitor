#include "Protocol.h"

#include <QDataStream>

namespace DncScada {

quint16 Protocol::crc16Modbus(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) {
                crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
            } else {
                crc = static_cast<quint16>(crc >> 1);
            }
        }
    }
    return crc;
}

QByteArray Protocol::encodeFrame(const ProtocolFrame &frame)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << Header;
    stream << frame.deviceId;
    stream << static_cast<quint8>(frame.function);
    stream << static_cast<quint16>(frame.payload.size());
    packet.append(frame.payload);

    const quint16 crc = crc16Modbus(packet);
    QDataStream crcStream(&packet, QIODevice::Append);
    crcStream.setByteOrder(QDataStream::LittleEndian);
    crcStream << crc;
    return packet;
}

QByteArray Protocol::encodeStatusPayload(const DeviceStatus &status)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << status.x
           << status.y
           << status.z
           << static_cast<qint32>(status.spindleRpm)
           << status.servoLoad
           << status.temperature
           << static_cast<quint8>(status.runState)
           << status.alarmCode
           << status.timestamp.toMSecsSinceEpoch();
    return payload;
}

bool Protocol::decodeStatusPayload(const QByteArray &payload, DeviceStatus *status)
{
    if (!status) {
        return false;
    }

    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::LittleEndian);
    qint32 rpm = 0;
    quint8 runState = 0;
    qint64 timestamp = 0;
    stream >> status->x
           >> status->y
           >> status->z
           >> rpm
           >> status->servoLoad
           >> status->temperature
           >> runState
           >> status->alarmCode
           >> timestamp;

    if (stream.status() != QDataStream::Ok) {
        return false;
    }

    status->spindleRpm = rpm;
    status->runState = static_cast<MachineRunState>(runState);
    status->timestamp = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC);
    return true;
}

QByteArray Protocol::encodeParameterPayload(const ProcessParameter &parameter)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << parameter.deviceId
           << parameter.name
           << parameter.value
           << parameter.minimum
           << parameter.maximum
           << parameter.coreParameter;
    return payload;
}

bool Protocol::decodeParameterPayload(const QByteArray &payload, ProcessParameter *parameter)
{
    if (!parameter) {
        return false;
    }

    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> parameter->deviceId
           >> parameter->name
           >> parameter->value
           >> parameter->minimum
           >> parameter->maximum
           >> parameter->coreParameter;
    return stream.status() == QDataStream::Ok;
}

QVector<ProtocolFrame> ProtocolParser::feed(const QByteArray &bytes)
{
    buffer.append(bytes);

    QVector<ProtocolFrame> frames;
    ProtocolFrame frame;
    while (tryExtractOne(&frame)) {
        frames.append(frame);
    }
    return frames;
}

void ProtocolParser::reset()
{
    buffer.clear();
}

bool ProtocolParser::tryExtractOne(ProtocolFrame *frame)
{
    while (buffer.size() >= 2) {
        const quint8 lo = static_cast<quint8>(buffer.at(0));
        const quint8 hi = static_cast<quint8>(buffer.at(1));
        if (static_cast<quint16>(lo | (hi << 8)) == Protocol::Header) {
            break;
        }
        buffer.remove(0, 1);
    }

    if (buffer.size() < Protocol::FixedHeaderSize) {
        return false;
    }

    const quint16 payloadLength = static_cast<quint8>(buffer.at(4))
        | (static_cast<quint16>(static_cast<quint8>(buffer.at(5))) << 8);
    if (payloadLength > Protocol::MaxPayloadSize) {
        buffer.remove(0, 2);
        return tryExtractOne(frame);
    }

    const int packetSize = Protocol::FixedHeaderSize + payloadLength + Protocol::ChecksumSize;
    if (buffer.size() < packetSize) {
        return false;
    }

    const QByteArray packet = buffer.left(packetSize);
    const QByteArray checkedBytes = packet.left(packetSize - Protocol::ChecksumSize);
    const quint16 expected = Protocol::crc16Modbus(checkedBytes);
    const quint16 actual = static_cast<quint8>(packet.at(packetSize - 2))
        | (static_cast<quint16>(static_cast<quint8>(packet.at(packetSize - 1))) << 8);

    if (expected != actual) {
        buffer.remove(0, 1);
        return tryExtractOne(frame);
    }

    frame->deviceId = static_cast<quint8>(packet.at(2));
    frame->function = static_cast<FunctionCode>(static_cast<quint8>(packet.at(3)));
    frame->payload = packet.mid(Protocol::FixedHeaderSize, payloadLength);
    buffer.remove(0, packetSize);
    return true;
}

} // namespace DncScada
