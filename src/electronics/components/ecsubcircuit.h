/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ECSUBCIRCUIT_H
#define ECSUBCIRCUIT_H

#include <component.h>

#include <QVector>

/**
"Container" component for subcircuits
@author David Saxton
*/
class ECSubcircuit : public Component
{
    Q_OBJECT
public:
    ECSubcircuit(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
    ~ECSubcircuit() override;

    static Item *construct(ItemDocument *itemDocument, bool newItem, const char *id);
    static LibraryItem *libraryItem();

    /**
     * Create numExtCon nodes, deleting any old ones
     */
    void setNumExtCon(unsigned numExtCon);
    /**
     * Give the connecting node at position numId the given name
     */
    void setExtConName(unsigned numId, const QString &name);
    /**
     * Called from SubcircuitData once the subcircuit has been fully attached
     */
    void doneSCInit();

public Q_SLOTS:
    void removeItem() override;

Q_SIGNALS:
    /**
     * Emitted when the current subcircuit is deleted
     */
    void subcircuitDeleted();

protected:
    void dataChanged() override;
    void drawShape(QPainter &p) override;
    QVector<QString> m_conNames;
};

#endif
