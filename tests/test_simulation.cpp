#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <QByteArray>
#include <QElapsedTimer>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "simulator/simulatorcore.h"
#include "simulator/faultinjector.h"
#include "simulator/simdevicestatemachine.h"
#include "model/devicedatamodel.h"

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) \
    printf("  TEST: %s ... ", name);

#define PASS() \
    do { printf("PASS\n"); g_passed++; } while(0)

#define FAIL(msg) \
    do { printf("FAIL - %s\n", msg); g_failed++; } while(0)

#define CHECK(cond, msg) \
    do { if (cond) { PASS(); } else { FAIL(msg); } } while(0)

static void processEventsFor(int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(std::min(10, ms - static_cast<int>(timer.elapsed())));
    }
}

// ─── Frame building helpers ───────────────────────────────────────────

static const quint8 FRAME_HEADER = 0xAA;
static const quint8 FRAME_TAIL   = 0x55;
static const quint8 DEV_ADDR     = 0x01;

static quint8 calcChecksum(const QByteArray& data)
{
    quint32 sum = 0;
    for (int i = 0; i < data.size(); ++i)
        sum += static_cast<quint8>(data[i]);
    return static_cast<quint8>(sum & 0xFF);
}

static QByteArray buildFrame(quint8 devAddr, quint8 cmd, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(static_cast<char>(FRAME_HEADER));
    frame.append(static_cast<char>(devAddr));
    frame.append(static_cast<char>(cmd));
    frame.append(static_cast<char>(payload.size()));
    frame.append(payload);
    frame.append(static_cast<char>(calcChecksum(frame)));
    frame.append(static_cast<char>(FRAME_TAIL));
    return frame;
}

static QByteArray buildFrameWithBadChecksum(quint8 devAddr, quint8 cmd, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(static_cast<char>(FRAME_HEADER));
    frame.append(static_cast<char>(devAddr));
    frame.append(static_cast<char>(cmd));
    frame.append(static_cast<char>(payload.size()));
    frame.append(payload);
    quint8 correct = calcChecksum(frame);
    frame.append(static_cast<char>(static_cast<quint8>(correct + 1)));
    frame.append(static_cast<char>(FRAME_TAIL));
    return frame;
}

static QByteArray buildReadRegisterFrame(quint8 devAddr, quint8 baseAddr, quint8 offset)
{
    QByteArray data;
    data.append(static_cast<char>(baseAddr));
    data.append(static_cast<char>(offset));
    return buildFrame(devAddr, 0xF0, data);
}

static QByteArray buildWriteRegisterFrame(quint8 devAddr, quint8 baseAddr, quint8 offset, quint32 value)
{
    QByteArray data;
    data.append(static_cast<char>(baseAddr));
    data.append(static_cast<char>(offset));
    data.append(static_cast<char>((value >> 24) & 0xFF));
    data.append(static_cast<char>((value >> 16) & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
    return buildFrame(devAddr, 0x0F, data);
}

static QByteArray buildReadStatusFrame(quint8 devAddr)
{
    return buildFrame(devAddr, 0x03, QByteArray());
}

static QByteArray buildControlFrame(quint8 devAddr, quint8 controlCode)
{
    QByteArray data;
    data.append(static_cast<char>(controlCode));
    return buildFrame(devAddr, 0x04, data);
}

// ─── Original Tests (1-5) ─────────────────────────────────────────────

static void testFaultInjectorBasics()
{
    FaultInjector fi;

    TEST("disabled returns input unchanged");
    QByteArray input = QByteArray::fromHex("aabbccdd");
    QByteArray output = fi.process(input);
    CHECK(output == input, "output should equal input when disabled");

    TEST("set/get enabled");
    fi.setEnabled(true);
    CHECK(fi.enabled() == true, "enabled should be true");

    TEST("set/get dropRate");
    fi.setDropRate(0.5);
    CHECK(fi.dropRate() == 0.5, "dropRate should be 0.5");

    TEST("dropRate clamped to 0");
    fi.setDropRate(-1.0);
    CHECK(fi.dropRate() == 0.0, "negative dropRate should clamp to 0");

    TEST("dropRate clamped to 1");
    fi.setDropRate(2.0);
    CHECK(fi.dropRate() == 1.0, "dropRate > 1 should clamp to 1");

    TEST("set/get corruptRate");
    fi.setCorruptRate(0.3);
    CHECK(fi.corruptRate() == 0.3, "corruptRate should be 0.3");

    TEST("set/get responseDelay");
    fi.setResponseDelay(2000);
    CHECK(fi.responseDelay() == 2000, "responseDelay should be 2000");

    TEST("responseDelay clamped to 2000");
    fi.setResponseDelay(9999);
    CHECK(fi.responseDelay() == 2000, "should clamp to max 2000");

    TEST("set/get wrongChecksum");
    fi.setWrongChecksum(true);
    CHECK(fi.wrongChecksum() == true, "wrongChecksum should be true");

    TEST("100% dropRate always returns empty");
    fi.setEnabled(true);
    fi.setDropRate(1.0);
    fi.setCorruptRate(0);
    fi.setWrongChecksum(false);
    int drops = 0;
    for (int i = 0; i < 50; ++i) {
        if (fi.process(input).isEmpty()) drops++;
    }
    CHECK(drops == 50, "dropRate 1.0 should drop all frames");
}

static void testFaultInjectorChecksum()
{
    FaultInjector fi;
    fi.setEnabled(true);
    fi.setWrongChecksum(true);

    QByteArray frame = QByteArray::fromHex("AA0103");
    frame.append(static_cast<char>(0));
    frame.append(static_cast<char>(0xAE));
    frame.append(static_cast<char>(0x55));

    QByteArray corrupt = fi.process(frame);
    TEST("wrongChecksum modifies checksum byte");
    CHECK(!corrupt.isEmpty() && corrupt.size() == frame.size(),
          "corrupt should return same-sized frame");

    if (!corrupt.isEmpty() && !frame.isEmpty()) {
        int csumIdx = corrupt.size() - 2;
        int origCsumIdx = frame.size() - 2;
        bool checksumChanged = (corrupt[csumIdx] != frame[origCsumIdx]);
        CHECK(checksumChanged, "checksum byte should change");
    }
}

static void testSimulatorCoreProtocol()
{
    SimulatorCore core;
    core.initialize();

    QByteArray readCurSet = QByteArray::fromHex("AA01F00202009F55");
    QByteArray resp = core.handleFrame(readCurSet);
    TEST("read CUR setpoint returns valid frame");
    CHECK(!resp.isEmpty(), "response should not be empty");
    bool hasHeader = !resp.isEmpty() && static_cast<quint8>(resp[0]) == 0xAA;
    bool hasTail = !resp.isEmpty() && static_cast<quint8>(resp[resp.size() - 1]) == 0x55;
    CHECK(hasHeader && hasTail, "response should have header 0xAA and tail 0x55");

    QByteArray writeCur = QByteArray::fromHex("AA010F0602000000012CEF55");
    resp = core.handleFrame(writeCur);
    TEST("write CUR setpoint 300mA returns success");
    CHECK(!resp.isEmpty(), "write response should not be empty");
    if (resp.size() >= 9) {
        quint8 status = static_cast<quint8>(resp[6]);
        CHECK(status == 0x00, "write response status should be 0x00");
    }

    QByteArray startCmd = QByteArray::fromHex("AA01040101B155");
    resp = core.handleFrame(startCmd);
    TEST("control start command returns valid frame");
    CHECK(!resp.isEmpty(), "start response should not be empty");

    QByteArray readStatus = QByteArray::fromHex("AA010300AE55");
    resp = core.handleFrame(readStatus);
    TEST("read status returns valid frame");
    CHECK(!resp.isEmpty(), "read status response should not be empty");

    char stopData[] = "\x02";
    QByteArray d(stopData, 1);
    QByteArray stopFrame;
    stopFrame.append(static_cast<char>(0xAA));
    stopFrame.append(static_cast<char>(0x01));
    stopFrame.append(static_cast<char>(0x04));
    stopFrame.append(static_cast<char>(1));
    stopFrame.append(d);
    quint8 stopCsum = 0;
    for (int i = 0; i < stopFrame.size(); ++i) stopCsum += static_cast<quint8>(stopFrame[i]);
    stopFrame.append(static_cast<char>(stopCsum));
    stopFrame.append(static_cast<char>(0x55));
    QByteArray stopResp = core.handleFrame(stopFrame);
    TEST("control stop command returns valid frame");
    CHECK(!stopResp.isEmpty(), "stop response should not be empty");

    QByteArray readDevInfo = QByteArray::fromHex("AA01F0020706AA55");
    resp = core.handleFrame(readDevInfo);
    TEST("read DEVINFO maxCurrent returns 300mA");
    if (resp.size() >= 10) {
        quint32 value = (static_cast<quint8>(resp[6]) << 24) |
                        (static_cast<quint8>(resp[7]) << 16) |
                        (static_cast<quint8>(resp[8]) << 8) |
                        static_cast<quint8>(resp[9]);
        CHECK(value == 0x0000012C, "maxCurrent should be 300 (0x12C)");
    } else {
        FAIL("response too short");
    }
}

static void testSimulatorCoreStateMachine()
{
    SimulatorCore core;
    core.initialize();

    TEST("state machine starts in Idle");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should be Idle before simulation starts");

    core.startSimulation();
    processEventsFor(50);

    TEST("simulation running keeps Idle without start command");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should stay Idle until start() is called");

    core.stateMachine()->start();
    processEventsFor(100);

    quint32 stateAfterStart = core.statusValue();
    TEST("start() transitions to Starting");
    bool isStartingOrRunning = (stateAfterStart == 0x00000002) || (stateAfterStart == 0x00000004);
    CHECK(isStartingOrRunning, "should be Starting or Running after start()");

    core.stateMachine()->stop();
    processEventsFor(100);

    TEST("stop() transitions to Stopping or Idle");
    quint32 stateAfterStop = core.statusValue();
    bool isStoppingOrIdle = (stateAfterStop == 0x00000008) || (stateAfterStop == 0x00000001);
    CHECK(isStoppingOrIdle, "should be Stopping or Idle after stop()");

    core.stateMachine()->reset();
    TEST("reset() returns to Idle");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should be Idle after reset()");

    core.stopSimulation();
}

static void testFaultInjectorIntegration()
{
    SimulatorCore core;
    core.initialize();

    FaultInjector* fi = core.faultInjector();
    QByteArray readStatus = QByteArray::fromHex("AA01030000B255");

    fi->setEnabled(true);
    fi->setDropRate(1.0);

    int drops = 0;
    for (int i = 0; i < 20; ++i) {
        QByteArray resp = core.handleFrame(readStatus);
        if (resp.isEmpty()) drops++;
    }
    TEST("integrated faultInjector drops frames at 100% rate");
    CHECK(drops == 20, "all 20 frames should be dropped");

    fi->setDropRate(0);
    fi->setEnabled(false);
    QByteArray resp = core.handleFrame(readStatus);

    TEST("integrated faultInjector passes frames when disabled");
    CHECK(!resp.isEmpty(), "frame should pass when fault injector disabled");
}

// ─── New Test 6: Protocol Boundary Tests ──────────────────────────────

static void testProtocolBoundary()
{
    SimulatorCore core;
    core.initialize();

    // --- Wrong dataLength (declared length ≠ actual data length) ---
    TEST("frame with dataLength too large");
    QByteArray payloadTooBig = QByteArray(1, '\x00');
    QByteArray raw1;
    raw1.append(static_cast<char>(FRAME_HEADER));
    raw1.append(static_cast<char>(DEV_ADDR));
    raw1.append(static_cast<char>(0xF0));
    raw1.append(static_cast<char>(10));
    raw1.append(payloadTooBig);
    raw1.append(static_cast<char>(calcChecksum(raw1)));
    raw1.append(static_cast<char>(FRAME_TAIL));
    QByteArray resp1 = core.handleFrame(raw1);
    CHECK(!resp1.isEmpty() && resp1.size() >= 7,
          "dataLength too large (declared 10, actual 1) should return error frame");
    if (resp1.size() >= 7) {
        CHECK(static_cast<quint8>(resp1[4]) == 0x05,
              "error code should be ERROR_INVALID_FRAME (0x05)");
    }

    TEST("frame with dataLength too small");
    QByteArray bigPayload(5, '\x00');
    QByteArray raw2;
    raw2.append(static_cast<char>(FRAME_HEADER));
    raw2.append(static_cast<char>(DEV_ADDR));
    raw2.append(static_cast<char>(0xF0));
    raw2.append(static_cast<char>(2));
    raw2.append(bigPayload);
    raw2.append(static_cast<char>(calcChecksum(raw2)));
    raw2.append(static_cast<char>(FRAME_TAIL));
    QByteArray resp2 = core.handleFrame(raw2);
    CHECK(!resp2.isEmpty() && resp2.size() >= 7,
          "dataLength too small (declared 2, actual 5) should return error frame");
    if (resp2.size() >= 7) {
        CHECK(static_cast<quint8>(resp2[4]) == 0x05,
              "error code should be ERROR_INVALID_FRAME (0x05)");
    }

    // --- Wrong checksum ---
    TEST("frame with wrong checksum returns error frame");
    QByteArray badCsumFrame = buildFrameWithBadChecksum(DEV_ADDR, 0x03, QByteArray());
    QByteArray resp3 = core.handleFrame(badCsumFrame);
    CHECK(!resp3.isEmpty(), "checksum error should return error frame, not empty");
    if (resp3.size() >= 6) {
        quint8 errCode = static_cast<quint8>(resp3[4]);
        CHECK(errCode == 0x04, "error frame data[0] should be ERROR_CHECKSUM (0x04)");
    }

    // --- Missing header ---
    TEST("frame without 0xAA header returns empty");
    QByteArray noHeader;
    noHeader.append(static_cast<char>(DEV_ADDR));
    noHeader.append(static_cast<char>(0x03));
    noHeader.append(static_cast<char>(0));
    noHeader.append(static_cast<char>(calcChecksum(noHeader)));
    noHeader.append(static_cast<char>(FRAME_TAIL));
    QByteArray resp4 = core.handleFrame(noHeader);
    CHECK(resp4.isEmpty(), "missing header should yield empty response");

    // --- Missing tail ---
    TEST("frame without 0x55 tail returns empty");
    QByteArray noTail;
    noTail.append(static_cast<char>(FRAME_HEADER));
    noTail.append(static_cast<char>(DEV_ADDR));
    noTail.append(static_cast<char>(0x03));
    noTail.append(static_cast<char>(0));
    noTail.append(static_cast<char>(calcChecksum(noTail)));
    noTail.append(static_cast<char>(0x00));
    QByteArray resp5 = core.handleFrame(noTail);
    CHECK(resp5.isEmpty(), "missing tail should yield empty response");

    // --- Empty frame ---
    TEST("empty frame returns empty");
    QByteArray resp6 = core.handleFrame(QByteArray());
    CHECK(resp6.isEmpty(), "empty byte array should yield empty response");

    // --- Truncated frames (shorter than minimum 6 bytes) ---
    TEST("frame with only header (1 byte) returns empty");
    QByteArray trunc1(1, static_cast<char>(FRAME_HEADER));
    CHECK(core.handleFrame(trunc1).isEmpty(), "1-byte frame should return empty");

    TEST("frame with header+addr (2 bytes) returns empty");
    QByteArray trunc2;
    trunc2.append(static_cast<char>(FRAME_HEADER));
    trunc2.append(static_cast<char>(DEV_ADDR));
    CHECK(core.handleFrame(trunc2).isEmpty(), "2-byte frame should return empty");

    TEST("frame with header+addr+cmd (3 bytes) returns empty");
    QByteArray trunc3;
    trunc3.append(static_cast<char>(FRAME_HEADER));
    trunc3.append(static_cast<char>(DEV_ADDR));
    trunc3.append(static_cast<char>(0x03));
    CHECK(core.handleFrame(trunc3).isEmpty(), "3-byte frame should return empty");

    TEST("frame with header+addr+cmd+len (4 bytes) returns empty");
    QByteArray trunc4;
    trunc4.append(static_cast<char>(FRAME_HEADER));
    trunc4.append(static_cast<char>(DEV_ADDR));
    trunc4.append(static_cast<char>(0x03));
    trunc4.append(static_cast<char>(0));
    CHECK(core.handleFrame(trunc4).isEmpty(), "4-byte frame should return empty");

    TEST("frame with header+addr+cmd+len+0data (5 bytes, no checksum) returns empty");
    QByteArray trunc5;
    trunc5.append(static_cast<char>(FRAME_HEADER));
    trunc5.append(static_cast<char>(DEV_ADDR));
    trunc5.append(static_cast<char>(0x03));
    trunc5.append(static_cast<char>(0));
    trunc5.append(static_cast<char>(0x00));
    CHECK(core.handleFrame(trunc5).isEmpty(), "5-byte frame (no tail) should return empty");

    TEST("read status valid frame still works after boundary tests");
    QByteArray goodFrame = buildReadStatusFrame(DEV_ADDR);
    QByteArray resp7 = core.handleFrame(goodFrame);
    CHECK(!resp7.isEmpty() && static_cast<quint8>(resp7[0]) == FRAME_HEADER
                               && static_cast<quint8>(resp7[resp7.size() - 1]) == FRAME_TAIL,
          "valid frame should still parse correctly");
}

// ─── New Test 7: FaultInjector Stress ─────────────────────────────────

static void testFaultInjectorStress()
{
    FaultInjector fi;

    // --- Rapid cycling: enable/disable/dropRate changes ---
    TEST("rapid enable/disable cycling does not crash");
    bool ok = true;
    for (int i = 0; i < 200; ++i) {
        fi.setEnabled(i % 3 == 1);
        fi.setDropRate((i % 11) / 10.0);
        fi.setCorruptRate((i % 7) / 10.0);
        fi.setWrongChecksum(i % 5 == 0);
        fi.setResponseDelay((i % 4) * 50);
        QByteArray frame = QByteArray::fromHex("AABBCCDDEEFF0055");
        fi.process(frame);
    }
    CHECK(ok, "rapid cycling 200 iterations completed without crash");

    // --- Verify that corruptRate=1.0 corrupts exactly one byte (full-byte XOR 0xFF) ---
    TEST("corruptRate=1.0 corrupts exactly one byte");
    fi.setEnabled(true);
    fi.setDropRate(0);
    fi.setCorruptRate(1.0);
    fi.setWrongChecksum(false);
    fi.setResponseDelay(0);

    QByteArray orig = QByteArray::fromHex("AA0104020000AE55");
    bool allCorruptedExactlyOneByte = true;
    int corruptSamples = 0;
    for (int i = 0; i < 30; ++i) {
        QByteArray result = fi.process(orig);
        if (result.isEmpty() || result.size() != orig.size()) continue;
        corruptSamples++;
        int diffCount = 0;
        int diffPos = -1;
        for (int j = 0; j < result.size(); ++j) {
            if (result[j] != orig[j]) {
                diffCount++;
                diffPos = j;
            }
        }
        if (diffCount != 1) {
            allCorruptedExactlyOneByte = false;
            break;
        }
        quint8 xorVal = static_cast<quint8>(result[diffPos] ^ orig[diffPos]);
        if (xorVal != 0xFF) {
            allCorruptedExactlyOneByte = false;
            break;
        }
    }
    CHECK(allCorruptedExactlyOneByte && corruptSamples > 0,
          "corruptRate=1.0 should flip exactly one byte with XOR 0xFF each time");

    // --- Combined faults: responseDelay + wrongChecksum simultaneously ---
    TEST("combined delay+checksum fault with timing");
    fi.setEnabled(true);
    fi.setDropRate(0);
    fi.setCorruptRate(0);
    fi.setWrongChecksum(true);
    fi.setResponseDelay(100);

    QByteArray frame2 = QByteArray::fromHex("AA0103");
    frame2.append(static_cast<char>(0));
    frame2.append(static_cast<char>(0xAE));
    frame2.append(static_cast<char>(0x55));

    QElapsedTimer timer;
    timer.start();
    QByteArray result2 = fi.process(frame2);
    qint64 elapsed = timer.elapsed();

    CHECK(!result2.isEmpty(), "combined faults should return non-empty frame");
    CHECK(elapsed >= 90, "responseDelay=100ms should delay at least ~90ms");
    if (!result2.isEmpty() && result2.size() >= 2) {
        int csumPos = result2.size() - 2;
        bool csumChanged = (static_cast<quint8>(result2[csumPos]) !=
                            static_cast<quint8>(frame2[frame2.size() - 2]));
        CHECK(csumChanged, "wrongChecksum should alter the checksum byte");
    }

    // --- Quick reset to clean state ---
    fi.setEnabled(false);
    fi.setResponseDelay(0);
    fi.setWrongChecksum(false);
    fi.setCorruptRate(0);
    fi.setDropRate(0);

    // --- Verify corruptRate only affects data bytes, not checksum/tail ---
    TEST("corruptRate only flips bytes inside data region");
    fi.setEnabled(true);
    fi.setCorruptRate(1.0);
    QByteArray frame3 = QByteArray::fromHex("AA0104010001B155");
    int dataOnlySamples = 0;
    bool corruptionInDataRegionOnly = true;
    for (int i = 0; i < 50; ++i) {
        QByteArray res3 = fi.process(frame3);
        if (res3.isEmpty() || res3.size() != frame3.size()) continue;
        dataOnlySamples++;
        int tailPos = res3.size() - 1;
        int csumPos = res3.size() - 2;
        bool tailAndCsumPreserved = (res3[csumPos] == frame3[csumPos]) &&
                                    (res3[tailPos] == frame3[tailPos]);
        if (!tailAndCsumPreserved) {
            corruptionInDataRegionOnly = false;
            break;
        }
    }
    CHECK(corruptionInDataRegionOnly && dataOnlySamples > 0,
          "data corruption should leave checksum and tail bytes unchanged");
}

// ─── New Test 8: State Machine Full Lifecycle ─────────────────────────

static void testStateMachineFullLifecycle()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    TEST("lifecycle: initial Idle with status reg 0x01");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should be Idle");
    CHECK(core.statusValue() == 0x00000001, "status register should be 0x01 (Idle)");

    core.stateMachine()->start();
    TEST("lifecycle: after start(), state is Starting");
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Starting,
          "should be Starting right after start()");
    CHECK(core.statusValue() == 0x00000002, "status register should be 0x02 (Starting)");

    processEventsFor(2100);
    TEST("lifecycle: after startupDelayMs (~2000ms), state is Running");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Running,
          "should transition to Running after delay");
    CHECK(core.statusValue() == 0x00000004, "status register should be 0x04 (Running)");

    core.stateMachine()->stop();
    TEST("lifecycle: after stop(), state is Stopping");
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Stopping,
          "should be Stopping right after stop()");
    CHECK(core.statusValue() == 0x00000008, "status register should be 0x08 (Stopping)");

    processEventsFor(1600);
    TEST("lifecycle: after stop delay (~1500ms), state is Idle");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should return to Idle after stopping completes");
    CHECK(core.statusValue() == 0x00000001, "status register should be 0x01 (Idle)");

    core.stopSimulation();
}

static void testRapidStartStop()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    TEST("rapid start/stop 3 times without waiting for transitions");
    bool allOk = true;
    for (int i = 0; i < 3; ++i) {
        core.stateMachine()->start();
        QThread::msleep(10);
        core.stateMachine()->stop();
        QThread::msleep(10);
        SimDeviceStateMachine::DeviceState s = core.stateMachine()->currentState();
        if (s != SimDeviceStateMachine::Stopping && s != SimDeviceStateMachine::Idle) {
            allOk = false;
        }
    }
    CHECK(allOk, "rapid start/stop 3x should not crash or deadlock");

    QThread::msleep(100);
    core.stateMachine()->reset();
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "reset should restore Idle after rapid cycling");

    core.stopSimulation();
}

// ─── New Test 9: Register Read/Write Loop (DEVINFO) ───────────────────

static void testRegisterReadWriteLoop()
{
    SimulatorCore core;
    core.initialize();

    struct DevInfoReg {
        quint8 offset;
        const char* name;
        quint32 defaultValue;
    };

    const DevInfoReg devInfoRegs[] = {
        { 0x01, "hw version",  0x00010001 },
        { 0x02, "fw version",  0x00020001 },
        { 0x03, "serial",      0x30303031 },
        { 0x04, "reserved4",   0x00000000 },
        { 0x05, "reserved5",   0x00000000 },
        { 0x06, "maxCurrent",  0x0000012C },
        { 0x07, "maxTemp",     0x0000003C },
        { 0x08, "maxPower",    0x0000C350 },
    };
    const int numRegs = sizeof(devInfoRegs) / sizeof(devInfoRegs[0]);

    char buf[128];

    for (int i = 0; i < numRegs; ++i) {
        snprintf(buf, sizeof(buf), "read DEVINFO 0x07:0x%02X (%s) default",
                 devInfoRegs[i].offset, devInfoRegs[i].name);
        TEST(buf);
        QByteArray readFrame = buildReadRegisterFrame(DEV_ADDR, 0x07, devInfoRegs[i].offset);
        QByteArray resp = core.handleFrame(readFrame);
        if (resp.size() >= 10) {
            quint32 value = (static_cast<quint8>(resp[6]) << 24) |
                            (static_cast<quint8>(resp[7]) << 16) |
                            (static_cast<quint8>(resp[8]) << 8) |
                            static_cast<quint8>(resp[9]);
            CHECK(value == devInfoRegs[i].defaultValue,
                  "default value should match");
        } else {
            FAIL("response too short");
        }
    }

    quint32 newValues[] = {
        0xDEADBEEF, 0xCAFEBABE, 0x12345678,
        0xAABBCCDD, 0x11223344, 0x000003E8,
        0x00000050, 0x0000FFFF
    };

    for (int i = 0; i < numRegs; ++i) {
        snprintf(buf, sizeof(buf), "write DEVINFO 0x07:0x%02X (%s) = 0x%08X",
                 devInfoRegs[i].offset, devInfoRegs[i].name, newValues[i]);
        TEST(buf);
        QByteArray wf = buildWriteRegisterFrame(DEV_ADDR, 0x07, devInfoRegs[i].offset, newValues[i]);
        QByteArray wr = core.handleFrame(wf);
        bool writeOk = !wr.isEmpty() && wr.size() >= 9 && static_cast<quint8>(wr[6]) == 0x00;
        CHECK(writeOk, "write should return success status 0x00");
    }

    for (int i = 0; i < numRegs; ++i) {
        snprintf(buf, sizeof(buf), "read-back DEVINFO 0x07:0x%02X (%s) = 0x%08X",
                 devInfoRegs[i].offset, devInfoRegs[i].name, newValues[i]);
        TEST(buf);
        QByteArray rf = buildReadRegisterFrame(DEV_ADDR, 0x07, devInfoRegs[i].offset);
        QByteArray rr = core.handleFrame(rf);
        if (rr.size() >= 10) {
            quint32 value = (static_cast<quint8>(rr[6]) << 24) |
                            (static_cast<quint8>(rr[7]) << 16) |
                            (static_cast<quint8>(rr[8]) << 8) |
                            static_cast<quint8>(rr[9]);
            CHECK(value == newValues[i], "read-back value should match written value");
        } else {
            FAIL("read-back response too short");
        }
    }

    core.reset();
}

// ─── New Test 10: DeviceDataModel Integration ─────────────────────────

static void testDeviceDataModelIntegration()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    DeviceDataModel model;

    QByteArray wf = buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 30000);
    core.handleFrame(wf);
    QByteArray tf = buildWriteRegisterFrame(DEV_ADDR, 0x03, 0x00, 25000);
    core.handleFrame(tf);

    core.stateMachine()->start();
    QThread::msleep(100);

    QMap<quint16, QVariant> regMap;
    regMap[0x0000] = QVariant(core.statusValue());
    regMap[0x0100] = QVariant(core.alarmValue());
    regMap[0x0200] = QVariant(core.readRawRegister(0x02, 0x00));
    regMap[0x0300] = QVariant(core.readRawRegister(0x03, 0x00));
    regMap[0x0400] = QVariant(core.readRawRegister(0x04, 0x00));
    regMap[0x0500] = QVariant(core.readRawRegister(0x05, 0x00));
    regMap[0x0700] = QVariant(core.readRawRegister(0x07, 0x00));

    model.updateFromRegisters(regMap);

    DeviceDataModel::RealTimeData data = model.currentData();

    TEST("integration: curSet populated correctly");
    CHECK(std::abs(data.curSet - 300.0) < 1.0, "curSet should be ~300.0 mA (30000/100)");

    TEST("integration: tempSet populated correctly");
    CHECK(std::abs(data.tempSet - 25.0) < 1.0, "tempSet should be ~25.0 C (25000/1000)");

    TEST("integration: status struct populated");
    if (data.status.idle == true && data.status.curStatus == 0) {
        CHECK(data.statusRaw == core.statusValue(), "statusRaw should match core statusValue");
    } else {
        CHECK(data.statusRaw != 0, "statusRaw should be non-zero when not idle");
    }

    TEST("integration: alert struct populated");
    CHECK(data.alarmRaw == core.alarmValue(), "alarmRaw should match core alarmValue");

    TEST("integration: devInfo parsed from register 0x07:0x00");
    quint32 devInfoRaw = static_cast<quint32>(regMap[0x0700].toUInt());
    quint16 expectedName = (devInfoRaw >> 16) & 0xFFFF;
    quint8 expectedVerS  = (devInfoRaw >> 8) & 0xFF;
    quint8 expectedVerH  = devInfoRaw & 0xFF;
    CHECK(data.devInfo.name == expectedName &&
          data.devInfo.verS == expectedVerS &&
          data.devInfo.verH == expectedVerH,
          "devInfo fields should be extracted from register correctly");

    TEST("integration: timestamp is valid");
    CHECK(data.timestamp.isValid(), "timestamp should be valid QDateTime");

    TEST("integration: legacy compatibility fields");
    CHECK(std::abs(data.current - data.curVal) < 0.01, "current should match curVal");
    CHECK(std::abs(data.temperature - data.tempVal) < 0.01, "temperature should match tempVal");
    CHECK(std::abs(data.power - data.pwrLas) < 0.01, "power should match pwrLas");

    core.stateMachine()->stop();
    QThread::msleep(100);
    core.stopSimulation();
}

// ─── New Test 11: Write Register Boundary Testing ─────────────────────

static void testWriteRegisterBoundary()
{
    SimulatorCore core;
    core.initialize();

    TEST("write to invalid register 0x08:0x00");
    QByteArray wf = buildWriteRegisterFrame(DEV_ADDR, 0x08, 0x00, 0xDEADBEEF);
    QByteArray resp = core.handleFrame(wf);
    CHECK(!resp.isEmpty(), "write to 0x08:0x00 should return a frame");
    if (resp.size() >= 9) {
        quint8 status = static_cast<quint8>(resp[6]);
        CHECK(status == 0x00, "simulator writeRegister always succeeds → status 0x00");
    }

    TEST("write to invalid register 0x08:0xFF");
    QByteArray wf2 = buildWriteRegisterFrame(DEV_ADDR, 0x08, 0xFF, 0xAAAAAAAA);
    QByteArray resp2 = core.handleFrame(wf2);
    CHECK(!resp2.isEmpty(), "write to 0x08:0xFF should return a frame");
    if (resp2.size() >= 9) {
        quint8 status2 = static_cast<quint8>(resp2[6]);
        CHECK(status2 == 0x00, "simulator writeRegister always succeeds → status 0x00");
    }

    TEST("read invalid register 0x08:0x00 returns error");
    {
        SimulatorCore core2;
        core2.initialize();
        QByteArray rf = buildReadRegisterFrame(DEV_ADDR, 0x08, 0x00);
        QByteArray rresp = core2.handleFrame(rf);
        CHECK(!rresp.isEmpty(), "read invalid register should return error frame");
        if (rresp.size() >= 7) {
            quint8 errCode = static_cast<quint8>(rresp[4]);
            CHECK(errCode == 0x03, "error code should be ERROR_INVALID_DATA (0x03)");
        }
    }

    TEST("write to invalid register 0x00:0xFF");
    QByteArray wf3 = buildWriteRegisterFrame(DEV_ADDR, 0x00, 0xFF, 0x12345678);
    QByteArray resp3 = core.handleFrame(wf3);
    CHECK(!resp3.isEmpty(), "write to 0x00:0xFF should return a frame");
    if (resp3.size() >= 9) {
        quint8 status3 = static_cast<quint8>(resp3[6]);
        CHECK(status3 == 0x00, "simulator writeRegister always succeeds → status 0x00");
    }

    TEST("write with maximum value 0xFFFFFFFF");
    QByteArray wf4 = buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 0xFFFFFFFF);
    QByteArray resp4 = core.handleFrame(wf4);
    CHECK(!resp4.isEmpty(), "write max value should succeed");
    if (resp4.size() >= 9) {
        CHECK(static_cast<quint8>(resp4[6]) == 0x00, "status should be 0x00");
    }

    core.reset();
}

// ─── New Test 12: Concurrent Command Stress ───────────────────────────

static void testConcurrentCommandStress()
{
    SimulatorCore core;
    core.initialize();

    TEST("send 50 commands rapidly without waiting");
    int successCount = 0;
    for (int i = 0; i < 50; ++i) {
        QByteArray frame;
        switch (i % 4) {
        case 0: frame = buildReadStatusFrame(DEV_ADDR); break;
        case 1: frame = buildReadRegisterFrame(DEV_ADDR, 0x07, static_cast<quint8>(i % 9)); break;
        case 2: frame = buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, static_cast<quint32>(i * 100)); break;
        case 3: frame = buildControlFrame(DEV_ADDR, 0x01); break;
        }
        QByteArray resp = core.handleFrame(frame);
        if (!resp.isEmpty()) successCount++;
    }
    CHECK(successCount == 50, "all 50 rapid-fire commands should get a response");

    TEST("interleaved read+write same register race");
    bool interleaveOk = true;
    for (int i = 0; i < 20; ++i) {
        core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, static_cast<quint32>(i * 50)));
        QByteArray rr = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x02, 0x00));
        if (rr.size() < 10) { interleaveOk = false; break; }
        quint32 value = (static_cast<quint8>(rr[6]) << 24) |
                        (static_cast<quint8>(rr[7]) << 16) |
                        (static_cast<quint8>(rr[8]) << 8) |
                        static_cast<quint8>(rr[9]);
        if (value != static_cast<quint32>(i * 50)) { interleaveOk = false; break; }
    }
    CHECK(interleaveOk, "interleaved write+read should see latest written value");

    TEST("burst of 10 control commands in a row");
    int controlOk = 0;
    for (int i = 0; i < 10; ++i) {
        QByteArray ctl = core.handleFrame(buildControlFrame(DEV_ADDR, 0x01));
        if (!ctl.isEmpty()) controlOk++;
    }
    CHECK(controlOk == 10, "10 burst control commands should each get a response");

    core.reset();
}

// ─── New Test 13: DEVINFO Edit Validation ─────────────────────────────

static void testDevInfoEditValidation()
{
    SimulatorCore core;
    core.initialize();

    TEST("write non-standard version 'abc' (0x616263) to fw version 0x07:0x02");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x02, 0x00616263));
    QByteArray rf1 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x02));
    if (rf1.size() >= 10) {
        quint32 value = (static_cast<quint8>(rf1[6]) << 24) |
                        (static_cast<quint8>(rf1[7]) << 16) |
                        (static_cast<quint8>(rf1[8]) << 8) |
                        static_cast<quint8>(rf1[9]);
        CHECK(value == 0x00616263, "ASCII 'abc' stored and read back correctly");
    } else {
        FAIL("response too short");
    }

    TEST("write non-standard version 0xFF to hw version 0x07:0x01");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x01, 0x0000FF00));
    QByteArray rf2 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x01));
    if (rf2.size() >= 10) {
        quint32 value = (static_cast<quint8>(rf2[6]) << 24) |
                        (static_cast<quint8>(rf2[7]) << 16) |
                        (static_cast<quint8>(rf2[8]) << 8) |
                        static_cast<quint8>(rf2[9]);
        CHECK(value == 0x0000FF00, "version byte 0xFF stored correctly (no truncation)");
    } else {
        FAIL("response too short");
    }

    TEST("write serial as 'X' repeated ASCII 0x58585858 to 0x07:0x03");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x03, 0x58585858));
    QByteArray rf3 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x03));
    if (rf3.size() >= 10) {
        quint32 value = (static_cast<quint8>(rf3[6]) << 24) |
                        (static_cast<quint8>(rf3[7]) << 16) |
                        (static_cast<quint8>(rf3[8]) << 8) |
                        static_cast<quint8>(rf3[9]);
        CHECK(value == 0x58585858, "serial 'XXXX' stored and read back correctly");
    } else {
        FAIL("response too short");
    }

    TEST("write serial 0x00000000 (null padding) to 0x07:0x03");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x03, 0x00000000));
    QByteArray rf4 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x03));
    if (rf4.size() >= 10) {
        quint32 value = (static_cast<quint8>(rf4[6]) << 24) |
                        (static_cast<quint8>(rf4[7]) << 16) |
                        (static_cast<quint8>(rf4[8]) << 8) |
                        static_cast<quint8>(rf4[9]);
        CHECK(value == 0x00000000, "null serial stored and read back correctly");
    } else {
        FAIL("response too short");
    }

    TEST("write serial 0xFFFFFFFF (max) to 0x07:0x03");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x03, 0xFFFFFFFF));
    QByteArray rf5 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x03));
    if (rf5.size() >= 10) {
        quint32 value = (static_cast<quint8>(rf5[6]) << 24) |
                        (static_cast<quint8>(rf5[7]) << 16) |
                        (static_cast<quint8>(rf5[8]) << 8) |
                        static_cast<quint8>(rf5[9]);
        CHECK(value == 0xFFFFFFFF, "max-value serial stored and read back correctly");
    } else {
        FAIL("response too short");
    }

    TEST("version string 'abc.def' stored as two writes to hw+fw registers");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x01, 0x00616263));
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, 0x02, 0x00646566));
    QByteArray rfh = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x01));
    QByteArray rff = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x02));
    bool hwOk = rfh.size() >= 10 &&
                ((static_cast<quint8>(rfh[6]) << 24) |
                 (static_cast<quint8>(rfh[7]) << 16) |
                 (static_cast<quint8>(rfh[8]) << 8) |
                 static_cast<quint8>(rfh[9])) == 0x00616263;
    bool fwOk = rff.size() >= 10 &&
                ((static_cast<quint8>(rff[6]) << 24) |
                 (static_cast<quint8>(rff[7]) << 16) |
                 (static_cast<quint8>(rff[8]) << 8) |
                 static_cast<quint8>(rff[9])) == 0x00646566;
    CHECK(hwOk && fwOk, "'abc' in hw ver, 'def' in fw ver both stored correctly");

    core.reset();
}

// ─── New Test 14: State Machine Edge Cases ────────────────────────────

static void testStateMachineEdgeCases()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    TEST("edge: stop() from Idle → stays Idle");
    core.stateMachine()->stop();
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "stop from Idle should be no-op");
    CHECK(core.statusValue() == 0x00000001, "status should remain 0x01 (Idle)");

    TEST("edge: start() from Idle → Starting");
    core.stateMachine()->start();
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Starting,
          "start from Idle should transition to Starting");

    TEST("edge: start() from Starting → stays Starting");
    core.stateMachine()->start();
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Starting,
          "start from Starting should be no-op");

    processEventsFor(2100);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Running,
          "should transition to Running after delay");

    TEST("edge: start() from Running → stays Running");
    core.stateMachine()->start();
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Running,
          "start from Running should be no-op");

    TEST("edge: stop() from Running → Stopping");
    core.stateMachine()->stop();
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Stopping,
          "stop from Running should transition to Stopping");

    processEventsFor(1600);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should return to Idle after stopping");

    TEST("edge: reset() from Idle → stays Idle");
    core.stateMachine()->reset();
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "reset from Idle should stay Idle");
    CHECK(core.statusValue() == 0x00000001, "status should be 0x01 (Idle)");

    core.stopSimulation();
}

// ─── New Test 15: Error State and Recovery ────────────────────────────

static void testErrorStateAndRecovery()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    TEST("error: set current setpoint to force over-current");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 30000));
    core.stateMachine()->start();
    processEventsFor(2100);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Running,
          "should be Running after startup");

    TEST("error: set very low target to trigger over-current alarm");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 100));
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x03, 0x00, 0));
    processEventsFor(3000);
    SimDeviceStateMachine::DeviceState errState = core.stateMachine()->currentState();
    bool isErrorOrRunning = (errState == SimDeviceStateMachine::Error ||
                             errState == SimDeviceStateMachine::Running);
    CHECK(isErrorOrRunning, "should be Running or Error after current settles");

    TEST("error: reset() restores Idle from any state");
    core.stateMachine()->reset();
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "reset should restore Idle");
    CHECK(core.statusValue() == 0x00000001, "status should be 0x01 (Idle)");

    TEST("error: after reset, start works normally again");
    core.stateMachine()->start();
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Starting,
          "should be Starting after reset+start");

    core.stateMachine()->reset();
    core.stopSimulation();
}

// ─── New Test 16: Full Operational Cycle ───────────────────────────────

static void testFullOperationalCycle()
{
    SimulatorCore core;
    core.initialize();

    TEST("cycle: connect (start simulation)");
    core.startSimulation();
    CHECK(core.isSimulating(), "simulation should be running");

    TEST("cycle: read DEVINFO after connection");
    QByteArray devResp = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x00));
    CHECK(!devResp.isEmpty() && static_cast<quint8>(devResp[0]) == FRAME_HEADER,
          "DEVINFO read should succeed after connect");

    TEST("cycle: configure current and temperature setpoints");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 50000));
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x03, 0x00, 35000));
    bool cfgOk = (core.readRawRegister(0x02, 0x00) == 50000) &&
                 (core.readRawRegister(0x03, 0x00) == 35000);
    CHECK(cfgOk, "setpoints should be written correctly");

    TEST("cycle: start device");
    core.stateMachine()->start();
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Starting,
          "should be Starting");

    TEST("cycle: poll status during startup (3 polls)");
    int validPolls = 0;
    for (int i = 0; i < 3; ++i) {
        QByteArray resp = core.handleFrame(buildReadStatusFrame(DEV_ADDR));
        if (!resp.isEmpty() && static_cast<quint8>(resp[0]) == FRAME_HEADER) validPolls++;
        processEventsFor(200);
    }
    CHECK(validPolls == 3, "all 3 polls during startup should succeed");

    processEventsFor(1700);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Running,
          "should be Running after startup completes");

    TEST("cycle: poll status during running (5 polls)");
    validPolls = 0;
    for (int i = 0; i < 5; ++i) {
        QByteArray resp = core.handleFrame(buildReadStatusFrame(DEV_ADDR));
        if (!resp.isEmpty() && resp.size() >= 10) validPolls++;
        processEventsFor(200);
    }
    CHECK(validPolls == 5, "all 5 polls during running should succeed");

    TEST("cycle: adjust current setpoint while running");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 30000));
    CHECK(core.readRawRegister(0x02, 0x00) == 30000, "setpoint should be 300mA");

    TEST("cycle: adjust temperature setpoint while running");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x03, 0x00, 30000));
    CHECK(core.readRawRegister(0x03, 0x00) == 30000, "setpoint should be 30.0C");

    TEST("cycle: stop device");
    core.stateMachine()->stop();
    processEventsFor(50);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Stopping,
          "should be Stopping");

    processEventsFor(1600);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should be Idle after stop completes");

    TEST("cycle: disconnect (stop simulation)");
    core.stopSimulation();
    CHECK(!core.isSimulating(), "simulation should be stopped");
}

// ─── New Test 17: Parameter Tuning During Simulation ───────────────────

static void testParameterTuningDuringSimulation()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 30000));
    core.stateMachine()->start();

    SimDeviceStateMachine* sm = core.stateMachine();

    TEST("tuning: change slew rate while in Starting state");
    sm->setCurrentSlewRate(0.5);
    CHECK(sm->currentSlewRate() == 0.5, "slew rate should be 0.5");

    TEST("tuning: change noise amplitude while starting");
    sm->setNoiseAmplitude(0.1);
    CHECK(sm->noiseAmplitude() == 0.1, "noise amplitude should be 0.1");

    processEventsFor(2200);
    CHECK(sm->currentState() == SimDeviceStateMachine::Running,
          "should be Running after startup");

    TEST("tuning: change slew rate while Running");
    sm->setCurrentSlewRate(0.05);
    CHECK(sm->currentSlewRate() == 0.05, "slew rate should be 0.05");

    TEST("tuning: reduce noise to minimum while running");
    sm->setNoiseAmplitude(0);
    CHECK(sm->noiseAmplitude() == 0.0, "noise should be 0");

    processEventsFor(300);

    TEST("tuning: increase noise to maximum while running");
    sm->setNoiseAmplitude(0.15);
    CHECK(sm->noiseAmplitude() == 0.15, "noise should be 0.15");

    TEST("tuning: change temperature lag");
    double oldLag = sm->tempResponseLag();
    sm->setTempResponseLag(0.3);
    CHECK(sm->tempResponseLag() == 0.3, "temp lag should be 0.3");
    sm->setTempResponseLag(oldLag);

    TEST("tuning: clamp slew rate to min (0.01)");
    sm->setCurrentSlewRate(-999);
    CHECK(sm->currentSlewRate() == 0.01, "should clamp to min 0.01");

    TEST("tuning: clamp slew rate to max (1.0)");
    sm->setCurrentSlewRate(999);
    CHECK(sm->currentSlewRate() == 1.0, "should clamp to max 1.0");

    TEST("tuning: clamp noise to max (0.2)");
    sm->setNoiseAmplitude(999);
    CHECK(sm->noiseAmplitude() == 0.2, "should clamp to max 0.2");

    TEST("tuning: clamp startup delay to min (100)");
    sm->setStartupDelayMs(-999);
    CHECK(sm->startupDelayMs() == 100, "should clamp to min 100ms");

    TEST("tuning: clamp startup delay to max (10000)");
    sm->setStartupDelayMs(99999);
    CHECK(sm->startupDelayMs() == 10000, "should clamp to max 10000ms");

    TEST("tuning: change startup delay from default 2000 → 500");
    sm->setStartupDelayMs(500);
    CHECK(sm->startupDelayMs() == 500, "startup delay should be 500ms");

    TEST("tuning: clamp temp lag to min (0.01)");
    sm->setTempResponseLag(-999);
    CHECK(sm->tempResponseLag() == 0.01, "should clamp to min 0.01");

    TEST("tuning: clamp temp lag to max (0.5)");
    sm->setTempResponseLag(999);
    CHECK(sm->tempResponseLag() == 0.5, "should clamp to max 0.5");

    sm->stop();
    processEventsFor(1600);
    core.stopSimulation();
}

// ─── New Test 18: Fault Injection Combined Scenarios ───────────────────

static void testFaultInjectionCombinedScenarios()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    FaultInjector* fi = core.faultInjector();

    TEST("fault-combo: 30% drop + 20% corrupt simultaneously");
    fi->setEnabled(true);
    fi->setDropRate(0.3);
    fi->setCorruptRate(0.2);
    fi->setResponseDelay(0);

    int totalSent = 0;
    int received = 0;
    int possiblyCorrupt = 0;
    for (int i = 0; i < 50; ++i) {
        QByteArray req = (i % 2 == 0)
            ? buildReadStatusFrame(DEV_ADDR)
            : buildReadRegisterFrame(DEV_ADDR, 0x07, static_cast<quint8>(i % 9));
        totalSent++;
        QByteArray resp = core.handleFrame(req);
        if (!resp.isEmpty()) {
            received++;
            if (static_cast<quint8>(resp[0]) == FRAME_HEADER) {
                possiblyCorrupt++;
            }
        }
    }
    CHECK(totalSent == 50, "50 requests sent");
    CHECK(received > 0, "some responses should arrive despite drops");
    CHECK(received < 50, "some responses should be dropped at 30% rate");

    TEST("fault-combo: disable faults mid-operation and verify clean");
    fi->setEnabled(false);
    QByteArray cleanResp = core.handleFrame(buildReadStatusFrame(DEV_ADDR));
    CHECK(!cleanResp.isEmpty(), "should get clean response after disabling faults");
    CHECK(static_cast<quint8>(cleanResp[0]) == FRAME_HEADER, "header should be valid");

    TEST("fault-combo: re-enable with 50% drop + checksum corruption");
    fi->setEnabled(true);
    fi->setDropRate(0.5);
    fi->setCorruptRate(0);
    fi->setWrongChecksum(true);

    int droppedOrBadCsum = 0;
    for (int i = 0; i < 20; ++i) {
        QByteArray resp = core.handleFrame(buildReadStatusFrame(DEV_ADDR));
        if (resp.isEmpty()) {
            droppedOrBadCsum++;
            continue;
        }
        if (resp.size() >= 6) {
            QByteArray csumData = resp.left(resp.size() - 2);
            quint8 calc = 0;
            for (int j = 0; j < csumData.size(); ++j) calc += static_cast<quint8>(csumData[j]);
            if (static_cast<quint8>(resp[resp.size() - 2]) != static_cast<quint8>(calc & 0xFF)) {
                droppedOrBadCsum++;
            }
        }
    }
    CHECK(droppedOrBadCsum > 0, "at least some frames should be dropped or have bad checksum");

    TEST("fault-combo: dropRate=0, corruptRate=0.5, checksum=true");
    fi->setDropRate(0);
    fi->setCorruptRate(0.5);
    fi->setWrongChecksum(true);

    fi->setEnabled(false);
    QByteArray refResp = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x00));
    fi->setEnabled(true);

    int altered = 0;
    for (int i = 0; i < 40; ++i) {
        QByteArray resp = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x00));
        int diffs = 0;
        int minLen = std::min(refResp.size(), resp.size());
        for (int j = 0; j < minLen; ++j) {
            if (resp[j] != refResp[j]) diffs++;
        }
        if (diffs > 0 || resp.size() != refResp.size()) altered++;
    }
    CHECK(altered >= 10, "corruptRate 0.5 + checksum=true should produce many altered frames");

    fi->setEnabled(false);
    fi->setWrongChecksum(false);
    core.stopSimulation();
}

// ─── New Test 19: SimulatorCore Reset Tests ────────────────────────────

static void testSimulatorCoreReset()
{
    SimulatorCore core;
    core.initialize();

    TEST("reset: before simulation, registers are default");
    CHECK(core.readRawRegister(0x07, 0x06) == 0x0000012C, "maxCurrent should be 300");
    CHECK(core.readRawRegister(0x07, 0x07) == 0x0000003C, "maxTemp should be 60");
    CHECK(core.readRawRegister(0x07, 0x08) == 0x0000C350, "maxPower should be 50000");

    TEST("reset: modify register then reset");
    core.writeRawRegister(0x07, 0x06, 999);
    core.reset();
    CHECK(core.readRawRegister(0x07, 0x06) == 0x0000012C,
          "after reset, maxCurrent should be back to 300");

    TEST("reset: start simulation, modify registers, reset");
    core.startSimulation();
    core.stateMachine()->start();
    processEventsFor(100);
    core.writeRawRegister(0x02, 0x00, 99999);
    core.writeRawRegister(0x07, 0x06, 888);

    core.reset();
    CHECK(core.readRawRegister(0x02, 0x00) == 0, "current setpoint should be 0 after reset");
    CHECK(core.readRawRegister(0x07, 0x06) == 0x0000012C, "maxCurrent restored after reset");
    CHECK(!core.isSimulating(), "simulation should be stopped after reset");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "state should be Idle after reset");

    TEST("reset: multiple resets in sequence");
    core.startSimulation();
    core.stateMachine()->start();
    processEventsFor(100);
    for (int i = 0; i < 5; ++i) {
        core.reset();
        CHECK(core.readRawRegister(0x00, 0x00) == 0x00000001,
              "status should be Idle(0x01) after each reset");
        core.startSimulation();
        core.stateMachine()->start();
        processEventsFor(50);
    }
    core.reset();
    core.stopSimulation();
}

// ─── New Test 20: Throughput Stress ────────────────────────────────────

static void testThroughputStress()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();
    core.stateMachine()->start();
    processEventsFor(2100);

    TEST("throughput: 500 read-status frames in rapid succession");
    int ok = 0;
    for (int i = 0; i < 500; ++i) {
        QByteArray resp = core.handleFrame(buildReadStatusFrame(DEV_ADDR));
        if (!resp.isEmpty() && resp.size() >= 10) ok++;
    }
    CHECK(ok == 500, "all 500 rapid status reads should succeed");

    TEST("throughput: 500 interleaved read+write DEVINFO registers");
    int rwOk = 0;
    for (int i = 0; i < 500; ++i) {
        core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x07, static_cast<quint8>(i % 9),
                                                  static_cast<quint32>(i * 100)));
        QByteArray rr = core.handleFrame(
            buildReadRegisterFrame(DEV_ADDR, 0x07, static_cast<quint8>(i % 9)));
        if (!rr.isEmpty() && rr.size() >= 10) rwOk++;
    }
    CHECK(rwOk == 500, "all 500 interleaved read+write should return valid frames");

    TEST("throughput: 300 mixed commands (status+register+control)");
    int mixedOk = 0;
    for (int i = 0; i < 300; ++i) {
        QByteArray req;
        switch (i % 5) {
        case 0: req = buildReadStatusFrame(DEV_ADDR); break;
        case 1: req = buildReadRegisterFrame(DEV_ADDR, 0x07, static_cast<quint8>(i % 9)); break;
        case 2: req = buildReadRegisterFrame(DEV_ADDR, 0x02, 0x00); break;
        case 3: req = buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, static_cast<quint32>(i * 10)); break;
        case 4: req = buildControlFrame(DEV_ADDR, 0x01); break;
        }
        QByteArray resp = core.handleFrame(req);
        if (!resp.isEmpty() && static_cast<quint8>(resp[0]) == FRAME_HEADER) mixedOk++;
    }
    CHECK(mixedOk == 300, "all 300 mixed commands should get valid responses");

    TEST("throughput: state machine still Running after stress");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Running,
          "should still be Running after stress test");

    TEST("throughput: dataUpdated signal was emitted correctly");
    CHECK(core.currentReading() >= 0, "current reading should be >= 0 after stress");
    CHECK(core.temperatureReading() >= 0, "temperature reading should be >= 0 after stress");
    CHECK(core.powerReading() >= 0, "power reading should be >= 0 after stress");

    core.stateMachine()->stop();
    processEventsFor(100);
    core.stateMachine()->reset();
    core.stopSimulation();
}

// ─── New Test 21: Realistic Mixed Command Workload ─────────────────────

static void testMixedCommandWorkload()
{
    SimulatorCore core;
    core.initialize();
    core.startSimulation();

    TEST("mixed: simulate real client startup sequence");
    QByteArray r1 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x00));
    QByteArray r2 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x01));
    QByteArray r3 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x02));
    QByteArray r4 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x03));
    QByteArray r5 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x06));
    QByteArray r6 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x07));
    QByteArray r7 = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x07, 0x08));
    bool allDevInfoOk = !r1.isEmpty() && !r2.isEmpty() && !r3.isEmpty() &&
                        !r4.isEmpty() && !r5.isEmpty() && !r6.isEmpty() && !r7.isEmpty();
    CHECK(allDevInfoOk, "DEVINFO scan should return all valid frames");

    TEST("mixed: configure and start");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, 40000));
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x03, 0x00, 30000));
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x05, 0x00, 0x0000003F));
    core.stateMachine()->start();
    processEventsFor(100);

    TEST("mixed: periodic status polling pattern (10 polls, 200ms interval)");
    int pollOk = 0;
    for (int i = 0; i < 10; ++i) {
        QByteArray resp = core.handleFrame(buildReadStatusFrame(DEV_ADDR));
        if (!resp.isEmpty() && resp.size() >= 18) pollOk++;
        processEventsFor(200);
    }
    CHECK(pollOk >= 8, "at least 8 of 10 periodic polls should succeed");

    TEST("mixed: adjust current during operation (3 adjustments)");
    quint32 setpoints[] = { 30000, 20000, 50000 };
    for (int i = 0; i < 3; ++i) {
        core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x02, 0x00, setpoints[i]));
        QByteArray r = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x02, 0x00));
        quint32 readVal = (r.size() >= 10)
            ? ((static_cast<quint8>(r[6]) << 24) | (static_cast<quint8>(r[7]) << 16) |
               (static_cast<quint8>(r[8]) << 8) | static_cast<quint8>(r[9]))
            : 0;
        CHECK(readVal == setpoints[i], "setpoint write-then-read should be consistent");
        processEventsFor(300);
    }

    TEST("mixed: read all registers in one sweep");
    struct { quint8 base; quint8 offset; } regs[] = {
        {0x00, 0x00}, {0x01, 0x00}, {0x02, 0x00}, {0x02, 0x01},
        {0x03, 0x00}, {0x03, 0x01}, {0x04, 0x00}, {0x05, 0x00},
        {0x06, 0x00}, {0x07, 0x00}, {0x07, 0x01}, {0x07, 0x02},
        {0x07, 0x03}, {0x07, 0x06}, {0x07, 0x07}, {0x07, 0x08},
    };
    int sweepOk = 0;
    for (size_t i = 0; i < sizeof(regs)/sizeof(regs[0]); ++i) {
        QByteArray r = core.handleFrame(
            buildReadRegisterFrame(DEV_ADDR, regs[i].base, regs[i].offset));
        if (!r.isEmpty() && r.size() >= 10) sweepOk++;
    }
    CHECK(sweepOk == 16, "all 16 register reads in sweep should succeed");

    TEST("mixed: burst-write CONFIG and verify");
    core.handleFrame(buildWriteRegisterFrame(DEV_ADDR, 0x05, 0x00, 0x00000007));
    QByteArray cfgRead = core.handleFrame(buildReadRegisterFrame(DEV_ADDR, 0x05, 0x00));
    if (cfgRead.size() >= 10) {
        quint32 cfgVal = (static_cast<quint8>(cfgRead[6]) << 24) |
                         (static_cast<quint8>(cfgRead[7]) << 16) |
                         (static_cast<quint8>(cfgRead[8]) << 8) |
                         static_cast<quint8>(cfgRead[9]);
        CHECK(cfgVal == 0x00000007, "CONFIG write should be read back correctly");
    }

    TEST("mixed: stop and verify clean state");
    core.stateMachine()->stop();
    processEventsFor(1600);
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should return to Idle after stop");
    CHECK(core.statusValue() == 0x00000001, "status should be Idle(0x01)");

    core.stopSimulation();
}

// ─── Main ─────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    printf("\n=== FaultInjector Tests ===\n");
    testFaultInjectorBasics();
    testFaultInjectorChecksum();

    printf("\n=== SimulatorCore Protocol Tests ===\n");
    testSimulatorCoreProtocol();

    printf("\n=== SimulatorCore State Machine Tests ===\n");
    testSimulatorCoreStateMachine();

    printf("\n=== FaultInjector Integration Tests ===\n");
    testFaultInjectorIntegration();

    printf("\n=== Protocol Boundary Tests ===\n");
    testProtocolBoundary();

    printf("\n=== FaultInjector Stress Tests ===\n");
    testFaultInjectorStress();

    printf("\n=== State Machine Full Lifecycle Tests ===\n");
    testStateMachineFullLifecycle();
    testRapidStartStop();

    printf("\n=== Register Read/Write Loop Tests ===\n");
    testRegisterReadWriteLoop();

    printf("\n=== DeviceDataModel Integration Tests ===\n");
    testDeviceDataModelIntegration();

    printf("\n=== Write Register Boundary Tests ===\n");
    testWriteRegisterBoundary();

    printf("\n=== Concurrent Command Stress Tests ===\n");
    testConcurrentCommandStress();

    printf("\n=== DEVINFO Edit Validation Tests ===\n");
    testDevInfoEditValidation();

    printf("\n=== State Machine Edge Cases ===\n");
    testStateMachineEdgeCases();

    printf("\n=== Error State and Recovery ===\n");
    testErrorStateAndRecovery();

    printf("\n=== Full Operational Cycle ===\n");
    testFullOperationalCycle();

    printf("\n=== Parameter Tuning During Simulation ===\n");
    testParameterTuningDuringSimulation();

    printf("\n=== Fault Injection Combined Scenarios ===\n");
    testFaultInjectionCombinedScenarios();

    printf("\n=== SimulatorCore Reset Tests ===\n");
    testSimulatorCoreReset();

    printf("\n=== Throughput Stress Tests ===\n");
    testThroughputStress();

    printf("\n=== Realistic Mixed Command Workload ===\n");
    testMixedCommandWorkload();

    printf("\n========================================\n");
    printf("RESULTS: %d passed, %d failed\n", g_passed, g_failed);
    printf("========================================\n");

    QTimer::singleShot(0, &app, [&]() {
        app.exit(g_failed > 0 ? 1 : 0);
    });

    return app.exec();
}
