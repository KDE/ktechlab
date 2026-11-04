/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "oscilloscopedata.h"
#include "oscilloscope.h"

using namespace std;

// BEGIN class ProbeData
ProbeData::ProbeData(int id, uint64_t time)
    : m_id(id)
    , m_drawPosition(0.5)
    , m_resetTime(time)
    , m_color(Qt::black)
    , m_label("")
{
}

ProbeData::~ProbeData()
{
    unregisterProbe(m_id);
}

void ProbeData::setColor(const QColor& color)
{
    m_color = color;
    Q_EMIT displayAttributeChanged();
}

void ProbeData::setLabel(const QString &label)
{
    m_label = label;
    Q_EMIT displayAttributeChanged();
}

void ProbeData::setUnit(const QString &unit)
{
    if (m_unit == unit)
        return;
    m_unit = unit;
    Q_EMIT displayAttributeChanged();
}

// END class ProbeData

// BEGIN class LogicProbeData
LogicProbeData::LogicProbeData(int id, uint capacity, uint64_t time)
    : ProbeData(id, time)
    , m_data(capacity)
{
}

void LogicProbeData::eraseData(uint64_t time)
{
    bool lastValue = false;
    bool hasLastValue = false;

    if (!m_data.isEmpty()) {
        lastValue = m_data.last().value;
        hasLastValue = true;
    }

    m_data.clear();

    m_resetTime = time;

    if (hasLastValue)
        addDataPoint(LogicDataPoint(lastValue, m_resetTime));
}

uint64_t LogicProbeData::findPos(uint64_t time) const
{
    uint64_t pos = m_data.size();
    if (time <= m_resetTime || !pos)
        return m_data.firstIndex();
    // binary search

    // TODO: test and debug this code.
    uint64_t start_offset = m_data.firstIndex();
    uint64_t top = pos;
    pos >>= 1;
    uint64_t bottom = 0;

    do {
        uint64_t datatime = m_data.at(pos+start_offset).time;
        if (datatime == time)
            return pos;

        if (datatime >= time) {
            top = pos;
            pos -= (top - bottom) >> 1;
        } else {
            bottom = pos;
            pos += (top - bottom) >> 1;
        }
        // try to avoid infinite loop when top-bottom==1
        if (top - bottom == 1) {
            if (datatime >= time)
                bottom = top;
            else
                top = bottom;
        }

    } while (top != bottom && pos != bottom);

    return pos + start_offset;
}
// END class LogicProbeData

// BEGIN class FloatingProbeData
FloatingProbeData::FloatingProbeData(int id, uint capacity, uint64_t time, double timePerStep)
    : ProbeData(id, time)
    , m_posFromTime(1./timePerStep)
    , m_timeFromPos(timePerStep)
    , m_data(capacity)
{
    m_scaling = Linear;
    m_upperAbsValue = 10.0;
    m_lowerAbsValue = 0.1;
}

void FloatingProbeData::eraseData(uint64_t time)
{
    m_data.clear();
    m_resetTime = time;
}

uint64_t FloatingProbeData::findPos(uint64_t time) const
{
    if (time <= m_resetTime)
        return m_data.firstIndex();

    uint64_t at =  uint64_t(time - m_resetTime) * m_posFromTime;
    uint64_t maxAt = m_data.lastIndex();
    return std::min(at, maxAt);
}

uint64_t FloatingProbeData::toTime(uint64_t at) const
{
    return uint64_t(m_resetTime + (at * m_timeFromPos));
}

void FloatingProbeData::setScaling(Scaling scaling)
{
    if (m_scaling == scaling)
        return;

    m_scaling = scaling;
    Q_EMIT displayAttributeChanged();
}

void FloatingProbeData::setUpperAbsValue(double upperAbsValue)
{
    if (m_upperAbsValue == upperAbsValue)
        return;

    m_upperAbsValue = upperAbsValue;
    Q_EMIT displayAttributeChanged();
}

void FloatingProbeData::setLowerAbsValue(double lowerAbsValue)
{
    if (m_lowerAbsValue == lowerAbsValue)
        return;

    m_lowerAbsValue = lowerAbsValue;
    Q_EMIT displayAttributeChanged();
}
// END class FloatingProbeData

#include "moc_oscilloscopedata.cpp"
