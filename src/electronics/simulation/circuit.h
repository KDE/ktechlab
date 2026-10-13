/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <QList>
#include <QPointer>
#include <QMap>

#include "element.h"
#include "elementset.h"

class CircuitDocument;
class LogicOut;

typedef QList<QPointer<Pin>> PinList;
typedef QList<Element *> ElementList;
typedef QMap<Element *, PinList> ElementPinMapping;


class LogicCacheNode
{
public:
    LogicCacheNode();
    ~LogicCacheNode();

    LogicCacheNode *high;
    LogicCacheNode *low;
    QuickVector *data;
};

/**
Usage of this class (usually invoked from CircuitDocument):
(1) Add Wires, Pins and Elements to the class as appropriate
(2) Call init to initialize the simulation
(3) Control the simulation with step()

This class can be considered a bridge between the gui-tainted CircuitDocument - specific
to this implementation, and the pure untainted ElementSet. Please keep it that way.

@short Simulates a collection of components
@author David Saxton
*/
class Circuit
{
public:
    Circuit();
    ~Circuit();

    void addPin(Pin *node);
    void addPins(const PinList& pins)
    {
        for (Pin* p: pins)
            addPin(p);
    }
    const PinList& pinList() const
    {
        return m_pinList;
    }

    void addWire(Wire *wire)
    {
        if (wire && !m_wireList.contains(wire))
            m_wireList.append(wire);
    }

    void addWires(const WireList &wires)
    {
        for (Wire* wire: wires)
            addWire(wire);
    }
    const WireList& wireList() const
    {
        return m_wireList;
    }

    void addElement(Element *element);
    void addElements(const ElementList& elements)
    {
        for (Element* e: elements)
            addElement(e);
    }
    const ElementList& elementList() const
    {
        return m_elementList;
    }

    const ElementSet* elementSet() const
    {
        return m_elementSet;
    }

    /**
     * Set the elements to node/pin mapping. If any of the elements
     * or pins are not included in the circuit they will be added.
     * The pins list size must equal the elements number of CNodes.
     * This mapping is normally handled by the Component class and was
     * added for testing purposes.
     */
    void setElementPinMapping(Element *element, const PinList& pins);

    const ElementPinMapping& elementPinMapping() const {
        return m_elementPinMapping;
    }

    bool contains(Pin *node);
    bool containsNonLinear() const
    {
        return m_elementSet->containsNonLinear();
    }

    void init();
    /**
     * Called after everything else has been setup - before doNonLogic or
     * doLogic are called for the first time. Preps the circuit.
     */
    void initCache();
    /**
     * Marks all cached results as invalidated and removes them.
     */
    void setCacheInvalidated();
    /**
     * Solves for non-logic elements
     */
    void doNonLogic();
    /**
     * Solves for logic elements (i.e just does fbSub)
     */
    void doLogic()
    {
        m_elementSet->doLinear(false);
    }

    void displayEquations();
    void updateCurrents();

    void createMatrixMap();
    /**
        * This will identify the ground node and non-ground nodes in the given set.
        * Ground will be given the eqId -1, non-ground of 0.
        * @param highest The highest ground type of the groundnodes found. If no
        ground nodes were found, this will be (gt_never-1).
        * @returns the number of ground nodes. If all nodes are at or below the
        * 			gt_never threshold, then this will be zero.
        */
    static int identifyGround(PinList nodeList, int *highest = nullptr);

    void setNextChanged(Circuit *circuit, unsigned char chain)
    {
        m_pNextChanged[chain] = circuit;
    }
    Circuit *nextChanged(unsigned char chain) const
    {
        return m_pNextChanged[chain];
    }
    void setCanAddChanged(bool canAdd)
    {
        m_bCanAddChanged = canAdd;
    }
    bool canAddChanged() const
    {
        return m_bCanAddChanged;
    }

    /**
     * Sets element cnodes equation ids. This requires
     * all elements to have a pin mapping defined or it
     * will return false.
     * This should called after the circuit is initialized.
     * It was added for testing purposes.
     * @returns whether all of the assignment could be done.
     */
    [[nodiscard]] bool assignEquationIds();

    /**
     * Calls `add_initial_dc` on all elements.
     * This should be called after the cnode equations are assigned
     * and the matrix map is created. This is normally handled by
     * the Component class and was added for testing purposes.
     */
    void addInitialDc()
    {
        Q_ASSERT(m_elementSet);
        for (Element* e: std::as_const(m_elementList))
            e->add_initial_dc();
    }

    /**
     * Update nodal and wire currents from those calculated in ElementSet.
     * This only works for elements which have an ElementPinMapping
     * and was added for testing purposes.
     * See CircuitDocument::calculateConnectorCurrents
     */
    [[nodiscard]] bool calculateCurrents();

protected:
    void cacheAndUpdate();
    /**
     * Update the nodal voltages from those calculated in ElementSet
     */
    void updateNodalVoltages();
    /**
     * Step the reactive elements.
     */
    void stepReactive();
    /**
     * Returns true if any of the nodes are ground
     */
    static bool recursivePinAdd(Pin *node, PinList *unassignedNodes, PinList *associated, PinList *nodes);

    PinList m_pinList;
    ElementList m_elementList;
    ElementSet *m_elementSet = nullptr;
    ElementPinMapping m_elementPinMapping; // Used for testing (doesn't depend on Component)
    WireList m_wireList;

    // Stuff for caching
    bool m_bCanCache = false;
    LogicCacheNode *m_pLogicCacheBase;
    QList<LogicOut *> m_logicOut;

    bool m_bCanAddChanged = true;
    Circuit *m_pNextChanged[2];
};

#endif
