/* This file is part of the KDE project

   Copyright (C) 2009 Dario Freddi <drf@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef ACTIONWIDGET_H
#define ACTIONWIDGET_H

#include <QtGui/QWidget>
#include "../PKLAEntry.h"
#include <PolkitQt1/ActionDescription>

class QComboBox;
namespace Ui {
class ActionWidget;
}

namespace PolkitKde {

class ActionWidget : public QWidget
{
    Q_OBJECT
    public:
        explicit ActionWidget(PolkitQt1::ActionDescription *action, QWidget* parent = 0);
        virtual ~ActionWidget();

    public Q_SLOTS:
        void setAction(PolkitQt1::ActionDescription *action);
        void computeActionPolicies();

    private Q_SLOTS:
        void reloadPKLAs();

    private:
        PolkitQt1::ActionDescription::ImplicitAuthorization implFromText(const QString &text);
        void setImplicitAuthorization(PolkitQt1::ActionDescription::ImplicitAuthorization auth, QComboBox *box);

        Ui::ActionWidget *m_ui;
        PolkitQt1::ActionDescription *m_action;
        PKLAEntryList m_entries;
};

}

#endif // ACTIONWIDGET_H