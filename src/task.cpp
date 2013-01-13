/*  Copyright (C) 2012-2013 nel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include "task.h"
#include "commonlib.h"

QList<Task*> Task::s_tasks;

Task::Task(QObject* parent) : QObject(parent)
{
    s_tasks.append(this);
}

void Task::waitForTasksFinished()
{
//  for(int i=0; i < s_tasks.count(); i++)
//      qDebug() << QString().sprintf("tasks: %p", s_tasks.at(i));

    while( s_tasks.count() ) {
        QCoreApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        CommonLib::msleep(1);
    }
}

void Task::deleteObject()
{
    s_tasks.removeOne(this);
    deleteLater();
}

// ---------------------------------------------------------------------------------------
RenameFileTask::RenameFileTask(const QString& file, const QString& newName,
        QObject* parent) : Task(parent)
{
    _timer.setSingleShot(true);
    _timer.setInterval(1500);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(slot_timeout()));

    _file = file;
    _newName = newName;

    _timer.start();
}

void RenameFileTask::slot_timeout()
{
    if( QFile::exists(_file) ) {
        QString name = CommonLib::retTheFileNameNotExists(_newName);

        if( QFile::rename(_file, name) )
            _file = name;
    }

    deleteObject();
//  LogDialog::debug(QString("rename: %1").arg(_file));
}

// ---------------------------------------------------------------------------------------
PeercastStopTask::PeercastStopTask(const QString& host, const short port, const QString& id,
        QObject* parent) : Task(parent)
{
    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    QUrl url(QString("http://%1:%2/admin?cmd=stop&id=%3").arg(host).arg(port).arg(id));
    _nam.get(QNetworkRequest(url));
}

void PeercastStopTask::nam_finished(QNetworkReply* reply)
{
    reply->deleteLater();
    deleteObject();
}

