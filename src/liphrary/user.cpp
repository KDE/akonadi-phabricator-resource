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

#include "user.h"
#include "server.h"
#include "utils_p.h"

#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>

#include <Async>

#include <QJsonDocument>

using namespace Phrary;

class User::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
    {
    }

    Private(const Private &other)
        : QSharedData(other)
        , phid(other.phid)
        , userName(other.userName)
        , realName(other.realName)
        , image(other.image)
        , uri(other.uri)
        , roles(other.roles)
    {
    }

    static User::List parse(const QVariant &result)
    {
        const QVariantList results = result.toList();

        User::List users;
        users.reserve(results.size());

        Q_FOREACH (const QVariant &l, results) {
            const QVariantMap d = l.toMap();
            User user;
            user.d_ptr->phid = d[QStringLiteral("phid")].toByteArray();
            user.d_ptr->userName = d[QStringLiteral("userName")].toString();
            user.d_ptr->realName = d[QStringLiteral("realName")].toString();
            user.d_ptr->image = d[QStringLiteral("image")].toUrl();
            user.d_ptr->uri = d[QStringLiteral("uri")].toUrl();
            user.d_ptr->roles = d[QStringLiteral("roles")].toStringList();

            users.push_back(user);
        }

        return users;
    }

    QByteArray phid;
    QString userName;
    QString realName;
    QUrl image;
    QUrl uri;
    QStringList roles;
};

User::User()
    : d_ptr(new Private)
{
}

User::User(const User &other)
    : d_ptr(other.d_ptr)
{
}

User::~User()
{
}

User &User::operator=(const User &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

QByteArray User::phid() const
{
    return d_ptr->phid;
}

void User::setPHID(const QByteArray &phid)
{
    d_ptr->phid = phid;
}

QString User::userName() const
{
    return d_ptr->userName;
}

void User::setUserName(const QString &userName)
{
    d_ptr->userName = userName;
}

QString User::realName() const
{
    return d_ptr->realName;
}

void User::setRealName(const QString &realName)
{
    d_ptr->realName = realName;
}

QUrl User::image() const
{
    return d_ptr->image;
}

void User::setImage(const QUrl &image)
{
    d_ptr->image = image;
}

QUrl User::uri() const
{
    return d_ptr->uri;
}

void User::setUri(const QUrl &uri)
{
    d_ptr->uri = uri;
}

QStringList User::roles() const
{
    return d_ptr->roles;
}

void User::setRoles(const QStringList &roles)
{
    d_ptr->roles = roles;
}

KAsync::Job<User::List, Server> User::query(const QVector<QByteArray> &phids)
{
    return KAsync::start<QUrl, Server>(
        [phids](const Server &server) {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("api.token"), server.apiToken());
            for (int i = 0; i < phids.count(); ++i) {
                query.addQueryItem(QStringLiteral("phids[%1]").arg(i), QString::fromUtf8(phids.at(i)));
            }

            QUrl url(server.server());
            url.setPath(QStringLiteral("/api/user.query"));
            url.setQuery(query);
            return url;
        })
    .then<User::List, QUrl>(&Phrary::parseResponse<User>);
}
