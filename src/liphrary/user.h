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

#ifndef PHRARY_USER_H
#define PHRARY_USER_H

#include <QSharedDataPointer>
#include <QVector>

#include <Async>

class QString;
class QByteArray;
class QStringList;
class QUrl;

namespace Phrary {

class Server;

class User
{
public:
    class Private;
    typedef QVector<User> List;

    User();
    User(const User &other);
    ~User();
    User &operator=(const User &other);

    static KAsync::Job<User::List, QUrl> query(const QVector<QByteArray> &phids = {});

    QByteArray phid() const;
    void setPHID(const QByteArray &phid);

    QString userName() const;
    void setUserName(const QString &userName);

    QString realName() const;
    void setRealName(const QString &realName);

    QUrl image() const;
    void setImage(const QUrl &image);

    QUrl uri() const;
    void setUri(const QUrl &uri);

    QStringList roles() const;
    void setRoles(const QStringList &roles);

private:
    QSharedDataPointer<Private> d_ptr;
};
}

#endif // PHRARY_USER_H
