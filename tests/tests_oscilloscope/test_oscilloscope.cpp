/***************************************************************************
 *   Copyright (C) 2026 KTechLab developers                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "ktechlab.h"
#include "simulator.h"
#include "oscilloscopedata.h"
#include <QDebug>
#include <QTest>

class KtlOscilloscopeTests : public QObject {
    Q_OBJECT
public:
    KTechlab *ktechlab;

private Q_SLOTS:
    void initTestCase()
    {
        ktechlab = new KTechlab;
    }

    void logicProbeDataTest1()
    {
        uint64_t t = 100;
        uint64_t dt = 10;
        LogicProbeData probe(1, 3, t);
        QVERIFY(probe.id() == 1);
        probe.setLabel(QStringLiteral("CH1"));
        QVERIFY(probe.label() == QStringLiteral("CH1"));
        QVERIFY(probe.isEmpty());
        QCOMPARE(probe.data().capacity(), 3);
        uint64_t t0 = probe.resetTime();
        QCOMPARE(t0, t);
        probe.addDataPoint(LogicDataPoint(true, t));
        QVERIFY(!probe.isEmpty());
        QCOMPARE(probe.data().size(), 1);

        QCOMPARE(probe.findPos(1), 0);
        QCOMPARE(probe.findPos(100), 0);
        QCOMPARE(probe.findPos(200), 0);
        QCOMPARE(probe.dataAtTime(1).value, true);

        t += dt;
        probe.addDataPoint(LogicDataPoint(false, t));
        QCOMPARE(probe.data().size(), 2);

        QCOMPARE(probe.findPos(1), 0);
        QCOMPARE(probe.findPos(100), 0);
        // FIXME: QCOMPARE(probe.findPos(101), 0);
        QCOMPARE(probe.findPos(110), 1);
        QCOMPARE(probe.findPos(111), 1);
        QCOMPARE(probe.findPos(200), 1);
        QCOMPARE(probe.dataAtTime(100).value, true);
        QCOMPARE(probe.dataAtTime(110).value, false);

        probe.addDataPoint(LogicDataPoint(true, t));
        QCOMPARE(probe.data().size(), 3);

        QCOMPARE(probe.findPos(1), 0);
        QCOMPARE(probe.findPos(100), 0);
        QCOMPARE(probe.findPos(110), 1);
        QCOMPARE(probe.findPos(120), 2);
        QCOMPARE(probe.findPos(200), 2);

        // This causes the buffer to discard the earliest data point
        // but the QContiguousCache retains the proper time index
        t += dt;
        probe.addDataPoint(LogicDataPoint(false, t));
        QCOMPARE(probe.data().size(), 3);

        QCOMPARE(probe.findPos(1), 1);
        QCOMPARE(probe.findPos(100), 1); // Discarded
        QCOMPARE(probe.findPos(110), 1);
        QCOMPARE(probe.findPos(120), 2);
        QCOMPARE(probe.findPos(130), 3);
        QCOMPARE(probe.findPos(200), 3);

        probe.setCapacity(4);
        QCOMPARE(probe.data().capacity(), 4);

        t += dt;
        probe.addDataPoint(LogicDataPoint(true, t));
        QCOMPARE(probe.data().size(), 4);
        QCOMPARE(probe.findPos(110), 1); // Not discarded since capacity was increased
        QCOMPARE(probe.findPos(140), 4);

        probe.eraseData(t);
        QCOMPARE(probe.data().size(), 1);
        QCOMPARE(probe.findPos(t), 0);
        QCOMPARE(probe.dataAtTime(t).value, true);
        QCOMPARE(probe.resetTime(), t);
    }

    void floatingProbeDataTest1()
    {
        uint64_t t = 100;
        uint64_t dt = 10;
        FloatingProbeData probe(2, 3, t, dt);
        QVERIFY(probe.id() == 2);
        QVERIFY(probe.isEmpty());
        uint64_t t0 = probe.resetTime();
        QCOMPARE(t0, t);
        t += dt;
        probe.addDataPoint(1.);
        QVERIFY(!probe.isEmpty());
        QCOMPARE(probe.data().size(), 1);

        QCOMPARE(probe.findPos(1), 0);
        QCOMPARE(probe.findPos(100), 0);
        QCOMPARE(probe.toTime(0), 100);
        QCOMPARE(probe.findPos(200), 0);
        QCOMPARE(probe.dataAtTime(1), 1.);

        t += dt;
        probe.addDataPoint(2.);
        QCOMPARE(probe.data().size(), 2);

        QCOMPARE(probe.findPos(1), 0);
        QCOMPARE(probe.findPos(100), 0);
        QCOMPARE(probe.toTime(0), 100);
        QCOMPARE(probe.findPos(110), 1);
        QCOMPARE(probe.toTime(1), 110);
        QCOMPARE(probe.findPos(200), 1);
        QCOMPARE(probe.dataAtTime(100), 1.);
        QCOMPARE(probe.dataAtTime(110), 2.);


        probe.addDataPoint(3.);
        QCOMPARE(probe.data().size(), 3);

        QCOMPARE(probe.findPos(1), 0);
        QCOMPARE(probe.findPos(100), 0);
        QCOMPARE(probe.findPos(110), 1);
        QCOMPARE(probe.findPos(120), 2);
        QCOMPARE(probe.findPos(200), 2);
        QCOMPARE(probe.dataAtTime(100), 1.);
        QCOMPARE(probe.dataAtTime(110), 2.);
        QCOMPARE(probe.dataAtTime(120), 3.);

        // This causes the buffer to discard the earliest data point
        // but the QContiguousCache retains the proper time index
        t += dt;
        probe.addDataPoint(4.);
        QCOMPARE(probe.data().size(), 3);

        QCOMPARE(probe.findPos(1), 1);
        QCOMPARE(probe.findPos(100), 1); // The data at this was discarded so index 0 is gone
        QCOMPARE(probe.findPos(110), 1);
        QCOMPARE(probe.findPos(120), 2);
        QCOMPARE(probe.findPos(130), 3);
        QCOMPARE(probe.findPos(200), 3);
        QCOMPARE(probe.dataAtTime(110), 2.);
        QCOMPARE(probe.dataAtTime(120), 3.);
        QCOMPARE(probe.dataAtTime(130), 4.);
        QCOMPARE(probe.dataAtTime(140), 4.);

        probe.setCapacity(4);
        QCOMPARE(probe.data().capacity(), 4);

        t += dt;
        probe.addDataPoint(5.);
        QCOMPARE(probe.data().size(), 4);
        QCOMPARE(probe.findPos(110), 1); // Not discarded since capacity was increased
        QCOMPARE(probe.findPos(140), 4);
        QCOMPARE(probe.dataAtTime(140), 5.);

        probe.eraseData(t);
        QCOMPARE(probe.data().size(), 0);
        QCOMPARE(probe.findPos(110), 0);
        QCOMPARE(probe.findPos(t), 0);
        // Clear erases the size not the data
        // QCOMPARE(probe.dataAtTime(t), 0);
        QCOMPARE(probe.resetTime(), t);
    }

    void cleanupTestCase()
    {
        delete ktechlab;
        ktechlab = nullptr;
    }
};

QTEST_MAIN(KtlOscilloscopeTests)
#include "test_oscilloscope.moc"
