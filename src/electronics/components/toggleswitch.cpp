/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "toggleswitch.h"

#include "canvasitemparts.h"
#include "ecnode.h"
#include "libraryitem.h"
#include "switch.h"

#include <KLocalizedString>
#include <QPainter>

// BEGIN class ECDPDT
Item *ECDPDT::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new ECDPDT(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *ECDPDT::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/dpdt_toggle")), i18n("DPDT"), i18n("Switches"), "dpdt.png", LibraryItem::lit_component, ECDPDT::construct);
}

ECDPDT::ECDPDT(ICNDocument *icnDocument, bool newItem, const char *id)
    : Component(icnDocument, newItem, id ? id : "dpdt_toggle")
{
    m_name = i18n("DPDT Toggle");
    setSize(-16, -32, 32, 64);

    addButton("button", QRect(-16, 32, 32, 20), "", true);

    createProperty("button_text", Variant::Type::String);
    property("button_text")->setCaption(i18n("Button Text"));

    Variant *v = createProperty("bounce", Variant::Type::Bool);
    v->setCaption("Bounce");
    v->setAdvanced(true);
    v->setValue(false);

    v = createProperty("bounce_period", Variant::Type::Double);
    v->setCaption("Bounce Period");
    v->setAdvanced(true);
    v->setUnit("s");
    v->setValue(5e-3);

    init4PinRight(-24, -8, 8, 24);

    init2PinLeft(-16, 16);

    m_switch1 = createSwitch(m_pNNode[0], m_pPNode[0], false);
    m_switch2 = createSwitch(m_pNNode[0], m_pPNode[1], true);
    m_switch3 = createSwitch(m_pNNode[1], m_pPNode[2], false);
    m_switch4 = createSwitch(m_pNNode[1], m_pPNode[3], true);
    pressed = false;
}

ECDPDT::~ECDPDT()
{
}

void ECDPDT::dataChanged()
{
    button("button")->setText(dataString("button_text"));

    bool bounce = dataBool("bounce");
    int bouncePeriod_ms = int(dataDouble("bounce_period") * 1e3);

    m_switch1->setBounce(bounce, bouncePeriod_ms);
    m_switch2->setBounce(bounce, bouncePeriod_ms);
    m_switch3->setBounce(bounce, bouncePeriod_ms);
    m_switch4->setBounce(bounce, bouncePeriod_ms);
}

void ECDPDT::drawShape(QPainter &p)
{
    initPainter(p);

    int _x = int(x()) - 16;
    int _y = int(y()) - 32;
    const int radius = 2;

    p.drawEllipse(_x, _y + 15, 2 * radius, 2 * radius);
    p.drawEllipse(_x, _y + 47, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 7, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 23, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 39, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 55, 2 * radius, 2 * radius);

    const int dy = pressed ? 6 : -6;

    p.drawLine(_x + 2 * radius, _y + 16, _x + width() - 2 * radius + 2, _y + 16 + dy);
    p.drawLine(_x + 2 * radius, _y + 48, _x + width() - 2 * radius + 2, _y + 48 + dy);

    deinitPainter(p);
}

void ECDPDT::buttonStateChanged(const QString &, bool state)
{
    pressed = state;
    m_switch1->setState(state ? Switch::Open : Switch::Closed);
    m_switch2->setState(state ? Switch::Closed : Switch::Open);
    m_switch3->setState(state ? Switch::Open : Switch::Closed);
    m_switch4->setState(state ? Switch::Closed : Switch::Open);
}
// END class ECDPDT

// BEGIN class ECDPST
Item *ECDPST::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new ECDPST(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *ECDPST::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/dpst_toggle")), i18n("DPST"), i18n("Switches"), "dpst.png", LibraryItem::lit_component, ECDPST::construct);
}

ECDPST::ECDPST(ICNDocument *icnDocument, bool newItem, const char *id)
    : Component(icnDocument, newItem, id ? id : "dpst_toggle")
{
    m_name = i18n("DPST Toggle");
    setSize(-16, -16, 32, 32);

    addButton("button", QRect(-16, 16, 32, 20), "", true);

    createProperty("button_text", Variant::Type::String);
    property("button_text")->setCaption(i18n("Button Text"));

    Variant *v = createProperty("bounce", Variant::Type::Bool);
    v->setCaption("Bounce");
    v->setAdvanced(true);
    v->setValue(false);

    v = createProperty("bounce_period", Variant::Type::Double);
    v->setCaption("Bounce Period");
    v->setAdvanced(true);
    v->setUnit("s");
    v->setValue(5e-3);

    init2PinLeft(-8, 8);
    init2PinRight(-8, 8);

    m_switch1 = createSwitch(m_pPNode[0], m_pNNode[0], true);
    m_switch2 = createSwitch(m_pPNode[1], m_pNNode[1], true);
    pressed = false;
}

ECDPST::~ECDPST()
{
}

void ECDPST::dataChanged()
{
    button("button")->setText(dataString("button_text"));

    bool bounce = dataBool("bounce");
    int bouncePeriod_ms = int(dataDouble("bounce_period") * 1e3);

    m_switch1->setBounce(bounce, bouncePeriod_ms);
    m_switch2->setBounce(bounce, bouncePeriod_ms);
}

void ECDPST::drawShape(QPainter &p)
{
    initPainter(p);

    int _x = int(x()) - 16;
    int _y = int(y()) - 16;
    const int radius = 2;

    p.drawEllipse(_x, _y + 6, 2 * radius, 2 * radius);
    p.drawEllipse(_x, _y + 22, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 6, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 22, 2 * radius, 2 * radius);

    const int dy = pressed ? 6 : 0;

    p.drawLine(_x + 2 * radius, _y + 7, _x + width() - 2 * radius, _y + 1 + dy);
    p.drawLine(_x + 2 * radius, _y + 24, _x + width() - 2 * radius, _y + 18 + dy);

    deinitPainter(p);
}

void ECDPST::buttonStateChanged(const QString &, bool state)
{
    m_switch1->setState(state ? Switch::Closed : Switch::Open);
    m_switch2->setState(state ? Switch::Closed : Switch::Open);
    pressed = state;
}
// END class ECDPST

// BEGIN class ECSPDT
Item *ECSPDT::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new ECSPDT(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *ECSPDT::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/spdt_toggle")), i18n("SPDT"), i18n("Switches"), "spdt.png", LibraryItem::lit_component, ECSPDT::construct);
}

ECSPDT::ECSPDT(ICNDocument *icnDocument, bool newItem, const char *id)
    : Component(icnDocument, newItem, id ? id : "spdt_toggle")
{
    m_name = i18n("SPDT Toggle");
    setSize(-16, -16, 32, 32);

    addButton("button", QRect(-16, 16, width(), 20), "", true);

    createProperty("button_text", Variant::Type::String);
    property("button_text")->setCaption(i18n("Button Text"));

    Variant *v = createProperty("bounce", Variant::Type::Bool);
    v->setCaption("Bounce");
    v->setAdvanced(true);
    v->setValue(false);

    v = createProperty("bounce_period", Variant::Type::Double);
    v->setCaption("Bounce Period");
    v->setAdvanced(true);
    v->setUnit("s");
    v->setValue(5e-3);

    init1PinLeft(0);
    init2PinRight(-8, 8);

    m_switch1 = createSwitch(m_pNNode[0], m_pPNode[0], false);
    m_switch2 = createSwitch(m_pNNode[0], m_pPNode[1], true);

    pressed = false;
}

ECSPDT::~ECSPDT()
{
}

void ECSPDT::dataChanged()
{
    button("button")->setText(dataString("button_text"));

    bool bounce = dataBool("bounce");
    int bouncePeriod_ms = int(dataDouble("bounce_period") * 1e3);

    m_switch1->setBounce(bounce, bouncePeriod_ms);
    m_switch2->setBounce(bounce, bouncePeriod_ms);
}

void ECSPDT::drawShape(QPainter &p)
{
    initPainter(p);

    int _x = int(x()) - 16;
    int _y = int(y()) - 16;
    const int radius = 2;

    p.drawEllipse(_x, _y + 15, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 6, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 22, 2 * radius, 2 * radius);

    const int dy = pressed ? 21 : 10;
    p.drawLine(_x + 2 * radius, _y + 16, _x + width() - 2 * radius + 2, _y + dy);

    deinitPainter(p);
}

void ECSPDT::buttonStateChanged(const QString &, bool state)
{
    pressed = state;
    m_switch1->setState(state ? Switch::Open : Switch::Closed);
    m_switch2->setState(state ? Switch::Closed : Switch::Open);
}
// END class ECSPDT

// BEGIN class ECSPST
Item *ECSPST::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new ECSPST(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *ECSPST::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/spst_toggle")), i18n("SPST"), i18n("Switches"), "spst.png", LibraryItem::lit_component, ECSPST::construct);
}

ECSPST::ECSPST(ICNDocument *icnDocument, bool newItem, const char *id)
    : Component(icnDocument, newItem, id ? id : "spst_toggle")
{
    m_name = i18n("SPST Toggle");
    setSize(-16, -8, 32, 16);
    pressed = false;

    addButton("button", QRect(-16, 8, width(), 20), "", true);

    createProperty("button_text", Variant::Type::String);
    property("button_text")->setCaption(i18n("Button Text"));

    Variant *v = createProperty("bounce", Variant::Type::Bool);
    v->setCaption("Bounce");
    v->setAdvanced(true);
    v->setValue(false);

    v = createProperty("bounce_period", Variant::Type::Double);
    v->setCaption("Bounce Period");
    v->setAdvanced(true);
    v->setUnit("s");
    v->setValue(5e-3);

    button("button")->setState(pressed);

    init1PinLeft();
    init1PinRight();

    m_switch = createSwitch(m_pNNode[0], m_pPNode[0], !pressed);
}

ECSPST::~ECSPST()
{
}

void ECSPST::dataChanged()
{
    button("button")->setText(dataString("button_text"));

    bool bounce = dataBool("bounce");
    int bouncePeriod_ms = int(dataDouble("bounce_period") * 1e3);
    m_switch->setBounce(bounce, bouncePeriod_ms);
}

void ECSPST::drawShape(QPainter &p)
{
    initPainter(p);

    int _x = int(x()) - 16;
    int _y = int(y()) - 8;
    const int radius = 2;

    p.drawEllipse(_x, _y + 7, 2 * radius, 2 * radius);
    p.drawEllipse(_x + width() - 2 * radius + 1, _y + 7, 2 * radius, 2 * radius);
    const int dy = pressed ? 0 : -6;
    p.drawLine(_x + 2 * radius, _y + 8, _x + width() - 2 * radius, _y + 8 + dy);

    deinitPainter(p);
}

void ECSPST::buttonStateChanged(const QString &, bool state)
{
    pressed = state;
    m_switch->setState(state ? Switch::Closed : Switch::Open);
}
// END class ECSPST
