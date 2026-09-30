#include "pin.h"
#include "wire.h"

#include <QDebug>
#include <QTest>


class KtlElectronicsTests: public QObject
{
    Q_OBJECT


private Q_SLOTS:
    void initTestCase()
    {
    }

    void pinSanityTest()
    {
        Pin p;
        QCOMPARE(p.voltage(), 0);
        QCOMPARE(p.current(), 0);
        QVERIFY(p.numWires() == 0);
        QVERIFY(!p.currentIsKnown());

        p.setVoltage(10);
        QCOMPARE(p.voltage(), 10);

        p.mergeCurrent(0.001);
        QCOMPARE(p.current(), 0.001);
        p.resetCurrent();
        QCOMPARE(p.current(), 0);

        QVERIFY(p.groundType() == Pin::gt_never);
        p.setGroundType(Pin::gt_always);
        QVERIFY(p.groundType() == Pin::gt_always);

    }

    void wireConnectionTest()
    {
        /// Wire from a -- b
        Pin a, b;
        Wire w(&a, &b);
        QVERIFY(a.numWires() == 1);
        QVERIFY(b.numWires() == 1);
        QVERIFY(a.outputWireList().contains(&w));
        QVERIFY(a.inputWireList().isEmpty());
        QVERIFY(b.inputWireList().contains(&w));
        QVERIFY(b.outputWireList().isEmpty());
        auto localPins = a.localConnectedPins();
        QCOMPARE(localPins.length(), 1);
        QVERIFY(localPins.contains(&b));
        localPins = b.localConnectedPins();
        QCOMPARE(localPins.length(), 1);
        QVERIFY(localPins.contains(&a));
    }

    void wireConnectionTest2()
    {
        /// Wires from a -- b -- c
        Pin a, b, c;
        Wire w1(&a, &b);
        Wire w2(&b, &c);
        QVERIFY(a.outputWireList().contains(&w1));
        QVERIFY(a.numWires() == 1);
        QVERIFY(b.inputWireList().contains(&w1));
        QVERIFY(b.outputWireList().contains(&w2));
        QVERIFY(b.numWires() == 2);
        QVERIFY(c.inputWireList().contains(&w2));
        QVERIFY(c.numWires() == 1);

        auto localPins = a.localConnectedPins();
        QCOMPARE(localPins.length(), 1);
        QVERIFY(localPins.contains(&b));

        localPins = b.localConnectedPins();
        QCOMPARE(localPins.length(), 2);
        QVERIFY(localPins.contains(&a) && localPins.contains(&c));

        localPins = c.localConnectedPins();
        QCOMPARE(localPins.length(), 1);
        QVERIFY(localPins.contains(&b));
    }

    void wireCurrentTest1()
    {
        /// Wire from a -- b
        Pin a, b;
        Wire w(&a, &b);
        a.setVoltage(1);
        b.setVoltage(1);
        QCOMPARE(w.voltage(), 1);

        // Current is not known on either pin
        QVERIFY(!w.calculateCurrent());
        QVERIFY(!w.currentIsKnown());

        // Check first branch of calculate current
        QVERIFY(a.numWires() == 1);
        a.mergeCurrent(0.1);
        a.setCurrentKnown(true);
        QVERIFY(w.calculateCurrent());
        QVERIFY(w.currentIsKnown());
        QCOMPARE(w.current(), 0.1);

        // Check Pin::calculateCurrentFromWires from output wire
        QVERIFY(!b.currentIsKnown());
        QVERIFY(b.calculateCurrentFromWires());
        QCOMPARE(b.current(), -w.current());
        QVERIFY(b.currentIsKnown());

        // Check second branch of calculate current
        a.setCurrentKnown(false);
        a.resetCurrent();
        QVERIFY(b.numWires() == 1);
        b.resetCurrent();
        b.mergeCurrent(1.0);
        b.setCurrentKnown(true);
        w.calculateCurrent();
        QCOMPARE(w.current(), -1.0); // Negative since output pin

        // Check Pin::calculateCurrentFromWires from input wire
        QVERIFY(!a.currentIsKnown());
        QVERIFY(a.calculateCurrentFromWires());
        QCOMPARE(a.current(), w.current());
        QVERIFY(a.currentIsKnown());

    }

    void wireCurrentTest2()
    {
        /// Two wires sharing a common pin/node
        //  a -- b -- c
        Pin a, b, c;
        Wire w1(&a, &b);
        Wire w2(&b, &c);

        a.setVoltage(1);
        b.setVoltage(1);
        c.setVoltage(1);
        QCOMPARE(w1.voltage(), 1);
        QCOMPARE(w2.voltage(), 1);

        QVERIFY(!w1.calculateCurrent());
        QVERIFY(!w2.calculateCurrent());
        QVERIFY(!w1.currentIsKnown());
        QVERIFY(!w2.currentIsKnown());

        // Set current middle node to test
        // case where not all wires have known current
        b.resetCurrent(); // Middle node current in == current out
        b.setCurrentKnown(true);
        QVERIFY(!w1.calculateCurrent());
        QVERIFY(!w1.currentIsKnown());
        QVERIFY(!w2.calculateCurrent());
        QVERIFY(!w2.currentIsKnown());

        // Set current on first node
        // This casues w2 to check the startPin and numWires > 2 case
        a.mergeCurrent(0.1);
        a.setCurrentKnown(true);
        QVERIFY(w1.calculateCurrent());
        QVERIFY(w1.currentIsKnown());
        QCOMPARE(w1.current(), a.current());

        QVERIFY(w2.calculateCurrent());
        QVERIFY(w2.currentIsKnown());
        QCOMPARE(w2.current(), a.current());

        // Clear and set current on last node
        // This casues w1 to check the endPin and numWires > 2 case
        w1.setCurrentKnown(false);
        w2.setCurrentKnown(false);
        a.resetCurrent();
        a.setCurrentKnown(false);
        c.mergeCurrent(0.1);
        c.setCurrentKnown(true);
        QVERIFY(w2.calculateCurrent());
        QVERIFY(w2.currentIsKnown());
        QCOMPARE(w2.current(), -c.current());
        QVERIFY(w1.calculateCurrent());
        QVERIFY(w1.currentIsKnown());
        QCOMPARE(w1.current(), -c.current());
    }

    void wireCurrentTest3()
    {
        // Three wires sharing a common pin/node
        //  a -- b -- c
        //       |
        //       d
        Pin a, b, c, d;
        Wire w1(&a, &b);
        Wire w2(&b, &c);
        Wire w3(&b, &d);
        a.mergeCurrent(0.1);
        a.setCurrentKnown(true);
        b.resetCurrent(); // By KCL b's current is known to be 0
        b.setCurrentKnown(true);
        c.mergeCurrent(-0.2);
        c.setCurrentKnown(true);
        QVERIFY(w1.calculateCurrent());
        QVERIFY(w1.currentIsKnown());
        QCOMPARE(w1.current(), 0.1);
        QVERIFY(w2.calculateCurrent());
        QVERIFY(w2.currentIsKnown());
        QCOMPARE(w2.current(), 0.2);
        QVERIFY(w3.calculateCurrent());
        QVERIFY(w3.currentIsKnown());
        QCOMPARE(w3.current(), -0.1);
    }

    void cleanupTestCase()
    {
    }
};

QTEST_MAIN(KtlElectronicsTests)
#include "test_electronics.moc"

