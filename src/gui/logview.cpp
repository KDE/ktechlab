/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "logview.h"

#include <KLocalizedString>
#include <katemdi.h>

// #include <q3popupmenu.h>
#include <QEvent>
#include <QLayout>
#include <QMenu>

#include <ktechlab_debug.h>

// BEGIN class LogView
LogView::LogView(KateMDI::ToolView *parent)
    : KTextEdit(parent)
{
    if (parent->layout()) {
        parent->layout()->addWidget(this);
        qCDebug(KTL_LOG) << " added item selector to parent's layout " << parent;
    } else {
        qCWarning(KTL_LOG) << " unexpected null layout on parent " << parent;
    }

    setReadOnly(true);
    // setPaper( Qt::white ); // TODO re-enable this, get an equivalent
    // setTextFormat( Qt::LogText ); // 2018.12.07 - use toHtml() and html() methods
    // setWordWrap( WidgetWidth );
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setFocusPolicy(Qt::NoFocus);

    // Connect up signal emitted when the user doubleclicks on a paragraph in the log view
    // connect( this, SIGNAL(clicked(int,int)), this, SLOT(slotParaClicked(int,int)) );
    // ^ reimplemented by: mouseDoubleClickEvent()
}

LogView::~LogView()
{
}

void LogView::clear()
{
    m_messageInfoMap.clear();
    KTextEdit::clear();
}

void LogView::addOutput(QString text, OutputType outputType, MessageInfo messageInfo)
{
    tidyText(text);
    switch (outputType) {
    case LogView::ot_important:
        append(QLatin1StringView("<font color=\"#000000\"><b>%1</b></font>").arg(text));
        break;

    case LogView::ot_info:
        append(QLatin1StringView("<font color=\"#000000\"><i>%1</i></font>").arg(text));
        break;

    case LogView::ot_message:
        append(QLatin1StringView("<font color=\"#000000\">%1</font>").arg(text));
        break;

    case LogView::ot_warning:
        append(QLatin1StringView("<font color=\"#666666\">%1</font>").arg(text));
        break;

    case LogView::ot_error:
        append(QLatin1StringView("<font color=\"#800000\">%1</font>").arg(text));
        break;
    }

    // m_messageInfoMap[  paragraphs()-1 ] = messageInfo;
    m_messageInfoMap[document()->blockCount() - 1] = messageInfo;
}

void LogView::mouseDoubleClickEvent(QMouseEvent *e)
{
    QTextCursor curs = cursorForPosition(e->pos());
    slotParaClicked(curs.blockNumber(), curs.columnNumber());
    QTextEdit::mouseDoubleClickEvent(e); // note: is this needed?
}

void LogView::slotParaClicked(int para, int /*pos*/)
{
    // QString t = text(para);
    QString t = document()->findBlockByLineNumber(para).text();
    untidyText(t);
    Q_EMIT paraClicked(t, m_messageInfoMap[para]);
}

void LogView::tidyText(QString &t)
{
    t.replace(QLatin1StringView("&"), QLatin1StringView("&amp;"));
    t.replace(QLatin1StringView("<"), QLatin1StringView("&lt;"));
    t.replace(QLatin1StringView(">"), QLatin1StringView("&gt;"));
}

void LogView::untidyText(QString &t)
{
    t.replace(QLatin1StringView("&lt;"), QLatin1StringView("<"));
    t.replace(QLatin1StringView("&gt;"), QLatin1StringView(">"));
    t.replace(QLatin1StringView("&amp;"), QLatin1StringView("&"));
}

QMenu *LogView::createPopupMenu(const QPoint &pos)
{
    QMenu *menu = KTextEdit::createStandardContextMenu(pos);

    // menu->insertSeparator(); // 2018.12.07
    menu->addSeparator();
    // int id = menu->insertItem( i18n("Clear All"), this, SLOT(clear()) ); // 2018.12.07
    QAction *clearAllAct = menu->addAction(i18n("Clear All"), this, SLOT(clear()));
    clearAllAct->setEnabled(document()->blockCount() > 1);

    //// "an empty textedit is always considered to have one paragraph" - qt documentation
    //// although this does not always seem to be the case, so I don't know...
    ////menu->setItemEnabled( id, paragraphs() > 1 );
    // menu->setItemEnabled( id, document()->blockCount() > 1 );

    return menu;
}
// END class LogView

// BEGIN class MessageInfo
MessageInfo::MessageInfo()
{
    m_fileLine = -1;
}

MessageInfo::MessageInfo(QString fileURL, int fileLine)
{
    m_fileURL = fileURL;
    m_fileLine = fileLine;
}
// END class MessageInfo

#include "moc_logview.cpp"
