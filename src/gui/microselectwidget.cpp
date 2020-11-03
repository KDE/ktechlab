/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "microselectwidget.h"
#include "asminfo.h"
#include "microinfo.h"
#include "microlibrary.h"

#include <KComboBox>
#include <KLocalizedString>

// #include <q3groupbox.h>
#include <QLabel>
#include <QLayout>
#include <QVariant>

MicroSelectWidget::MicroSelectWidget(QWidget *parent, const char *name, Qt::WFlags)
    //: Q3GroupBox( 4, Qt::Horizontal, i18n("Microprocessor"), parent, name )
    : QGroupBox(parent /*, name */)
{
    setObjectName(name);
    setLayout(new QHBoxLayout);
    setTitle(i18n("Microprocessor"));

    m_allowedAsmSet = AsmInfo::AsmSetAll;
    m_allowedGpsimSupport = m_allowedFlowCodeSupport = m_allowedMicrobeSupport = MicroInfo::AllSupport;

    if (!name) {
        setObjectName("MicroSelectWidget");
    }
    m_pMicroFamilyLabel = new QLabel(nullptr);
    m_pMicroFamilyLabel->setObjectName("m_pMicroFamilyLabel");
    m_pMicroFamilyLabel->setText(i18n("Family"));
    layout()->addWidget(m_pMicroFamilyLabel);

    m_pMicroFamily = new KComboBox(false); //, "m_pMicroFamily" );
    m_pMicroFamily->setObjectName("m_pMicroFamily");
    m_pMicroFamily->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    layout()->addWidget(m_pMicroFamily);

    m_pMicroLabel = new QLabel(nullptr /*, "m_pMicroLabel" */);
    m_pMicroLabel->setObjectName("m_pMicroLabel");
    m_pMicroLabel->setText(i18n("Micro"));
    layout()->addWidget(m_pMicroLabel);

    m_pMicro = new KComboBox(false); //, "m_pMicro" );
    m_pMicro->setObjectName("m_pMicro");
    m_pMicro->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_pMicro->setEditable(true);
    m_pMicro->setAutoCompletion(true);
    layout()->addWidget(m_pMicro);

    updateFromAllowed();
    setMicro("P16F84");
    connect(m_pMicroFamily, qOverload<const QString&>(&KComboBox::activated),
            this, &MicroSelectWidget::microFamilyChanged);
}

MicroSelectWidget::~MicroSelectWidget()
{
}

void MicroSelectWidget::setAllowedAsmSet(unsigned allowed)
{
    m_allowedAsmSet = allowed;
    updateFromAllowed();
}
void MicroSelectWidget::setAllowedGpsimSupport(unsigned allowed)
{
    m_allowedGpsimSupport = allowed;
    updateFromAllowed();
}
void MicroSelectWidget::setAllowedFlowCodeSupport(unsigned allowed)
{
    m_allowedFlowCodeSupport = allowed;
    updateFromAllowed();
}
void MicroSelectWidget::setAllowedMicrobeSupport(unsigned allowed)
{
    m_allowedMicrobeSupport = allowed;
    updateFromAllowed();
}

void MicroSelectWidget::updateFromAllowed()
{
    QString oldFamily = m_pMicroFamily->currentText();

    m_pMicroFamily->clear();

#define CHECK_ADD(family)                                                                                                                                                                                                                      \
    if ((m_allowedAsmSet & AsmInfo::family) && !MicroLibrary::self()->microIDs(AsmInfo::family, m_allowedGpsimSupport, m_allowedFlowCodeSupport, m_allowedMicrobeSupport).isEmpty()) {                                                         \
        m_pMicroFamily->insertItem(m_pMicroFamily->count(), AsmInfo::setToString(AsmInfo::family));                                                                                                                                            \
    }
    CHECK_ADD(PIC12)
    CHECK_ADD(PIC14)
    CHECK_ADD(PIC16);
#undef CHECK_ADD

    if (m_pMicroFamily->contains(oldFamily)) {
        // m_pMicroFamily->setCurrentText(oldFamily); // 2018.12.07
        {
            QComboBox *c = m_pMicroFamily;
            QString text = oldFamily;
            int i = c->findText(text);
            if (i != -1)
                c->setCurrentIndex(i);
            else if (c->isEditable())
                c->setEditText(text);
            else
                c->setItemText(c->currentIndex(), text);
        }
    }

    microFamilyChanged(oldFamily);
}

void MicroSelectWidget::setMicro(const QString &id)
{
    MicroInfo *info = MicroLibrary::self()->microInfoWithID(id);
    if (!info)
        return;

    m_pMicro->clear();
    m_pMicro->insertItems(m_pMicro->count(), MicroLibrary::self()->microIDs(info->instructionSet()->set()));
    // m_pMicro->setCurrentText(id); // 2018.12.07
    {
        QComboBox *c = m_pMicro;
        QString text = id;
        int i = c->findText(text);
        if (i != -1)
            c->setCurrentIndex(i);
        else if (c->isEditable())
            c->setEditText(text);
        else
            c->setItemText(c->currentIndex(), text);
    }

    // m_pMicroFamily->setCurrentText( AsmInfo::setToString( info->instructionSet()->set() ) ); // 2018.12.07
    {
        QComboBox *c = m_pMicroFamily;
        QString text = AsmInfo::setToString(info->instructionSet()->set());
        int i = c->findText(text);
        if (i != -1)
            c->setCurrentIndex(i);
        else if (c->isEditable())
            c->setEditText(text);
        else
            c->setItemText(c->currentIndex(), text);
    }
}

QString MicroSelectWidget::micro() const
{
    return m_pMicro->currentText();
}

void MicroSelectWidget::microFamilyChanged(const QString &family)
{
    QString oldID = m_pMicro->currentText();

    m_pMicro->clear();
    m_pMicro->insertItems(m_pMicro->count(), MicroLibrary::self()->microIDs(AsmInfo::stringToSet(family), m_allowedGpsimSupport, m_allowedFlowCodeSupport, m_allowedMicrobeSupport));

    if (m_pMicro->contains(oldID)) {
        // m_pMicro->setCurrentText(oldID); // 2018.12.07
        {
            int i = m_pMicro->findText(oldID);
            if (i != -1)
                m_pMicro->setCurrentIndex(i);
            else if (m_pMicro->isEditable())
                m_pMicro->setEditText(oldID);
            else
                m_pMicro->setItemText(m_pMicro->currentIndex(), oldID);
        }
    }
}
