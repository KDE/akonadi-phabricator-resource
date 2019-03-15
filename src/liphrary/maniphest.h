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

#ifndef MANIPHEST_H_
#define MANIPHEST_H_

#include <QSharedDataPointer>

#include <Async>

class QByteArray;
class QString;
class QUrl;
class QDateTime;

#include <QVector>

namespace Phrary
{

class Server;

namespace Maniphest
{

class Task;
KAsync::Job<QVector<Task>, Server> queryTasksByProject(const QString &projectPHID,
                                                     int offset = 0);

KAsync::Job<QVector<Task>, Server> queryTasksByPHID(const QStringList &taskPHIDs,
                                                  int offset = 0);

class Task
{
public:
    class Private;

    typedef QVector<Task> List;

    Task();
    Task(const Task &other);
    ~Task();

    QByteArray phid() const;
    void setPHID(const QByteArray &phid);

    uint id() const;
    void setId(uint id);

    QByteArray authorPHID() const;
    void setAuthorPHID(const QByteArray &author);

    QByteArray ownerPHID() const;
    void setOwnerPHID(const QByteArray &owner);

    QVector<QByteArray> ccPHIDs() const;
    void setCcPHIDs(const QVector<QByteArray> &ccPHID);

    QString status() const;
    void setStatus(const QString &status);

    QString statusName() const;
    void setStatusName(const QString &statusName);

    bool isClosed() const;
    void setIsClosed(bool isClosed);

    QString priority() const;
    void setPriority(const QString &priority);

    QString priorityColor() const;
    void setPriorityColor(const QString &priorityColor);

    QString title() const;
    void setTitle(const QString &title);

    QString description() const;
    void setDescription(const QString &description);

    QVector<QByteArray> projectPHIDs() const;
    void setProjectPHIDs(const QVector<QByteArray> &projectPHIDs);

    QUrl uri() const;
    void setUri(const QUrl &uri);

    QByteArray objectName() const;
    void setObjectName(const QByteArray &objectName);

    QDateTime dateCreated() const;
    void setDateCreated(const QDateTime &dateCreated);

    QDateTime dateModified() const;
    void setDateModified(const QDateTime &dateModified);

    QVector<QByteArray> dependsOnTaskPHIDs() const;
    void setDependsOnTaskPHIDs(const QVector<QByteArray> &dependsOn);

private:
    QSharedDataPointer<Private> d_ptr;
};


class Transaction
{
public:
    class Private;

    typedef QVector<Transaction> List;

    Transaction();
    Transaction(const Transaction &other);
    ~Transaction();

    int taskId() const;
    void setTaskId(int taskId);

    QByteArray transactionPHID() const;
    void setTransactionPHID(const QByteArray &transactionPHID);

    QByteArray transactionType() const;
    void setTransactionType(const QByteArray &transactionType);

    QString comments() const;
    void setComments(const QString &comments);

    QByteArray authorPHID() const;
    void setAuthorPHID(const QByteArray &authorPHID);

    QDateTime dateCreated() const;
    void setDateCreated(const QDateTime &dateTime);

    /* NOTE: we don't support the oldValue/newValue thing, because it
     * changes based on transactionType and I cannot really represent that with
     * Ph++ API and I didn't have enough beer to come up with something better
     * (I know had enough beer to come up with QVariantMap, but cannot be bothered
     * to implement it. We don't need it anyway).
     */
private:
    QSharedDataPointer<Private> d_ptr;
};

KAsync::Job<Transaction::List, Server> queryTransactionsByTask(const QVector<uint> &taskIds);



} // namespace Maniphest

} // namespace Phrary


#endif
