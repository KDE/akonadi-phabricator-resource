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

#include "maniphest.h"
#include "server.h"
#include "utils_p.h"

#include <QByteArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QJsonDocument>

#include <KIO/StoredTransferJob>

using namespace Phrary;

class Maniphest::Task::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
    {
    }

    Private(const Private &other)
        : QSharedData(other)
        , phid(other.phid)
        , id(other.id)
        , authorPHID(other.authorPHID)
        , ownerPHID(other.ownerPHID)
        , ccPHIDs(other.ccPHIDs)
        , status(other.status)
        , statusName(other.statusName)
        , isClosed(other.isClosed)
        , priority(other.priority)
        , priorityColor(other.priorityColor)
        , title(other.title)
        , description(other.description)
        , projectPHIDs(other.projectPHIDs)
        , uri(other.uri)
        , objectName(other.objectName)
        , dateCreated(other.dateCreated)
        , dateModified(other.dateModified)
        , dependsOnTaskPHIDs(other.dependsOnTaskPHIDs)
    {
    }

    ~Private()
    {
    }

    static Task::List parse(const QVariant &data)
    {
        const QVariantMap results = data.toMap();
        Task::List tasks;
        tasks.reserve(results.size());

        for (auto iter = results.cbegin(), end = results.cend(); iter != end; ++iter) {
            const QVariantMap d = iter.value().toMap();
            Task task;
            task.d_ptr->phid = iter.key().toLatin1();
            task.d_ptr->id = d[QStringLiteral("id")].toUInt();
            task.d_ptr->authorPHID = d[QStringLiteral("authorPHID")].toByteArray();
            task.d_ptr->ownerPHID = d[QStringLiteral("ownerPHID")].toByteArray();
            const QVariantList ccPHIDs = d[QStringLiteral("ccPHIDs")].toList();
            task.d_ptr->ccPHIDs.reserve(ccPHIDs.size());
            Q_FOREACH (const QVariant &ccPHID, ccPHIDs) {
                task.d_ptr->ccPHIDs.push_back(ccPHID.toByteArray());
            }
            task.d_ptr->status = d[QStringLiteral("status")].toString();
            task.d_ptr->statusName = d[QStringLiteral("statusName")].toString();
            task.d_ptr->isClosed = d[QStringLiteral("isClosed")].toBool();
            task.d_ptr->priority = d[QStringLiteral("priority")].toString();
            task.d_ptr->priorityColor = d[QStringLiteral("priorityColor")].toString();
            task.d_ptr->title = d[QStringLiteral("title")].toString();
            task.d_ptr->description = d[QStringLiteral("description")].toString();
            const QVariantList projectPHIDs = d[QStringLiteral("projectPHIDs")].toList();
            task.d_ptr->projectPHIDs.reserve(projectPHIDs.size());
            Q_FOREACH (const QVariant &projectPHID, projectPHIDs) {
                task.d_ptr->projectPHIDs.push_back(projectPHID.toByteArray());
            }
            task.d_ptr->uri = d[QStringLiteral("uri")].toUrl();
            task.d_ptr->objectName = d[QStringLiteral("objectName")].toByteArray();
            task.d_ptr->dateCreated = QDateTime::fromTime_t(d[QStringLiteral("dateCreated")].toUInt());
            task.d_ptr->dateModified = QDateTime::fromTime_t(d[QStringLiteral("dateModified")].toUInt());
            const QVariantList dependsPHIDs = d[QStringLiteral("dependsOnTaskPHIDs")].toList();
            task.d_ptr->dependsOnTaskPHIDs.reserve(dependsPHIDs.size());
            Q_FOREACH (const QVariant &dependsPHID, dependsPHIDs) {
                task.d_ptr->dependsOnTaskPHIDs.push_back(dependsPHID.toByteArray());
            }

            tasks.push_back(task);
        }

        return tasks;
    }

    QByteArray phid;
    uint id;
    QByteArray authorPHID;
    QByteArray ownerPHID;
    QVector<QByteArray> ccPHIDs;
    QString status;
    QString statusName;
    bool isClosed;
    QString priority;
    QString priorityColor;
    QString title;
    QString description;
    QVector<QByteArray> projectPHIDs;
    QUrl uri;
    QByteArray objectName;
    QDateTime dateCreated;
    QDateTime dateModified;
    QVector<QByteArray> dependsOnTaskPHIDs;
};

Maniphest::Task::Task()
    : d_ptr(new Private)
{
}

Maniphest::Task::Task(const Maniphest::Task &other)
    : d_ptr(other.d_ptr)
{
}

Maniphest::Task::~Task()
{
}

QByteArray Maniphest::Task::phid() const
{
    return d_ptr->phid;
}

void Maniphest::Task::setPHID(const QByteArray &phid)
{
    d_ptr->phid = phid;
}

uint Maniphest::Task::id() const
{
    return d_ptr->id;
}

void Maniphest::Task::setId(uint id)
{
    d_ptr->id = id;
}

QByteArray Maniphest::Task::authorPHID() const
{
    return d_ptr->authorPHID;
}

void Maniphest::Task::setAuthorPHID(const QByteArray &authorPHID)
{
    d_ptr->authorPHID = authorPHID;
}

QByteArray Maniphest::Task::ownerPHID() const
{
    return d_ptr->ownerPHID;
}

void Maniphest::Task::setOwnerPHID(const QByteArray &ownerPHID)
{
    d_ptr->ownerPHID = ownerPHID;
}

QVector<QByteArray> Maniphest::Task::ccPHIDs() const
{
    return d_ptr->ccPHIDs;
}

void Maniphest::Task::setCcPHIDs(const QVector<QByteArray> &ccPHIDs)
{
    d_ptr->ccPHIDs = ccPHIDs;
}

QString Maniphest::Task::status() const
{
    return d_ptr->status;
}

void Maniphest::Task::setStatus(const QString &status)
{
    d_ptr->status = status;
}

QString Maniphest::Task::statusName() const
{
    return d_ptr->statusName;
}
void Maniphest::Task::setStatusName(const QString &statusName)
{
    d_ptr->statusName = statusName;
}

bool Maniphest::Task::isClosed() const
{
    return d_ptr->isClosed;
}

void Maniphest::Task::setIsClosed(bool isClosed)
{
    d_ptr->isClosed = isClosed;
}

QString Maniphest::Task::priority() const
{
    return d_ptr->priority;
}

void Maniphest::Task::setPriority(const QString &priority)
{
    d_ptr->priority = priority;
}

QString Maniphest::Task::priorityColor() const
{
    return d_ptr->priorityColor;
}

void Maniphest::Task::setPriorityColor(const QString &priorityColor)
{
    d_ptr->priorityColor = priorityColor;
}

QString Maniphest::Task::title() const
{
    return d_ptr->title;
}

void Maniphest::Task::setTitle(const QString &title)
{
    d_ptr->title = title;
}

QString Maniphest::Task::description() const
{
    return d_ptr->description;
}

void Maniphest::Task::setDescription(const QString &description)
{
    d_ptr->description = description;
}

QVector<QByteArray> Maniphest::Task::projectPHIDs() const
{
    return d_ptr->projectPHIDs;
}
void Maniphest::Task::setProjectPHIDs(const QVector<QByteArray> &projectPHIDs)
{
    d_ptr->projectPHIDs = projectPHIDs;
}

QUrl Maniphest::Task::uri() const
{
    return d_ptr->uri;
}
void Maniphest::Task::setUri(const QUrl &uri)
{
    d_ptr->uri = uri;
}

QByteArray Maniphest::Task::objectName() const
{
    return d_ptr->objectName;
}

void Maniphest::Task::setObjectName(const QByteArray &objectName)
{
    d_ptr->objectName = objectName;
}

QDateTime Maniphest::Task::dateCreated() const
{
    return d_ptr->dateCreated;
}

void Maniphest::Task::setDateCreated(const QDateTime &dateCreated)
{
    d_ptr->dateCreated = dateCreated;
}

QDateTime Maniphest::Task::dateModified() const
{
    return d_ptr->dateModified;
}

void Maniphest::Task::setDateModified(const QDateTime &dateModified)
{
    d_ptr->dateModified = dateModified;
}

QVector<QByteArray> Maniphest::Task::dependsOnTaskPHIDs() const
{
    return d_ptr->dependsOnTaskPHIDs;
}

void Maniphest::Task::setDependsOnTaskPHIDs(const QVector<QByteArray> &dependsOn)
{
    d_ptr->dependsOnTaskPHIDs = dependsOn;
}

KAsync::Job<Maniphest::Task::List, Server> Maniphest::queryTasksByProject(const QString &projectPHID, int offset)
{
    return KAsync::start<QUrl, Server>(
        [projectPHID, offset](const Server &server)
        {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("api.token"), server.apiToken());
            if (!projectPHID.isEmpty()) {
                query.addQueryItem(QStringLiteral("projectPHIDs[0]"), projectPHID);
            }
            if (offset > 0) {
                query.addQueryItem(QStringLiteral("offset"), QString::number(offset));
            }

            QUrl url(server.server());
            url.setPath(QStringLiteral("/api/maniphest.query"));
            url.setQuery(query);
            return url;
        })
    .then<Maniphest::Task::List, QUrl>(&Phrary::parseResponse<Maniphest::Task>);
}

KAsync::Job<Maniphest::Task::List, Server> Maniphest::queryTasksByPHID(const QStringList &taskPHIDs, int offset)
{
    return KAsync::start<QUrl, Server>(
        [taskPHIDs, offset](const Server &server)
        {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("api.token"), server.apiToken());
            for (int i = 0; i < taskPHIDs.count(); ++i) {
                query.addQueryItem(QStringLiteral("ids[%i]").arg(i),
                                   taskPHIDs.at(i));
            }
            if (offset > 0) {
                query.addQueryItem(QStringLiteral("offset"),
                                   QString::number(offset));
            }

            QUrl url(server.server());
            url.setPath(QStringLiteral("/api/maniphest.query"));
            url.setQuery(query);
            return url;
        })
    .then<Maniphest::Task::List, QUrl>(&Phrary::parseResponse<Maniphest::Task>);
}


class Maniphest::Transaction::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , taskId(-1)
    {}

    Private(const Private &other)
        : QSharedData(other)
        , taskId(other.taskId)
        , transactionPHID(other.transactionPHID)
        , transactionType(other.transactionType)
        , comments(other.comments)
        , authorPHID(other.authorPHID)
        , dateCreated(other.dateCreated)
    {}

    static Transaction::List parse(const QVariant &data)
    {
        const QVariantMap results = data.toMap();

        Transaction::List trxs;
        trxs.reserve(results.size());

        for (auto iter = results.cbegin(), end = results.cend(); iter != end; ++iter) {
            const QVariantList taskTransactions = iter.value().toList();
            Q_FOREACH (const QVariant &dv, taskTransactions) {
                const QVariantMap d = dv.toMap();
                Transaction trx;
                trx.d_ptr->taskId = iter.key().toInt();
                trx.d_ptr->transactionPHID = d[QStringLiteral("transactionPHID")].toByteArray();
                trx.d_ptr->transactionType = d[QStringLiteral("transactionType")].toByteArray();
                trx.d_ptr->comments = d[QStringLiteral("comments")].toString();
                trx.d_ptr->authorPHID = d[QStringLiteral("authorPHID")].toByteArray();
                trx.d_ptr->dateCreated = QDateTime::fromTime_t(d[QStringLiteral("dateCreated")].toInt());

                trxs.push_back(trx);
            }
        }

        return trxs;
    }

    int taskId;
    QByteArray transactionPHID;
    QByteArray transactionType;
    QString comments;
    QByteArray authorPHID;
    QDateTime dateCreated;
};


Maniphest::Transaction::Transaction()
    : d_ptr(new Private)
{
}

Maniphest::Transaction::Transaction(const Transaction &other)
    : d_ptr(other.d_ptr)
{
}

Maniphest::Transaction::~Transaction()
{
}

int Maniphest::Transaction::taskId() const
{
    return d_ptr->taskId;
}

void Maniphest::Transaction::setTaskId(int taskId)
{
    d_ptr->taskId = taskId;
}

QByteArray Maniphest::Transaction::transactionPHID() const
{
    return d_ptr->transactionPHID;
}

void Maniphest::Transaction::setTransactionPHID(const QByteArray &transactionPHID)
{
    d_ptr->transactionPHID = transactionPHID;
}

QByteArray Maniphest::Transaction::transactionType() const
{
    return d_ptr->transactionType;
}

void Maniphest::Transaction::setTransactionType(const QByteArray &transactionType)
{
    d_ptr->transactionType = transactionType;
}

QString Maniphest::Transaction::comments() const
{
    return d_ptr->comments;
}

void Maniphest::Transaction::setComments(const QString &comments)
{
    d_ptr->comments = comments;
}

QByteArray Maniphest::Transaction::authorPHID() const
{
    return d_ptr->authorPHID;
}

void Maniphest::Transaction::setAuthorPHID(const QByteArray &authorPHID)
{
    d_ptr->authorPHID = authorPHID;
}

QDateTime Maniphest::Transaction::dateCreated() const
{
    return d_ptr->dateCreated;
}

void Maniphest::Transaction::setDateCreated(const QDateTime &dateCreated)
{
    d_ptr->dateCreated = dateCreated;
}

KAsync::Job<Maniphest::Transaction::List, Server> Maniphest::queryTransactionsByTask(const QVector<uint> &taskIds)
{
    return KAsync::start<QUrl, Server>(
        [taskIds](const Server &server)
        {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("api.token"), server.apiToken());
            for (int i = 0; i < taskIds.count(); ++i) {
                query.addQueryItem(QStringLiteral("ids[%1]").arg(i),
                                   QString::number(taskIds.at(i)));
            }

            QUrl url(server.server());
            url.setPath(QStringLiteral("/api/maniphest.gettasktransactions"));
            url.setQuery(query);
            return url;
        })
    .then<Maniphest::Transaction::List, QUrl>(&Phrary::parseResponse<Maniphest::Transaction>);
}
