/*  Copyright (C) 2012 nel

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

QList<Task*> Task::s_listTask;

Task::Task(QObject* parent) : QObject(parent)
{
    s_listTask.append(this);
}

void Task::waitForTasksFinished()
{
//  for(int i=0; i < s_listTask.count(); i++)
//      qDebug() << QString().sprintf("renameTimerTaskList: %p", s_listTask.at(i));

    while( s_listTask.count() ) {
        QCoreApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        CommonLib::msleep(1);
    }
}

void Task::deleteObject()
{
    s_listTask.removeOne(this);
    deleteLater();
}

// ---------------------------------------------------------------------------------------
RenameTimerTask::RenameTimerTask(const QString& fileName, const QString& baseName,
        QObject* parent) : Task(parent)
{
    _timer.setSingleShot(true);
    _timer.setInterval(1500);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(slot_timeout()));

    _fileName = fileName;
    _baseName = baseName;

    _timer.start();
}

void RenameTimerTask::slot_timeout()
{
    // ファイルリネーム処理
    if( QFile::exists(_fileName) ) {
        QString suffix = QFileInfo(_fileName).suffix();
        if( !suffix.isEmpty() )
            suffix = "." + suffix;

        // 重複しないリネームファイル名を決定する
        QString name = CommonLib::retTheFileNameNotExists(_baseName + suffix);
/*
        QFileInfo file(_baseName + suffix);
        int num = 1;
        while( file.exists() || file.isSymLink() ) {
            num++;
            if( num > LIMIT_NUM ) break;

            file.setFile(QString("%1_%2%3").arg(_baseName).arg(num).arg(suffix));

//          LogDialog::debug(file.fileName());
        }
*/
        // リネーム
        if( QFile::rename(_fileName, name) )
            _fileName = name;
    }

    deleteObject();
//  LogDialog::debug(QString("rename: %1").arg(_fileName));
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

