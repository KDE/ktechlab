/***************************************************************************
 *   Copyright (C) 2005-2006 David Saxton <david@bluehaze.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "viewcontainer.h"
#include "docmanager.h"
#include "document.h"
#include "itemview.h"
#include "ktechlab.h"
#include "view.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
//#include <qobjectlist.h>

#include <ktechlab_debug.h>

// BEGIN class ViewContainer
ViewContainer::ViewContainer(const QString &caption, QWidget *parent)
    : QWidget(parent ? parent : KTechlab::self()->tabWidget())
{
    b_deleted = false;
    connect(KTechlab::self(), &KTechlab::needUpdateCaptions, this, &ViewContainer::updateCaption);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    m_baseViewArea = new ViewArea(this, this, 0, false);
    m_baseViewArea->setObjectName("viewarea_0");
    connect(m_baseViewArea, &ViewArea::destroyed, this, &ViewContainer::baseViewAreaDestroyed);

    layout->addWidget(m_baseViewArea);

    m_activeViewArea = 0;
    setFocusProxy(m_baseViewArea);

    if (!parent) {
        KTechlab::self()->tabWidget()->addTab(this, caption);
        QTabWidget *tabWidget = KTechlab::self()->tabWidget();
        tabWidget->setCurrentIndex(tabWidget->indexOf(this));
    }

    show();
}

ViewContainer::~ViewContainer()
{
    disconnect(m_baseViewArea, &ViewArea::destroyed, this, &ViewContainer::baseViewAreaDestroyed);

    b_deleted = true;
}

void ViewContainer::setActiveViewArea(uint id)
{
    if (m_activeViewArea == int(id))
        return;

    m_activeViewArea = id;
    View *newView = view(id);
    setFocusProxy(newView);

    if (newView) {
        setWindowTitle(newView->windowTitle());

        if (!DocManager::self()->getFocusedView() && newView->isVisible())
            newView->setFocus();
    }
}

View *ViewContainer::view(uint id) const
{
    ViewArea *va = viewArea(id);
    if (!va)
        return nullptr;

    // We do not want a recursive search as ViewAreas also hold other ViewAreas
    // QObjectList l = va->queryList( "View", 0, false, false ); // 2018.12.02
    QList<View *> l = va->findChildren<View *>();
    View *view = nullptr;
    if (!l.isEmpty())
        view = dynamic_cast<View *>(l.first());
    // delete l;

    return view;
}

ViewArea *ViewContainer::viewArea(uint id) const
{
    if (!m_viewAreaMap.contains(id))
        return nullptr;

    return m_viewAreaMap[id];
}

bool ViewContainer::closeViewContainer()
{
    bool didClose = true;
    while (didClose && !m_viewAreaMap.isEmpty()) {
        didClose = closeViewArea(m_viewAreaMap.begin().key());
    }

    return m_viewAreaMap.isEmpty();
}

bool ViewContainer::closeViewArea(uint id)
{
    ViewArea *va = viewArea(id);
    if (!va)
        return true;

    bool doClose = false;
    View *v = view(id);
    if (v && v->document()) {
        doClose = v->document()->numberOfViews() > 1;
        if (!doClose)
            doClose = v->document()->fileClose();
    } else
        doClose = true;

    if (!doClose)
        return false;

    m_viewAreaMap.remove(id);
    va->deleteLater();

    if (m_activeViewArea == int(id)) {
        m_activeViewArea = -1;
        findActiveViewArea();
    }

    return true;
}

int ViewContainer::createViewArea(int relativeViewArea, ViewArea::Position position, bool showOpenButton)
{
    if (relativeViewArea == -1)
        relativeViewArea = activeViewArea();

    ViewArea *relative = viewArea(relativeViewArea);
    if (!relative) {
        qCCritical(KTL_LOG) << "Could not find relative view area";
        return -1;
    }

    uint id = uniqueNewId();
    // 	setActiveViewArea(id);

    ViewArea *viewArea = relative->createViewArea(position, id, showOpenButton);
    // 	ViewArea *viewArea = new ViewArea( m_splitter, id, (const char*)("viewarea_"+QString::number(id)) );
    viewArea->show(); // remove?

    return id;
}

void ViewContainer::setViewAreaId(ViewArea *viewArea, uint id)
{
    m_viewAreaMap[id] = viewArea;
    m_usedIDs.append(id);
}

void ViewContainer::setViewAreaRemoved(uint id)
{
    if (b_deleted)
        return;

    ViewAreaMap::iterator it = m_viewAreaMap.find(id);
    if (it == m_viewAreaMap.end())
        return;

    m_viewAreaMap.erase(it);

    if (m_activeViewArea == int(id))
        findActiveViewArea();
}

void ViewContainer::findActiveViewArea()
{
    if (m_viewAreaMap.isEmpty())
        return;

    setActiveViewArea((--m_viewAreaMap.end()).key());
}

void ViewContainer::baseViewAreaDestroyed(QObject *obj)
{
    if (!obj)
        return;

    if (!b_deleted) {
        b_deleted = true;
        close();
        deleteLater();
    }
}

bool ViewContainer::canSaveUsefulStateInfo() const
{
    return m_baseViewArea && m_baseViewArea->canSaveUsefulStateInfo();
}

void ViewContainer::saveState(KConfigGroup *config)
{
    if (!m_baseViewArea)
        return;

    config->writeEntry("BaseViewArea", m_baseViewArea->id());
    m_baseViewArea->saveState(config);
}

void ViewContainer::restoreState(KConfigGroup *config, const QString &groupName)
{
    // config->setGroup(groupName);
    int baseAreaId = config->readEntry("BaseViewArea", 0);
    m_baseViewArea->restoreState(config, baseAreaId, groupName);
}

int ViewContainer::uniqueParentId()
{
    int lowest = -1;
    const IntList::iterator end = m_usedIDs.end();
    for (IntList::iterator it = m_usedIDs.begin(); it != end; ++it) {
        if (*it < lowest)
            lowest = *it;
    }
    int newId = lowest - 1;
    m_usedIDs.append(newId);
    return newId;
}

int ViewContainer::uniqueNewId()
{
    int highest = 0;
    const IntList::iterator end = m_usedIDs.end();
    for (IntList::iterator it = m_usedIDs.begin(); it != end; ++it) {
        if (*it > highest)
            highest = *it;
    }
    int newId = highest + 1;
    m_usedIDs.append(newId);
    return newId;
}

void ViewContainer::setIdUsed(int id)
{
    m_usedIDs.append(id);
}

void ViewContainer::updateCaption()
{
    QString caption;

    if (!activeView() || !activeView()->document())
        caption = i18n("(empty)");

    else {
        Document *doc = activeView()->document();
        caption = doc->url().isEmpty() ? doc->caption() : doc->url().fileName();
        if (viewCount() > 1)
            caption += " ...";
    }

    setWindowTitle(caption);
    // KTechlab::self()->tabWidget()->setTabLabel( this, caption ); // 2018.12.02
    KTechlab::self()->tabWidget()->setTabText(KTechlab::self()->tabWidget()->indexOf(this), caption);
}
// END class ViewContainer

// BEGIN class ViewArea
ViewArea::ViewArea(QWidget *parent, ViewContainer *viewContainer, int id, bool showOpenButton)
    : QSplitter(parent)
{
    p_viewContainer = viewContainer;
    m_id = id;
    p_view = nullptr;
    p_viewArea1 = nullptr;
    p_viewArea2 = nullptr;

    if (id >= 0)
        p_viewContainer->setViewAreaId(this, uint(id));

    p_viewContainer->setIdUsed(id);

    m_pEmptyViewArea = nullptr;
    if (showOpenButton)
        m_pEmptyViewArea = new EmptyViewArea(this);
}

ViewArea::~ViewArea()
{
    if (m_id >= 0)
        p_viewContainer->setViewAreaRemoved(uint(m_id));

    if (p_viewArea1) {
        disconnect(p_viewArea1, &ViewArea::destroyed, this, &ViewArea::viewAreaDestroyed);
    }
    if (p_viewArea2) {
        disconnect(p_viewArea2, &ViewArea::destroyed, this, &ViewArea::viewAreaDestroyed);
    }
    if (p_view) {
        View *view = p_view.data();
        disconnect(view, &View::destroyed, this, &ViewArea::viewDestroyed);
    }
}

ViewArea *ViewArea::createViewArea(Position position, uint id, bool showOpenButton)
{
    if (p_viewArea1 || p_viewArea2) {
        qCCritical(KTL_LOG) << "Attempting to create ViewArea when already containing ViewAreas!";
        return nullptr;
    }
    if (!p_view) {
        qCCritical(KTL_LOG) << "We don't have a view yet, so creating a new ViewArea is redundant";
        return nullptr;
    }

    setOrientation((position == Right) ? Qt::Horizontal : Qt::Vertical);

    p_viewArea1 = new ViewArea(this, p_viewContainer, m_id, false);
    p_viewArea1->setObjectName("viewarea_" + QString::number(m_id));
    p_viewArea2 = new ViewArea(this, p_viewContainer, id, showOpenButton);
    p_viewArea2->setObjectName("viewarea_" + QString::number(id));

    connect(p_viewArea1, &ViewArea::destroyed, this, &ViewArea::viewAreaDestroyed);
    connect(p_viewArea2, &ViewArea::destroyed, this, &ViewArea::viewAreaDestroyed);

    p_view->clearFocus();
    // p_view->reparent( p_viewArea1, QPoint(), true ); // 2018.12.02
    p_view->setParent(p_viewArea1);
    p_view->move(QPoint());
    p_view->show();
    p_viewArea1->setView(p_view);
    setView(nullptr);

    m_id = p_viewContainer->uniqueParentId();

    QList<int> splitPos;
    int pos = ((orientation() == Qt::Horizontal) ? width() / 2 : height() / 2);
    splitPos << pos << pos;
    setSizes(splitPos);

    p_viewArea1->show();
    p_viewArea2->show();
    return p_viewArea2;
}

void ViewArea::viewAreaDestroyed(QObject *obj)
{
    ViewArea *viewArea = static_cast<ViewArea *>(obj);

    if (viewArea == p_viewArea1)
        p_viewArea1 = nullptr;

    if (viewArea == p_viewArea2)
        p_viewArea2 = nullptr;

    if (!p_viewArea1 && !p_viewArea2)
        deleteLater();
}

void ViewArea::setView(View *view)
{
    if (!view) {
        p_view = nullptr;
        setFocusProxy(nullptr);
        return;
    }

    delete m_pEmptyViewArea;

    if (p_view) {
        qCCritical(KTL_LOG) << "Attempting to set already contained view!";
        return;
    }

    p_view = view;

    // 	qCDebug(KTL_LOG) << "p_view->isFocusEnabled()="<<p_view->isFocusEnabled()<<" p_view->isHidden()="<<p_view->isHidden();

    connect(view, &View::destroyed, this, &ViewArea::viewDestroyed);
    bool hadFocus = hasFocus();
    setFocusProxy(p_view);
    if (hadFocus && !p_view->isHidden())
        p_view->setFocus();

    // The ViewContainer by default has a view area as its focus proxy.
    // This is because there is no view when it is constructed. So give
    // it our view as the focus proxy if it doesn't have one.
    if (!dynamic_cast<View *>(p_viewContainer->focusProxy()))
        p_viewContainer->setFocusProxy(p_view);
}

void ViewArea::viewDestroyed()
{
    if (!p_view && !p_viewArea1 && !p_viewArea2)
        deleteLater();
}

bool ViewArea::canSaveUsefulStateInfo() const
{
    if (p_viewArea1 && p_viewArea1->canSaveUsefulStateInfo())
        return true;

    if (p_viewArea2 && p_viewArea2->canSaveUsefulStateInfo())
        return true;

    if (p_view && p_view->document() && !p_view->document()->url().isEmpty())
        return true;

    return false;
}

void ViewArea::saveState(KConfigGroup *config)
{
    bool va1Ok = p_viewArea1 && p_viewArea1->canSaveUsefulStateInfo();
    bool va2Ok = p_viewArea2 && p_viewArea2->canSaveUsefulStateInfo();

    if (va1Ok || va2Ok) {
        config->writeEntry(orientationKey(m_id), (orientation() == Qt::Horizontal) ? "LeftRight" : "TopBottom");

        QList<int> contains;
        if (va1Ok)
            contains << p_viewArea1->id();
        if (va2Ok)
            contains << p_viewArea2->id();

        config->writeEntry(containsKey(m_id), contains);
        if (va1Ok)
            p_viewArea1->saveState(config);
        if (va2Ok)
            p_viewArea2->saveState(config);
    } else if (p_view && !p_view->document()->url().isEmpty()) {
        config->writePathEntry(fileKey(m_id), p_view->document()->url().toDisplayString(QUrl::PreferLocalFile));
    }
}

void ViewArea::restoreState(KConfigGroup *config, int id, const QString &groupName)
{
    if (!config)
        return;

    if (id != m_id) {
        if (m_id >= 0)
            p_viewContainer->setViewAreaRemoved(uint(m_id));

        m_id = id;

        if (m_id >= 0)
            p_viewContainer->setViewAreaId(this, uint(m_id));

        p_viewContainer->setIdUsed(id);
    }

    // config->setGroup(groupName);
    if (config->hasKey(orientationKey(id))) {
        QString orientation = config->readEntry(orientationKey(m_id));
        setOrientation((orientation == "LeftRight") ? Qt::Horizontal : Qt::Vertical);
    }

    // config->setGroup(groupName);
    if (config->hasKey(containsKey(m_id))) {
        typedef QList<int> IntList;
        IntList contains = config->readEntry(containsKey(m_id), IntList());

        if (contains.isEmpty() || contains.size() > 2)
            qCCritical(KTL_LOG) << "Contained list has wrong size of " << contains.size();

        else {
            if (contains.size() >= 1) {
                int viewArea1Id = contains[0];
                p_viewArea1 = new ViewArea(this, p_viewContainer, viewArea1Id, false);
                p_viewArea1->setObjectName("viewarea_" + QString::number(viewArea1Id));
                connect(p_viewArea1, &ViewArea::destroyed, this, &ViewArea::viewAreaDestroyed);
                p_viewArea1->restoreState(config, viewArea1Id, groupName);
                p_viewArea1->show();
            }

            if (contains.size() >= 2) {
                int viewArea2Id = contains[1];
                p_viewArea2 = new ViewArea(this, p_viewContainer, viewArea2Id, false);
                p_viewArea2->setObjectName("viewarea_" + QString::number(viewArea2Id));
                connect(p_viewArea2, &ViewArea::destroyed, this, &ViewArea::viewAreaDestroyed);
                p_viewArea2->restoreState(config, viewArea2Id, groupName);
                p_viewArea2->show();
            }
        }
    }

    // config->setGroup(groupName);
    if (config->hasKey(fileKey(m_id))) {
        const QUrl url = QUrl::fromUserInput(config->readPathEntry(fileKey(m_id), QString()));
        bool openedOk = DocManager::self()->openURL(url, this);
        if (!openedOk)
            deleteLater();
    }
}

QString ViewArea::fileKey(int id)
{
    return QString("ViewArea ") + QString::number(id) + QString(" file");
}
QString ViewArea::containsKey(int id)
{
    return QString("ViewArea ") + QString::number(id) + QString(" contains");
}
QString ViewArea::orientationKey(int id)
{
    return QString("ViewArea ") + QString::number(id) + QString(" orientation");
}
// END class ViewArea

// BEGIN class EmptyViewArea
EmptyViewArea::EmptyViewArea(ViewArea *parent)
    : QWidget(parent)
{
    m_pViewArea = parent;

    QGridLayout *layout = new QGridLayout(this /*, 5, 3, 0, 6 */);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(6);

    layout->setRowStretch(0, 20);
    layout->setRowStretch(2, 1);
    layout->setRowStretch(4, 20);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(2, 1);

    QPushButton *newDocButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open Document"), this);
    layout->addWidget(newDocButton, 1, 1);
    connect(newDocButton, &QPushButton::clicked, this, &EmptyViewArea::openDocument);

    QPushButton *cancelButton = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-cancel")), i18n("Cancel"), this);
    layout->addWidget(cancelButton, 3, 1);
    connect(cancelButton, &QPushButton::clicked, m_pViewArea, &EmptyViewArea::deleteLater);
}

EmptyViewArea::~EmptyViewArea()
{
}

void EmptyViewArea::openDocument()
{
    KTechlab::self()->openFile(m_pViewArea);
}
// END class EmptyViewArea

#include "moc_viewcontainer.cpp"
