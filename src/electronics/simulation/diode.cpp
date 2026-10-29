/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "diode.h"
#include "elementset.h"

#include <algorithm>
#include <vector>

#include <cmath>

// BEGIN class Diode
Diode::Diode(const DiodeSettings& settings)
    : NonLinear()
{
    g_new = g_old = I_new = I_old = V_prev = 0.0;
    m_diodeSettings = settings;
    m_numCNodes = settings.R == 0. ? 2 : 3;
    updateLim();
}

Diode::~Diode()
{
}

void Diode::add_initial_dc()
{
    g_new = g_old = g_rs = I_new = I_old = V_prev = 0.0;

    update_dc();
}

double Diode::current() const
{
    if (!b_status)
        return 0.0;

    const uint n = m_numCNodes - 2;
    const uint p = m_numCNodes - 1;
    double I;
    calcIg(p_cnode[n]->v - p_cnode[p]->v, &I, nullptr);

    return I;
}

void Diode::updateCurrents()
{
    if (!b_status)
        return;

    const uint p = m_numCNodes - 1;
    m_cnodeI[p] = current();
    m_cnodeI[0] = -m_cnodeI[p];
}

void Diode::update_dc()
{
    if (!b_status)
        return;

    // Add series resistance component
    if (m_diodeSettings.R != 0.0) {
        Q_ASSERT(m_numCNodes == 3);
        double g = 1./m_diodeSettings.R;
        if (g != g_rs) {
            A_g(0, 0) += g - g_rs;
            A_g(1, 1) += g - g_rs;
            A_g(0, 1) -= g - g_rs;
            A_g(1, 0) -= g - g_rs;
            g_rs = g;
        }
    }

    // Add ideal diode components
    // n is negative terminal index
    // p is positive terminal index
    const uint n = m_numCNodes - 2;
    const uint p = m_numCNodes - 1;
    calc_eq();

    A_g(n, n) += g_new - g_old;
    A_g(p, p) += g_new - g_old;
    A_g(n, p) -= g_new - g_old;
    A_g(p, n) -= g_new - g_old;

    b_i(n) -= I_new - I_old;
    b_i(p) += I_new - I_old;

    g_old = g_new;
    I_old = I_new;
}

void Diode::calc_eq()
{
    const uint n = m_numCNodes - 2;
    const uint p = m_numCNodes - 1;
    double N = m_diodeSettings.N;
    double V_B = m_diodeSettings.V_B;
    double v = p_cnode[n]->v - p_cnode[p]->v;

    // adjust voltage to help convergence
    if (V_B != 0 && v < std::min<double>(0, -V_B + 10 * N)) {
        double V = -(v + V_B);
        V = diodeVoltage(V, -(V_prev + V_B), N, V_lim);
        v = -(V + V_B);
    } else
        v = diodeVoltage(v, V_prev, N, V_lim);

    V_prev = v;

    double I_D;
    calcIg(v, &I_D, &g_new);

    I_new = I_D - (v * g_new);
}

void Diode::calcIg(double V, double *I_D, double *g) const
{
    double I_S = m_diodeSettings.I_S;
    double N = m_diodeSettings.N;
    double V_B = m_diodeSettings.V_B;

    double g_tiny = (V < -10 * V_T * N && V_B != 0) ? I_S : 0;

    if (V >= (-3 * N * V_T)) {
        if (g)
            *g = diodeConductance(V, I_S, N) + g_tiny;
        *I_D = diodeCurrent(V, I_S, N) + (g_tiny * V);
    } else if (V_B == 0 || V >= -V_B) {
        double a = (3 * N * V_T) / (V * M_E);
        a = a * a * a;
        *I_D = (-I_S * (1 + a)) + (g_tiny * V);
        if (g)
            *g = ((I_S * 3 * a) / V) + g_tiny;
    } else {
        double a = exp(-(V_B + V) / N / V_T);
        *I_D = (-I_S * a) + (g_tiny * V);
        if (g)
            *g = I_S * a / V_T / N + g_tiny;
    }
}

void Diode::setDiodeSettings(const DiodeSettings &settings)
{
    const uint oldCount = m_numCNodes;
    m_diodeSettings = settings;
    m_numCNodes = settings.R == 0. ? 2 : 3;
    updateLim();
    if (b_status && oldCount != m_numCNodes) {
        b_status = false; // This change requires the circuit to be rebuilt
    }
    if (p_eSet)
        p_eSet->setCacheInvalidated();
}

void Diode::updateLim()
{
    double I_S = m_diodeSettings.I_S;
    double N = m_diodeSettings.N;
    V_lim = diodeLimitedVoltage(I_S, N);
}
// END class Diode
