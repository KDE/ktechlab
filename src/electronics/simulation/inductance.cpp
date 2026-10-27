/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "inductance.h"
#include "elementset.h"

Inductance::Inductance(double inductance, double delta, Method method)
    : Reactive(delta)
{
    m_inductance = inductance;
    m_numCNodes = 2;
    m_numCBranches = 1;
    setMethod(method);
}

Inductance::~Inductance()
{
}

void Inductance::setInductance(double i)
{
    m_inductance = i;
}

void Inductance::add_initial_dc()
{
    A_c(0, 0) = 1;
    A_b(0, 0) = 1;
    A_c(0, 1) = -1;
    A_b(1, 0) = -1;

    // The adding of r_eg and v_eq will be done for us by time_step.
    // So for now, just reset the constants used.
    m_r = m_v_old = v_eq_old = 0.0;
}

void Inductance::updateCurrents()
{
    if (!b_status)
        return;

    m_cnodeI[0] = -p_cbranch[0]->i;
    m_cnodeI[1] = -m_cnodeI[0];
}

void Inductance::time_step()
{
    if (!b_status)
        return;

    double i = p_cbranch[0]->i;
    double v_eq_new, r_eq_new;

    // For a description of these equations see
    // https://qucs.sourceforge.net/tech/node26.html
    // https://courses.grainger.illinois.edu/ece552/fa2017/ECE552-F17-Chapter9.pdf
    if (m_method == Inductance::m_euler) {
        r_eq_new = m_inductance / m_delta;
        v_eq_new = -i * r_eq_new;
    } else if (m_method == Inductance::m_trap) {
        r_eq_new = 2.0 * m_inductance / m_delta;
        v_eq_new = -i * r_eq_new - m_v_old;
        m_v_old = p_cnode[0]->v - p_cnode[1]->v;
    } else {
        Q_UNREACHABLE(); // LCOV_EXCL_LINE
    }

    if (m_r != r_eq_new) {
        A_d(0, 0) -= r_eq_new - m_r;
        m_r = r_eq_new;
    }

    if (v_eq_new != v_eq_old) {
        b_v(0) += v_eq_new - v_eq_old;
        v_eq_old = v_eq_new;
    }

}

void Inductance::setMethod(Method m)
{
    m_method = m;
    updateStatus();
}
