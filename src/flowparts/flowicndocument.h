//
// C++ Interface: flowicndocument
//
// Description:
//
//
// Author: Zoltan P <zoltan.padrah@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef FLOWICNDOCUMENT_H
#define FLOWICNDOCUMENT_H

#include <icndocument.h>

class Node;
class FPNode;

typedef QMap<QString, FPNode *> FPNodeMap;

/**
    A document holding a flow-diagram
    @author Zoltan P <zoltan.padrah@gmail.com>
*/
class FlowICNDocument : public ICNDocument
{
    Q_OBJECT

public:
    FlowICNDocument(const QString &caption);

    ~FlowICNDocument() override;

    /**
     * Reinherit this function to perform special checks on whether the two
     * given QCanvasItems (either nodes or connectors or both) can be
     * connected together.
     */
    bool canConnect(KtlQCanvasItem *qcanvasItem1, KtlQCanvasItem *qcanvasItem2) const override;

    /**
     * Splits Connector con into two connectors at point pos2, and creates a connector from the node
     * to the intersection of the two new connectors. If pointList is non-null, then the new connector
     * from the node will be assigned those points
     */
    Connector *createConnector(Node *node, Connector *con, const QPoint &pos2, QPointList *pointList = nullptr) override;
    /**
     * Splits con1 and con2 into two new connectors each at points pos1 and pos2, and creates a new connector
     * between the two points of intersection given by pos1 and pos2. If pointList is non-null, then the new
     * connector between the two points will be assigned those points
     */
    Connector *createConnector(Connector *con1, Connector *con2, const QPoint &pos1, const QPoint &pos2, QPointList *pointList = nullptr) override;
    /**
     * Creates a connector between two nodes, and returns a pointer to it
     * and adds the operation to the undo list
     */
    Connector *createConnector(const QString &startNodeId, const QString &endNodeId, QPointList *pointList = nullptr) override;

    /**
     * Returns a pointer to a node on the canvas with the given id,
     * or nullptr if no such node exists
     */
    Node *nodeWithID(const QString &id) override;
    FPNode *getFPnodeWithID(const QString &id);
    /**
     * Assigns the orphan nodes into NodeGroups. You shouldn't call this
     * function directly - instead use ItemDocument::requestEvent.
     */
    void slotAssignNodeGroups() override;

    /**
     * Permanently deletes all items that have been added to the delete list with
     * the appendDeleteList( KtlQCanvasItem *qcanvasItem ) function.
     */
    void flushDeleteList() override;

    /**
     * registers (adds to the document) an item (a connector or a node)
     * @param qcanvasItem the item to be registered
     * @return true if succeeded, false if it didn't
     */
    bool registerItem(KtlQCanvasItem *qcanvasItem) override;
    void unregisterUID(const QString &uid) override;
    NodeList nodeList() const override;

protected:
    /**
     * If there are two connectors joined to a node, then they can be merged
     * into one connector. The node will not be removed.
     * @param fpnode The node between the two connectors
     * @returns true if it was successful in merging the connectors
     */
    bool joinConnectors(FPNode *fpnode);
    /**
     *        Selects all nodes on the document. Should be overridden.
     */
    void selectAllNodes() override;

    /**
     *        deletes all the elements containde in the nodeList. Should be overridden.
     */
    void deleteAllNodes() override;

    /// the list of flownodes in the documents
    FPNodeMap m_flowNodeList;
};

#endif
