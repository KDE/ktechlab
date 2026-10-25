/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ELEMENTSET_H
#define ELEMENTSET_H

#include "cbranch.h"
#include "cnode.h"
#include <Eigen/Dense>
#include <QList>

class Circuit;
class Element;
class ElementSet;
class LogicIn;
class NonLinear;

typedef QList<Element *> ElementList;
typedef QList<NonLinear *> NonLinearList;

typedef Eigen::MatrixXd Matrix;
typedef Eigen::VectorXd Vector;


/**
Steps in simulation of a set of elements:
(1) Create this class with given number of nodes "n" and voltage sources "m"
(2) Add the various elements with addElement.
(3) Call performDC()
(4) Get the nodal voltages and voltage currents with x()
(5) Repeat steps 3 and 4 if necessary for further transient analysis.

This class shouldn't be confused with the Circuit class, but considered a helper class to Circuit.
Circuit will handle the simulation of a set of components over time. This just finds the DC-operating
point of the circuit for a given set of elements.

@short Handles a set of circuit elements
@author David Saxton
*/
class ElementSet
{
public:
    /**
     * Create a new circuit, with "n" nodes and "m" voltage sources.
     * After creating the circuit, you must call setGround to specify
     * the ground nodes, before adding any elements.
     */
    ElementSet(Circuit *circuit, const uint n, const uint m);
    /**
     * Destructor. Note that only the matrix and supporting data is deleted.
     * i.e. Any elements added to the circuit will not be deleted.
     */
    ~ElementSet();
    Circuit *circuit() const
    {
        return m_pCircuit;
    }
    void addElement(Element *e);
    void removeElement(Element *e);
    void setCacheInvalidated();
    /**
     * Returns the matrix in use. This is created once on the creation of the ElementSet
     * class, and deleted in the destructor, so the pointer returned will never change.
     */
    Matrix& matrix()
    {
        return m_A;
    }
    const Matrix &matrix() const
    {
        return m_A;
    }
    /**
     * Returns the vector for b (i.e. the independent currents & voltages)
     */
    Vector &b()
    {
        return m_b;
    }

    const Vector &b() const
    {
        return m_b;
    }
    /**
     * Returns the vector for x (i.e. the currents & voltages at the branches and nodes)
     */
    Vector &x()
    {
        return m_x;
    }

    const Vector &x() const
    {
        return m_x;
    }
    /**
     * @return if we have any nonlinear elements (e.g. diodes, tranaistors).
     */
    bool containsNonLinear() const
    {
        return m_cnonLinearList.size() > 0;;
    }
    /**
     * Solves for nonlinear elements, or just does linear if it doesn't contain
     * any nonlinear.
     */
    void doNonLinear(int maxIterations, double maxErrorV = 1e-9, double maxErrorI = 1e-12);
    /**
     * Solves for linear and logic elements.
     * @returns true if anything changed
     */
    bool doLinear(bool performLU);

    const CBranch& cbranch(const uint i) const
    {
        return m_cbranches[i];
    }

    const CNode& cnode(const uint i) const
    {
        return m_cnodes[i];
    }

    const QList<CBranch>& cbranches() const
    {
        return m_cbranches;
    }
    const QList<CNode>& cnodes() const
    {
        return m_cnodes;
    }
    const CNode& ground() const
    {
        return m_ground;
    }
    /**
     * Returns the number of nodes in the circuit (excluding ground 'nodes')
     */
    uint cnodeCount() const
    {
        return m_cnodes.size();
    }
    /**
     * Returns the number of voltage sources in the circuit
     */
    uint cbranchCount() const
    {
        return m_cbranches.size();
    }

    void createMatrixMap();
    /**
     * Displays the matrix equations Ax=b and J(dx)=-r
     */
    void displayEquations();
    /**
     * Update the nodal voltages and branch currents from the x vector
     */
    void updateInfo();

    bool a_changed = false;
    bool b_changed = false;
private:
    // calc engine stuff
    Matrix m_A;
    Vector m_x;
    Vector m_x_prev;
    Vector m_b;
    Eigen::FullPivLU<Matrix> m_LU;
    // end calc engine stuff.

    ElementList m_elementList;
    NonLinearList m_cnonLinearList;

    QList<CBranch> m_cbranches; // Pointer to an array of cbranches
    QList<CNode> m_cnodes; // Pointer to an array of cnodes
    CNode m_ground;

    QList<LogicIn *> m_logicIn;
    Circuit *m_pCircuit;
};

#endif
