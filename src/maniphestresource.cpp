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

#include "maniphestresource.h"

#include <QScopedPointer>

#include <AkonadiCore/Collection>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/CachePolicy>

#include "configdialog.h"
#include "settings.h"
#include "liphrary/server.h"
#include "liphrary/project.h"

#include <KCalCore/Todo>

#include <Async/Async>

ManiphestResource::ManiphestResource(const QString &identifier)
    : Akonadi::ResourceBase(identifier)
{
    connect(this, &Akonadi::AgentBase::reloadConfiguration,
            this, &ManiphestResource::doReconfigure);
}

ManiphestResource::~ManiphestResource()
{
}

void ManiphestResource::abortActivity()
{
}

void ManiphestResource::aboutToQuit()
{
    abortActivity();
}

void ManiphestResource::configure(WId windowId)
{
    QScopedPointer<ConfigDialog> dialog(new ConfigDialog(windowId));
    dialog->exec();
    if (!dialog) {
        return;
    }

    if (dialog->result() == QDialog::Accepted) {
        configurationDialogAccepted();
    } else {
        configurationDialogRejected();
    }
}

void ManiphestResource::doReconfigure()
{
}


void ManiphestResource::retrieveCollections()
{
    Akonadi::Collection rootCollection;
    rootCollection.setName(QUrl::fromUserInput(Settings::self()->url()).host(QUrl::PrettyDecoded));
    auto attribute = rootCollection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Entity::AddIfMissing);
    attribute->setDisplayName(rootCollection.name());
    rootCollection.setContentMimeTypes({ Akonadi::Collection::mimeType() });
    rootCollection.setRights(Akonadi::Collection::ReadOnly);
    rootCollection.setRemoteId(QStringLiteral("root"));
    if (Settings::self()->interval() > -1) {
        Akonadi::CachePolicy cp;
        cp.setIntervalCheckTime(Settings::self()->interval());
        rootCollection.setCachePolicy(cp);
    }

    Phrary::Project::query(Settings::self()->projects())
        .each<Akonadi::Collection::List, Phrary::Project>(
            [rootCollection](const Phrary::Project &project) -> Akonadi::Collection::List {
                Akonadi::Collection collection;
                collection.setName(project.name());
                auto attribute = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Entity::AddIfMissing);
                attribute->setDisplayName(project.name());
                collection.setContentMimeTypes({ KCalCore::Todo::todoMimeType() });
                collection.setParentCollection(rootCollection);
                collection.setRights(Akonadi::Collection::ReadOnly);
                qDebug() << "Got" << collection.name();
                return { collection };
            })
        .then<void, Akonadi::Collection::List>(
            [this, rootCollection](const Akonadi::Collection::List &collections) {
                qDebug() << "Got" << collections.count() << "collections!";
                collectionsRetrieved(Akonadi::Collection::List{ rootCollection } +  collections);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
}

bool ManiphestResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    return true;
}

void ManiphestResource::retrieveItems(const Akonadi::Collection &collection)
{
}

AKONADI_RESOURCE_MAIN(ManiphestResource)