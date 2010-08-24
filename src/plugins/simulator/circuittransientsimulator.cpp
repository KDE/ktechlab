/***************************************************************************
 *    KTechLab Circuit Simulator in Transient Analysis                     *
 *       Simulates circuit documents in time domain                        *
 *    Copyright (c) 2010 Zoltan Padrah                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "circuittransientsimulator.h"

#include "interfaces/icomponentdocument.h"
#include "interfaces/idocumentmodel.h"


#include <kdebug.h>
#include <interfaces/simulator/isimulationmanager.h>
#include <interfaces/simulator/ielementfactory.h>
#include "pingroup.h"
#include "interfaces/simulator/ielement.h"
#include "interfaces/simulator/iwire.h"

using namespace KTechLab;

CircuitTransientSimulator::CircuitTransientSimulator(IComponentDocument* doc):
    ISimulator(doc)
{
    // check for the correct document type
    if(doc->documentType() != "Circuit")
    {
        kError() << "BUG: trying to simulate a non-circuit document as a circuit!\n";
        // FIXME what to do here? save all and exit?
    }
    m_doc = doc->documentModel();
    // TODO connect the dataUpdated from the document model to the simulator
}

void CircuitTransientSimulator::start()
{
    kDebug() << "start\n";
}

void CircuitTransientSimulator::pause()
{
    kDebug() << "pause\n";
}

void CircuitTransientSimulator::tooglePause()
{
    kDebug() << "togglePause\n";

}

IElement* CircuitTransientSimulator::getModelForComponent(QVariantMap* component)
{
    kDebug() << "getModelForComponent\n";
    return NULL;
}

void CircuitTransientSimulator::componentParameterChanged(QVariantMap* component)
{
    kDebug() << "componentParameterChanged signaled\n";
    /*
    the circuit state values have to be reset here, and the new state
    has to be recalculated
    */
}

void CircuitTransientSimulator::documentStructureChanged()
{
    kDebug() << "documentStructureChanged\n";
    /*
    in case of document structure change, the simulator data
    structures should be updated
    - generate Pins and Elements from Components
    - based on Connectors, connect the Pins with Wires; possibly create
        new Pins, if needed (between 2 connectors)
    - split the Circuit into smaller connected units, for example ElementSets
    - for each ElementSet:
        - group the connected Pins in CNodes
        - count the number of node in an ElementSet
        - count the number of independent sources in element set
        - allocate matrix for equations
        - define voltage and current-solution chains in order to
            find all the voltages and currents
        - define chanins to find voltages and currents on all Pins and Wires
    */
    m_canBeSimulated = false;
    if( ! recreateElementList() ){
        kError() << "failed to recreate element list\n";
        return;
    }
    if( ! recreateNodeList() ){
        kError() << "failed to recreate node list\n";
        return;
    }
    if( ! recreateWireList() ){
        kError() << "failed to recreated the wire list\n";
        return;
    }
    if( ! splitPinsInGroups() ){
        kError() << "failed to split pins in groups\n";
        return;
    }
    /*
    splitDocumentInCircuits(); 
    foreach(Circuit *c, m_circuits){ // what kind of abstraction?
        stepSimulation();
        createElementSet();
        solveElementSet();
    }
    */
    m_canBeSimulated = true;
}

bool CircuitTransientSimulator::recreateElementList()
{
    // clear the list
    qDeleteAll(m_allElementList);
    m_allElementList.clear();
    m_idToElement.clear();
    // cache a pointer
    ISimulationManager *simMng = ISimulationManager::self();

    QVariantMap allComponents = m_doc->components();
    foreach(QVariant componentVariant, m_doc->components() ){
        if( componentVariant.type() != QVariant::Map){
            kError() << "BUG: this component is not a QVariantMap: " << componentVariant << "\n";
            return false;
        }
        QVariantMap componentVarMap = componentVariant.toMap();
        // get the type of the component
        if(! componentVarMap.contains("type") ){
            kError() << "BUG: a component doesn't have a \"type\" field!\n";
            return false;
        }
        QString compType = componentVarMap.value("type").toString();

        QList<IElementFactory*> elemFactList = simMng->registeredFactories("transient", compType, "application/x-circuit");
        if( elemFactList.isEmpty() ){
            kError() << "cannot create simulation model for the component type \""
                + compType + "\": unknown component type\n";
            return false;
        }
        if( elemFactList.size() > 1){
            kWarning() << "more than one model factory for component type " << compType
                << ", selecting first one \n";
        }
        // pick a factory
        IElement *element = elemFactList.first()->createElement(compType);
        // place in the list
        m_allElementList.append(element);
            // is the following line efficient?
        m_idToElement.insert(m_doc->components().key(componentVariant),
                             element);
        kDebug() << componentVarMap << "\n";
    }
    kDebug() << "created " << m_allElementList.count() << " elements\n";
}

bool CircuitTransientSimulator::recreateNodeList()
{
    QVariantMap allConnectors = m_doc->connectors();
}

bool CircuitTransientSimulator::recreateWireList()
{
    // clear the list
    qDeleteAll(m_allWireList);
    m_allWireList.clear();
    m_idToWire.clear();
    // repopulate the list
    // TODO implment
    //
    kDebug() << "created " << m_allWireList.count() << " wires\n";
}

bool CircuitTransientSimulator::splitPinsInGroups()
{
    // clean up
    qDeleteAll(m_pinGroups);
    m_pinGroups.clear();
    // repopulate the list
    // TODO implement
    kDebug() << "created " << m_pinGroups.count() << " pin groups\n";
}


void CircuitTransientSimulator::simulationTimerTicked()
{
    kDebug() << "simulationTimerTicked\n";
    /*
    general algorithm for circuit simulation:
    - notify the elements about the new simulation time
    - run all the logic circuits
    - for all ElementSets
        - while not converged:
            - ask for coefficients (A and z matrix)
            - solve equations
            - back-substitute the results
    */
}

bool CircuitTransientSimulator::variantToBool(const QVariant& variant, bool &success)
{
    if(variant.isValid()){
        kError() << "cannot convert invalid variant to bool!\n";
        success = false;
        return false;
    }
    if(variant.type() != QVariant::String){
        // XML for the looser: everything is a string...
        kError() << "cannot convert non-string variant to bool: "
            << variant << "\n";
        success = false;
        return false;
    }
    if(variant.toString() == "0"){
        success = true;
        return false;
    }
    if(variant.toString() == "1"){
        success = true;
        return true;
    }
    // got here, so it must be junk
    kError() << "junk instead of boolean value: " << variant << "\n";
    success = true;
    return false;
}

QString CircuitTransientSimulator::variantToString(const QVariant& string, bool& success)
{
    if(string.isValid()){
        kError() << "cannot convert invalid variant to string!\n";
        success = false;
        return false;
    }
    if(string.type() != QVariant::String){
        // XML for the loose: everything is a string...
        kError() << "cannot convert non-string variant to bool: "
            << string << "\n";
        success = false;
        return QString();
    }
    success = true;
    return string.toString();
}

