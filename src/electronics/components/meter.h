/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef METER_H
#define METER_H

#include <component.h>
#include <measurements.h>
#include <QTimer>

/**
@author David Saxton
*/
class Meter : public Component
{
public:
    Meter(ICNDocument *icnDocument, bool newItem, const char *id);
    ~Meter() override;

    void stepNonLogic() override;
    bool doesStepNonLogic() const override
    {
        return true;
    }
    void drawShape(QPainter &p) override;
    bool contentChanged() const override;

protected:
    QString displayText();
    void dataChanged() override;

    /**
     * Update the meter arrow and text
     */
    void updateDisplay();

    /**
     * Return the value / current, or whatever the meter is measuring
     */
    virtual double meterValue() = 0;

    double calcProp(double v) const;

    bool b_firstRun = true;     // If true, then update the text displayed
    double m_value = 0.;
    double m_old_value = 0.;
    double m_minValue;
    double m_maxValue;
    Text *p_displayText;
    QString m_unit;

    double m_prevProp = 0.; // Used in contentChanged()
    Measurements m_measurements;
    QTimer m_refreshTimer;
};

/**
@short Measures the frequency at a point in the circuit
@author David Saxton
*/
class FrequencyMeter : public Meter
{
public:
    FrequencyMeter(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~FrequencyMeter() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

protected:
    double meterValue() override;
    ECNode *m_probeNode;
};

/**
@short Simple resistor
@author David Saxton
*/
class ECAmmeter : public Meter
{
public:
    ECAmmeter(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~ECAmmeter() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

protected:
    double meterValue() override;
    void dataChanged() override;

private:
    VoltageSource *m_voltageSource;
};

/**
@short Displays voltage across terminals
@author David Saxton
*/
class ECVoltMeter : public Meter
{
public:
    ECVoltMeter(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~ECVoltMeter() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

protected:
    double meterValue() override;
    void dataChanged() override;
};

#endif
