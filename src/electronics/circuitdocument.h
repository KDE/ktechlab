/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef CIRCUITDOCUMENT_H
#define CIRCUITDOCUMENT_H

#include "circuiticndocument.h"
#include "pin.h"

class Circuit;
class Component;
class Connector;
class ECNode;
class Element;
class CircuitICNDocument;
class KTechlab;
class Pin;
class QTimer;
class Switch;
class Wire;

class KActionMenu;

typedef QList<Circuit *> CircuitList;
typedef QList<Component *> ComponentList;
typedef QList<QPointer<Connector>> ConnectorList;
typedef QList<ECNode *> ECNodeList;
typedef QList<Element *> ElementList;
typedef QList<QPointer<Pin>> PinList;
typedef QList<Switch *> SwitchList;
typedef QList<QPointer<Wire>> WireList;

class Circuitoid
{
public:
    bool contains(Pin *node)
    {
        return pinList.contains(node);
    }
    bool contains(Element *ele)
    {
        return elementList.contains(ele);
    }

    void addPin(Pin *node)
    {
        if (node && !contains(node))
            pinList += node;
    }
    void addElement(Element *ele)
    {
        if (ele && !contains(ele))
            elementList += ele;
    }

    PinList pinList;
    ElementList elementList;
};

/**
CircuitDocument handles allocation of the components displayed in the ICNDocument
to various Circuits, where the simulation can be performed, and displays the
information from those simulations back on the ICNDocument
@short Circuit view
@author David Saxton
*/
class CircuitDocument : public CircuitICNDocument
{
    Q_OBJECT
public:
    CircuitDocument(const QString &caption);
    ~CircuitDocument() override;

    View *createView(ViewContainer *viewContainer, uint viewAreaId) override;

    void calculateConnectorCurrents();
    /**
     * Count the number of ExternalConnection components in the CNItemList
     */
    int countExtCon(const ItemList &cnItemList) const;

    void update() override;

public Q_SLOTS:
    /**
     * Creates a subcircuit from the currently selected components
     */
    void createSubcircuit();
    void displayEquations();
    void setOrientation0();
    void setOrientation90();
    void setOrientation180();
    void setOrientation270();
    void rotateCounterClockwise();
    void rotateClockwise();
    void flipHorizontally();
    void flipVertically();
    /**
     * Enables / disables / selects various actions depending on what is
     * selected or not.
     */
    void slotInitItemActions() override;
    void requestAssignCircuits();
    void componentAdded(Item *item);
    void componentRemoved(Item *item);
    void connectorAddedSlot(Connector *connector);
    void slotUpdateConfiguration() override;

protected:
    void itemAdded(Item *item) override;
    void fillContextMenu(const QPoint &pos) override;
    bool isValidItem(Item *item) override;
    bool isValidItem(const QString &itemId) override;

    KActionMenu *m_pOrientationAction;

private Q_SLOTS:
    void assignCircuits();

private:
    /**
     * If the given circuitoid can be a LogicCircuit, then it will be added to
     * m_logicCircuits, and return true. Else returns false.
     */
    bool tryAsLogicCircuit(Circuitoid *circuitoid);
    /**
     * Creates a circuit from the circuitoid
     */
    Circuit *createCircuit(Circuitoid *circuitoid);

    /**
     * @param pin Current node (will be added, then tested for further
     * connections).
     * @param pinList List of nodes in current partition.
     * @param unassignedPins The pool of all nodes in the CircuitDocument
     * waiting for assignment.
     * @param onlyGroundDependent if true, then the partition will not use
     * circuit-dependent pins to include new pins while growing the
     * partition.
     */
    void getPartition(Pin *pin, PinList *pinList, PinList *unassignedPins, bool onlyGroundDependent = false);
    /**
     * Takes the nodeList (generated by getPartition), splits it at ground nodes,
     * and creates circuits from each split.
     */
    void splitIntoCircuits(PinList *pinList);
    /**
     * Construct a circuit from the given node, stopping at the groundnodes
     */
    void recursivePinAdd(Pin *pin, Circuitoid *circuitoid, PinList *unassignedPins);

    void deleteCircuits();

    QTimer *m_updateCircuitsTmr;
    CircuitList m_circuitList;
    ComponentList m_toSimulateList;
    ComponentList m_componentList; // List is built up during call to assignCircuits

    // hmm, we have one of these in circuit too....
    PinList m_pinList;
    WireList m_wireList;
    SwitchList m_switchList;
};

#endif
