/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef DIODE_H
#define DIODE_H

#include "nonlinear.h"

class DiodeSettings
{
public:
    double I_S = 1e-15; ///< Diode saturation current (A)
    double N = 1.0;   ///< Emission coefficient
    double V_B = 4.7; ///< Reverse breakdown (V)
    double R = 0.0;	///< Series resistance (Ω)
};

/**
This simulates a diode. The simulated diode characteristics are:

@li I_s: Diode saturation current
@li V_T: Thermal voltage = kT/4 = 25mV at 20 C
@li n: Emission coefficient, typically between 1 and 2
@li V_RB: Reverse breakdown (large negative voltage)
@li G_RB: Reverse breakdown conductance
@li R_D: Base resistance of diode

@short Simulates the electrical property of diode-ness
@author David Saxton
*/
class Diode : public NonLinear
{
public:
    Diode(const DiodeSettings& settings = DiodeSettings());
    ~Diode() override;

    void update_dc() override;
    void add_initial_dc() override;
    Element::Type type() const override
    {
        return Element_Diode;
    }
    const DiodeSettings& settings() const
    {
        return m_diodeSettings;
    }
    void setDiodeSettings(const DiodeSettings &settings);
    /**
     * Returns the current flowing through the diode
     */
    double current() const;
    /**
     * Returns the internal pin used when the series resistance is non-zero
     */
    Pin& internalPin()
    {
        return m_internalPin;
    }
    const Pin& internalPin() const
    {
        return m_internalPin;
    }

protected:
    void updateCurrents() override;
    void calc_eq();
    void calcIg(double V, double *I, double *g) const;
    void updateLim();

    double g_new, g_old;
    double g_rs = 0.; // Series conductance component of large singal model
    double I_new, I_old;
    double V_prev;
    double V_lim;
    Pin m_internalPin;
    DiodeSettings m_diodeSettings;
};

#endif
