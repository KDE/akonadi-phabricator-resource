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

#include "resource.h"

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
#include "liphrary/markup.h"

#include <KCalCore/Todo>
#include <KCalCore/Attendee>

#include <KAsync/Async>

#include <KLocalizedString>

QHash<QByteArray, Phrary::User> PhabricatorResource::mUserCache;

PhabricatorResource::PhabricatorResource(const QString &identifier)
    : Akonadi::ResourceBase(identifier)
    , Akonadi::AgentBase::Observer()
{
    connect(this, &Akonadi::AgentBase::reloadConfiguration,
            this, &PhabricatorResource::doReconfigure);

    // Initialize server configuration
    doReconfigure();
}

PhabricatorResource::~PhabricatorResource()
{
}

void PhabricatorResource::abortActivity()
{
}

void PhabricatorResource::aboutToQuit()
{
    abortActivity();
}

void PhabricatorResource::configure(WId windowId)
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

void PhabricatorResource::doReconfigure()
{
    if (Settings::self()->url().isEmpty()) {
        setName(i18nc("Name of the resource",
                      "Phabricator Resource"));
    } else {
        setName(i18nc("Name of the resource (with URL of the server)",
                      "Phabricator Resource (%1)", Settings::self()->url()));
        Phrary::Markup::setPhabricatorUrl(Settings::self()->url());
    }
}

void PhabricatorResource::payloadToItem(const Phrary::Maniphest::Task &task,
                                        const Phrary::Maniphest::Transaction::List &taskTransactions,
                                        Akonadi::Item &item)
{
    item.setRemoteId(QString::fromUtf8(task.phid()));
    item.setRemoteRevision(QString::number(task.dateModified().toTime_t()));

    KCalCore::Todo *todo = new KCalCore::Todo;
    todo->setUid(item.remoteId());
    todo->setSummary(QStringLiteral("[%1] %2").arg(QString::fromUtf8(task.objectName()), task.title()));
    todo->setCompleted(task.isClosed());
    todo->setUrl(task.uri());
    if (task.priority() == QLatin1String("Wishlist")) {
        todo->setPriority(1);
    } else if (task.priority() == QLatin1String("Low")) {
        todo->setPriority(3);
    } else if (task.priority() == QLatin1String("Normal")) {
        todo->setPriority(5);
    } else if (task.priority() == QLatin1String("High")) {
        todo->setPriority(7);
    } else if (task.priority() == QLatin1String("Unbreak Now!")) {
        todo->setPriority(9);
    } else if (task.priority() == QLatin1String("Needs Triage")) {
        todo->setPriority(0);
    } else {
        qWarning() << "Unknown task priority" << task.priority();
    }

    todo->setOrganizer(mUserCache.value(task.authorPHID()).realName());
    Q_FOREACH (const QByteArray &cc, task.ccPHIDs()) {
        KCalCore::Attendee attee(mUserCache.value(cc).realName(), QString());
        todo->addAttendee(attee);
    }

    QString description = Phrary::Markup::markupToHTML(task.description())
        + QStringLiteral("<br><br><hr><br>");

    int commentsCount = 0;
    // Iterate in reverse order, because Conduit returns transactions in order
    // from newest to oldest, which makes no sense when displaying comments
    auto iter = taskTransactions.cend();
    while (iter != taskTransactions.cbegin()) {
        --iter;
        if (iter->transactionType() != "core:comment") {
            continue;
        }

        ++commentsCount;
        auto author = mUserCache.constFind(iter->authorPHID());
        if (author == mUserCache.constEnd()) {
            description += i18nc("Header to a task comment: On DATE, unknown user wrote",
                                 "On %1, unknown user wrote:",
                                 iter->dateCreated().toString(Qt::LocaleDate));
        } else {
            description += i18nc("Header to a task comment: On DATE, REAL NAME (USERNAME) wrote",
                                 "On %1, %2 (%3) wrote:",
                                 iter->dateCreated().toString(Qt::LocalDate),
                                 author->realName(),
                                 author->userName());
        }
        description += QStringLiteral("<br>%1<br><hr>").arg(Phrary::Markup::markupToHTML(iter->comments()));
    }
    if (commentsCount == 0) {
        description = Phrary::Markup::markupToHTML(task.description());
    }
    todo->setDescription(description, true);

    // This must be set as last, otherwise all other set* are ignored
    todo->setReadOnly(true);

    item.setMimeType(KCalCore::Todo::todoMimeType());
    item.setPayload<KCalCore::Todo::Ptr>(KCalCore::Todo::Ptr(todo));
}

void PhabricatorResource::retrieveCollections()
{
    Akonadi::Collection rootCollection;
    rootCollection.setName(QUrl::fromUserInput(Settings::self()->url()).host(QUrl::PrettyDecoded));
    auto attribute = rootCollection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
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
        .then<Akonadi::Collection::List, Phrary::Project::List>(
            [rootCollection](const Phrary::Project::List &projects) -> Akonadi::Collection::List {
                Akonadi::Collection::List cols;
                cols.reserve(projects.size());
                std::transform(projects.cbegin(), projects.cend(), std::back_inserter(cols),
                [&rootCollection](const Phrary::Project &project) {
                    Akonadi::Collection collection;
                    collection.setName(project.name());
                    collection.setRemoteId(QString::fromUtf8(project.phid()));
                    auto attribute = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
                    attribute->setDisplayName(project.name());
                    collection.setContentMimeTypes({ KCalCore::Todo::todoMimeType() });
                    collection.setParentCollection(rootCollection);
                    collection.setRights(Akonadi::Collection::ReadOnly);
                    return collection;
                });
                return cols;
            })
        .then<void, Akonadi::Collection::List>(
            [this, rootCollection](const Akonadi::Collection::List &collections) {
                collectionsRetrieved(Akonadi::Collection::List{ rootCollection } +  collections);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
}

bool PhabricatorResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
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
                PhabricatorResource::payloadToItem(task, future.value(), i);
                itemRetrieved(i);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));

    return true;
}

void PhabricatorResource::fetchUsers(const QVector<QByteArray> &phids)
{
    auto future = Phrary::User::query(phids)
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
    // FIXME: Nope nope nope nope nope nope
    future.waitForFinished();

    Q_FOREACH (const Phrary::User &user, future.value()) {
        mUserCache.insert(user.phid(), user);
    }
}

void PhabricatorResource::retrieveItems(const Akonadi::Collection &collection)
{
    Phrary::Maniphest::queryTasksByProject(collection.remoteId())
        .then<Akonadi::Item::List, Phrary::Maniphest::Task::List>(
            [this, collection](const Phrary::Maniphest::Task::List &tasks) {
                Akonadi::Item::List items;
                for (const auto &task : tasks) {
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
                        // remove duplicates. They somehow happen and they are expensive
                        // to fetch
                        // TODO: Make them not happen in the first place
                        std::sort(usersToFetch.begin(), usersToFetch.end());
                        for (auto iter = usersToFetch.begin(), end = usersToFetch.end(); iter != end; end = usersToFetch.end() ) {
                            auto next = iter;
                            if (iter == ++next) {
                                iter = usersToFetch.erase(iter);
                            } else {
                                iter = next;
                            }
                        }
                        fetchUsers(usersToFetch);
                    }

                    Akonadi::Item item;
                    item.setParentCollection(collection);
                    PhabricatorResource::payloadToItem(task, future.value(), item);
                    items.push_back(item);
                }
                return items;
            })
        .then<void, Akonadi::Item::List>(
            [this](const Akonadi::Item::List &items) {
                itemsRetrieved(items);
            })
        .exec(Phrary::Server(Settings::self()->url(), Settings::self()->aPIToken()));
}

AKONADI_RESOURCE_MAIN(PhabricatorResource)
