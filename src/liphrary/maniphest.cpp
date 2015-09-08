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

#include <QByteArray>
#include <QUrl>
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

    static Task::List parse(const QVariantMap &data)
    {
        Task::List tasks;
        tasks.reserve(data.size());

        for (auto iter = data.cbegin(), end = data.cend(); iter != end; ++iter) {
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

namespace Phrary
{
namespace Maniphest
{
void handleQueryResult(KJob *job,
                       KAsync::Future<Task::List> future) // (yes, this is a copy!)
{
    KIO::StoredTransferJob *stj = qobject_cast<KIO::StoredTransferJob*>(job);
    if (stj->error()) {
        qWarning() << "maniphest.query error:" << stj->errorString();
        future.setError(stj->error(), stj->errorString());
        return;
    }

    const QByteArray json = stj->data();
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    const QVariantMap map = doc.toVariant().toMap();
    if (!map[QStringLiteral("error_code")].isNull()) {
        qWarning() << "Project::query() error:" << map[QStringLiteral("error_info")].toString();
        future.setError(map[QStringLiteral("error_code")].toInt(),
                        map[QStringLiteral("error_info")].toString());
        return;
    }
    const QVariantMap result = map[QStringLiteral("result")].toMap();
    future.setValue(Task::Private::parse(result));
    future.setFinished();
}
}
}

KAsync::Job<Maniphest::Task::List, Server> Maniphest::queryProject(const QString &projectPHID,
                                                                    int offset)
{
    return KAsync::start<Maniphest::Task::List, Server>(
        [projectPHID, offset](const Server &server, KAsync::Future<Maniphest::Task::List> &future)
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
            qDebug() << "Requesting" << url.toDisplayString();
            KIO::StoredTransferJob *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
            QObject::connect(job, &KIO::Job::result,
                            [future](KJob *job) {
                                Maniphest::handleQueryResult(job, future);
                            });
        });
}

KAsync::Job<Maniphest::Task::List, Server> Maniphest::queryTasks(const QStringList &taskPHIDs,
                                                                 int offset)
{
    return KAsync::start<Maniphest::Task::List, Server>(
        [taskPHIDs, offset](const Server &server, KAsync::Future<Maniphest::Task::List> &future)
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
            qDebug() << "Requesting" << url.toDisplayString();
            KIO::StoredTransferJob *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
            QObject::connect(job, &KIO::Job::result,
                            [future](KJob *job) {
                                 Maniphest::handleQueryResult(job, future);
                             });
        });
}