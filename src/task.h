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
#ifndef TASK_H
#define TASK_H

#include <QObject>

class Task : public QObject
{
    Q_OBJECT

public:
    Task(QObject* parent);
    virtual ~Task();

    static void push(Task*);
    static void waitForFinished();
    static void waitForFinished(Task*);

signals:
    void finished();

protected:
    virtual void start() { deleteLater(); }

private:
    static QList<Task*> s_tasks;

    bool  _started;
    bool* _pFinished;
};

class RenameFileTask : public Task
{
    Q_OBJECT

public:
    RenameFileTask(const QString& file, const QString& newName, QObject* parent);

protected slots:
    void timerSingleShot_timeout();

protected:
    void start();

private:
    QString _file;
    QString _newName;
};

#endif // TASK_H

