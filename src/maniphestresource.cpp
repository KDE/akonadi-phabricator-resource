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
#include <QUrl>

#include <AkonadiCore/Collection>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/CachePolicy>

#include "configdialog.h"
#include "settings.h"
#include "liphrary/server.h"
#include "liphrary/project.h"
#include "liphrary/maniphest.h"
#include "liphrary/user.h"

#include <KCalCore/Todo>
#include <KCalCore/Attendee>

#include <Async/Async>

#include <KLocalizedString>

QHash<QByteArray, Phrary::User> ManiphestResource::mUserCache;

ManiphestResource::ManiphestResource(const QString &identifier)
    : Akonadi::ResourceBase(identifier)
{
    connect(this, &Akonadi::AgentBase::reloadConfiguration,
            this, &ManiphestResource::doReconfigure);

    // Initialize server configuration
    doReconfigure();
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
        if (!Settings::self()->url().isEmpty() && !Settings::self()->aPIToken().isEmpty()) {
            synchronize();
        }
    } else {
        configurationDialogRejected();
    }
}

void ManiphestResource::doReconfigure()
{
    if (Settings::self()->url().isEmpty()) {
        setName(i18nc("Name of the resource",
                      "Phabricator Resource"));
    } else {
        setName(i18nc("Name of the resource (with URL of the server)",
                      "Phabricator Resource (%1)", Settings::self()->url()));
    }
}

void ManiphestResource::payloadToItem(const Phrary::Maniphest::Task &task,
                                      const Phrary::Maniphest::Transaction::List &taskTransactions,
                                      Akonadi::Item &item)
{
    item.setRemoteId(task.phid());
    item.setRemoteRevision(QString::number(task.dateModified().toTime_t()));

    KCalCore::Todo::Ptr todoPtr(new KCalCore::Todo);
    todoPtr->setUid(task.phid());
    todoPtr->setSummary(QStringLiteral("[%1] %2").arg(task.objectName(), task.title()));
    todoPtr->setCompleted(task.isClosed());
    todoPtr->setReadOnly(true);
    todoPtr->setUrl(task.uri());
    if (task.priority() == QLatin1String("Wishlist")) {
        todoPtr->setPriority(1);
    } else if (task.priority() == QLatin1String("Low")) {
        todoPtr->setPriority(3);
    } else if (task.priority() == QLatin1String("Normal")) {
        todoPtr->setPriority(5);
    } else if (task.priority() == QLatin1String("High")) {
        todoPtr->setPriority(7);
    } else if (task.priority() == QLatin1String("Unbreak Now!")) {
        todoPtr->setPriority(9);
    } else if (task.priority() == QLatin1String("Needs Triage")) {
        todoPtr->setPriority(0);
    } else {
        qWarning() << "Unknown task priority" << task.priority();
    }

    todoPtr->setOrganizer(mUserCache.value(task.authorPHID()).realName());
    Q_FOREACH (const QByteArray &cc, task.ccPHIDs()) {
        KCalCore::Attendee::Ptr atteePtr(new KCalCore::Attendee(mUserCache.value(cc).realName(),
                                                                QString()));
        todoPtr->addAttendee(atteePtr);
    }
    qDebug() << "Task" << todoPtr->uid() << "has organizer:" << todoPtr->organizer()->fullName() << "(from" << mUserCache.value(task.authorPHID()).realName() << ")";

    QString description = task.description()
        + QStringLiteral("\n\n")
        + QStringLiteral("-").repeated(40)
        + QStringLiteral("\n");

    int commentsCount = 0;
    Q_FOREACH (const Phrary::Maniphest::Transaction &trx, taskTransactions) {
        if (trx.transactionType() != "core:comment") {
            continue;
        }

        ++commentsCount;
        description += QStringLiteral("%1 on %2 wrote:\n%3\n\n")
            .arg(mUserCache.value(trx.authorPHID()).realName())
            .arg(trx.dateCreated().toString(Qt::LocaleDate))
            .arg(trx.comments());
    }
    if (commentsCount == 0) {
        description = task.description();
    }
    todoPtr->setDescription(description.replace(QLatin1Char('\n'), QStringLiteral("<br>")));


    item.setMimeType(KCalCore::Todo::todoMimeType());
    item.setPayload(todoPtr);
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
                collection.setRemoteId(project.phid());
                auto attribute = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Entity::AddIfMissing);
                attribute->setDisplayName(project.name());
                collection.setContentMimeTypes({ KCalCore::Todo::todoMimeType() });
                collection.setParentCollection(rootCollection);
                collection.setRights(Akonadi::Collection::ReadOnly);
                return { collection };
            },
            [this](int, const QString &error)
            {
                cancelTask(error);
            })
        .then<void, Akonadi::Collection::List>(
            [this, rootCollection](const Akonadi::Collection::List &collections) {
                collectionsRetrieved(Akonadi::Collection::List{ rootCollection } +  collections);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
}

bool ManiphestResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts);

    Phrary::Maniphest::queryTasksByPHID({ item.remoteId() })
        .then<void, Phrary::Maniphest::Task::List>(
            [this, item](const Phrary::Maniphest::Task::List &tasks)
            {
                if (tasks.count() == 0) {
                    // error
                    cancelTask();
                    return;
                }

                const Phrary::Maniphest::Task &task = tasks[0];

                auto future = Phrary::Maniphest::queryTransactionsByTask(QVector<uint>{ task.id() })
                    .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
                // FIXME: nope nope nope nope nope nope nope nope
                future.waitForFinished();

                Akonadi::Item i(item);
                ManiphestResource::payloadToItem(task, future.value(), i);
                itemRetrieved(i);
            },
            [this](int, const QString &error)
            {
                cancelTask(error);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));

    return true;
}

void ManiphestResource::fetchUsers(const QVector<QByteArray> &phids)
{
    auto future = Phrary::User::query(phids)
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
    // FIXME: Nope nope nope nope nope nope
    future.waitForFinished();

    Q_FOREACH (const Phrary::User &user, future.value()) {
        mUserCache.insert(user.phid(), user);
    }
}

void ManiphestResource::retrieveItems(const Akonadi::Collection &collection)
{
    Phrary::Maniphest::queryTasksByProject(collection.remoteId())
        .each<Akonadi::Item::List, Phrary::Maniphest::Task>(
            [this, collection](const Phrary::Maniphest::Task &task)
            {
                QVector<QByteArray> usersToFetch;
                auto author = mUserCache.constFind(task.authorPHID());
                if (author == mUserCache.cend()) {
                    usersToFetch.push_back(task.authorPHID());
                }
                Q_FOREACH (const QByteArray &user, task.ccPHIDs()) {
                    auto ccd = mUserCache.constFind(user);
                    if (ccd == mUserCache.cend()) {
                        usersToFetch.push_back(user);
                    }
                }

                auto future = Phrary::Maniphest::queryTransactionsByTask(QVector<uint>{ task.id() })
                    .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
                // FIXME: Nope nope nope nope nope nope nope
                future.waitForFinished();

                Q_FOREACH (const Phrary::Maniphest::Transaction &trx, future.value()) {
                    auto author = mUserCache.constFind(trx.authorPHID());
                    if (author == mUserCache.cend()) {
                        usersToFetch.push_back(trx.authorPHID());
                    }
                }

                if (!usersToFetch.isEmpty()) {
                    fetchUsers(usersToFetch);
                }

                Akonadi::Item item;
                item.setParentCollection(collection);
                ManiphestResource::payloadToItem(task, future.value(), item);
                return Akonadi::Item::List{ item };
            },
            [this](int, const QString &error)
            {
                cancelTask(error);
            })
        .then<void, Akonadi::Item::List>(
            [this](const Akonadi::Item::List &items)
            {
                itemsRetrieved(items);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
}

AKONADI_RESOURCE_MAIN(ManiphestResource)