/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "serialportcomponent.h"
#include "port.h"

#include "ecnode.h"
#include "itemdocument.h"
#include "libraryitem.h"
#include "pin.h"
#include "resistance.h"

#include <KLocalizedString>

#include <QPainter>

#include <cmath>

#include <ktechlab_debug.h>

void SerialPortComponent_tdCallback(void *objV, bool state) {
    SerialPortComponent *objT = static_cast<SerialPortComponent*>(objV);
    objT->tdCallback(state);
}
void SerialPortComponent_dtrCallback(void *objV, bool state) {
    SerialPortComponent *objT = static_cast<SerialPortComponent*>(objV);
    objT->dtrCallback(state);
}

Item *SerialPortComponent::construct(ItemDocument *itemDocument, bool newItem, const char *id)
{
    return new SerialPortComponent(static_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem *SerialPortComponent::libraryItem()
{
    return new LibraryItem(QStringList(QString("ec/serial_port")), i18n("Serial Port"), i18n("Connections"), "ic1.png", LibraryItem::lit_component, SerialPortComponent::construct);
}

SerialPortComponent::SerialPortComponent(ICNDocument *icnDocument, bool newItem, const char *id)
    : Component(icnDocument, newItem, id ? id : "serial_port")
{
    m_name = i18n("Serial Port");

    QPolygon pa(4);
    pa[0] = QPoint(-32, -48);
    pa[1] = QPoint(32, -40);
    pa[2] = QPoint(32, 40);
    pa[3] = QPoint(-32, 48);

    setItemPoints(pa);

    m_pSerialPort = new SerialPort();

    ECNode *pin = nullptr;

    // Works
    pin = createPin(-40, 32, 0, "CD");
    addDisplayText("CD", QRect(-28, 24, 28, 16), "CD", true, Qt::AlignLeft | Qt::AlignVCenter);
    m_pCD = createLogicOut(pin, false);

    // Doesn't work
    // 	pin = createPin( -40,  16,   0, "RD" );
    addDisplayText("RD", QRect(-28, 8, 28, 16), "RD", true, Qt::AlignLeft | Qt::AlignVCenter);
    // 	m_pRD = createLogicOut( pin, false  );

    // Works
    pin = createPin(-40, 0, 0, "TD");
    addDisplayText("TD", QRect(-28, -8, 28, 16), "TD", true, Qt::AlignLeft | Qt::AlignVCenter);
    m_pTD = createLogicIn(pin);
    //m_pTD->setCallback(this, (CallbackPtr)(&SerialPortComponent::tdCallback));
    m_pTD->setCallback2(SerialPortComponent_tdCallback, this);

    // Works
    pin = createPin(-40, -16, 0, "DTR");
    addDisplayText("DTR", QRect(-28, -24, 28, 16), "DTR", true, Qt::AlignLeft | Qt::AlignVCenter);
    m_pDTR = createLogicIn(pin);
    //m_pDTR->setCallback(this, (CallbackPtr)(&SerialPortComponent::dtrCallback));
    m_pDTR->setCallback2(SerialPortComponent_dtrCallback, this);

    // N/A
    pin = createPin(-40, -32, 0, "GND");
    addDisplayText("GND", QRect(-28, -40, 28, 16), "GND", true, Qt::AlignLeft | Qt::AlignVCenter);
    pin->pin()->setGroundType(Pin::gt_always);

    // Doesn't work
    // 	pin = createPin(  40,  24, 180, "DSR" );
    addDisplayText("DSR", QRect(0, 16, 28, 16), "DSR", true, Qt::AlignRight | Qt::AlignVCenter);
    // 	m_pDSR = createLogicIn( pin );
    // 	//m_pDSR->setCallback( this, (CallbackPtr)(&SerialPortComponent::dsrCallback) );
    //  m_pDSR->setCallback2( SerialPortComponent_dsrCallback, this);

    // Doesn't work
    // 	pin = createPin(  40,   8, 180, "RTS" );
    addDisplayText("RTS", QRect(0, 0, 28, 16), "RTS", true, Qt::AlignRight | Qt::AlignVCenter);
    // 	m_pRTS = createLogicIn( pin );
    // 	//m_pRTS->setCallback( this, (CallbackPtr)(&SerialPortComponent::rtsCallback) );
    //  m_pDSR->setCallback2( SerialPortComponent_rtsCallback, this);

    // Works
    pin = createPin(40, -8, 180, "CTS");
    addDisplayText("CTS", QRect(0, -16, 28, 16), "CTS", true, Qt::AlignRight | Qt::AlignVCenter);
    m_pCTS = createLogicOut(pin, false);

    // Works
    pin = createPin(40, -24, 180, "RI");
    addDisplayText("RI", QRect(0, -32, 28, 16), "RI", true, Qt::AlignRight | Qt::AlignVCenter);
    m_pRI = createLogicOut(pin, false);

    Variant *v = createProperty("port", Variant::Type::Combo);
    v->setAllowed(SerialPort::ports());
    v->setCaption(i18n("Port"));

    // 	v = createProperty( "baudRate", Variant::Type::Select );
    // 	v->setAllowed( QStringList::split( ",", "B0,B50,B75,B110,B134,B150,B200,B300,B600,B1200,B1800,B2400,B4800,B9600,B19200,B38400" ) );
    // 	v->setCaption( i18n("Baud rate") );
    // 	v->setValue("B9600");
}

SerialPortComponent::~SerialPortComponent()
{
    delete m_pSerialPort;
}

void SerialPortComponent::dataChanged()
{
#if 0
	QString baudString = dataString("baudRate");
	unsigned baudRate = 0;
	
	if ( baudString == "B0" )
		baudRate = B0;
	else if ( baudString == "B50" )
		baudRate = B50;
	else if ( baudString == "B75" )
		baudRate = B75;
	else if ( baudString == "B110" )
		baudRate = B110;
	else if ( baudString == "B134" )
		baudRate = B134;
	else if ( baudString == "B150" )
		baudRate = B150;
	else if ( baudString == "B200" )
		baudRate = B200;
	else if ( baudString == "B300" )
		baudRate = B300;
	else if ( baudString == "B600" )
		baudRate = B600;
	else if ( baudString == "B1200" )
		baudRate = B1200;
	else if ( baudString == "B1800" )
		baudRate = B1800;
	else if ( baudString == "B2400" )
		baudRate = B2400;
	else if ( baudString == "B4800" )
		baudRate = B4800;
	else if ( baudString == "B9600" )
		baudRate = B9600;
	else if ( baudString == "B19200" )
		baudRate = B19200;
	else if ( baudString == "B38400" )
		baudRate = B38400;
	else
	{
		qCCritical(KTL_LOG) << "Unknown baud rate = \""<<baudString<<"\"";
		return;
	}
	
	initPort( dataString("port"), baudRate );
#endif

    initPort(dataString("port"), 200);
}

void SerialPortComponent::initPort(const QString &port, qint32 baudRate)
{
    if (port.isEmpty()) {
        m_pSerialPort->closePort();
        return;
    }

    if (!m_pSerialPort->openPort(port, baudRate)) {
        p_itemDocument->canvas()->setMessage(i18n("Could not open port %1", port));
        return;
    }
}

void SerialPortComponent::stepNonLogic()
{
    m_pCD->setHigh(m_pSerialPort->getDataCarrierDetectSignal());
    // 	m_pRD->setHigh(m_pSerialPort->getSecondaryReceivedDataSignal());
    m_pCTS->setHigh(m_pSerialPort->getClearToSendSignal());
    m_pRI->setHigh(m_pSerialPort->getRingIndicatorSignal());
}

void SerialPortComponent::tdCallback(bool isHigh)
{
    m_pSerialPort->setBreakEnabled(isHigh);
}

void SerialPortComponent::dtrCallback(bool isHigh)
{
    m_pSerialPort->setDataTerminalReady(isHigh);
}

void SerialPortComponent::dsrCallback(bool isHigh)
{
    m_pSerialPort->setDataSetReady(isHigh);
}

void SerialPortComponent::rtsCallback(bool isHigh)
{
    m_pSerialPort->setRequestToSend(isHigh);
}

void SerialPortComponent::drawShape(QPainter &p)
{
    drawPortShape(p);
}
