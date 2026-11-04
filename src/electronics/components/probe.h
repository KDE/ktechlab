/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PROBE_H
#define PROBE_H

#include <component.h>
#include <logic.h>

class LogicProbeData;
class ProbeData;
class FloatingProbeData;
class Simulator;

/**
@author David Saxton
*/
class Probe : public Component
{
    Q_OBJECT
public:
    Probe(ICNDocument *icnDocument, bool newItem, const char *id=nullptr, const QString& unit=QStringLiteral(""));
    ~Probe() override;

protected:
    void dataChanged() override;

    ProbeData *p_probeData; // As obtained via registering with the oscilloscope
    QColor m_color;
    QString m_label;
    QString m_unit;
};

/**
@author David Saxton
 */
class FloatingProbe : public Probe
{
    Q_OBJECT
public:
    FloatingProbe(ICNDocument *icnDocument, bool newItem, const char *id=nullptr, const QString& unit=QStringLiteral(""));
    ~FloatingProbe() override;

    bool doesStepNonLogic() const override
    {
        return true;
    }

protected:
    void dataChanged() override;
    void drawShape(QPainter &p) override;

    FloatingProbeData *m_pFloatingProbeData;
};

/**
@author David Saxton
 */
class VoltageProbe : public FloatingProbe
{
    Q_OBJECT
public:
    VoltageProbe(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~VoltageProbe() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

    void stepNonLogic() override;

protected:
    Pin *m_pPin1;
    Pin *m_pPin2;
};

/**
@author David Saxton
 */
class CurrentProbe : public FloatingProbe
{
    Q_OBJECT
public:
    CurrentProbe(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~CurrentProbe() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

    void stepNonLogic() override;

protected:
    VoltageSource *m_voltageSource;
};

/**
@author David Saxton
 */
class LogicProbe : public Probe, public CallbackClass
{
    Q_OBJECT
public:
    LogicProbe(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~LogicProbe() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

    void logicCallback(bool value);

protected:
    void drawShape(QPainter &p) override;

    LogicProbeData *p_logicProbeData;
    LogicIn *m_pIn;
    Simulator *m_pSimulator;
};

#endif
