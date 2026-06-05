#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <QByteArray>
#include <cstdio>
#include <cstdlib>

#include "simulator/simulatorcore.h"
#include "simulator/faultinjector.h"
#include "simulator/simdevicestatemachine.h"

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

    TEST("responseDelay clamped to 5000");
    fi.setResponseDelay(9999);
    CHECK(fi.responseDelay() == 5000, "should clamp to max 5000");

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
    if (resp.size() >= 6) {
        quint8 status = static_cast<quint8>(resp[5]);
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
    QThread::msleep(50);

    TEST("simulation running keeps Idle without start command");
    CHECK(core.stateMachine()->currentState() == SimDeviceStateMachine::Idle,
          "should stay Idle until start() is called");

    core.stateMachine()->start();
    QThread::msleep(100);

    quint32 stateAfterStart = core.statusValue();
    TEST("start() transitions to Starting");
    bool isStartingOrRunning = (stateAfterStart == 0x00000002) || (stateAfterStart == 0x00000004);
    CHECK(isStartingOrRunning, "should be Starting or Running after start()");

    core.stateMachine()->stop();
    QThread::msleep(100);

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

    printf("\n========================================\n");
    printf("RESULTS: %d passed, %d failed\n", g_passed, g_failed);
    printf("========================================\n");

    QTimer::singleShot(0, &app, [&]() {
        app.exit(g_failed > 0 ? 1 : 0);
    });

    return app.exec();
}
