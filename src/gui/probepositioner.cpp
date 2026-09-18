/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "probepositioner.h"
#include "oscilloscope.h"
#include "oscilloscopedata.h"
#include "oscilloscopeview.h"

#include <QPaintEvent>
#include <QPainter>
// #include <q3pointarray.h> // 2018.08.14

#include <algorithm>
#include <cmath>

#include <ktlconfig.h>
#include <ktechlab_debug.h>

ProbePositioner::ProbePositioner(QWidget *parent)
    : QWidget(parent /* , Qt::WNoAutoErase */)
{
    m_probePosOffset = 0;
    p_draggedProbe = nullptr;
    // setBackgroundMode(Qt::NoBackground); // 2018.12.07
    setBackgroundRole(QPalette::NoRole);
    m_labelSize = probeLabelMinSize;
    m_arrowWidth = probeArrowMinSize.width();
    setLabelMaxCharacters(KTLConfig::probeLabelMaxCharacters());
    setShowLabels(KTLConfig::showProbeLabels());
    b_needRedraw = true;
    m_pixmap = nullptr;
}

ProbePositioner::~ProbePositioner()
{
    delete m_pixmap;
}

void ProbePositioner::forceRepaint()
{
    b_needRedraw = true;
    updateSize();
    repaint(/* false - 2018.12.07 */);
}

int ProbePositioner::probeOutputHeight() const
{
    int height = int(Oscilloscope::self()->oscilloscopeView->height() - m_labelSize.height());
    int numProbes = Oscilloscope::self()->numberOfProbes();
    if (numProbes == 0)
        numProbes = 1;
    return height / numProbes;
}

int ProbePositioner::probePosition(ProbeData *probeData) const
{
    if (!probeData)
        return -1;

    int spacing = probeOutputHeight();
    int probeNum = Oscilloscope::self()->probeNumber(probeData->id());

    return int(m_labelSize.height() / 2 + spacing * (probeNum + probeData->drawPosition()));
}

void ProbePositioner::setShowLabels(bool show)
{
    m_showLabels = show;
    b_needRedraw = true;
    setFixedWidth(m_showLabels ? m_arrowWidth+m_labelSize.width() : m_arrowWidth);
}

void ProbePositioner::setArrowWidth(int width)
{
    m_arrowWidth = std::max(width, probeArrowMinSize.width());
    b_needRedraw = true;
    setFixedWidth(m_showLabels ? m_arrowWidth+m_labelSize.width() : m_arrowWidth);
}

void ProbePositioner::setLabelMaxCharacters(int n)
{
    m_labelMaxCharacters = std::max(0, n);
    updateSize();
}

void ProbePositioner::updateSize()
{
    QFontMetrics fm(font());
    QSize newSize = probeLabelMinSize;
    if (m_labelMaxCharacters > 0) {
        for (auto [_, probe]: m_probeDataMap.asKeyValueRange()) {
            const auto w = fm.horizontalAdvance(probe->label(), m_labelMaxCharacters);
            newSize = newSize.expandedTo(QSize(w, fm.height()));
        }
    }
    if (newSize != m_labelSize) {
        m_labelSize = newSize;
        b_needRedraw = true;
        setFixedWidth(m_showLabels ? m_arrowWidth+m_labelSize.width() : m_arrowWidth);
    }

}

void ProbePositioner::setProbePosition(ProbeData *probeData, int position)
{
    if (!probeData)
        return;

    const auto probeArrowHeight = m_labelSize.height();
    int height = int(Oscilloscope::self()->oscilloscopeView->height() - probeArrowHeight);
    int numProbes = Oscilloscope::self()->numberOfProbes();
    int spacing = height / numProbes;
    int probeNum = Oscilloscope::self()->probeNumber(probeData->id());

    int minPos = int(probeArrowHeight / 2);
    int maxPos = int(Oscilloscope::self()->oscilloscopeView->height() - (probeArrowHeight / 2)) - 1;
    if (position < minPos)
        position = minPos;
    else if (position > maxPos)
        position = maxPos;

    probeData->setDrawPosition(float(position - probeArrowHeight / 2) / float(spacing) - probeNum);

    forceRepaint();
    Oscilloscope::self()->oscilloscopeView->updateView();
}

ProbeData *ProbePositioner::probeAtPosition(const QPoint &pos)
{
    const auto probeArrowHeight = m_labelSize.height();
    int relativeArrowHeight = int(probeArrowHeight * (1. - float(pos.x() / width())));

    const ProbeDataMap::const_iterator end = m_probeDataMap.end();
    for (ProbeDataMap::const_iterator it = m_probeDataMap.begin(); it != end; ++it) {
        ProbeData *probeData = it.value();
        int currentPos = probePosition(probeData);
        m_probePosOffset = pos.y() - currentPos;
        if (std::abs(m_probePosOffset) <= relativeArrowHeight)
            return probeData;
    }
    m_probePosOffset = 0;
    return nullptr;
}

void ProbePositioner::slotProbeDataRegistered(int id, ProbeData *probe)
{
    m_probeDataMap[id] = probe;
    connect(probe, &ProbeData::displayAttributeChanged, this, &ProbePositioner::forceRepaint);

    // This connect doesn't really belong here, but it save a lot of code
    connect(probe, &ProbeData::displayAttributeChanged, Oscilloscope::self()->oscilloscopeView, &OscilloscopeView::updateView);
    forceRepaint();
    Oscilloscope::self()->oscilloscopeView->updateView();
}

void ProbePositioner::slotProbeDataUnregistered(int id)
{
    m_probeDataMap.remove(id);

    // We "set" the position of each probe to force it into proper bounds
    const ProbeDataMap::const_iterator end = m_probeDataMap.end();
    for (ProbeDataMap::const_iterator it = m_probeDataMap.begin(); it != end; ++it)
        setProbePosition(it.value(), probePosition(it.value()));

    forceRepaint();
}

void ProbePositioner::resizeEvent(QResizeEvent *e)
{
    delete m_pixmap;
    m_pixmap = new QPixmap(e->size());
    QWidget::resizeEvent(e);
    forceRepaint();
}

void ProbePositioner::mousePressEvent(QMouseEvent *e)
{
    p_draggedProbe = probeAtPosition(e->pos());
    if (p_draggedProbe)
        e->accept();
    else
        e->ignore();
}

void ProbePositioner::mouseReleaseEvent(QMouseEvent *e)
{
    if (p_draggedProbe)
        e->accept();
    else
        e->ignore();
}

void ProbePositioner::mouseMoveEvent(QMouseEvent *e)
{
    if (!p_draggedProbe) {
        e->ignore();
        return;
    }
    e->accept();

    setProbePosition(p_draggedProbe, e->pos().y() - m_probePosOffset);
    forceRepaint();
}

void ProbePositioner::paintEvent(QPaintEvent *e)
{
    QRect r = e->rect();

    if (b_needRedraw) {
        if (!m_pixmap) {
            qCWarning(KTL_LOG) << " unexpected null m_pixmap in " << this;
            return;
        }

        QPainter p;
        // m_pixmap->fill( paletteBackgroundColor() );
        m_pixmap->fill(palette().color(backgroundRole()));
        const bool startSuccess = p.begin(m_pixmap);
        if ((!startSuccess) || (!p.isActive())) {
            qCWarning(KTL_LOG) << " painter is not active";
        }

        p.setClipRegion(e->region());
        const auto probeArrowWidth = m_arrowWidth;
        const auto probeArrowHeight = m_labelSize.height();
        const auto labelWidth = m_labelSize.width();

        const ProbeDataMap::const_iterator end = m_probeDataMap.end();
        for (ProbeDataMap::const_iterator it = m_probeDataMap.begin(); it != end; ++it) {
            ProbeData *probeData = it.value();
            p.setBrush(probeData->color());
            p.setPen(palette().color(backgroundRole()));
            int currentPos = probePosition(probeData);
            const int ymin = currentPos - (probeArrowHeight / 2);
            const int ymax = currentPos + (probeArrowHeight / 2);
            if (m_showLabels && labelWidth > 0) {
                QPolygon pa(5);
                pa[0] = QPoint(labelWidth, ymin);
                pa[1] = QPoint(labelWidth + int(probeArrowWidth), currentPos);
                pa[2] = QPoint(labelWidth, ymax);
                pa[3] = QPoint(0, ymax);
                pa[4] = QPoint(0, ymin);
                p.drawPolygon(pa);
                p.setPen(palette().color(foregroundRole()));
                QRectF rect(2, ymin, labelWidth, probeArrowHeight);
                p.drawText(rect, Qt::AlignLeft, probeData->label());
            } else {
                QPolygon pa(3);
                pa[0] = QPoint(0, ymin);
                pa[1] = QPoint(int(probeArrowWidth), currentPos);
                pa[2] = QPoint(0, ymax);
                p.drawPolygon(pa);
            }
        }
        b_needRedraw = false;
    }

    // bitBlt( this, r.x(), r.y(), m_pixmap, r.x(), r.y(), r.width(), r.height() ); // 2018.12.07
    QPainter p;
    const bool paintStarted = p.begin(this);
    if (!paintStarted) {
        qCWarning(KTL_LOG) << " failed to start painting ";
    }
    p.drawImage(r, m_pixmap->toImage(), r);
}

#include "moc_probepositioner.cpp"
