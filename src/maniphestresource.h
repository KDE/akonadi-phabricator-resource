/*
 * Copyright 2015  Daniel Vrátil <dvratil@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MANIPHESTRESOURCE_H
#define MANIPHESTRESOURCE_H

#include <AkonadiAgentBase/ResourceBase>

#include "liphrary/maniphest.h"

#include <QHash>

namespace Phrary {
class User;
}

class ManiphestResource : public Akonadi::ResourceBase
                        , public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    ManiphestResource(const QString &identifier);
    ~ManiphestResource();

    void configure(WId windowId) Q_DECL_OVERRIDE;
    void abortActivity() Q_DECL_OVERRIDE;
    void aboutToQuit() Q_DECL_OVERRIDE;

    void retrieveCollections() Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    // This is a read-only resource

private Q_SLOTS:
    void doReconfigure();

private:
    static void payloadToItem(const Phrary::Maniphest::Task &task,
                              const Phrary::Maniphest::Transaction::List &taskTransactions,
                              Akonadi::Item &item);

    void fetchUsers(const QVector<QByteArray> &userPHIDs);

private:
    static QHash<QByteArray, Phrary::User> mUserCache;
};

#endif // MANIPHESTRESOURCE_H
