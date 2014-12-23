/*  Copyright (C) 2013-2014 nel

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
#ifndef PEERCAST_H
#define PEERCAST_H

#include <QTimer>
#include <QNetworkAccessManager>
#include "task.h"

class ChannelInfo
{
public:
    enum STATUS { ST_UNKNOWN, ST_CONNECT, ST_RECEIVE, ST_SEARCH, ST_ERROR };
    QString chName;
    QString rootIp;     // 不使用
    QString contactUrl;
    int     bitrate;    // kbps
    STATUS  status;

    void channelInfo() { clear(); }
    void clear() {
        chName = "";
        rootIp = "";
        contactUrl = "";
        bitrate = 0;
        status = ST_UNKNOWN;
    }

    QString statusString() {
        const char* str[] = { "UNKNOWN", "CONNECT", "RECEIVE", "SEARCH", "ERROR" };
        return str[this->status];
    }
};

class Peercast : public QObject
{
    Q_OBJECT

public:
    enum TYPE { TYPE_UNKNOWN, TYPE_VP, TYPE_ST };

    Peercast(QObject* parent=0);
    QString host() { return _host; }
    ushort  port() { return _port; }
    QString id()   { return _id; }
    TYPE    type() { return _type; }
    void setHostPortId(QString host, ushort port, QString id);
    void stop();
    void bump();
    void getChannelInfo();
    void disconnectChannel(int startSec);

signals:
    void gotChannelInfo(const ChannelInfo&);

protected slots:
    void getChannelInfo_GetPeercastTypeTask_finished();
    void disconnectChannel_GetPeercastTypeTask_finished();
    void nam_finished(QNetworkReply*);

private:
    QNetworkAccessManager _nam;
    int _disconnectStartSec;

    QString _host;
    ushort  _port;
    QString _id;
    TYPE    _type;
};

class GetPeercastTypeTask : public Task
{
    Q_OBJECT

public:
    GetPeercastTypeTask(const QString& host, ushort port, Peercast::TYPE* pType, QObject* parent);

protected slots:
    void nam_finished(QNetworkReply*);

protected:
    void start();
    void queryPeercastType();
    bool whetherPcVp(const QString& reply);
    bool whetherPcSt(const QString& reply);

private:
    QNetworkAccessManager _nam;
    QList<Peercast::TYPE> _attemptTypes;

    QString _host;
    ushort  _port;
    Peercast::TYPE* _pType;
};

class GetChannelInfoTask : public Task
{
    Q_OBJECT

public:
    GetChannelInfoTask(const QString& host, ushort port, const QString& id,
                       Peercast::TYPE type, QObject* parent);

signals:
    void finished(const ChannelInfo&);

protected slots:
    void nam_finished(QNetworkReply*);

protected:
    void start();
    void getChannelStatusPcSt();
    bool parseChannelInfoPcVp(const QString& reply);
    bool parseChannelInfoPcSt(const QString& reply);
    bool parseChannelStatusPcSt(const QString& reply);
    void debugChannelInfo();

private:
    QNetworkAccessManager _nam;
    QNetworkReply* _replyChannelInfo;
    QNetworkReply* _replyChannelStatusPcSt;

    QString _host;
    ushort  _port;
    QString _id;
    Peercast::TYPE _type;
    ChannelInfo _chInfo;
};

class StopChannelTask : public Task
{
    Q_OBJECT

public:
    StopChannelTask(const QString& host, ushort port, const QString& id, QObject* parent);

protected slots:
    void nam_finished(QNetworkReply*);

protected:
    void start();

private:
    QNetworkAccessManager _nam;
    QString _host;
    ushort  _port;
    QString _id;
};

class DisconnectChannelTask : public Task
{
    Q_OBJECT

public:
    DisconnectChannelTask(const QString& host, ushort port, const QString& id,
                          Peercast::TYPE type, int startSec, QObject* parent);

protected slots:
    void timerSingleShot_timeout();
    void nam_finished(QNetworkReply*);

protected:
    void start();
    bool getChannelStatusPcVp(const QString& reply);
    bool getChannelStatusPcSt(const QString& reply);

private:
    enum { MONITORING_MSEC = 15000, REPETITION_MSEC = 5000 };

    QTimer  _timer;
    QNetworkAccessManager _nam;
    QString _host;
    ushort  _port;
    QString _id;
    Peercast::TYPE _peercastType;
    int     _startSec;
    int     _listeners;
    bool    _retryGetStatus;
};

#endif // PEERCAST_H

