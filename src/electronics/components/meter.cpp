/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "meter.h"
#include "canvasitemparts.h"
#include "ecnode.h"
#include "element.h"
#include "libraryitem.h"
#include "pin.h"
#include "simulator.h"
#include "variant.h"
#include "voltagesource.h"

#include <KLocalizedString>
#include <QPainter>
#include <cmath>

// BEGIN class Meter
Meter::Meter(ICNDocument *icnDocument, bool newItem, const char *id)
    : Component(icnDocument, newItem, id)
{
    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Meter::updateDisplay);
    setSize(-16, -16, 32, 32);

    p_displayText = addDisplayText("meter", QRect(-16, 16, 32, 16), displayText());

    createProperty("0-minValue", Variant::Type::Double);
    property("0-minValue")->setCaption(i18n("Minimum Value"));
    property("0-minValue")->setMinValue(1e-12);
    property("0-minValue")->setMaxValue(1e12);
    property("0-minValue")->setValue(1e-3);

    createProperty("1-maxValue", Variant::Type::Double);
    property("1-maxValue")->setCaption(i18n("Maximum Value"));
    property("1-maxValue")->setMinValue(1e-12);
    property("1-maxValue")->setMaxValue(1e12);
    property("1-maxValue")->setValue(1e3);

    createProperty("samples", Variant::Type::Int);
    property("samples")->setCaption(i18n("Samples"));
    property("samples")->setMinValue(1);
    property("samples")->setMaxValue(10000000);
    property("samples")->setValue(10000);
}

Meter::~Meter()
{
}

void Meter::dataChanged()
{
    m_minValue = dataDouble("0-minValue");
    m_maxValue = dataDouble("1-maxValue");
    m_measurements.clear();
    m_measurements.setCapacity(dataInt("samples"));
    setChanged();
}

void Meter::stepNonLogic()
{
    m_value = meterValue();
    if (m_value != m_old_value && !m_refreshTimer.isActive()) {
        m_refreshTimer.start(100);
    }
}

void Meter::updateDisplay()
{
    if (b_firstRun) {
        p_displayText->setText(displayText());
        updateAttachedPositioning();
        setChanged();
        property("0-minValue")->setUnit(m_unit);
        property("1-maxValue")->setUnit(m_unit);
        b_firstRun = false;
    } else {
        if (p_displayText->setText(displayText()))
            updateAttachedPositioning();
    }
}

bool Meter::contentChanged() const
{
    return (m_prevProp != calcProp(m_old_value));
}

void Meter::drawShape(QPainter &p)
{
    initPainter(p);
    p.drawEllipse(int(x()) - 16, int(y()) - 16, width(), width());
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(Qt::black);

    // The proportion between 0.1mV and 10KV, on a logarithmic scale
    m_prevProp = calcProp(m_old_value);
    double sin_prop = 10 * std::sin(m_prevProp * 1.571); // 1.571 = pi/2
    double cos_prop = 10 * std::cos(m_prevProp * 1.571); // 1.571 = pi/2

    int cx = int(x() - 16 + (width() / 2));
    int cy = int(y() - 16 + (height() / 2));
    p.drawLine(int(cx - sin_prop), int(cy - cos_prop), int(cx + sin_prop), int(cy + cos_prop));

    QPolygon pa(3);
    pa[0] = QPoint(int(cx - sin_prop), int(cy - cos_prop)); // Arrow head
    pa[1] = QPoint(int(cx - sin_prop + 8 * std::sin(1.571 * (-0.3 + m_prevProp))), int(cy - cos_prop + 8 * std::cos(1.571 * (-0.3 + m_prevProp))));
    pa[2] = QPoint(int(cx - sin_prop + 8 * std::sin(1.571 * (0.3 + m_prevProp))), int(cy - cos_prop + 8 * std::cos(1.571 * (0.3 + m_prevProp))));
    p.drawPolygon(pa);

    deinitPainter(p);
}

double Meter::calcProp(double v) const
{
    double abs_value = std::abs(v);

    double prop;
    if (abs_value <= m_minValue)
        prop = 0.0;
    else if (abs_value >= m_maxValue)
        prop = 1.0;
    else
        prop = std::log10(abs_value / m_minValue) / std::log10(m_maxValue / m_minValue);

    if (m_old_value > 0)
        prop *= -1;

    return prop;
}

QString Meter::displayText()
{
    double value = m_value;
    if (!std::isfinite(value))
        value = m_old_value;
    if (std::abs((value)) < 1e-9)
        value = 0.;
    m_old_value = value;
    return QString::number(value / CNItem::getMultiplier(value), 'g', 3) + CNItem::getNumberMag(value) + m_unit;
}
// END class Meter

// BEGIN class FrequencyMeter
Item *FrequencyMeter::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new FrequencyMeter(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *FrequencyMeter::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/frequencymeter")), i18n("Frequency Meter (TODO)"), i18n("Outputs"), "frequencymeter.png", LibraryItem::lit_component, FrequencyMeter::construct);
}

FrequencyMeter::FrequencyMeter(ICNDocument *icnDocument, bool newItem, const char *id)
    : Meter(icnDocument, newItem, id ? id : "frequencymeter")
{
    m_name = i18n("Frequency Meter");
    m_unit = "Hz";

    m_probeNode = createPin(0, -24, 90, "n1");
}

FrequencyMeter::~FrequencyMeter()
{
}

double FrequencyMeter::meterValue()
{
    return 0;
}
// END class FrequencyMeter

// BEGIN class ECAmmeter
Item *ECAmmeter::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new ECAmmeter(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *ECAmmeter::libraryItem()
{
    QStringList ids;
    ids << "ec/ammeter"
        << "ec/ammmeter";
    return new LibraryItem(ids, i18n("Ammeter"), i18n("Outputs"), "ammeter.png", LibraryItem::lit_component, ECAmmeter::construct);
}

ECAmmeter::ECAmmeter(ICNDocument *icnDocument, bool newItem, const char *id)
    : Meter(icnDocument, newItem, id ? id : "ammeter")
{
    m_name = i18n("Ammeter");
    setSize(-16, -16, 32, 32);
    m_unit = "A";

    createProperty("mode", Variant::Type::Select);
    property("mode")->setCaption(i18n("Mode"));
    QStringMap allowed;
    allowed["DC"] = i18n("DC");
    allowed["AC"] = i18n("AC");
    allowed["MAX"] = i18n("MAX");
    allowed["MIN"] = i18n("MIN");
    property("mode")->setAllowed(allowed);
    property("mode")->setValue("DC");

    init1PinLeft(0);
    init1PinRight(0);

    m_voltageSource = createVoltageSource(m_pNNode[0], m_pPNode[0], 0.);
}

void ECAmmeter::dataChanged()
{
    const QString& mode = dataString("mode");
    if (mode == QLatin1String("DC")) {
        m_measurements.setMode(Measurements::AVG);
    } else if (mode == QLatin1String("AC")) {
        m_measurements.setMode(Measurements::RMS);
    } else if (mode == QLatin1String("MAX")) {
        m_measurements.setMode(Measurements::MAX);
    } else if (mode == QLatin1String("MIN")) {
        m_measurements.setMode(Measurements::MIN);
    }
    Meter::dataChanged();
}

ECAmmeter::~ECAmmeter()
{
}

double ECAmmeter::meterValue()
{
    const double i = -m_voltageSource->cbranchCurrent(0);
    return m_measurements.update(i);
}
// END class ECAmmeter

// BEGIN class ECVoltmeter
Item *ECVoltMeter::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new ECVoltMeter(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *ECVoltMeter::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/voltmeter")), i18n("Voltmeter"), i18n("Outputs"), "voltmeter.png", LibraryItem::lit_component, ECVoltMeter::construct);
}

ECVoltMeter::ECVoltMeter(ICNDocument *icnDocument, bool newItem, const char *id)
    : Meter(icnDocument, newItem, id ? id : "voltmeter")
{
    m_name = i18n("Voltmeter");
    m_unit = "V";

    createProperty("mode", Variant::Type::Select);
    property("mode")->setCaption(i18n("Mode"));
    QStringMap allowed;
    allowed["DC"] = i18n("DC");
    allowed["AC"] = i18n("AC");
    allowed["MAX"] = i18n("MAX");
    allowed["MIN"] = i18n("MIN");
    property("mode")->setAllowed(allowed);
    property("mode")->setValue("DC");

    init1PinLeft(0);
    init1PinRight(0);
}


void ECVoltMeter::dataChanged()
{
    const QString& mode = dataString("mode");
    if (mode == QLatin1String("DC")) {
        m_measurements.setMode(Measurements::AVG);
    } else if (mode == QLatin1String("AC")) {
        m_measurements.setMode(Measurements::RMS);
    } else if (mode == QLatin1String("MAX")) {
        m_measurements.setMode(Measurements::MAX);
    } else if (mode == QLatin1String("MIN")) {
        m_measurements.setMode(Measurements::MIN);
    }
    Meter::dataChanged();
}

ECVoltMeter::~ECVoltMeter()
{
}

double ECVoltMeter::meterValue()
{
    const double v = m_pNNode[0]->pin()->voltage() - m_pPNode[0]->pin()->voltage();
    return m_measurements.update(v);
}
// END class ECVoltMeter
