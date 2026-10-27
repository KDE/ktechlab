/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "capacitance.h"

Capacitance::Capacitance(const double capacitance, const double delta, Method method)
    : Reactive(delta)
{
    m_cap = capacitance;
    m_numCNodes = 2;
    setMethod(method);
}

Capacitance::~Capacitance()
{
}

void Capacitance::setCapacitance(const double c)
{
    m_cap = c;
}

void Capacitance::add_initial_dc()
{
    // We don't need to do anything here, as time_step() will do that for us,
    // apart from to make sure our old values are 0
    m_g = m_i_old = i_eq_old = 0.;
}

void Capacitance::updateCurrents()
{
    if (!b_status)
        return;
    const double i = (p_cnode[1]->v - p_cnode[0]->v) * m_g;
    m_cnodeI[0] = i - i_eq_old;
    m_cnodeI[1] = -m_cnodeI[0];
}

void Capacitance::time_step()
{
    if (!b_status)
        return;

    const double v = p_cnode[1]->v - p_cnode[0]->v;
    double i_eq_new, g;

    // For a description of these equations see
    // https://qucs.sourceforge.net/tech/node26.html
    // https://courses.grainger.illinois.edu/ece552/fa2017/ECE552-F17-Chapter9.pdf
    if (m_method == Capacitance::m_euler) {
        g = m_cap / m_delta;
        i_eq_new = v * g;
    } else if (m_method == Capacitance::m_trap) {
        g = 2. * m_cap / m_delta;
        i_eq_new = v * g + m_i_old;
        m_i_old = v * g - i_eq_old;
    } else {
        Q_UNREACHABLE(); // LCOV_EXCL_LINE
    }

    if (m_g != g) {
        const double tmp = g - m_g;
        A_g(0, 0) += tmp;
        A_g(0, 1) -= tmp;
        A_g(1, 0) -= tmp;
        A_g(1, 1) += tmp;
        m_g = g;
    }

    if (i_eq_new != i_eq_old) {
        const double tmp = i_eq_new - i_eq_old;
        b_i(0) -= tmp;
        b_i(1) += tmp;
        i_eq_old = i_eq_new;
    }
}

void Capacitance::setMethod(Method m)
{
    m_method = m;
    updateStatus();
}
