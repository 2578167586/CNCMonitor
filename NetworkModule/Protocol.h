#ifndef DNC_SCADA_PROTOCOL_H
#define DNC_SCADA_PROTOCOL_H

#include "Types.h"

#include <QByteArray>
#include <QVector>

namespace DncScada {

class Protocol
{
public:
    static constexpr quint16 Header = 0xAA55;
    static constexpr int FixedHeaderSize = 6;
    static constexpr int ChecksumSize = 2;
    static constexpr int MaxPayloadSize = 4096;

    static quint16 crc16Modbus(const QByteArray &data);
    static QByteArray encodeFrame(const ProtocolFrame &frame);
    static QByteArray encodeStatusPayload(const DeviceStatus &status);
    static bool decodeStatusPayload(const QByteArray &payload, DeviceStatus *status);
    static QByteArray encodeParameterPayload(const ProcessParameter &parameter);
    static bool decodeParameterPayload(const QByteArray &payload, ProcessParameter *parameter);
};

class ProtocolParser
{
public:
    QVector<ProtocolFrame> feed(const QByteArray &bytes);
    void reset();

private:
    bool tryExtractOne(ProtocolFrame *frame);

    QByteArray buffer;
};

} // namespace DncScada

#endif // DNC_SCADA_PROTOCOL_H
