/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt3Support module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef KTL_Q3POLYGONSCANNER_H
#define KTL_Q3POLYGONSCANNER_H

#include <QtCore/qglobal.h>

// QT_BEGIN_HEADER

// QT_BEGIN_NAMESPACE

// QT_MODULE(Qt3SupportLight)

class QPolygon;
class QPoint;

class /* Q_COMPAT_EXPORT */ KtlQ3PolygonScanner
{
public:
    virtual ~KtlQ3PolygonScanner()
    {
    }
    void scan(const QPolygon &pa, bool winding, int index = 0, int npoints = -1);
    void scan(const QPolygon &pa, bool winding, int index, int npoints, bool stitchable);
    enum Edge { Left = 1, Right = 2, Top = 4, Bottom = 8 };
    void scan(const QPolygon &pa, bool winding, int index, int npoints, KtlQ3PolygonScanner::Edge edges);
    virtual void processSpans(int n, QPoint *point, int *width) = 0;
};

QT_END_NAMESPACE

// QT_END_HEADER

#endif // KTL_Q3POLYGONSCANNER_H
