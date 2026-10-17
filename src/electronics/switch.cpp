/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <cmath>
#include <stdlib.h> // for rand
#include <time.h>

#include <QTimer>

#include "circuitdocument.h"
#include "component.h"
#include "ecnode.h"
#include "pin.h"
#include "resistance.h"
#include "simulator.h"
#include "switch.h"

#include <ktechlab_debug.h>

Switch::Switch(Pin *p1, Pin *p2, State state, Component *parent): Wire(p1, p2, parent)
{
    m_bouncePeriod_ms = 5;
    m_bBounce = false;
    m_bounceStart = 0;
    m_pBounceResistance = nullptr;
    connect(&m_pStopBouncingTimer, SIGNAL(timeout()), this, SLOT(stopBouncing()));

    // Force update
    m_state = state;
    m_connected = state == Closed;
}

Switch::~Switch()
{
}

Component* Switch::parentComponent() const
{
    return static_cast<Component*>(parent());
}

void Switch::setState(State state)
{
    if (m_state == state)
        return;

    m_state = state;

    if (m_bBounce)
        startBouncing();
    else {
        // I'm being lazy...calling stopBouncing will connect the stuff
        stopBouncing();
    }
}

void Switch::setBounce(bool bounce, int msec)
{
    m_bBounce = bounce;
    m_bouncePeriod_ms = msec;
}

void Switch::startBouncing()
{
    if (m_pBounceResistance) {
        // Already active?
        return;
    }
    Component* c = parentComponent();
    if (!c || !c->circuitDocument())
        return;

    // 	qCDebug(KTL_LOG);
    m_connected = false; // Break the wire connection and replace it with the resistor
    m_pBounceResistance = c->createResistance(m_pStartPin, m_pEndPin, 10000);
    m_bounceStart = Simulator::self()->time();

    // FIXME: I broke the bounce feature when I cleaned this out of the simulator,
    // should probably be put into circuit document or some other solution which doesn't
    // contaminate that many other classes.

    //	Simulator::self()->attachSwitch( this );
    // 	qCDebug(KTL_LOG) << "m_bounceStart="<<m_bounceStart<<" m_bouncePeriod_ms="<<m_bouncePeriod_ms;

    // initialize random generator
    srand(time(nullptr));

    // Give our bounce resistor an initial value
    bounce();
}

void Switch::bounce()
{
    int bounced_ms = ((Simulator::self()->time() - m_bounceStart) * 1000) / LOGIC_UPDATE_RATE;

    if (bounced_ms >= m_bouncePeriod_ms) {
        if (!m_pStopBouncingTimer.isActive()) {
            m_pStopBouncingTimer.setSingleShot(true);
            m_pStopBouncingTimer.start(0 /*, true */);
        }

        return;
    }

    double g = double(rand()) / double(RAND_MAX);

    // 4th power of the conductance seems to give a nice distribution
    g = pow(g, 4);
    m_pBounceResistance->setConductance(g);
}

void Switch::stopBouncing()
{
    //	Simulator::self()->detachSwitch( this );
    Component* c = parentComponent();
    CircuitDocument *cd = nullptr;
    if (c) {
        c->removeElement(m_pBounceResistance, true);
        cd = c->circuitDocument();
    }
    m_pBounceResistance = nullptr;

    // Reconnect the wire if the switch is closed
    m_connected = m_state == Closed;

    if (cd)
        cd->requestAssignCircuits();
}


#include "moc_switch.cpp"
