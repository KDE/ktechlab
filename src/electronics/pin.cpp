/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pin.h"
#include "ecnode.h"

#include <QDebug>

#include <cassert>

#include <ktechlab_debug.h>

Pin::Pin(ECNode *parent) : QObject(parent)
{
    m_voltage = 0.;
    m_current = 0.;
    m_eqId = -2;
    m_bCurrentIsKnown = false;
    m_groundType = Pin::gt_never;
}

Pin::~Pin()
{
    qDeleteAll(m_inputWireList);
    qDeleteAll(m_outputWireList);
}

ECNode* Pin::parentECNode() const
{
    Q_ASSERT(dynamic_cast<ECNode*>(parent()));
    return static_cast<ECNode*>(parent());
}

PinList Pin::localConnectedPins() const
{
    // 	qCDebug(KTL_LOG) << "Input wires: "<<m_inputWireList.size()<<"   Output wires: " << m_outputWireList.size() << "   Switch connected: " << m_switchConnectedPins.size();

    PinList pins;

    for (Wire* w: std::as_const(m_inputWireList)) {
        if (w && w->isConnected())
            pins << w->startPin();
    }

    for (Wire* w: std::as_const(m_outputWireList)) {
        if (w && w->isConnected())
            pins << w->endPin();
    }
    return pins;
}

void Pin::addCircuitDependentPin(Pin *pin)
{
    if (pin && !m_circuitDependentPins.contains(pin))
        m_circuitDependentPins.append(pin);
}

void Pin::addGroundDependentPin(Pin *pin)
{
    if (pin && !m_groundDependentPins.contains(pin))
        m_groundDependentPins.append(pin);
}

void Pin::removeDependentPins()
{
    m_circuitDependentPins.clear();
    m_groundDependentPins.clear();
}

void Pin::addElement(Element *e)
{
    if (!e || m_elementList.contains(e))
        return;
    m_elementList.append(e);
}

void Pin::removeElement(Element *e)
{
    m_elementList.removeAll(e);
}

void Pin::removeWire(Wire *w)
{
    m_inputWireList.removeAll(w);
    m_outputWireList.removeAll(w);
}

void Pin::addInputWire(Wire *wire)
{
    if (wire && !m_inputWireList.contains(wire))
        m_inputWireList << wire;
}

void Pin::addOutputWire(Wire *wire)
{
    if (wire && !m_outputWireList.contains(wire))
        m_outputWireList << wire;
}

uint Pin::numConnectedWires() const
{
    uint n = 0;
    for (Wire* w: std::as_const(m_inputWireList)) {
        if (w && w->isConnected())
            n += 1;
    }
    for (Wire* w: std::as_const(m_outputWireList)) {
        if (w && w->isConnected())
            n += 1;
    }
    return n;
}

bool Pin::calculateCurrentFromWires()
{
    m_inputWireList.removeAll(static_cast<Wire *>(nullptr));
    m_outputWireList.removeAll(static_cast<Wire *>(nullptr));

    m_current = 0.0;

    for (Wire* w: std::as_const(m_inputWireList)) {
        if (!w->currentIsKnown())
            return false;

        m_current -= w->current();
    }

    for (Wire* w: std::as_const(m_outputWireList)) {
        if (!w->currentIsKnown())
            return false;

        m_current += w->current();
    }

    m_bCurrentIsKnown = true;
    return true;
}
