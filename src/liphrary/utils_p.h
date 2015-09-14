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

#ifndef PHRARY_UTILS_P_H
#define PHRARY_UTILS_P_H

#include <functional>

#include <KIO/StoredTransferJob>

#include <Async>

#include <QUrl>
#include <QJsonDocument>

namespace Phrary {

template<typename T>
void parseResponse(const QUrl &url,
                    KAsync::Future<typename T::List> &future)
{
    qDebug() << "Requesting" << url.toDisplayString();
    KIO::StoredTransferJob *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    QObject::connect(job, &KIO::Job::result,
        [future](KJob *job) {
            auto f = future;
            KIO::StoredTransferJob *stj = qobject_cast<KIO::StoredTransferJob*>(job);
            if (stj->error()) {
                qWarning() << typeid(T).name() << "request error:" << stj->errorString();
                f.setError(stj->error(), stj->errorString());
                return;
            } else {
                const QByteArray json = stj->data();
                const QJsonDocument doc = QJsonDocument::fromJson(json);
                const QVariantMap map = doc.toVariant().toMap();
                if (!map[QStringLiteral("error_code")].isNull()) {
                    qWarning() << typeid(T).name() << "API error:" << map[QStringLiteral("error_info")].toString();
                    f.setError(map[QStringLiteral("error_code")].toInt(),
                                map[QStringLiteral("error_info")].toString());
                    return;
                }
                f.setValue(T::Private::parse(map[QStringLiteral("result")]));
                f.setFinished();
            }
        });
}

} // namespace Phrary

#endif