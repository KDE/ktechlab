/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef VOLTAGESIGNAL_H
#define VOLTAGESIGNAL_H

#include "elementsignal.h"
#include "reactive.h"

/**
@short VoltageSignal
@author David saxton
*/
class VoltageSignal : public Reactive, public ElementSignal
{
public:
    VoltageSignal(const double delta, const double voltage, const double offset=0.);
    ~VoltageSignal() override;

    Element::Type type() const override
    {
        return Element_VoltageSignal;
    }
    void setVoltage(const double voltage);
    double voltage() const
    {
        return m_voltage;
    }

    void setOffset(const double offset);
    double offset() const
    {
        return m_offset;
    }

    void time_step() override;

protected:
    void updateCurrents() override;
    void add_initial_dc() override;

private:
    double m_voltage; // Voltage
    double m_offset;
};

#endif
