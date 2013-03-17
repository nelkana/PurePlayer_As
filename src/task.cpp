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
#include <QTimer>
#include <QDebug>
#include "task.h"
#include "commonlib.h"

QList<Task*> Task::s_tasks;

Task::Task(QObject* parent) : QObject(parent)
{
    _started = false;
    _pFinished = NULL;
}

Task::~Task()
{
//  qDebug() << "task_dest: start";
    s_tasks.removeOne(this);
    if( _pFinished != NULL )
        *_pFinished = true;

    if( _started )
        emit finished();

//  qDebug() << "task_dest: end";
}

void Task::push(Task* task)
{
    if( task == NULL ) return;
    if( s_tasks.contains(task) ) return;

    s_tasks.append(task);
    task->start();
    task->_started = true;
}

void Task::waitForFinished()
{
//  qDebug() << "wait: start";
    while( s_tasks.size() ) {
        QCoreApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        CommonLib::msleep(1);
    }

//  qDebug() << "wait: end" << s_tasks.size();
}

void Task::waitForFinished(Task* task)
{
    if( task == NULL ) return;
    if( !s_tasks.contains(task) ) return;

    bool finished = false;
    task->_pFinished = &finished;

    while( !finished ) {
        QCoreApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        CommonLib::msleep(1);
    }
}

// ---------------------------------------------------------------------------------------
RenameFileTask::RenameFileTask(const QString& file, const QString& newName,
        QObject* parent) : Task(parent)
{
    _file = file;
    _newName = newName;
}

void RenameFileTask::timerSingleShot_timeout()
{
    if( QFile::exists(_file) ) {
        QString name = CommonLib::retTheFileNameNotExists(_newName);

        if( QFile::rename(_file, name) )
            _file = name;
    }

    deleteLater();
//  LogDialog::debug(QString("rename: %1").arg(_file));
}

void RenameFileTask::start()
{
    QTimer::singleShot(1500, this, SLOT(timerSingleShot_timeout()));
}

