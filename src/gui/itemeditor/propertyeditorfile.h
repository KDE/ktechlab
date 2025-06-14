/***************************************************************************
 *   Copyright (C) 2003 Cedric Pasteur <cedric.pasteur@free.fr>            *
 *   Copyright (C) 2006 David Saxton <david@bluehaze.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PROPERTYEDITORFILE_H
#define PROPERTYEDITORFILE_H

#include "propertysubeditor.h"

class KLineEdit;
class QPushButton;
class QResizeEvent;
class QLabel;
class Variant;
typedef Variant Property;

class PropertyEditorFile : public PropertySubEditor
{
    Q_OBJECT

public:
    PropertyEditorFile(QWidget *parent, Property *property);
    ~PropertyEditorFile() override
    {
        ;
    }

    void resizeEvent(QResizeEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *e) override;

protected Q_SLOTS:
    void selectFile();

protected:
    KLineEdit *m_lineedit;
    QPushButton *m_button;
};

#endif
