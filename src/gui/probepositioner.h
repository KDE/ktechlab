/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PROBEPOSITIONER_H
#define PROBEPOSITIONER_H

#include <QMap>
#include <QWidget>

class ProbeData;
typedef QMap<int, ProbeData *> ProbeDataMap;

const QSize probeArrowMinSize = QSize(9, 12);
const QSize probeLabelMinSize = QSize(10, probeArrowMinSize.height());

/**
Widget for positioning the output of Probes in the OscilloscopeView
@author David Saxton
*/
class ProbePositioner : public QWidget
{
    Q_OBJECT
public:
    ProbePositioner(QWidget *parent = nullptr);
    ~ProbePositioner() override;
    /**
     * Returns the amount of space (height in pixels) that a probe output
     * takes up
     */
    int probeOutputHeight() const;
    /**
     * Returns the probe position (from the top) in pixels that the probe
     * with the given id should be displayed at, or -1 if probe with the
     * given id couldn't be found
     */
    int probePosition(ProbeData *probeData) const;
    /**
     * Sets the probe position relative to the top of this widget (and hence
     * relative to the top of the oscilloscope view) in pixels
     */
    void setProbePosition(ProbeData *probeData, int position);
    /**
     * Returns the probe at the given position (plus or minus an arrow),
     * or nullptr if none. Records the offset of the position from the mouse
     * in m_probePosOffset.
     */
    ProbeData *probeAtPosition(const QPoint &pos);

    /**
     * Gets the size of the arrow
     */
    void setArrowWidth(int width);

    /**
     * Get the size of the arrow
     */
    QSize arrowSize() const {
        return QSize(m_arrowWidth, m_labelSize.height());
    }

    /**
     * Sets the maximum number of characters shown
     */
    void setLabelMaxCharacters(int n);

    /**
     * Get the maximum number of label characters shown
     */
    int labelMaxCharacters() {
        return m_labelMaxCharacters;
    }

    /**
     * Returns the size of the label text area
     */
    const QSize& labelSize() const {
        return m_labelSize;
    }

    /**
     * Set whether labels should be shown
     */
    void setShowLabels(bool show);

    /**
     * Returns whether labels are shown
     */
    bool showLabels() const {
        return m_showLabels;
    }

public Q_SLOTS:
    void forceRepaint();

protected Q_SLOTS:
    void slotProbeDataRegistered(int id, ProbeData *probe);
    void slotProbeDataUnregistered(int id);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;
    void updateSize();

    ProbeDataMap m_probeDataMap;
    ProbeData *p_draggedProbe;
    int m_probePosOffset;
    int m_arrowWidth;
    int m_labelMaxCharacters;
    QSize m_labelSize;
    bool m_showLabels;
    bool b_needRedraw;
    QPixmap *m_pixmap;
};

#endif
