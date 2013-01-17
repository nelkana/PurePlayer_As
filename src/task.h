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

#include <QTimer>
#include <QNetworkAccessManager>
#include "pureplayer.h"

class Task : public QObject
{
    Q_OBJECT;

public:
    Task(QObject* parent);
    virtual ~Task() {}

    static void waitForTasksFinished();

protected:
    void deleteObject();

private:
    static QList<Task*> s_tasks;
};

class RenameFileTask : public Task
{
    Q_OBJECT;

public:
    RenameFileTask(const QString& file, const QString& newName, QObject* parent);

protected slots:
    void timerSingleShot_timeout();

private:
    QString _file;
    QString _newName;
};

class StopChannelTask : public Task
{
    Q_OBJECT;

public:
    StopChannelTask(const QString& host, const short port, const QString& id, QObject* parent);

protected slots:
    void nam_finished(QNetworkReply*);

private:
    QNetworkAccessManager _nam;
};

class DisconnectChannelTask : public Task
{
    Q_OBJECT;

public:
    DisconnectChannelTask(const QString& host, const short port, const QString& id,
                           const PurePlayer::PEERCAST_TYPE type, QObject* parent);

protected:
    bool getChannelStatusPcVp(const QString& reply);
    bool getChannelStatusPcSt(const QString& reply);

protected slots:
    void timerSingleShot_timeout();
    void nam_finished(QNetworkReply*);

private:
    QNetworkAccessManager _nam;
    QString _host;
    short   _port;
    QString _id;
    PurePlayer::PEERCAST_TYPE _peercastType;
    int     _listeners;
    bool    _searchingConnection;
};

#endif // TASK_H

