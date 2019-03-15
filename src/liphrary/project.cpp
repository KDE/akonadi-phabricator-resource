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

#include "project.h"
#include "server.h"
#include "utils_p.h"

#include <QDateTime>
#include <QVariantMap>
#include <QByteArray>
#include <QUrl>
#include <QUrlQuery>

#include <QJsonDocument>

#include <KIO/StoredTransferJob>

using namespace Phrary;

class Project::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , id(0)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
        , phid(other.phid)
        , id(other.id)
        , name(other.name)
        , profileImagePHID(other.profileImagePHID)
        , icon(other.icon)
        , color(other.color)
        , memberPHIDs(other.memberPHIDs)
        , slugs(other.slugs)
        , dateCreated(other.dateCreated)
        , dateModified(other.dateModified)
    {
    }

    static Project::List parse(const QVariant &result)
    {
        const QVariantMap &data = result.toMap()[QStringLiteral("data")].toMap();

        Project::List projects;
        projects.reserve(data.size());

        for (auto iter = data.cbegin(), end = data.cend(); iter != end; ++iter) {
            const QVariantMap d = iter.value().toMap();
            Project project;
            project.d_ptr->phid = iter.key().toLatin1();
            project.d_ptr->id = d[QStringLiteral("id")].toInt();
            project.d_ptr->phid = d[QStringLiteral("phid")].toByteArray();
            project.d_ptr->name = d[QStringLiteral("name")].toString();
            project.d_ptr->profileImagePHID = d[QStringLiteral("profileImagePHID")].toByteArray();
            project.d_ptr->icon = d[QStringLiteral("icon")].toString();
            project.d_ptr->color = d[QStringLiteral("color")].toString();
            const QVariantList memberPHIDs = d[QStringLiteral("members")].toList();
            project.d_ptr->memberPHIDs.reserve(memberPHIDs.size());
            Q_FOREACH (const QVariant &memberPHID, memberPHIDs) {
                project.d_ptr->memberPHIDs.push_back(memberPHID.toByteArray());
            }
            project.d_ptr->slugs = d[QStringLiteral("slugs")].toStringList();
            project.d_ptr->dateCreated = QDateTime::fromTime_t(d[QStringLiteral("dateCreated")].toUInt());
            project.d_ptr->dateModified = QDateTime::fromTime_t(d[QStringLiteral("dateModified")].toUInt());

            projects.push_back(project);
        }

        return projects;
    }

    QByteArray phid;
    uint id;
    QString name;
    QByteArray profileImagePHID;
    QString icon;
    QString color;
    QVector<QByteArray> memberPHIDs;
    QStringList slugs;
    QDateTime dateCreated;
    QDateTime dateModified;
};

Project::Project()
    : d_ptr(new Private)
{
}

Project::Project(const Project &other)
    : d_ptr(other.d_ptr)
{
}

Project::~Project()
{
}

KAsync::Job<Project::List, Server> Project::query(const QStringList &phids)
{
    return KAsync::start<QUrl, Server>(
        [phids](const Server &server) {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("api.token"), server.apiToken());
            for (int i = 0; i < phids.count(); ++i) {
                query.addQueryItem(QStringLiteral("phids[%1]").arg(i),
                                   phids.at(i));
            }

            QUrl url(server.server());
            url.setPath(QStringLiteral("/api/project.query"));
            url.setQuery(query);
            return url;
        })
    .then<Project::List, QUrl>(&Phrary::parseResponse<Project>);
}

QByteArray Project::phid() const
{
    return d_ptr->phid;
}

void Project::setPHID(const QByteArray &phid)
{
    d_ptr->phid = phid;
}

uint Project::id() const
{
    return d_ptr->id;
}

void Project::setId(uint id)
{
    d_ptr->id = id;
}

QString Project::name() const
{
    return d_ptr->name;
}

void Project::setName(const QString &name)
{
    d_ptr->name = name;
}

QByteArray Project::profileImagePHID() const
{
    return d_ptr->profileImagePHID;
}

void Project::setProfileImagePHID(const QByteArray &phid)
{
    d_ptr->profileImagePHID = phid;
}

QString Project::icon() const
{
    return d_ptr->icon;
}

void Project::setIcon(const QString &icon)
{
    d_ptr->icon = icon;
}

QString Project::color() const
{
    return d_ptr->color;
}

void Project::setColor(const QString &color)
{
    d_ptr->color = color;
}

QVector<QByteArray> Project::memberPHIDs() const
{
    return d_ptr->memberPHIDs;
}

void Project::setMemberPHIDs(const QVector<QByteArray> &memberPHIDs)
{
    d_ptr->memberPHIDs = memberPHIDs;
}

QStringList Project::slugs() const
{
    return d_ptr->slugs;
}

void Project::setSlugs(const QStringList &slugs)
{
    d_ptr->slugs = slugs;
}

QDateTime Project::dateCreated() const
{
    return d_ptr->dateCreated;
}

void Project::setDateCreated(const QDateTime &dateCreated)
{
    d_ptr->dateCreated = dateCreated;
}

QDateTime Project::dateModified() const
{
    return d_ptr->dateModified;
}

void Project::setDateModified(const QDateTime &dateModified)
{
    d_ptr->dateModified = dateModified;
}
