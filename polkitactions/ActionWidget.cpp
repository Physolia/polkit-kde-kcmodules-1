/* This file is part of the KDE project

   Copyright (C) 2009-2010 Dario Freddi <drf@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#include "ActionWidget.h"

#include "ui_actionwidget.h"

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <qdbuspendingcall.h>
#include <QtDBus/qdbusmetatype.h>
#include <PolkitQt1/ActionDescription>
#include <KDebug>
#include "pkitemdelegate.h"
#include "explicitauthorizationdialog.h"
#include <QSettings>

namespace PolkitKde {

ActionWidget::ActionWidget(PolkitQt1::ActionDescription* action, QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::ActionWidget)
        , m_action(0)
{
    m_ui->setupUi(this);

    // Initialize
    reloadPKLAs();

    setAction(action);
    m_ui->localAuthListWidget->setItemDelegate(new PKLAItemDelegate);
    connect(m_ui->localAuthListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(editExplicitPKLAEntry(QListWidgetItem*)));
    connect(m_ui->addLocalButton, SIGNAL(clicked(bool)),
            this, SLOT(addExplicitPKLAEntry()));
}

ActionWidget::~ActionWidget()
{

}

void ActionWidget::reloadPKLAs()
{
    m_entries.clear();
    QDBusMessage message = QDBusMessage::createMethodCall("org.kde.polkitkde1.helper",
                                                          "/Helper",
                                                          "org.kde.polkitkde1.helper",
                                                          QLatin1String("retrievePolicies"));
    QDBusPendingCall reply = QDBusConnection::systemBus().asyncCall(message);
    reply.waitForFinished();
    if (reply.reply().arguments().count() >= 1) {
        QVariantList vlist;
        reply.reply().arguments().first().value<QDBusArgument>() >> vlist;
        foreach (const QVariant &variant, vlist) {
            PKLAEntry entry;
            variant.value<QDBusArgument>() >> entry;
            qDebug() << entry.title;
            m_entries.append(entry);
        }
    }

    if (m_action) {
        computeActionPolicies();
    }
}

void ActionWidget::computeActionPolicies()
{
    kDebug();
    m_ui->localAuthListWidget->clear();
    foreach (const PKLAEntry &entry, m_entries) {
        QStringList realActions = entry.action.split(';');
        kDebug() << entry.action << m_action->actionId();
        if (realActions.contains(m_action->actionId())) {
            // Match! Is it, actually, an implicit override?
            if (entry.title == "PolkitKdeOverrideImplicit") {
                // It is!
                kDebug() << "Found implicit override";
                setImplicitAuthorization(PKLAEntry::implFromText(entry.resultActive), m_ui->activeComboBox);
                setImplicitAuthorization(PKLAEntry::implFromText(entry.resultInactive), m_ui->inactiveComboBox);
                setImplicitAuthorization(PKLAEntry::implFromText(entry.resultAny), m_ui->anyComboBox);
            } else {
                // TODO: Add it to the local auths
                //LocalAuthorization *auth = new LocalAuthorization(entry);
                kDebug() << "Found PKLA override";
                QListWidgetItem *item = new QListWidgetItem(entry.title);
                item->setData(Qt::UserRole, formatPKLAEntry(entry));
                m_ui->localAuthListWidget->addItem(item);
            }
        }
    }
}

QString ActionWidget::formatPKLAEntry(const PKLAEntry& entry)
{
    QString authorizationText;

    if (PKLAEntry::implFromText(entry.resultActive) != m_action->implicitActive()) {
        authorizationText.append(i18n("'%1' on active console", entry.resultActive));
        authorizationText.append(", ");
    }
    if (PKLAEntry::implFromText(entry.resultInactive) != m_action->implicitInactive()) {
        authorizationText.append(i18n("'%1' on inactive console", entry.resultActive));
        authorizationText.append(", ");
    }
    if (PKLAEntry::implFromText(entry.resultAny) != m_action->implicitAny()) {
        authorizationText.append(i18n("'%1' on any console", entry.resultActive));
        authorizationText.append(", ");
    }

    if (authorizationText.endsWith(", ")) {
        authorizationText.remove(-1, 2);
    }

    return i18np("%2 has the following policy: %3", "%2 have the following policy: %3",
                 entry.identity.split(';').count(), formatIdentities(entry.identity), authorizationText);
}

QString ActionWidget::formatIdentities(const QString& identities)
{
    QString rettext;
    QStringList realIdentities = identities.split(';');

    foreach (const QString &identity, realIdentities) {
        if (identity.startsWith("unix-user:")) {
            rettext.append(identity.split("unix-user:").last());
            rettext.append(", ");
        }
        if (identity.startsWith("unix-group:")) {
            rettext.append(i18n("%1 group", identity.split("unix-group:").last()));
            rettext.append(", ");
        }
    }

    if (rettext.endsWith(", ")) {
        rettext = rettext.remove(rettext.length() - 2, 2);
    }

    return rettext;
}

void ActionWidget::setImplicitAuthorization(PolkitQt1::ActionDescription::ImplicitAuthorization auth, QComboBox* box)
{
    box->setCurrentIndex(comboBoxIndexFor(auth));
}

int ActionWidget::comboBoxIndexFor(PolkitQt1::ActionDescription::ImplicitAuthorization auth)
{
    switch (auth) {
        case PolkitQt1::ActionDescription::Authorized:
            return 0;
        case PolkitQt1::ActionDescription::NotAuthorized:
            return 1;
        case PolkitQt1::ActionDescription::AuthenticationRequired:
            return 4;
        case PolkitQt1::ActionDescription::AuthenticationRequiredRetained:
            return 5;
        case PolkitQt1::ActionDescription::AdministratorAuthenticationRequired:
            return 2;
        case PolkitQt1::ActionDescription::AdministratorAuthenticationRequiredRetained:
            return 3;
        default:
            break;
    }

    return 1;
}

PolkitQt1::ActionDescription::ImplicitAuthorization ActionWidget::implicitAuthorizationFor(int comboBoxIndex)
{
    switch (comboBoxIndex) {
        case 0:
            return PolkitQt1::ActionDescription::Authorized;
        case 1:
            return PolkitQt1::ActionDescription::NotAuthorized;
        case 4:
            return PolkitQt1::ActionDescription::AuthenticationRequired;
        case 5:
            return PolkitQt1::ActionDescription::AuthenticationRequiredRetained;
        case 2:
            return PolkitQt1::ActionDescription::AdministratorAuthenticationRequired;
        case 3:
            return PolkitQt1::ActionDescription::AdministratorAuthenticationRequiredRetained;
        default:
            break;
    }

    return PolkitQt1::ActionDescription::Unknown;
}

void ActionWidget::setAction(PolkitQt1::ActionDescription* action)
{
    m_action = action;
    setImplicitAuthorization(action->implicitActive(), m_ui->activeComboBox);
    setImplicitAuthorization(action->implicitInactive(), m_ui->inactiveComboBox);
    setImplicitAuthorization(action->implicitAny(), m_ui->anyComboBox);

    m_ui->descriptionLabel->setText(action->description());
    m_ui->vendorLabel->setText(action->vendorName());
    m_ui->vendorLabel->setUrl(action->vendorUrl());
    m_ui->pixmapLabel->setPixmap(KIcon(action->iconName()).pixmap(64));

    computeActionPolicies();
}

void ActionWidget::editExplicitPKLAEntry(QListWidgetItem* item)
{
    foreach (const PKLAEntry &entry, m_entries) {
        if (entry.title == item->text()) {
            QWeakPointer<ExplicitAuthorizationDialog> dialog = new ExplicitAuthorizationDialog(entry, this);
            if (dialog.data()->exec() == KDialog::Ok) {
                dialog.data()->commitChangesToPKLA();
                PKLAEntry result = dialog.data()->pkla();
                // Register the entry. But first remove the previous one to avoid duplicates
                for (PKLAEntryList::iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                    if ((*it).title == result.title) {
                        // Erase the old one
                        m_entries.erase(it);
                        break;
                    }
                }

                addNewPKLAEntry(result);
            }

            if (dialog) {
                dialog.data()->deleteLater();
            }
        }
    }
}

void ActionWidget::addExplicitPKLAEntry()
{
    QWeakPointer<ExplicitAuthorizationDialog> dialog = new ExplicitAuthorizationDialog(m_action->actionId(), this);
    if (dialog.data()->exec() == KDialog::Accepted) {
        dialog.data()->commitChangesToPKLA();
        PKLAEntry result = dialog.data()->pkla();
        // Register the entry.
        addNewPKLAEntry(result);
    }

    if (dialog) {
        dialog.data()->deleteLater();
    }
}

void ActionWidget::addNewPKLAEntry(const PKLAEntry& entry)
{
    PKLAEntry toInsert(entry);
    // Match it to the current config value
    QSettings settings("/etc/polkit-1/polkit-kde-1.conf", QSettings::IniFormat);
    settings.beginGroup("General");
    toInsert.filePriority = settings.value("PoliciesPriority", 75).toInt();

    // If there's no file order, append it to the end of the current entries
    if (toInsert.fileOrder < 0) {
        int max = 0;
        foreach (const PKLAEntry &entry, m_entries) {
            if (entry.fileOrder > max) {
                max = entry.fileOrder;
            }
        }
        ++max;
        toInsert.fileOrder = max;
    }

    // Ok, now append it to the list
    m_entries.append(toInsert);
    kDebug() << "Inserting entry named " << toInsert.title << " for " << toInsert.action;

    // And reload the policies
    computeActionPolicies();
}

}
