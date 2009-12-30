/* This file is part of the KDE project

   Copyright (C) 2008 Dario Freddi <drf@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef POLKITKDE1HELPER_H
#define POLKITKDE1HELPER_H

#include <QtCore/QObject>
#include <QtDBus/QDBusContext>
#include <QtDBus/qdbusargument.h>

class PKLAEntry {
    public:
    QString title;
    QString identity;
    QString action;
    QString resultAny;
    QString resultInactive;
    QString resultActive;

    int filePriority;
    int fileOrder;
};
Q_DECLARE_METATYPE(PKLAEntry)

typedef QList<PKLAEntry> PKLAEntryList;

class PolkitKde1Helper : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.polkitkde1.helper")

    public:
        PolkitKde1Helper(QObject* parent = 0);
        virtual ~PolkitKde1Helper();

    public slots:
        void saveGlobalConfiguration(const QString &adminIdentities, int systemPriority, int policiesPriority);
        void retrievePolicies();

    Q_SIGNALS:
        void policiesRetrieved(const PKLAEntryList &policies);

    private:
        QList<PKLAEntry> entriesFromFile(int filePriority, const QString &fileContents);
};

QDBusArgument& operator<<(QDBusArgument& argument, const PKLAEntry& entry);
const QDBusArgument& operator>>(const QDBusArgument& argument, PKLAEntry& entry);

#endif // POLKITKDE1HELPER_H
