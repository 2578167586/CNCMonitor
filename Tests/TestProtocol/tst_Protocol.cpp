#include <QtTest>

#include "Protocol.h"
#include "Types.h"

using namespace DncScada;

class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    // ==================== CRC-16/MODBUS ====================
    void crc16_knownAnswer_data();
    void crc16_knownAnswer();
    void crc16_emptyData();
    void crc16_singleByte();
    void crc16_consistency();
    void crc16_damagedFrame();

    // ==================== encodeFrame ====================
    void encodeFrame_minimalFrame();
    void encodeFrame_hasHeader();
    void encodeFrame_lengthField();
    void encodeFrame_includesPayload();
    void encodeFrame_hasCrc();

    // ==================== encodeStatusPayload / decodeStatusPayload ====================
    void statusPayload_roundTrip_data();
    void statusPayload_roundTrip();
    void decodeStatusPayload_nullPointer();
    void decodeStatusPayload_truncatedPayload();

    // ==================== encodeParameterPayload / decodeParameterPayload ====================
    void parameterPayload_roundTrip_data();
    void parameterPayload_roundTrip();
    void decodeParameterPayload_nullPointer();

    // ==================== ProtocolParser ====================
    void parser_singleFrame();
    void parser_stickyPacket_twoFrames();
    void parser_halfPacket();
    void parser_garbageBeforeHeader();
    void parser_payloadExceedsMaxSize();
    void parser_crcMismatch();
    void parser_reset();
    void parser_emptyFeed();
    void parser_multipleGarbageInterleaved();
    void parser_partialHeaderAcrossChunks();
};

// ==================== CRC-16/MODBUS tests ====================

void TestProtocol::crc16_knownAnswer_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<quint16>("expected");

    // Standard MODBUS CRC-16 test vector: "123456789" → 0x4B37
    QTest::newRow("standard_vector") << QByteArray("123456789", 9) << quint16(0x4B37);

    // Empty data → CRC stays at initial value 0xFFFF
    QTest::newRow("empty") << QByteArray() << quint16(0xFFFF);

    // Known value: single byte 0x00 → CRC = 0x4040 (post-computation)
    QTest::newRow("single_zero") << QByteArray("\x00", 1) << quint16(0x40BF);
}

void TestProtocol::crc16_knownAnswer()
{
    QFETCH(QByteArray, data);
    QFETCH(quint16, expected);
    QCOMPARE(Protocol::crc16Modbus(data), expected);
}

void TestProtocol::crc16_emptyData()
{
    // CRC of empty data: initial value 0xFFFF, no bytes processed
    QCOMPARE(Protocol::crc16Modbus(QByteArray()), quint16(0xFFFF));
}

void TestProtocol::crc16_singleByte()
{
    // CRC should be deterministic
    QByteArray data("\x42", 1);
    quint16 a = Protocol::crc16Modbus(data);
    quint16 b = Protocol::crc16Modbus(data);
    QCOMPARE(a, b);
}

void TestProtocol::crc16_consistency()
{
    // Same data → same CRC
    QByteArray data = QByteArray::fromHex("00112233445566778899AABBCCDDEEFF");
    QCOMPARE(Protocol::crc16Modbus(data), Protocol::crc16Modbus(data));
}

void TestProtocol::crc16_damagedFrame()
{
    QByteArray original = QByteArray::fromHex("AA5501010000");
    quint16 ok = Protocol::crc16Modbus(original);

    // Flip one bit
    QByteArray damaged = original;
    damaged[2] = damaged.at(2) ^ 0x01;
    quint16 bad = Protocol::crc16Modbus(damaged);

    QVERIFY(ok != bad);
}

// ==================== encodeFrame tests ====================

void TestProtocol::encodeFrame_minimalFrame()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::Heartbeat;
    // 6 header + 0 payload + 2 crc = 8
    QByteArray packet = Protocol::encodeFrame(frame);
    QCOMPARE(packet.size(), Protocol::FixedHeaderSize + Protocol::ChecksumSize);
}

void TestProtocol::encodeFrame_hasHeader()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::StatusReport;
    QByteArray packet = Protocol::encodeFrame(frame);

    quint16 header = static_cast<quint8>(packet.at(0))
        | (static_cast<quint16>(static_cast<quint8>(packet.at(1))) << 8);
    QCOMPARE(header, Protocol::Header);
}

void TestProtocol::encodeFrame_lengthField()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::StatusReport;
    frame.payload = QByteArray(100, '\x42');
    QByteArray packet = Protocol::encodeFrame(frame);

    quint16 payloadLen = static_cast<quint8>(packet.at(4))
        | (static_cast<quint16>(static_cast<quint8>(packet.at(5))) << 8);
    QCOMPARE(payloadLen, quint16(100));
}

void TestProtocol::encodeFrame_includesPayload()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::StatusReport;
    frame.payload = QByteArray("test_payload");
    QByteArray packet = Protocol::encodeFrame(frame);

    QByteArray extractedPayload = packet.mid(Protocol::FixedHeaderSize, frame.payload.size());
    QCOMPARE(extractedPayload, frame.payload);
}

void TestProtocol::encodeFrame_hasCrc()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::Heartbeat;
    QByteArray packet = Protocol::encodeFrame(frame);

    const int crcPos = packet.size() - Protocol::ChecksumSize;
    QByteArray checkedBytes = packet.left(crcPos);
    quint16 expected = Protocol::crc16Modbus(checkedBytes);
    quint16 actual = static_cast<quint8>(packet.at(crcPos))
        | (static_cast<quint16>(static_cast<quint8>(packet.at(crcPos + 1))) << 8);
    QCOMPARE(actual, expected);
}

// ==================== StatusPayload encode/decode tests ====================

void TestProtocol::statusPayload_roundTrip_data()
{
    QTest::addColumn<DeviceStatus>("original");

    {
        DeviceStatus s;
        s.deviceId = 1;
        s.x = 0.0; s.y = 0.0; s.z = 0.0;
        s.spindleRpm = 0;
        s.servoLoad = 0.0;
        s.temperature = 0.0;
        s.runState = MachineRunState::Stop;
        s.alarmCode = 0;
        QTest::newRow("all_zeros") << s;
    }
    {
        DeviceStatus s;
        s.deviceId = 42;
        s.x = 123.456; s.y = -78.9; s.z = 0.001;
        s.spindleRpm = 15000;
        s.servoLoad = 85.5;
        s.temperature = 62.3;
        s.runState = MachineRunState::Run;
        s.alarmCode = 0;
        QTest::newRow("normal_run") << s;
    }
    {
        DeviceStatus s;
        s.deviceId = 255;
        s.x = -9999.999; s.y = 9999.999; s.z = -0.5;
        s.spindleRpm = -1;       // edge case
        s.servoLoad = 100.0;
        s.temperature = -40.0;
        s.runState = MachineRunState::Alarm;
        s.alarmCode = 0xFFFF;
        QTest::newRow("boundary_alarm") << s;
    }
}

void TestProtocol::statusPayload_roundTrip()
{
    QFETCH(DeviceStatus, original);

    QByteArray payload = Protocol::encodeStatusPayload(original);
    QVERIFY(!payload.isEmpty());

    DeviceStatus decoded;
    QVERIFY(Protocol::decodeStatusPayload(payload, &decoded));

    // Verify all fields survive the round trip
    QCOMPARE(decoded.x, original.x);
    QCOMPARE(decoded.y, original.y);
    QCOMPARE(decoded.z, original.z);
    QCOMPARE(decoded.spindleRpm, original.spindleRpm);
    QCOMPARE(decoded.servoLoad, original.servoLoad);
    QCOMPARE(decoded.temperature, original.temperature);
    QCOMPARE(static_cast<int>(decoded.runState), static_cast<int>(original.runState));
    QCOMPARE(decoded.alarmCode, original.alarmCode);
}

void TestProtocol::decodeStatusPayload_nullPointer()
{
    QVERIFY(!Protocol::decodeStatusPayload(QByteArray(100, '\x00'), nullptr));
}

void TestProtocol::decodeStatusPayload_truncatedPayload()
{
    QByteArray truncated(4, '\x00'); // way too short
    DeviceStatus status;
    QVERIFY(!Protocol::decodeStatusPayload(truncated, &status));
}

// ==================== ParameterPayload encode/decode tests ====================

void TestProtocol::parameterPayload_roundTrip_data()
{
    QTest::addColumn<ProcessParameter>("original");

    {
        ProcessParameter p;
        p.deviceId = 10;
        p.name = QStringLiteral("FeedRate");
        p.value = 1500.0;
        p.minimum = 100.0;
        p.maximum = 3000.0;
        p.coreParameter = true;
        QTest::newRow("core_parameter") << p;
    }
    {
        ProcessParameter p;
        p.deviceId = 5;
        p.name = QStringLiteral("CoolantFlow");
        p.value = 2.5;
        p.minimum = 1.0;
        p.maximum = 5.0;
        p.coreParameter = false;
        QTest::newRow("non_core_parameter") << p;
    }
    {
        ProcessParameter p;
        p.deviceId = 255;
        p.name = QString();   // empty name
        p.value = 0.0;
        p.minimum = 0.0;
        p.maximum = 0.0;
        p.coreParameter = false;
        QTest::newRow("empty_name") << p;
    }
}

void TestProtocol::parameterPayload_roundTrip()
{
    QFETCH(ProcessParameter, original);

    QByteArray payload = Protocol::encodeParameterPayload(original);
    QVERIFY(!payload.isEmpty());

    ProcessParameter decoded;
    QVERIFY(Protocol::decodeParameterPayload(payload, &decoded));

    QCOMPARE(decoded.deviceId, original.deviceId);
    QCOMPARE(decoded.name, original.name);
    QCOMPARE(decoded.value, original.value);
    QCOMPARE(decoded.minimum, original.minimum);
    QCOMPARE(decoded.maximum, original.maximum);
    QCOMPARE(decoded.coreParameter, original.coreParameter);
}

void TestProtocol::decodeParameterPayload_nullPointer()
{
    QVERIFY(!Protocol::decodeParameterPayload(QByteArray(50, '\x00'), nullptr));
}

// ==================== ProtocolParser tests ====================

void TestProtocol::parser_singleFrame()
{
    ProtocolFrame frame;
    frame.deviceId = 3;
    frame.function = FunctionCode::StatusReport;
    DeviceStatus status;
    status.x = 10.0; status.y = 20.0; status.z = 30.0;
    status.spindleRpm = 1000;
    status.servoLoad = 50.0;
    status.temperature = 45.0;
    status.runState = MachineRunState::Run;
    status.alarmCode = 0;
    frame.payload = Protocol::encodeStatusPayload(status);

    QByteArray packet = Protocol::encodeFrame(frame);

    ProtocolParser parser;
    QVector<ProtocolFrame> frames = parser.feed(packet);

    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames[0].deviceId, frame.deviceId);
    QCOMPARE(static_cast<int>(frames[0].function), static_cast<int>(frame.function));
    QCOMPARE(frames[0].payload, frame.payload);
}

void TestProtocol::parser_stickyPacket_twoFrames()
{
    // Build two valid frames
    ProtocolFrame f1; f1.deviceId = 1; f1.function = FunctionCode::Heartbeat;
    ProtocolFrame f2; f2.deviceId = 2; f2.function = FunctionCode::StatusReport;
    DeviceStatus s; s.x = 5.0; s.runState = MachineRunState::Stop;
    f2.payload = Protocol::encodeStatusPayload(s);

    QByteArray sticky = Protocol::encodeFrame(f1) + Protocol::encodeFrame(f2);

    ProtocolParser parser;
    QVector<ProtocolFrame> frames = parser.feed(sticky);

    QCOMPARE(frames.size(), 2);
    QCOMPARE(frames[0].deviceId, quint8(1));
    QCOMPARE(frames[1].deviceId, quint8(2));
}

void TestProtocol::parser_halfPacket()
{
    ProtocolFrame frame;
    frame.deviceId = 7;
    frame.function = FunctionCode::StatusReport;
    DeviceStatus s; s.x = 99.0; s.runState = MachineRunState::Run;
    frame.payload = Protocol::encodeStatusPayload(s);
    QByteArray packet = Protocol::encodeFrame(frame);

    const int split = packet.size() / 2;
    QByteArray firstHalf = packet.left(split);
    QByteArray secondHalf = packet.mid(split);

    ProtocolParser parser;
    QVector<ProtocolFrame> frames1 = parser.feed(firstHalf);
    QVERIFY(frames1.isEmpty()); // incomplete frame

    QVector<ProtocolFrame> frames2 = parser.feed(secondHalf);
    QCOMPARE(frames2.size(), 1);
    QCOMPARE(frames2[0].deviceId, quint8(7));
}

void TestProtocol::parser_garbageBeforeHeader()
{
    ProtocolFrame frame;
    frame.deviceId = 5;
    frame.function = FunctionCode::Heartbeat;
    QByteArray packet = Protocol::encodeFrame(frame);

    QByteArray garbage = QByteArray::fromHex("DEADBEEFCAFE");
    QByteArray input = garbage + packet;

    ProtocolParser parser;
    QVector<ProtocolFrame> frames = parser.feed(input);

    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames[0].deviceId, quint8(5));
}

void TestProtocol::parser_payloadExceedsMaxSize()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::StatusReport;
    frame.payload = QByteArray(Protocol::MaxPayloadSize + 100, '\x42');
    QByteArray packet = Protocol::encodeFrame(frame);

    ProtocolParser parser;
    QVector<ProtocolFrame> frames = parser.feed(packet);
    QVERIFY(frames.isEmpty()); // rejected due to oversized payload
}

void TestProtocol::parser_crcMismatch()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::Heartbeat;
    QByteArray packet = Protocol::encodeFrame(frame);

    // Corrupt the CRC bytes at the end
    const int last = packet.size() - 1;
    packet[last] = static_cast<char>(packet.at(last) ^ 0xFF);
    packet[last - 1] = static_cast<char>(packet.at(last - 1) ^ 0xFF);

    ProtocolParser parser;
    QVector<ProtocolFrame> frames = parser.feed(packet);
    QVERIFY(frames.isEmpty());
}

void TestProtocol::parser_reset()
{
    ProtocolFrame frame;
    frame.deviceId = 1;
    frame.function = FunctionCode::Heartbeat;
    QByteArray packet = Protocol::encodeFrame(frame);

    const int split = packet.size() / 2;
    ProtocolParser parser;
    parser.feed(packet.left(split)); // half a frame in buffer
    parser.reset();

    QVector<ProtocolFrame> frames = parser.feed(packet); // fresh start
    QCOMPARE(frames.size(), 1);
}

void TestProtocol::parser_emptyFeed()
{
    ProtocolParser parser;
    QVector<ProtocolFrame> frames = parser.feed(QByteArray());
    QVERIFY(frames.isEmpty());
}

void TestProtocol::parser_multipleGarbageInterleaved()
{
    ProtocolFrame frame;
    frame.deviceId = 9;
    frame.function = FunctionCode::Heartbeat;
    QByteArray packet = Protocol::encodeFrame(frame);

    ProtocolParser parser;

    // Feed garbage between valid frames
    QVector<ProtocolFrame> frames1 = parser.feed(
        QByteArray::fromHex("FF00FF") + packet + QByteArray::fromHex("112233"));
    QCOMPARE(frames1.size(), 1);
    QCOMPARE(frames1[0].deviceId, quint8(9));
    QVERIFY(parser.feed(packet).size() == 1);
}

void TestProtocol::parser_partialHeaderAcrossChunks()
{
    ProtocolFrame frame;
    frame.deviceId = 3;
    frame.function = FunctionCode::Heartbeat;
    QByteArray packet = Protocol::encodeFrame(frame);

    // Feed one byte at a time to stress-test the state machine
    ProtocolParser parser;
    int totalFrames = 0;
    for (char byte : packet) {
        QByteArray chunk(&byte, 1);
        totalFrames += parser.feed(chunk).size();
    }
    QCOMPARE(totalFrames, 1);
}

QTEST_MAIN(TestProtocol)

#include "tst_Protocol.moc"
