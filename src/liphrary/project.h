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

#ifndef PROJECT_H_
#define PROJECT_H_

#include <Async>

#include <QVector>
#include <QSharedDataPointer>

class QString;
class QByteArray;
class QDateTime;

namespace Phrary
{

class Server;

class Project
{
public:
    typedef QVector<Project> List;

    Project();
    Project(const Project &other);

    ~Project();

    static KAsync::Job<Project::List, Phrary::Server> query(
                            const QStringList &projectPHIDs = QStringList());

    QByteArray phid() const;
    void setPHID(const QByteArray &phid);

    uint id() const;
    void setId(uint id);

    QString name() const;
    void setName(const QString &name);

    QByteArray profileImagePHID() const;
    void setProfileImagePHID(const QByteArray &phid);

    QString icon() const;
    void setIcon(const QString &icon);

    QString color() const;
    void setColor(const QString &color);

    QVector<QByteArray> memberPHIDs() const;
    void setMemberPHIDs(const QVector<QByteArray> &memberPHIDs);

    QStringList slugs() const;
    void setSlugs(const QStringList &slugs);

    QDateTime dateCreated() const;
    void setDateCreated(const QDateTime &dateCreated);

    QDateTime dateModified() const;
    void setDateModified(const QDateTime &dateModified);

private:
    class Private;
    QSharedDataPointer<Private> d_ptr;
};

}


#endif
