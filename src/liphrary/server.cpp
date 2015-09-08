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

#include "server.h"

#include <QString>

using namespace Phrary;

class Server::Private : public QSharedData
{
public:
    Private()
    {
    }

    Private(const Private &other)
        : QSharedData(other)
        , host(other.host)
        , apiToken(other.apiToken)
    {
    }

    Private(const QString &host, const QString &apiToken)
        : host(host)
        , apiToken(apiToken)
    {
    }

    QString host;
    QString apiToken;
};

Server::Server()
    : d_ptr(new Private)
{
}

Server::Server(const QString &host, const QString &apiToken)
    : d_ptr(new Private(host, apiToken))
{
}

Server::Server(const Server &other)
    : d_ptr(other.d_ptr)
{
}

Server::~Server()
{
}

Phrary::Server &Phrary::Server::operator=(const Server &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

void Server::setServer(const QString &server)
{
    d_ptr->host = server;
}

QString Server::server() const
{
    return d_ptr->host;
}

void Server::setAPIToken(const QString &token)
{
    d_ptr->apiToken = token;
}

QString Server::apiToken() const
{
    return d_ptr->apiToken;
}
