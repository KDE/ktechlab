#include "circuit.h"
#include "capacitance.h"
#include "diode.h"
#include "elementset.h"
#include "inductance.h"
#include "measurements.h"
#include "pin.h"
#include "resistance.h"
#include "switch.h"
#include "voltagepoint.h"
#include "voltagesignal.h"
#include "wire.h"

#include <QDebug>
#include <QTest>


// Calculates the Lambert W function for large x
// Using the approximation W(x) = lnx-lnlnx+o(1)
// See https://en.wikipedia.org/wiki/Lambert_W_function
static double lambert(double x)
{
    // TODO: What is the + o(1) part
    Q_ASSERT(x > 1e10);
    const double r = std::log(x) - std::log(std::log(x));
    qDebug() << "lambert(x=" << x << ") = " << r;
    return r;
}

// Checks that actual is within expected +- the tolerance
#define QCOMPARE_TOL(actual, expected, tol) if (!std::isfinite(actual) || !std::isfinite(expected) || std::abs((actual) - (expected)) > tol) QCOMPARE(actual, expected);


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

    void resistanceTest1()
    {
        // Circuit with (V1) - [R1] - GND
        Circuit c;
        ElementSet eset(&c, 1, 1);
        QCOMPARE(eset.cnodeCount(), 1);
        QCOMPARE(eset.cbranchCount(), 1);
        QVERIFY(eset.ground().num == CNode::GROUND);

        VoltagePoint v1(5.0);
        QVERIFY(v1.type() == Element::Element_VoltagePoint);
        QCOMPARE(v1.voltage(), 5.0);
        Resistance r1(1000);
        QVERIFY(r1.type() == Element::Element_Resistance);
        QCOMPARE(r1.resistance(), 1000);
        QCOMPARE(r1.conductance(), 0.001);

        eset.addElement(&v1);
        eset.addElement(&r1);
        QVERIFY(!eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        r1.setCNodes(0, -1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();

        eset.doLinear(true);
        eset.displayEquations();

        QCOMPARE(eset.cnodes()[0].v, 5.0);
        QCOMPARE(eset.cbranches()[0].i, 0.005);
    }

    void resistanceSeriesTest2()
    {
        // Circuit with (V1) - [R1] - [R2] - GND
        Circuit c;
        ElementSet eset(&c, 2, 1);
        QCOMPARE(eset.cnodeCount(), 2);
        QCOMPARE(eset.cbranchCount(), 1);

        VoltagePoint v1(12.0);
        Resistance r1(1000);
        Resistance r2(3000);
        eset.addElement(&v1);
        eset.addElement(&r1);
        eset.addElement(&r2);
        QVERIFY(!eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        r1.setCNodes(0, 1);
        r2.setCNodes(1, -1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();
        r2.add_initial_dc();

        eset.doLinear(true);
        eset.displayEquations();

        QCOMPARE(eset.cnodes()[0].v, 12.0);
        QCOMPARE(eset.cnodes()[1].v, 9.0);
        QCOMPARE(eset.cbranches()[0].i, 0.003);
    }

    void capacitanceTest1()
    {
        // Circuit with (V1) - [R1] - |C1| - GND
        // See https://en.wikipedia.org/wiki/RC_circuit#Time-domain_considerations
        Circuit c;
        ElementSet eset(&c, 2, 1);
        QCOMPARE(eset.cnodeCount(), 2);
        QCOMPARE(eset.cbranchCount(), 1);

        const double dt = 1./10000.;
        VoltagePoint v1(12.0);
        Resistance r1(1000);
        Capacitance c1(22e-6, dt);
        QVERIFY(c1.type() == Element::Element_Capacitance);
        eset.addElement(&v1);
        eset.addElement(&r1);
        eset.addElement(&c1);
        QVERIFY(!eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        r1.setCNodes(0, 1);
        c1.setCNodes(1, -1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();
        c1.add_initial_dc();

        const double tolerance = 0.05;
        const double RC = r1.resistance()*c1.capacitance();
        for (uint i = 1; i < 10000; i++) {
            c1.time_step();
            eset.doLinear(true);
            if (i == 1) {
                eset.displayEquations();
            }
            const double t = i * dt;
            const double Vc = 12 * (1 - std::exp(-t/RC));
            const double I = (12-Vc)/r1.resistance();
            if (i % 100 == 0) {
                qDebug() << "t=" << t << "Vc=" << Vc << "V1=" << eset.cnodes()[1].v
                << "I=" << I << "I1=" << eset.cbranches()[0].i;
            }
            QCOMPARE_TOL(eset.cnodes()[0].v, 12, tolerance);
            QCOMPARE_TOL(eset.cnodes()[1].v, Vc, tolerance);
            QCOMPARE_TOL(eset.cbranches()[0].i, I, tolerance);
        }

        QCOMPARE_TOL(eset.cnodes()[1].v, 12, tolerance);
        QCOMPARE_TOL(eset.cbranches()[0].i, 0, tolerance);
    }

    void inductanceTest1()
    {
        // Circuit with (V1) - [R1] - |L1| - GND
        // See https://en.wikipedia.org/wiki/RL_circuit#Time-domain_considerations
        Circuit c;
        ElementSet eset(&c, 2, 2);
        QCOMPARE(eset.cnodeCount(), 2);
        QCOMPARE(eset.cbranchCount(), 2);

        const double dt = 1./10000.;
        VoltagePoint v1(12.0);
        Resistance r1(10);
        Inductance l1(47e-3, dt);
        QVERIFY(l1.type() == Element::Element_Inductance);
        eset.addElement(&v1);
        eset.addElement(&r1);
        eset.addElement(&l1);
        QVERIFY(!eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        r1.setCNodes(0, 1);
        l1.setCNodes(1, -1);
        l1.setCBranches(1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();
        l1.add_initial_dc();

        const double tolerance = 0.05;
        const double R_L = r1.resistance()/l1.inductance();
        for (uint i = 1; i < 10000; i++) {
            l1.time_step();
            eset.doLinear(true);
            if (i == 1) {
                eset.displayEquations();
            }
            const double t = i * dt;
            const double Vl = 12 * std::exp(-t*R_L);
            const double I = (12-Vl)/r1.resistance();
            if (i == 1 || i % 100 == 0) {
                qDebug() << "t=" << t << "Vl=" << Vl << "V1=" << eset.cnodes()[1].v
                << "I=" << I << "I1=" << eset.cbranches()[0].i << "I2=" << eset.cbranches()[1].i;
            }
            QCOMPARE_TOL(eset.cnodes()[0].v, 12, tolerance);
            QCOMPARE_TOL(eset.cnodes()[1].v, Vl, tolerance);
            QCOMPARE_TOL(eset.cbranches()[0].i, I,  tolerance);
            QCOMPARE_TOL(eset.cbranches()[1].i, I, tolerance);
        }

        QCOMPARE_TOL(eset.cnodes()[1].v, 0, tolerance);
        QCOMPARE_TOL(eset.cbranches()[0].i, 1.2, tolerance);
    }

    void diodeForwardBiasTest1()
    {
        // Circuit with (V1) - [D1] - |R1| - GND
        // This checks the diode in forward biased region
        // See https://en.wikipedia.org/wiki/Diode_modelling
        Circuit c;
        ElementSet eset(&c, 2, 1);
        QCOMPARE(eset.cnodeCount(), 2);
        QCOMPARE(eset.cbranchCount(), 1);

        VoltagePoint v1(12.0);
        Resistance r1(100);
        // Is = 10e-12 for Silicon diode
        Diode d1({1e-12, 1.0, 4.7});
        QVERIFY(d1.type() == Element::Element_Diode);
        eset.addElement(&v1);
        eset.addElement(&r1);
        eset.addElement(&d1);
        QVERIFY(eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        d1.setCNodes(0, 1);
        r1.setCNodes(1, -1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();
        d1.add_initial_dc();

        eset.doNonLinear(150);
        eset.displayEquations();

        const double R = r1.resistance();
        const double Is = d1.settings().I_S;
        const double n = d1.settings().N;
        const double Vs = 12;
        const double nVT = n*V_T;
        const double tolerance = 0.05;
        const double I = nVT/R*lambert(Is*R/nVT*std::exp(Vs/nVT));
        const double Vr = I*R;
        const double Vd = Vs-Vr;
        qDebug() << "Vd=" << Vd << "Vr=" << Vr  << "V1=" << eset.cnodes()[1].v
        << "I=" << I << "I1=" << eset.cbranches()[0].i;
        QCOMPARE_TOL(eset.cnodes()[0].v, Vs, tolerance);
        QCOMPARE_TOL(eset.cnodes()[1].v, Vr, tolerance);
        QCOMPARE_TOL(eset.cbranches()[0].i, I, tolerance);
    }

    void diodeReverseBiasTest1()
    {
        // Circuit with (V1) - [D1] - |R1| - GND
        // This checks the diode in reverse biased region
        // See https://en.wikipedia.org/wiki/Diode_modelling
        Circuit c;
        ElementSet eset(&c, 2, 1);
        QCOMPARE(eset.cnodeCount(), 2);
        QCOMPARE(eset.cbranchCount(), 1);

        VoltagePoint v1(-3.0);
        Resistance r1(100);
        Diode d1;
        eset.addElement(&v1);
        eset.addElement(&r1);
        eset.addElement(&d1);
        QVERIFY(eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        d1.setCNodes(0, 1);
        r1.setCNodes(1, -1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();
        d1.add_initial_dc();

        eset.doNonLinear(150);
        eset.displayEquations();

        QCOMPARE(eset.cnodes()[0].v, -3.0);
        QCOMPARE(eset.cnodes()[1].v, 0.0);
        QCOMPARE_TOL(eset.cbranches()[0].i, 0, 1e-6);
    }

    void diodeReverseBreakdownTest1()
    {
        // Circuit with (V1) - [D1] - |R1| - GND
        // This checks the diode in reverse breakdown region
        // See https://en.wikipedia.org/wiki/Diode_modelling
        Circuit c;
        ElementSet eset(&c, 2, 1);
        QCOMPARE(eset.cnodeCount(), 2);
        QCOMPARE(eset.cbranchCount(), 1);

        VoltagePoint v1(-12.0);
        Resistance r1(100);
        Diode d1({1e-12, 1.0, 6.0});
        eset.addElement(&v1);
        eset.addElement(&r1);
        eset.addElement(&d1);
        QVERIFY(eset.containsNonLinear());

        v1.setCNodes(0);
        v1.setCBranches(0);
        d1.setCNodes(0, 1);
        r1.setCNodes(1, -1);

        eset.createMatrixMap();

        v1.add_initial_dc();
        r1.add_initial_dc();
        d1.add_initial_dc();

        eset.doNonLinear(150);
        eset.displayEquations();

        const double R = r1.resistance();
        const double Is = d1.settings().I_S;
        const double Vb = d1.settings().V_B;
        const double n = d1.settings().N;
        const double Vs = -12;
        const double nVT = n*V_T;
        const double tolerance = 0.005;
        const double I = -nVT/R*lambert(Is*R/nVT*std::exp(-(Vs+Vb)/nVT));
        const double Vr = I*R;
        const double Vd = Vs-Vr;
        qDebug() << "Vd=" << Vd << "Vr=" << Vr  << "V1=" << eset.cnodes()[1].v
        << "I=" << I << "I1=" << eset.cbranches()[0].i;

        QCOMPARE_TOL(eset.cnodes()[0].v, Vs, tolerance);
        QCOMPARE_TOL(eset.cnodes()[1].v, Vr, tolerance);
        QCOMPARE_TOL(eset.cbranches()[0].i, I, tolerance);
    }

    void circuitResistorSanityTest()
    {
        // Circuit
        // [5V]a--b[R1]c--d[GND]
        Circuit circuit;
        Pin a, b, c, d;
        Wire w1(&a, &b);
        Wire w2(&c, &d);

        // See Component::createVoltagePoint
        VoltagePoint source(5.0);
        a.addElement(&source);

        // See Component::createResistance
        Resistance resistor(1000);
        b.addElement(&resistor);
        b.addCircuitDependentPin(&c);
        c.addElement(&resistor);
        c.addCircuitDependentPin(&b);
        d.setGroundType(Pin::gt_always);

        // See CircuitDocument::createCircuit
        circuit.addPin(&d);
        circuit.setElementPinMapping(&source, {&a});
        circuit.setElementPinMapping(&resistor, {&b, &c});

        // See usage in CircuitDocument::assignCircuits
        // This sets the eqId's of some of the pins
        // It's unclear why this is not in circuit.init
        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();

        QVERIFY(!circuit.containsNonLinear());
        QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        QCOMPARE(circuit.elementSet()->cnodeCount(), 1);
        QCOMPARE(circuit.wireList().size(), 2);

        // Component::initElements(0)
        QVERIFY(circuit.assignEquationIds());
        circuit.createMatrixMap();
        // Component::initElements(1)
        circuit.addInitialDc();

        // doNonLogic updates pin/node voltages
        QCOMPARE(a.voltage(), 0);
        QCOMPARE(b.voltage(), 0);
        QCOMPARE(c.voltage(), 0);
        circuit.doNonLogic();
        circuit.displayEquations();
        QCOMPARE(a.voltage(), 5.0);
        QCOMPARE(b.voltage(), 5.0);
        QCOMPARE(c.voltage(), 0);

        // CircuitDocument::calculateConnectorCurrents
        bool solved = circuit.calculateCurrents();
        // Expected current is I = V/R
        QCOMPARE(w1.current(), 0.005);
        QCOMPARE(w2.current(), 0.005);
        QVERIFY(solved);
    }

    void circuitResistorSeriesSanityTest()
    {
        // Circuit
        // [5V]a--b[R1]c--d[R2]e--f[GND]
        Circuit circuit;
        Pin a, b, c, d, e, f;
        Wire w1(&a, &b);
        Wire w2(&c, &d);
        Wire w3(&e, &f);

        VoltagePoint source(12.0);
        Resistance r1(1000);
        Resistance r2(2000);
        circuit.setElementPinMapping(&source, {&a});
        circuit.setElementPinMapping(&r1, {&b, &c});
        circuit.setElementPinMapping(&r2, {&d, &e});

        f.setGroundType(Pin::gt_always);
        circuit.addPin(&f);
        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();

        QVERIFY(!circuit.containsNonLinear());
        QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        QCOMPARE(circuit.elementSet()->cnodeCount(), 2);
        QCOMPARE(circuit.wireList().size(), 3);

        QVERIFY(circuit.assignEquationIds());
        circuit.createMatrixMap();
        circuit.addInitialDc();

        circuit.doNonLogic();
        circuit.displayEquations();
        QCOMPARE(a.voltage(), 12.0);
        QCOMPARE(b.voltage(), 12.0);
        QCOMPARE(c.voltage(), 8.0);
        QCOMPARE(d.voltage(), 8.0);
        QCOMPARE(e.voltage(), 0.0);
        QCOMPARE(f.voltage(), 0.0);

        bool solved = circuit.calculateCurrents();
        // Expected current is I = V/R
        QCOMPARE(w1.current(), 0.004);
        QCOMPARE(w2.current(), 0.004);
        QCOMPARE(w3.current(), 0.004);
        QVERIFY(solved);
    }

    void circuitParallelResistorSanityTest()
    {
        // Circuit  c[R1]d
        //          |    |
        // [12V]a --b    g--h[GND]
        //          |    |
        //          e[R2]f
        Circuit circuit;
        Pin a, b, c, d, e, f, g, h;
        Wire w1(&a, &b);
        Wire w2(&b, &c);
        Wire w3(&b, &e);
        Wire w4(&d, &g);
        Wire w5(&f, &g);
        Wire w6(&g, &h);

        circuit.addPins({&a, &b, &c, &d, &e, &f, &g, &h});
        VoltagePoint source(12.0);
        circuit.setElementPinMapping(&source, {&a});
        Resistance r1(2000);
        circuit.setElementPinMapping(&r1, {&c, &d});
        Resistance r2(3000);
        circuit.setElementPinMapping(&r2, {&e, &f});
        h.setGroundType(Pin::gt_always);

        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();

        QVERIFY(!circuit.containsNonLinear());
        QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        QCOMPARE(circuit.elementSet()->cnodeCount(), 1);
        QCOMPARE(circuit.wireList().size(), 6);

        QVERIFY(circuit.assignEquationIds());
        circuit.createMatrixMap();
        circuit.addInitialDc();
        circuit.doNonLogic();
        circuit.displayEquations();
        QCOMPARE(a.voltage(), 12.0);
        QCOMPARE(b.voltage(), 12.0);
        QCOMPARE(c.voltage(), 12.0);
        QCOMPARE(e.voltage(), 12.0);

        QCOMPARE(d.voltage(), 0.0);
        QCOMPARE(g.voltage(), 0.0);
        QCOMPARE(f.voltage(), 0.0);
        QCOMPARE(h.voltage(), 0.0);

        bool solved = circuit.calculateCurrents();
        QCOMPARE(w1.current(), 0.010);
        QCOMPARE(w2.current(), 0.006);
        QCOMPARE(w3.current(), 0.004);
        QCOMPARE(w4.current(), 0.006);
        QCOMPARE(w5.current(), 0.004);
        QCOMPARE(w6.current(), 0.010);
        QVERIFY(solved);
    }

    void circuitRLCSeriesTest()
    {
        // RLC series circuit
        // (VS)a--b[R1]c--d[L1]e--f[C1]g--h[GND]
        // See https://physicstasks.eu/1540/series-rlc-
        // https://en.wikipedia.org/wiki/RLC_circuit
        // https://circuits.mit.edu/_static/S23/handouts/lec09a/lecture09a.pdf
        const double dt = 1./100000.;
        Circuit circuit;
        Pin a, b, c, d, e, f, g, h;
        Wire w1(&a, &b);
        Wire w2(&c, &d);
        Wire w3(&e, &f);
        Wire w4(&g, &h);
        circuit.addPins({&a, &b, &c, &d, &e, &f, &g, &h});
        VoltagePoint vs(25.0);
        Resistance r1(50);
        Inductance l1(0.3, dt);
        Capacitance c1(15e-6, dt);
        circuit.setElementPinMapping(&vs, {&a});
        circuit.setElementPinMapping(&r1, {&b, &c});
        circuit.setElementPinMapping(&l1, {&d, &e});
        circuit.setElementPinMapping(&c1, {&f, &g});
        h.setGroundType(Pin::gt_always);

        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();

        QVERIFY(!circuit.containsNonLinear());
        QCOMPARE(circuit.elementSet()->cbranchCount(), 2); // Inductor has one branch
        QCOMPARE(circuit.elementSet()->cnodeCount(), 3);
        QCOMPARE(circuit.wireList().size(), 4);

        QVERIFY(circuit.assignEquationIds());
        circuit.createMatrixMap();
        circuit.addInitialDc();
        // eqns 39
        const double Vs = vs.voltage();
        const double R = r1.resistance();
        const double L = l1.inductance();
        const double C = c1.capacitance();
        const double alpha = R / (2. * L);
        const double omega0 = 1.0 / sqrt(L * C);
        const double omegad = sqrt(1./(L * C) - (R * R) / (4. * L * L));
        const double dfactor = R / 2.0 * sqrt(C / L);
        QVERIFY(dfactor < 1);
        const double phi = atan2(-omegad, alpha) - M_PI_2; // eqn 50
        const double V0 = -Vs;
        const double A0 = V0*sqrt(1+alpha*alpha/(omegad*omegad)); // Constant in eqn 50
        const double B0 = -V0/(omegad*L); // Constant in eqn 42
        qInfo() << "alpha" << alpha << "omega0" << omega0 << "dfactor" << dfactor <<
            "phi" << phi << "V0" << V0 << "A0" << A0 << "B0" << B0;

        uint n = 10000;
        double ierr = 0;
        double verr = 0;
        double t = 0;
        for (uint i = 0; i < n; i++) {
            t += dt;
            circuit.doNonLogic();
            if (i == 0) {
                circuit.displayEquations();
            }
            bool solved = circuit.calculateCurrents();

            // See eqns 42 and 50 in MIT lecture notes
            const double Il = B0 * exp(-alpha*t) * sin(omegad*t);
            const double Vr = Il*R;
            const double Vc = Vs - Vr - A0 * exp(-alpha*t) * cos(omegad*t+phi);

            if (i == 1 || i % 100 == 0) {
                qDebug() << "t=" << t << "Vc=" << Vc << "V1" << e.voltage()
                    << "Il=" << Il << "I" << w1.current();
            }
            ierr += pow(w1.current() - Il, 2);
            verr += pow(e.voltage() - Vc, 2);
            QCOMPARE_TOL(w1.current(), Il, 0.001);
            QCOMPARE_TOL(w2.current(), Il, 0.001);
            QCOMPARE_TOL(w3.current(), Il, 0.001);
            QCOMPARE_TOL(w4.current(), Il, 0.001);
            QCOMPARE_TOL(a.voltage(), Vs, 0.05);
            QCOMPARE_TOL(b.voltage(), Vs, 0.05);
            QCOMPARE_TOL(c.voltage(), Vs-Vr, 0.05);
            QCOMPARE_TOL(d.voltage(), Vs-Vr, 0.05);
            // TODO: Why is this off so much?
            QCOMPARE_TOL(e.voltage(), Vc, 0.5);
            QCOMPARE_TOL(f.voltage(), Vc, 0.5);
            QCOMPARE(g.voltage(), 0.0);
            QCOMPARE(h.voltage(), 0.0);
            QVERIFY(solved);
        }

        const double Irms = sqrt(ierr/n);
        const double Vrms = sqrt(verr/n);
        qDebug() << "Irms" << Irms << "Vrms" << Vrms;
    }

    void circuitFullWaveRectifier()
    {
        // Test the following full wave rectifier circuit
        // +-a[D1]b--c--d[D2]e-+
        // |         |         |
        // |         f         |
        // |        (VS)       |
        // |         g         |
        // |         |         |
        // h-i[D3]j--k--l[D4]m-n
        // |                   |
        // +-------o[R1]p------+
        const double dt = 1./10000.;

        Circuit circuit;
        Resistance r1(1000);
        Diode d1, d2, d3, d4;
        VoltageSignal vs(dt, 5.0);
        vs.setStep(VoltageSignal::st_sinusoidal, 1.0);
        QVERIFY(vs.type() == Element::Element_VoltageSignal);
        QCOMPARE(vs.voltage(), 5.0);
        QCOMPARE(vs.offset(), 0.0);

        Pin a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
        Wire w1(&h, &a);
        Wire w2(&h, &i);
        Wire w3(&h, &o);
        Wire w4(&b, &c);
        Wire w5(&c, &d);
        Wire w6(&c, &f);
        Wire w7(&g, &k);
        Wire w8(&j, &k);
        Wire w9(&k, &l);
        Wire w10(&e, &n);
        Wire w11(&m, &n);
        Wire w12(&n, &p);
        circuit.addPins({&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o, &p});
        o.setGroundType(Pin::gt_always);
        circuit.setElementPinMapping(&vs, {&g, &f});
        circuit.setElementPinMapping(&r1, {&o, &p});
        circuit.setElementPinMapping(&d1, {&a, &b});
        circuit.setElementPinMapping(&d2, {&d, &e});
        circuit.setElementPinMapping(&d3, {&i, &j});
        circuit.setElementPinMapping(&d4, {&l, &m});

        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();

        QVERIFY(circuit.containsNonLinear());
        QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        QCOMPARE(circuit.elementSet()->cnodeCount(), 3);
        QCOMPARE(circuit.wireList().size(), 12);

        QVERIFY(circuit.assignEquationIds());
        circuit.addInitialDc();
        double t = vs.frequency() / 4.0; // Voltage signal starts time at freq/4
        const double omega = 2 * M_PI * vs.frequency();
        for (uint i = 0; i < 10000; i++) {
            t += dt;
            circuit.doNonLogic();
            if (i == 0) {
                circuit.displayEquations();
            }
            // Input voltage should be
            const double v = vs.voltage() * sin(t * omega + vs.phase());
            const double vin = f.voltage()-g.voltage();
            QCOMPARE_TOL(vin, v, 1e-6);

            // The output vr should be the absolute of vs - two diode drops (~0.7*2)
            const double vout = p.voltage()-o.voltage();
            const double vexp = std::max(0., std::abs(v)-1.4);
            QCOMPARE_TOL(vout, vexp, 0.1);

            bool solved = circuit.calculateCurrents();
            const double I = vexp/r1.resistance();
            const double tol = 1e-4;
            QCOMPARE_TOL(w12.current(), I, tol);
            if (vin > 0) {
                // Current through D3 and D2
                QCOMPARE_TOL(w2.current(), I, tol);
                QCOMPARE_TOL(w10.current(), I, tol);
                QCOMPARE_TOL(w11.current(), 0, tol);
                QCOMPARE_TOL(w4.current(), 0, tol);
            } else {
                // Current through D1 and D4
                QCOMPARE_TOL(w10.current(), 0, tol);
                QCOMPARE_TOL(w11.current(), I, tol);
            }

            QVERIFY(solved);
        }

    }

    void circuitSingleClosedSwitch() {
        // Tests the following circuit
        // (VS)a--b{S1}c--d[R1]e--f[GND]
        Circuit circuit;
        VoltagePoint vs(12);
        Resistance r1(1000);
        Pin a, b, c, d, e, f;
        f.setGroundType(Pin::gt_always);
        Wire w1(&a, &b);
        Switch s1(&b, &c, Switch::Closed);
        QVERIFY(s1.state() == Switch::Closed);
        Wire w2(&c, &d);
        Wire w3(&e, &f);
        circuit.addPins({&a, &b, &c, &d, &e, &f});
        circuit.setElementPinMapping(&vs, {&a});
        circuit.setElementPinMapping(&r1, {&d, &e});

        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();
        QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        QCOMPARE(circuit.elementSet()->cnodeCount(), 1);
        QCOMPARE(circuit.wireList().size(), 4);
        QVERIFY(circuit.assignEquationIds());
        circuit.addInitialDc();
        circuit.doNonLogic();
        circuit.displayEquations();
        QVERIFY(circuit.calculateCurrents());
        QCOMPARE(a.voltage(), vs.voltage());
        QCOMPARE(b.voltage(), vs.voltage());
        QCOMPARE(c.voltage(), vs.voltage());
        QCOMPARE(d.voltage(), vs.voltage());
        QCOMPARE(e.voltage(), 0);
        QCOMPARE(f.voltage(), 0);
        QCOMPARE(w1.current(), 0.012);
        QCOMPARE(s1.current(), 0.012);
        QCOMPARE(w2.current(), 0.012);
        QCOMPARE(w3.current(), 0.012);

        // TODO: This requires the circuit to be rebuilt into two separate circuits
        // s1.setState(Switch::Open);
        // circuit.init();
        // QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        // QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        // QCOMPARE(circuit.elementSet()->cnodeCount(), 2);
        // QCOMPARE(circuit.wireList().size(), 4);
        // QVERIFY(circuit.assignEquationIds());
        // circuit.addInitialDc();
        // circuit.doNonLogic();
        //
        // QVERIFY(circuit.calculateCurrents());
        // QCOMPARE(a.voltage(), vs.voltage());
        // QCOMPARE(b.voltage(), vs.voltage());
        // QCOMPARE(c.voltage(), vs.voltage());
        // QCOMPARE(d.voltage(), 0);
        // QCOMPARE(e.voltage(), 0);
        // QCOMPARE(f.voltage(), 0);
        // QCOMPARE(w1.current(), 0);
        // QCOMPARE(s1.current(), 0);
        // QCOMPARE(w2.current(), 0);
        // QCOMPARE(w3.current(), 0);
    }

    void circuitTwoClosedSwitches() {
        // Tests the following circuit
        // (VS)a--b{S1}c--d{S2}e--f[R1]g--h[GND]
        // This tests the case where the pins
        // c and d have no known current set
        Circuit circuit;
        VoltagePoint vs(12);
        Resistance r1(1000);
        Pin a, b, c, d, e, f, g, h;
        h.setGroundType(Pin::gt_always);
        Wire w1(&a, &b);
        Switch s1(&b, &c, Switch::Closed);
        Wire w2(&c, &d);
        Switch s2(&d, &e, Switch::Closed);
        Wire w3(&e, &f);
        Wire w4(&g, &h);
        circuit.addPins({&a, &b, &c, &d, &e, &f, &g, &h});
        circuit.setElementPinMapping(&vs, {&a});
        circuit.setElementPinMapping(&r1, {&f, &g});

        QCOMPARE(circuit.identifyGround(circuit.pinList()), 1);
        circuit.init();
        QCOMPARE(circuit.elementSet()->cbranchCount(), 1);
        QCOMPARE(circuit.elementSet()->cnodeCount(), 1);
        QCOMPARE(circuit.wireList().size(), 6);
        QVERIFY(circuit.assignEquationIds());
        circuit.addInitialDc();
        circuit.doNonLogic();
        circuit.displayEquations();
        QVERIFY(circuit.calculateCurrents());
        QCOMPARE(a.voltage(), vs.voltage());
        QCOMPARE(b.voltage(), vs.voltage());
        QCOMPARE(c.voltage(), vs.voltage());
        QCOMPARE(d.voltage(), vs.voltage());
        QCOMPARE(e.voltage(), vs.voltage());
        QCOMPARE(f.voltage(), vs.voltage());
        QCOMPARE(g.voltage(), 0);
        QCOMPARE(h.voltage(), 0);
        QCOMPARE(w1.current(), 0.012);
        QCOMPARE(s1.current(), 0.012);
        QCOMPARE(s2.current(), 0.012);
        QCOMPARE(w2.current(), 0.012);
        QCOMPARE(w3.current(), 0.012);
    }


    void measurementsTest1()
    {
        Measurements mavg(Measurements::AVG, 3);
        QCOMPARE(mavg.update(1.), 1.);
        QCOMPARE(mavg.update(2.), 3./2.);
        QCOMPARE(mavg.update(3.), 2.);
        QCOMPARE(mavg.update(3.), 8./3.);

        Measurements mrms(Measurements::RMS, 3);
        QCOMPARE(mrms.update(1.), 1.);
        QCOMPARE(mrms.update(2.), sqrt(2.5));
        QCOMPARE(mrms.update(3.), sqrt(14./3.0));
        QCOMPARE(mrms.update(2.), sqrt(17.0/3.0));

        Measurements m(Measurements::MIN, 0);
        QCOMPARE(m.mode(), Measurements::MIN);
        m.setCapacity(2);

        QCOMPARE(m.samples().size(), 0);
        QCOMPARE(m.update(-1.), -1.);
        QCOMPARE(m.update(0), -1.);
        QCOMPARE(m.update(-2.), -2.);

        m.setMode(Measurements::MAX);
        QCOMPARE(m.mode(), Measurements::MAX);
        QCOMPARE(m.samples().size(), 0);
        QCOMPARE(m.update(1), 1.);
        QCOMPARE(m.update(-2.), 1.);
        QCOMPARE(m.update(4.), 4.);
        QCOMPARE(m.samples().size(), 2);
        m.clear();
        QCOMPARE(m.samples().size(), 0);
    }

    void cleanupTestCase()
    {
    }
};

QTEST_MAIN(KtlElectronicsTests)
#include "test_electronics.moc"

