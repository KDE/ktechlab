/***************************************************************************
 *   Copyright (C) 2003-2006 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <vector>
#include "circuit.h"
#include "elementset.h"
#include "logic.h"
#include "matrix.h"
#include "simulator.h"
#include "src/core/ktlconfig.h"

//BEGIN class LogicConfig
LogicConfig::LogicConfig() {
	risingTrigger = 0.0;
	fallingTrigger = 0.0;
	output = 5.0;
	highImpedance = 0.0001;
	lowImpedance = 0.0001;
}
//END class LogicConfig

//BEGIN class LogicIn
LogicIn::LogicIn(LogicConfig config)
		: Element::Element() {
	m_config = config;
	m_pCallbackFunction = 0;
	m_numCNodes = 1;
	m_bState = false;
	m_pNextLogic = 0;
	setLogic(getConfig());
}

LogicIn::~LogicIn() {
	Simulator::self()->removeLogicInReferences(this);
	m_pCallbackObject = 0;
	m_pCallbackFunction = 0;
}

void LogicIn::setCallback(CallbackClass *object, CallbackPtr func) {
	m_pCallbackFunction = func;
	m_pCallbackObject = object;
}

void LogicIn::check() {
	if (!b_status)
		return;

	bool newState;

	if (m_bState) {
		// Was high, will still be high unless voltage is less than falling trigger
		newState = p_cnode[0]->v > m_config.fallingTrigger;
	} else {
		// Was low, will still be low unless voltage is more than rising trigger
		newState = p_cnode[0]->v > m_config.risingTrigger;
	}

	if (m_pCallbackFunction && (newState != m_bState)) {
		m_bState = newState;
		(m_pCallbackObject->*m_pCallbackFunction)(newState);
	}

	m_bState = newState;
}

void LogicIn::setLogic(LogicConfig config) {
	m_config = config;
	check();
}

void LogicIn::setElementSet(ElementSet *c) {
	if(c) m_pNextLogic = 0;
	else m_cnodeI[0] = 0.;

	Element::setElementSet(c);
}

void LogicIn::add_initial_dc() {
}

void LogicIn::updateCurrents() {
}

LogicConfig LogicIn::getConfig() {
	LogicConfig c;
	c.risingTrigger = KTLConfig::logicRisingTrigger();
	c.fallingTrigger = KTLConfig::logicFallingTrigger();
	c.output = KTLConfig::logicOutputHigh();
	c.highImpedance = KTLConfig::logicOutputHighImpedance();
	c.lowImpedance = KTLConfig::logicOutputLowImpedance();
	return c;
}
//END class LogicIn

//BEGIN class LogicOut
LogicOut::LogicOut(LogicConfig config, bool _high)
		: LogicIn(config) {
	m_bCanAddChanged = true;
	m_pNextChanged[0] = m_pNextChanged[1] = 0;
	m_bUseLogicChain = false;
	m_numCNodes = 1;
	m_old_g_out = m_g_out = 0.0;
	m_old_v_out = m_v_out = 0.0;
	setHigh(_high);

	configChanged();
}

LogicOut::~LogicOut() {

	// Note that although this function will get called in the destructor of
	// LogicIn, we must call it here as well as it needs to be called before
	// removeLogicOutReferences(this) is called.

	Simulator *theSimulator = Simulator::self();

	theSimulator->removeLogicInReferences(this);
	theSimulator->removeLogicOutReferences(this);
}

void LogicOut::setUseLogicChain(bool use) {
	m_bUseLogicChain = use;

	if (use) setElementSet(0);
}

void LogicOut::setElementSet(ElementSet *c) {

	if (c) {
		m_bUseLogicChain = false;
		m_pNextChanged[0] = m_pNextChanged[1] = 0;
	}

	LogicIn::setElementSet(c);
}

void LogicOut::setOutputHighConductance(double g) {
	double impedance = 1.0 / g;

	if (impedance == m_config.highImpedance) return;

	m_config.highImpedance = impedance;
	configChanged();
}

void LogicOut::setOutputLowConductance(double g) {
	double impedance = 1.0 / g;

	if (impedance == m_config.lowImpedance) return;

	m_config.lowImpedance = impedance;
	configChanged();
}

void LogicOut::setOutputHighVoltage(double v) {

	if (v == m_config.output) return;

	m_config.output = v;
	configChanged();
}

void LogicOut::setLogic(LogicConfig config) {
	m_config = config;

	configChanged();
}

void LogicOut::configChanged() {
	if (m_bUseLogicChain) return;

	if (p_eSet)
		p_eSet->setCacheInvalidated();

	// Re-add the DC stuff using the new values
	m_old_g_out = m_g_out;
	m_old_v_out = m_v_out;

	// NOTE Make sure that the next two lines are the same as those in setElementSet and setHigh
	m_g_out = m_bState ? 1.0 / m_config.highImpedance : 1.0 / m_config.lowImpedance;
	m_v_out = m_bState ? m_config.output : 0.0;

	add_initial_dc();

	m_old_g_out = 0.;
	m_old_v_out = 0.;

	check();
}

void LogicOut::add_initial_dc() {
	if (!b_status)
		return;

	A_g(0, 0) += m_g_out - m_old_g_out;
	b_i(0) += m_g_out * m_v_out - m_old_g_out * m_old_v_out;
}

void LogicOut::updateCurrents() {
	if (m_bUseLogicChain) {
		m_cnodeI[0] = 0.0;
		return;
	}

	if (!b_status)
		return;

	m_cnodeI[0] = (m_v_out - p_cnode[0]->v) * m_g_out;
}

void LogicOut::setHigh(bool high) {
	if (high == m_bState)
		return;

	if (m_bUseLogicChain) {
		setChain(high);

		if (m_bCanAddChanged) {
			Simulator::self()->addChangedLogic(this);
			m_bCanAddChanged = false;
		}

		return;
	}

	m_old_g_out = m_g_out;
	m_old_v_out = m_v_out;

	// NOTE Make sure that the next two lines are the same as those in setElementSet and setLogic
	m_g_out = high ? 1.0 / m_config.highImpedance : 1.0 / m_config.lowImpedance;
	m_v_out = high ? m_config.output : 0.0;

	add_initial_dc();

	m_old_g_out = 0.;
	m_old_v_out = 0.;

	m_bState = high;

	if (p_eSet && p_eSet->circuit()->canAddChanged()) {
		Simulator::self()->addChangedCircuit(p_eSet->circuit());
		p_eSet->circuit()->setCanAddChanged(false);
	}
}
//END class LogicOut

void LogicIn::setChain(bool high) {
	m_bState = high; 
	if(m_pNextLogic) m_pNextLogic->setChain(high);
}
