/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef RESIZEOVERLAY_H
#define RESIZEOVERLAY_H

// This file contains class definitions for different types of resizing and rotating

//#include <canvas.h> // 2018.10.16 - not needed
#include "canvasitems.h"
#include "item.h"
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>

class MechanicsItem;
class ResizeHandle;
class ResizeOverlay;
class QEvent;

typedef QMap<int, QPointer<ResizeHandle>> ResizeHandleMap;

/**
@author David Saxton
*/
class ResizeHandle : /* public QObject, */ public KtlQCanvasRectangle
{
    Q_OBJECT
public:
    /**
     * Convenience enumeration for resize handle positioning. Note: this class
     * does not make use of the values in this enumeration - it is just
     * provided here for use by other classes.
     */
    enum ResizeHandlePosition { rhp_none, rhp_topLeft, rhp_topMiddle, rhp_topRight, rhp_middleRight, rhp_bottomRight, rhp_bottomMiddle, rhp_bottomLeft, rhp_middleLeft, rhp_center, rhp_start, rhp_end };

    enum DrawType {
        // Draws a simple rectangle
        dt_point_rect,

        // Crosshair
        dt_point_crosshair,

        // Straight arrows in various directions
        dt_resize_forwardsDiagonal,
        dt_resize_backwardsDiagonal,
        dt_resize_vertical,
        dt_resize_horizontal,

        // Arrows as part of an arc
        dt_rotate_topLeft,
        dt_rotate_topRight,
        dt_rotate_bottomRight,
        dt_rotate_bottomLeft
    };

    ResizeHandle(ResizeOverlay *resizeOverlay, int id, DrawType drawType, int xsnap, int ysnap);
    ~ResizeHandle() override;

    int id() const
    {
        return m_id;
    }

    void setDrawType(DrawType drawType);
    void moveRH(double x, double y);
    void setHover(bool hover);

    static const QPixmap &handlePixmap(DrawType drawType, bool hover);

    QPolygon areaPoints() const override;

public Q_SLOTS:
    void slotMoveByX(double dx)
    {
        moveBy(dx, 0);
    }
    void slotMoveByY(double dy)
    {
        moveBy(0, dy);
    }

Q_SIGNALS:
    void rhMovedBy(int id, double dx, double dy);
    void rhMovedByX(double dx);
    void rhMovedByY(double dy);

protected:
    void drawShape(QPainter &p) override;

    DrawType m_drawType;
    bool b_hover; // If true, then paint resize handle for mouse hovering over
    int m_id;
    int m_xsnap;
    int m_ysnap;
    ResizeOverlay *p_resizeOverlay;
};
typedef QList<ResizeHandle *> ResizeHandleList;

/**
@author David Saxton
*/
class ResizeOverlay : public QObject
{
    Q_OBJECT
public:
    ResizeOverlay(Item *parent);
    ~ResizeOverlay() override;

    Item *parentItem() const
    {
        return p_item;
    }

    /**
     * Shows / hides the resize handles. They are hidden by default.
     */
    void showResizeHandles(bool show);
    /**
     * Sets the visibility. Visibility is true by default.
     */
    void setVisible(bool visible);
    /**
     * Reinherit this function to determine whether the X coordinate of the spot
     * that the resize handle has moved into is valid or not
     */
    virtual bool isValidXPos(ResizeHandle *rh)
    {
        Q_UNUSED(rh);
        return true;
    }
    /**
     * Reinherit this function to determine whether the Y coordinate of the spot
     * that the resize handle has moved into is valid or not
     */
    virtual bool isValidYPos(ResizeHandle *rh)
    {
        Q_UNUSED(rh);
        return true;
    }

public Q_SLOTS:
    void slotMoveAllResizeHandles(double dx, double dy);

protected Q_SLOTS:
    virtual void slotResizeHandleMoved(int id, double dx, double dy) = 0;

protected:
    /**
     * Connects up the given resize handles so that they are always kept at the
     * same horizontal coordinate
     */
    void syncX(ResizeHandle *rh1, ResizeHandle *rh2);
    void syncX(ResizeHandle *rh1, ResizeHandle *rh2, ResizeHandle *rh3);
    /**
     * Connects up the given resize handles so that they are always kept at the
     * same vertical coordinate
     */
    void syncY(ResizeHandle *rh1, ResizeHandle *rh2);
    void syncY(ResizeHandle *rh1, ResizeHandle *rh2, ResizeHandle *rh3);
    /**
     * Returns a pointer to the ResizeHandle with the given id, or 0 if no such
     * handle exists
     */
    ResizeHandle *resizeHandle(int id);
    /**
     * Creates and attaches the resize handle with the given DrawType. If a
     * ResizeHandle with the given id exists, will return a pointer to that
     * instead
     */
    ResizeHandle *createResizeHandle(int id, ResizeHandle::DrawType drawType, int xsnap = 1, int ysnap = 1);
    /**
     * Removes the resize handle with the given id
     */
    void removeResizeHandle(int id);

    Item *p_item;
    ResizeHandleMap m_resizeHandleMap;
    bool b_showResizeHandles;
    bool b_visible;
};

/**
@author David Saxton
*/
class MechanicsItemOverlay : public ResizeOverlay
{
    Q_OBJECT
public:
    MechanicsItemOverlay(MechanicsItem *parent);
    ~MechanicsItemOverlay() override;

public Q_SLOTS:
    void slotUpdateResizeHandles();

protected Q_SLOTS:
    void slotResizeHandleMoved(int id, double dx, double dy) override;

protected:
    ResizeHandle *m_tl;
    ResizeHandle *m_tm;
    ResizeHandle *m_tr;
    ResizeHandle *m_mr;
    ResizeHandle *m_br;
    ResizeHandle *m_bm;
    ResizeHandle *m_bl;
    ResizeHandle *m_ml;
    ResizeHandle *m_mm;
    MechanicsItem *p_mechanicsItem;
};

/**
@author David Saxton
*/
class RectangularOverlay : public ResizeOverlay
{
    Q_OBJECT
public:
    RectangularOverlay(Item *item, int xsnap = 1, int ysnap = 1);
    void removeTopMiddle();
    void removeBotMiddle();
    /**
     * Get the size rectangle from the position of the handles. If the size
     * is invalid (e.g. the parent Item does not consider it a valid size,
     * then *ok is set to false; otherwise to true.
     * @returns the sizerect, regardless of whether or not it is valid
     */
    QRect getSizeRect(bool *ok = nullptr, bool *widthOk = nullptr, bool *heightOk = nullptr) const;

    bool isValidXPos(ResizeHandle *rh) override;
    bool isValidYPos(ResizeHandle *rh) override;

public Q_SLOTS:
    void slotUpdateResizeHandles();

protected Q_SLOTS:
    void slotResizeHandleMoved(int id, double dx, double dy) override;

protected:
    ResizeHandle *m_tl;
    ResizeHandle *m_tm;
    ResizeHandle *m_tr;
    ResizeHandle *m_mr;
    ResizeHandle *m_br;
    ResizeHandle *m_bm;
    ResizeHandle *m_bl;
    ResizeHandle *m_ml;
};

/**
@author David Saxton
*/
class LineOverlay : public ResizeOverlay
{
    Q_OBJECT
public:
    LineOverlay(Item *parent);
    QPoint startPoint() const;
    QPoint endPoint() const;

public Q_SLOTS:
    void slotUpdateResizeHandles();

protected Q_SLOTS:
    void slotResizeHandleMoved(int id, double dx, double dy) override;

protected:
    ResizeHandle *m_pStart;
    ResizeHandle *m_pEnd;
};

#endif
