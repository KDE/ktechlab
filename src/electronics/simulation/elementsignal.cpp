/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "elementsignal.h"
#include "klocalizedstring.h"
#include <cmath>

ElementSignal::ElementSignal()
{
    m_type = ElementSignal::st_sinusoidal;
    m_time = 0.;
    m_frequency = 0.;
    m_phase = 0.;
    if (signalTypes.isEmpty()) {
        signalTypes["Sinusoidal"] = i18n("Sinusoidal");
        signalTypes["Square"] = i18n("Square");
        signalTypes["Sawtooth"] = i18n("Sawtooth");
        signalTypes["Reverse Sawtooth"] = i18n("Reverse Sawtooth");
        signalTypes["Triangular"] = i18n("Triangular");
    }
}

ElementSignal::~ElementSignal()
{
}

QStringMap ElementSignal::signalTypes;

void ElementSignal::setStep(Type type, double frequency, double phase)
{
    m_type = type;
    m_frequency = frequency;
    m_omega = 2 * M_PI * m_frequency;
    m_time = 1. / (4. * m_frequency);
    m_phase = phase;
}

double ElementSignal::advance(double delta)
{
    m_time += delta;
    if (m_time >= 1. / m_frequency)
        m_time -= 1. / m_frequency;

    switch (m_type) {
    case ElementSignal::st_reverse_sawtooth:
        return remainder(m_time * 2 * m_frequency + m_phase, 2);
    case ElementSignal::st_sawtooth:
        return -remainder(m_time * 2 * m_frequency + m_phase, 2);
    case ElementSignal::st_square:
        return ((int(trunc(m_time * 2 * m_frequency + m_phase)) & 1) == 0) ? 1 : -1;
    case ElementSignal::st_triangular:
        return 2 / M_PI * asin(sin(m_omega * m_time + m_phase));
    case ElementSignal::st_sinusoidal:
        return sin(m_time * m_omega + m_phase);
    }
    return 0;
}
