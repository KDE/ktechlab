/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "oscilloscope.h"
#include "ktechlab.h"
#include "oscilloscopedata.h"
#include "oscilloscopeview.h"
#include "probe.h"
#include "probepositioner.h"
#include "simulator.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <cmath>

// #include <q3button.h>

#include <QLabel>
#include <QScrollBar>
#include <QSlider>
#include <QStyleFactory>
#include <QTimer>
#include <QToolButton>

#include <cassert>

#include <ktechlab_debug.h>

// BEGIN Oscilloscope Class
QColor probeColors[9] =
    {QColor(0x52, 0x22, 0x00), QColor(0xB5, 0x00, 0x2F), QColor(0xF9, 0xBA, 0x07), QColor(0x53, 0x93, 0x16), QColor(0x00, 0x66, 0x2F), QColor(0x00, 0x41, 0x88), QColor(0x1B, 0x2D, 0x83), QColor(0x55, 0x12, 0x7B), QColor(0x7B, 0x0C, 0x82)};

Oscilloscope *Oscilloscope::m_pSelf = nullptr;

Oscilloscope *Oscilloscope::self(KateMDI::ToolView *parent)
{
    if (!m_pSelf) {
        assert(parent);
        m_pSelf = new Oscilloscope(parent);
    }
    return m_pSelf;
}

Oscilloscope::Oscilloscope(KateMDI::ToolView *parent)
    : QWidget(parent)
{
    setupUi(this);

    if (parent->layout()) {
        parent->layout()->addWidget(this);
        qCDebug(KTL_LOG) << " added item selector to parent's layout " << parent;
    } else {
        qCWarning(KTL_LOG) << " unexpected null layout on parent " << parent;
    }

    m_nextColor = 0;
    m_nextId = 1;
    m_oldestId = -1;
    m_oldestProbe = nullptr;
    // 	b_isPaused = false;
    m_zoomLevel = 0.5;
    m_pSimulator = Simulator::self();

    horizontalScroll->setSingleStep(32);
    horizontalScroll->setPageStep(oscilloscopeView->width());

    zoomDial->setStyle(QStyleFactory::create(QLatin1StringView("Fusion")));
    zoomDial->setNotchesVisible(true);

    connect(resetBtn, &QPushButton::clicked, this, &Oscilloscope::reset);
    connect(zoomDial, &QDial::valueChanged, this, &Oscilloscope::slotZoomDialChanged);
    connect(horizontalScroll, &QScrollBar::valueChanged, this, &Oscilloscope::slotSliderValueChanged);

    // 	connect( pauseBtn, SIGNAL(clicked()), this, SLOT(slotTogglePause()));

    QTimer *updateScrollTmr = new QTimer(this);
    connect(updateScrollTmr, &QTimer::timeout, this, &Oscilloscope::updateScrollbars);
    updateScrollTmr->start(20);

    // KGlobal::config()->setGroup("Oscilloscope");
    KConfigGroup grOscill(KSharedConfig::openConfig(), QLatin1StringView("Oscilloscope"));
    setZoomLevel(grOscill.readEntry("ZoomLevel", 0.5));

    connect(this, SIGNAL(probeRegistered(int, ProbeData *)), probePositioner, SLOT(slotProbeDataRegistered(int, ProbeData *)));
    /*TODO  fix error: 'slotProbeDataRegistered(int, ProbeData*)’ is protected within this context
    connect(this, &Oscilloscope::probeRegistered,
            probePositioner, &ProbePositioner::slotProbeDataRegistered);*/
    connect(this, SIGNAL(probeUnregistered(int)), probePositioner, SLOT(slotProbeDataUnregistered(int)));
    /*TODO the same problem
    connect(this, &Oscilloscope::probeUnregistered,
            probePositioner, &ProbePositioner::slotProbeDataUnregistered);*/
}

Oscilloscope::~Oscilloscope()
{
    m_pSelf = nullptr;
}

bool Oscilloscope::isInstantiated()
{
    return m_pSelf != nullptr;
}

void Oscilloscope::slotTogglePause()
{
    // 	b_isPaused = !b_isPaused;
    // 	pauseBtn->setText( b_isPaused ? i18n("Resume") : i18n("Pause"));
    // 	const ProbeDataMap::iterator end = m_probeDataMap.end();
    // 	for( ProbeDataMap::iterator it = m_probeDataMap.begin(); it != end; ++it)
    // 		(*it)->setPaused(b_isPaused);
}

int Oscilloscope::sliderTicksPerSecond() const
{
    return int(1e4);
}

void Oscilloscope::setZoomLevel(double zoomLevel)
{
    if (zoomLevel < 0.0)
        zoomLevel = 0.0;

    else if (zoomLevel > 1.0)
        zoomLevel = 1.0;

    // KGlobal::config()->setGroup("Oscilloscope");
    KConfigGroup grOscill = KSharedConfig::openConfig()->group(QLatin1StringView("Oscilloscope"));
    // KGlobal::config()->writeEntry( "ZoomLevel", zoomLevel);
    grOscill.writeEntry("ZoomLevel", zoomLevel);

    // We want to maintain the position of the *center* of the view, not the
    // left edge, so have to record time at center of view... We also have to
    // handle the case where the scroll is at the end separately.
    bool wasAtUpperEnd = horizontalScroll->maximum() == horizontalScroll->value();
    int pageLength = int(oscilloscopeView->width() * sliderTicksPerSecond() / pixelsPerSecond());
    int at_ticks = horizontalScroll->value() + (pageLength / 2);

    m_zoomLevel = zoomLevel;
    zoomDial->setValue(int((double(zoomDial->maximum()) * zoomLevel) + 0.5));
    updateScrollbars();

    // And restore the center position of the slider
    if (!wasAtUpperEnd) {
        int pageLength = int(oscilloscopeView->width() * sliderTicksPerSecond() / pixelsPerSecond());
        horizontalScroll->setValue(at_ticks - (pageLength / 2));
        oscilloscopeView->updateView();
    }
}

void Oscilloscope::slotZoomDialChanged(int value)
{
    setZoomLevel(double(value) / double(zoomDial->maximum()));
}

ProbeData *Oscilloscope::registerProbe(Probe *probe)
{
    if (!probe)
        return nullptr;

    const uint id = m_nextId++;

    ProbeData *probeData = nullptr;

    if (dynamic_cast<LogicProbe *>(probe)) {
        probeData = new LogicProbeData(id);
        m_logicProbeDataMap[id] = static_cast<LogicProbeData *>(probeData);
    } else {
        probeData = new FloatingProbeData(id);
        m_floatingProbeDataMap[id] = static_cast<FloatingProbeData *>(probeData);
    }

    m_probeDataMap[id] = probeData;

    if (!m_oldestProbe) {
        m_oldestProbe = probeData;
        m_oldestId = id;
    }

    probeData->setColor(probeColors[m_nextColor]);
    m_nextColor = (m_nextColor + 1) % 9;
    //	probeData->setPaused(b_isPaused);

    Q_EMIT probeRegistered(id, probeData);
    return probeData;
}

void Oscilloscope::unregisterProbe(int id)
{
    ProbeDataMap::iterator it = m_probeDataMap.find(id);

    if (it == m_probeDataMap.end())
        return;

    m_logicProbeDataMap.remove(id);
    m_floatingProbeDataMap.remove(id);

    bool oldestDestroyed = it.value() == m_oldestProbe;

    if (it != m_probeDataMap.end())
        m_probeDataMap.erase(it);

    if (oldestDestroyed)
        getOldestProbe();

    Q_EMIT probeUnregistered(id);
}

ProbeData *Oscilloscope::probeData(int id) const
{
    const ProbeDataMap::const_iterator bit = m_probeDataMap.find(id);
    if (bit != m_probeDataMap.end())
        return bit.value();

    return nullptr;
}

int Oscilloscope::probeNumber(int id) const
{
    const ProbeDataMap::const_iterator end = m_probeDataMap.end();
    int i = 0;
    for (ProbeDataMap::const_iterator it = m_probeDataMap.begin(); it != end; ++it) {
        if (it.key() == id)
            return i;
        i++;
    }
    return -1;
}

int Oscilloscope::numberOfProbes() const
{
    return m_probeDataMap.size();
}

void Oscilloscope::getOldestProbe()
{
    if (m_probeDataMap.isEmpty()) {
        m_oldestProbe = nullptr;
        m_oldestId = -1;
        return;
    }

    m_oldestProbe = m_probeDataMap.begin().value();
    m_oldestId = m_probeDataMap.begin().key();
}

void Oscilloscope::reset()
{
    const ProbeDataMap::iterator end = m_probeDataMap.end();
    for (ProbeDataMap::iterator it = m_probeDataMap.begin(); it != end; ++it)
        (*it)->eraseData();

    oscilloscopeView->updateView();
}

void Oscilloscope::slotSliderValueChanged(int value)
{
    Q_UNUSED(value);
    oscilloscopeView->updateView();
}

void Oscilloscope::updateScrollbars()
{
    bool wasAtUpperEnd = horizontalScroll->maximum() == horizontalScroll->value();

    const float pps = pixelsPerSecond();

    int pageLength = int(oscilloscopeView->width() * sliderTicksPerSecond() / pps);
    int64_t timeAsTicks = time() * sliderTicksPerSecond() / LOGIC_UPDATE_RATE;
    int64_t upper = (timeAsTicks > pageLength) ? (timeAsTicks - pageLength) : 0;
    horizontalScroll->setRange(0, upper);

    horizontalScroll->setPageStep(uint64_t(oscilloscopeView->width() * sliderTicksPerSecond() / pps));

    if (wasAtUpperEnd) {
        horizontalScroll->setValue(horizontalScroll->maximum());
        oscilloscopeView->updateView();
    }
}

uint64_t Oscilloscope::time() const
{
    if (!m_oldestProbe)
        return 0;

    return uint64_t(m_pSimulator->time() - m_oldestProbe->resetTime());
}

int64_t Oscilloscope::scrollTime() const
{
    // 	if( b_isPaused || numberOfProbes() == 0)
    // 		return 0;

    if (numberOfProbes() == 0)
        return 0;

    if (horizontalScroll->maximum() == 0) {
        int64_t lengthAsTime = int64_t(oscilloscopeView->width() * LOGIC_UPDATE_RATE / pixelsPerSecond());
        int64_t ret = m_pSimulator->time() - lengthAsTime;
        if (ret < 0)
            return 0;
        return ret;
    } else
        return int64_t(m_oldestProbe->resetTime() + (int64_t(horizontalScroll->value()) * LOGIC_UPDATE_RATE / sliderTicksPerSecond()));
}

double Oscilloscope::pixelsPerSecond() const
{
    return 2 * MIN_BITS_PER_S * std::pow(2.0, m_zoomLevel * MIN_MAX_LOG_2_DIFF);
}
// END Oscilloscope Class

void addOscilloscopeAsToolView(KTechlab *ktechlab)
{
    KateMDI::ToolView *tv;
    tv = ktechlab->createToolView(Oscilloscope::toolViewIdentifier(), KMultiTabBar::Bottom,
                                  QIcon::fromTheme(QLatin1StringView("oscilloscope")), i18n("Oscilloscope"));

    Oscilloscope::self(tv);
}

ProbeData *registerProbe(Probe *probe)
{
    if (!Oscilloscope::isInstantiated()) {
        qCDebug(KTL_LOG) << "no oscilloscope to register to, doing nothing";
        return nullptr;
    }
    return Oscilloscope::self()->registerProbe(probe);
}

void unregisterProbe(int id)
{
    if (!Oscilloscope::isInstantiated()) {
        qCDebug(KTL_LOG) << "no oscilloscope to unregister from, doing nothing";
        return;
    }
    Oscilloscope::self()->unregisterProbe(id);
}

#include "moc_oscilloscope.cpp"
