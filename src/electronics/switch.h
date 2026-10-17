/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef SWITCH_H
#define SWITCH_H

#include "wire.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

class CircuitDocument;
class Component;
class Resistance;

/**
@author David Saxton
*/

class Switch : public Wire
{
    Q_OBJECT
public:
    enum State { Open, Closed };

    Switch(Pin *p1, Pin *p2, State state=Open, Component *parent=nullptr);
    ~Switch() override;

    Component* parentComponent() const;

    /**
     * If bouncing has been set to true, then the state will not switch
     * immediately to that given.
     */
    void setState(State state);
    State state() const
    {
        return m_state;
    }

    /**
     * Tell the switch whether to bounce or not, for the given duration,
     * when the state is changed.
     */
    void setBounce(bool bounce, int msec = 5);
    /**
     * Tell the switch to continue bouncing (updates the resistance value).
     * Called from the simulator.
     */
    void bounce();

protected Q_SLOTS:
    /**
     * Called from a QTimer timeout - our bouncing period has come to an
     * end. This will then fully disconnect or connect the pins depending
     * on the current state.
     */
    void stopBouncing();

protected:
    void startBouncing();

    bool m_bBounce;
    int m_bouncePeriod_ms;
    unsigned long long m_bounceStart; // Simulator time that bouncing started
    Resistance *m_pBounceResistance;
    State m_state;
    QTimer m_pStopBouncingTimer;
};

#endif
