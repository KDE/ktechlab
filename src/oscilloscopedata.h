/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OSCILLOSCOPEDATA_H
#define OSCILLOSCOPEDATA_H

#include <QColor>
#include <QContiguousCache>
#include <QObject>

/**
For use in LogicProbe: Every time the input changes state, the new input state
is recorded in value, along with the simulator time that it occurs at.
 */
class LogicDataPoint
{
public:
    LogicDataPoint(bool v = false, uint64_t t = 0)
        : value(v)
        , time(t)
    {
    }

    bool value : 1;
    uint64_t time : 63;
};

/**
@author David Saxton
 */
class ProbeData : public QObject
{
    Q_OBJECT
public:
    ProbeData(int id, uint64_t time);
    ~ProbeData() override;

    /**
     * @returns unique id for oscilloscope, set on construction
     */
    int id() const
    {
        return m_id;
    }
    /**
     * Set the proportion (0 = top, 1 = bottom) of the way down the
     * oscilloscope view that the probe output is drawn. If the proportion
     * is out of range ( <0, or >1), then the drawPosition is set to 0/1
     */
    void setDrawPosition(float drawPosition)
    {
        m_drawPosition = drawPosition;
    }
    /**
     * Returns the draw position. Default is 0.5.
     * @see setDrawPosition
     */
    float drawPosition() const
    {
        return m_drawPosition;
    }
    /**
     * Set the colour that is used to display the probe in the oscilloscope.
     * Default is black.
     */
    void setColor(const QColor &color);
    /**
     * @returns the colour that is used to display the probe in the oscilloscope
     */
    const QColor& color() const
    {
        return m_color;
    }

    /**
     * Set the label for the trace in the oscilloscope.
     */
    void setLabel(const QString &label);

    /**
     * @returns the label for the trace in the oscilloscope
     */
    const QString& label() const
    {
        return m_label;
    }

    /**
     * Set the units for the trace in the oscilloscope.
     */
    void setUnit(const QString &unit);

    /**
     * @returns the units for the trace in the oscilloscope
     */
    const QString& unit() const
    {
        return m_unit;
    }

    // 		/**
    // 		 * Will not record any data when paused
    // 		 */
    // 		void setPaused( bool isPaused) { b_isPaused = isPaused; }
    /**
     * Returns the time (in Simulator time) that this probe was created at,
     * or last reset.
     */
    uint64_t resetTime() const
    {
        return m_resetTime;
    }

    /**
     * Erases all recorded data, and sets the probe resetTime to the provided time
     */
    virtual void eraseData(uint64_t time) = 0;

    /**
     * Set the buffer capacity.
     */
    virtual void setCapacity(uint n) = 0;

    /**
     * Searches for and returns the position of the last DataPoint that was
     * added before or at the given Simulator time. If no DataPoints were
     * were recorded before the given time, then will return the one closest
     * to the given time. Will return 0 if no DataPoints have been recorded
     * yet.
     */
    virtual uint64_t findPos(uint64_t time) const = 0;

Q_SIGNALS:
    /**
     * Emitted when an attribute that affects how the probe is drawn in the
     * oscilloscope is changed.
     */
    void displayAttributeChanged();

protected:
    const int m_id;
    float m_drawPosition;
    uint64_t m_resetTime;
    QColor m_color;
    QString m_label;
    QString m_unit = QStringLiteral("");
};

/**
@author David Saxton
*/
class LogicProbeData : public ProbeData
{
    Q_OBJECT
public:
    LogicProbeData(int id, uint capacity, uint64_t time);

    /**
     * Appends the data point to the set of data.
     * If data points exceeds the capacity the oldest points are discarded.
     */
    void addDataPoint(LogicDataPoint data)
    {
       m_data.append(data);
    }

    void eraseData(uint64_t time) override;
    uint64_t findPos(uint64_t time) const override;

    void setCapacity(uint n) override
    {
        m_data.setCapacity(n);
    }

    bool isEmpty() const
    {
        return m_data.size() == 0;
    }

    const QContiguousCache<LogicDataPoint>& data() const
    {
        return m_data;
    }

    /**
     * Retrieve the data point closest to the given time.
     * This is expensive and should only be used for single point queries
     */
    LogicDataPoint dataAtTime(uint64_t time) const
    {
        return m_data.at(findPos(time));
    }

protected:
    QContiguousCache<LogicDataPoint> m_data;
};

/**
@author David Saxton
*/
class FloatingProbeData : public ProbeData
{
    Q_OBJECT
public:
    enum Scaling { Linear, Logarithmic };

    FloatingProbeData(int id, uint capacity, uint64_t time, double timePerStep);

    /**
     * Appends the data point to the set of data.
     * If data points exceeds the size the oldest points are discarded.
     */
    void addDataPoint(float data)
    {
        m_data.append(data);
    }
    /**
     * Converts the insert position to a Simulator time.
     */
    uint64_t toTime(uint64_t at) const;
    /**
     * Sets the scaling to use in the oscilloscope display.
     */
    void setScaling(Scaling scaling);
    /**
     * @return the scaling used for the oscilloscope display.
     */
    Scaling scaling() const
    {
        return m_scaling;
    }
    /**
     * Sets the value to use as the upper absolute value in the display.
     */
    void setUpperAbsValue(double upperAbsValue);
    /**
     * @return the upper absolute value to use in the display.
     */
    double upperAbsValue() const
    {
        return m_upperAbsValue;
    }
    /**
     * Sets the value to use as the lower absolute value in the display
     * (this is only used with logarithmic scaling).
     */
    void setLowerAbsValue(double lowerAbsValue);
    /**
     * @return the lower absolute value to use in the display (this is
     * only used with logarithmic scaling).
     */
    double lowerAbsValue() const
    {
        return m_lowerAbsValue;
    }

    void eraseData(uint64_t time) override;
    uint64_t findPos(uint64_t time) const override;

    void setCapacity(uint n) override
    {
        m_data.setCapacity(n);
    }

    bool isEmpty() const
    {
        return m_data.size() == 0;
    }

    const QContiguousCache<float>& data() const
    {
        return m_data;
    }

    /**
     * Retrieve the data point closest to the given time.
     * This is expensive and should only be used for single point queries
     */
    double dataAtTime(uint64_t time) const
    {
        return m_data.at(findPos(time));
    }

protected:
    Scaling m_scaling;
    double m_posFromTime; // Factor to convert a time to an index
    double m_timeFromPos; // Factor to convert an index to a time
    double m_upperAbsValue;
    double m_lowerAbsValue;
    QContiguousCache<float> m_data;
};

#endif
