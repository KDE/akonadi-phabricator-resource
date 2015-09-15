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

#include "configdialog.h"
#include "ui_configdialog.h"

#include <Async>

#include "liphrary/server.h"
#include "liphrary/project.h"

#include "settings.h"

#include <QValidator>
#include <QToolTip>
#include <QTimer>

#include <KMessageBox>
#include <KLocalizedString>

class UrlValidator : public QValidator
{
  public:
    UrlValidator(QObject *parent = Q_NULLPTR)
        : QValidator(parent)
    {
    }

    QValidator::State validate(QString &input , int &pos) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(pos);

        if (QUrl::fromUserInput(input).isValid()) {
            return Acceptable;
        }

        return Invalid;
    }
};


ConfigDialog::ConfigDialog(WId windowId)
    : QDialog()
    , ui(new Ui::ConfigDialog)
{
    mLoadProjectsTimer = new QTimer(this);
    mLoadProjectsTimer->setSingleShot(true);
    mLoadProjectsTimer->setInterval(200);
    connect(mLoadProjectsTimer, &QTimer::timeout,
            this, &ConfigDialog::loadProjects);

    ui->setupUi(this);

    const QString url = Settings::self()->url();
    ui->phabricatorUrlEdit->setText(url.isEmpty() ? QStringLiteral("https://") : url);
    ui->phabricatorUrlEdit->setValidator(new UrlValidator(this));
    ui->apiTokenEdit->setText(Settings::self()->aPIToken());
    ui->maniphestProgressBar->setVisible(false);

    connect(this, &QDialog::accepted,
            this, &ConfigDialog::onAccepted);

    connect(ui->phabricatorUrlEdit, &QLineEdit::textChanged,
            [this](const QString&) {
                scheduleLoadProjects();
            });
    connect(ui->apiTokenEdit, &QLineEdit::textChanged,
            [this](const QString &) {
                scheduleLoadProjects();
            });
    connect(ui->howToGetTokenHelp, &QLabel::linkActivated,
            [this](const QString &) {
                QToolTip::showText(ui->howToGetTokenHelp->mapToGlobal(QPoint(0, 0)),
                                   ui->howToGetTokenHelp->toolTip());
            });
    connect(ui->maniphestRefreshButton, &QPushButton::clicked,
            [this](bool) {
                scheduleLoadProjects();
            });

    scheduleLoadProjects();
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::scheduleLoadProjects()
{
    if (ui->phabricatorUrlEdit->text().isEmpty() || ui->apiTokenEdit->text().isEmpty()) {
        return;
    }

    if (mLoadProjectsTimer->isActive()) {
        return;
    }

    mLoadProjectsTimer->start();
}

void ConfigDialog::loadProjects()
{
    ui->maniphestProgressBar->setVisible(true);
    ui->maniphestRefreshButton->setEnabled(false);
    ui->maniphestProjectsView->setEnabled(false);
    ui->maniphestProjectsView->clear();

    Phrary::Project::query()
        .each<void, Phrary::Project>(
            [this](const Phrary::Project &project) {
                QListWidgetItem *item = new QListWidgetItem();
                item->setText(project.name());
                item->setData(Qt::UserRole, project.phid());
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                item->setCheckState(Settings::self()->projects().contains(project.phid()) ? Qt::Checked : Qt::Unchecked);
                ui->maniphestProjectsView->addItem(item);
            })
        .then<void>(
            [this]() {
                ui->maniphestProgressBar->setVisible(false);
                ui->maniphestRefreshButton->setEnabled(true);
                ui->maniphestProjectsView->setEnabled(true);
            },
            [this](int, const QString &error) {
                KMessageBox::error(this, i18n("An error occurred while retrieving projects: %1").arg(error));
                ui->maniphestProgressBar->setVisible(false);
                ui->maniphestRefreshButton->setEnabled(true);
                ui->maniphestProjectsView->setEnabled(true);
            })
        .exec(Phrary::Server(ui->phabricatorUrlEdit->text(), ui->apiTokenEdit->text()));
}


void ConfigDialog::onAccepted()
{
    Settings::self()->setUrl(ui->phabricatorUrlEdit->text());
    Settings::self()->setAPIToken(ui->apiTokenEdit->text());
    if (ui->refreshCheckBox->isChecked()) {
        Settings::self()->setInterval(ui->refreshInterval->value());
    } else {
        Settings::self()->setInterval(-1);
    }

    QStringList projects;
    for (int i = 0; i < ui->maniphestProjectsView->count(); ++i) {
        QListWidgetItem *item = ui->maniphestProjectsView->item(i);
        if (item->checkState() == Qt::Checked) {
            projects.push_back(QString::fromLatin1(item->data(Qt::UserRole).toByteArray()));
        }
    }
    Settings::self()->setProjects(projects);

    Settings::self()->save();
}
