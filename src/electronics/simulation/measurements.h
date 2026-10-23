/***************************************************************************
 *   Copyright (C) 2026 KTechLab developers                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#pragma once
#include <QContiguousCache>

class Measurements
{
public:
    /**
     * Measurement mode. This determines how samples are aggregated into
     * a single value returned by update.
     */
    enum Mode {
        AVG,
        RMS,
        MAX,
        MIN,
    };

    Measurements(Mode mode=AVG, uint capacity=0): m_mode(mode), m_samples(capacity) {}
    ~Measurements() {}

    /**
     * Update the measurement with a new value
     * and return the calculated result.
     */
    double update(double v);

    /**
     * Clear all measurement data
     */
    void clear()
    {
        m_cache = 0.;
        m_samples.clear();
    }

    /**
     * Set the number of samples
     */
    void setCapacity(uint capacity)
    {
        m_samples.setCapacity(capacity);
    }

    /**
     * Changes the mode of the measurement and
     * clears all stored data.
     */
    void setMode(Mode m)
    {
        m_mode = m;
        clear();
    }

    /**
     * Get the measurement mode
     */
    Mode mode() const
    {
        return m_mode;
    }
    /**
     * Return the samples
     */
    const QContiguousCache<double>& samples() const
    {
        return m_samples;
    }

protected:
    Mode m_mode = AVG;
    double m_cache = 0.;
    QContiguousCache<double> m_samples;
};

