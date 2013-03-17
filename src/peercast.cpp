/*  Copyright (C) 2013 nel

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
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScriptEngine>
#include <QDebug>
#include "peercast.h"
#include "task.h"
#include "logdialog.h"

Peercast::Peercast(QObject* parent) : QObject(parent)
{
    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(nam_finished(QNetworkReply*)));

    _disconnectStartSec = 0;

    _host = "";
    _port = 0;
    _id   = "";
    _type = TYPE_UNKNOWN;
}

void Peercast::setHostPortId(QString host, ushort port, QString id)
{
    const QString debugPrefix = "Peercast::setHostPortId(): ";

    _host = host;
    _port = port;
    _id   = id;

    LogDialog::debug(debugPrefix + QString("port %1").arg(_port));
    LogDialog::debug(debugPrefix + QString("ID %1").arg(_id));
}

void Peercast::stop()
{
    Task::push(new StopChannelTask(_host, _port, _id, this));
    LogDialog::debug("Peercast::stop(): ");
}

void Peercast::bump()
{
    QUrl url(QString("http://%1:%2/admin?cmd=bump&id=%3").arg(_host).arg(_port).arg(_id));
    _nam.get(QNetworkRequest(url));
}

void Peercast::getChannelInfo()
{
    Task* task = new GetPeercastTypeTask(_host, _port, &_type, this);
    connect(task, SIGNAL(finished()),
            this, SLOT(getChannelInfo_GetPeercastTypeTask_finished()));
    Task::push(task);
}

void Peercast::disconnectChannel(int startSec)
{
    if( _type == TYPE_UNKNOWN ) {
        _disconnectStartSec = startSec;
        Task* task = new GetPeercastTypeTask(_host, _port, &_type, this);
        connect(task, SIGNAL(finished()),
                this, SLOT(disconnectChannel_GetPeercastTypeTask_finished()));
        Task::push(task);
    }
    else
        Task::push(new DisconnectChannelTask(_host, _port, _id, _type, startSec, this));
}

void Peercast::getChannelInfo_GetPeercastTypeTask_finished()
{
    if( _type != TYPE_UNKNOWN ) {
        Task* task = new GetChannelInfoTask(_host, _port, _id, _type, this);
        connect(task, SIGNAL(finished(const ChannelInfo&)),
                this, SIGNAL(gotChannelInfo(const ChannelInfo&)));
        Task::push(task);
    }
}

void Peercast::disconnectChannel_GetPeercastTypeTask_finished()
{
    if( _type != TYPE_UNKNOWN )
        Task::push(new DisconnectChannelTask(_host, _port, _id, _type, _disconnectStartSec, this));
}

void Peercast::nam_finished(QNetworkReply* reply)
{
    reply->deleteLater();
}

// ---------------------------------------------------------------------------------------
GetPeercastTypeTask::GetPeercastTypeTask(const QString& host, ushort port,
        Peercast::TYPE* type, QObject* parent) : Task(parent)
{
    _host = host;
    _port = port;
    _type = type;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _attemptType = Peercast::TYPE_VP;
}

void GetPeercastTypeTask::nam_finished(QNetworkReply* reply)
{
    bool error = !(reply->error() == QNetworkReply::NoError);

    if( !error ) {
        QString out = reply->readAll();
        if( _attemptType == Peercast::TYPE_VP )
            error = !whetherPcVp(out);
        else
            error = !whetherPcSt(out);

        if( !error )
            *_type = _attemptType;
    }

    if( error ) {
        if( _attemptType == Peercast::TYPE_VP ) {
            _attemptType = Peercast::TYPE_ST;
            queryPeercastType();
            reply->deleteLater();
            return;
        }
        else
            *_type = Peercast::TYPE_UNKNOWN;
    }

    reply->deleteLater();
    deleteLater();
}

void GetPeercastTypeTask::start()
{
    queryPeercastType();
}

void GetPeercastTypeTask::queryPeercastType()
{
    if( _attemptType == Peercast::TYPE_VP ) {
        QUrl url(QString("http://%1:%2/html/ja/index.html").arg(_host).arg(_port));
        _nam.get(QNetworkRequest(url));
    }
    else {
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getVersionInfo").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _nam.post(request, data);
    }
}

bool GetPeercastTypeTask::whetherPcVp(const QString& reply)
{
    if( reply.indexOf("<span class=\"titlelinksBig\">peercast") != -1 )
        return true;
    else
        return false;
}

bool GetPeercastTypeTask::whetherPcSt(const QString& reply)
{
    QScriptEngine engine;
    QScriptValue  value;
    value = engine.evaluate("JSON.parse").call(QScriptValue(), QScriptValueList() << reply);

    if( value.isError() )
        return false;

    value = value.property("result");
    if( !value.isValid() )
        return false;

    return true;
}

// ---------------------------------------------------------------------------------------
GetChannelInfoTask::GetChannelInfoTask(const QString& host, ushort port, const QString& id,
    Peercast::TYPE type, QObject* parent) : Task(parent)
{
    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _replyChannelInfo       = NULL;
    _replyChannelStatusPcSt = NULL;

    _host = host;
    _port = port;
    _id   = id;
    _type = type;
}

void GetChannelInfoTask::nam_finished(QNetworkReply* reply)
{
    const QString debugPrefix = "GetChannelInfoTask::nam_finished(): ";

    if( reply->error() == QNetworkReply::NoError ) {
        QString out = reply->readAll();

        if( reply == _replyChannelInfo ) {
            _replyChannelInfo = NULL;

            if( _type == Peercast::TYPE_VP ) {
                if( parseChannelInfoPcVp(out) )
                    emit finished(_chInfo);
            }
            else { // _type == Peercast::TYPE_ST
                if( parseChannelInfoPcSt(out) ) {
                    getChannelStatusPcSt();
                    reply->deleteLater();
                    return;
                }
            }
        }
        else
        if( reply == _replyChannelStatusPcSt ) {
            _replyChannelStatusPcSt = NULL;

            if( parseChannelStatusPcSt(out) )
                emit finished(_chInfo);
        }
    }
    else
        LogDialog::debug(debugPrefix + "error " + QString::number(reply->error()), QColor(255,0,0));

    reply->deleteLater();
    deleteLater();
}

void GetChannelInfoTask::start()
{
    if( _type == Peercast::TYPE_UNKNOWN ) {
        deleteLater();
        return;
    }

    if( _type == Peercast::TYPE_VP ) {
        QUrl url(QString("http://%1:%2/html/ja/relayinfo.html?id=%3").arg(_host).arg(_port).arg(_id));
        _replyChannelInfo = _nam.get(QNetworkRequest(url));
    }
    else { // _type == Peercast::TYPE_ST
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelInfo").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _replyChannelInfo = _nam.post(request, data);
    }
}

void GetChannelInfoTask::getChannelStatusPcSt()
{
    if( _type == Peercast::TYPE_ST ) {
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelStatus").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _replyChannelStatusPcSt = _nam.post(request, data);
    }
}

bool GetChannelInfoTask::parseChannelInfoPcVp(const QString& reply)
{
    const QString debugPrefix = "GetChannelInfoTask::parseChannelInfoPcVp(): ";
    LogDialog::debug(debugPrefix + "called");

    int start, end;

    // チャンネル名の取得
    start = reply.indexOf("<td>チャンネル名");
    if( start == -1 )
        return false;

    start = reply.indexOf("\">", start);
    start += 2;
    end = reply.indexOf('<', start);

    _chInfo.chName = reply.mid(start, end - start);
    QTextDocument txt;
    txt.setHtml(_chInfo.chName);
    _chInfo.chName = txt.toPlainText();
    LogDialog::debug(debugPrefix + "name " + _chInfo.chName);

    // コンタクトURLの取得
    start = reply.indexOf("<td>URL", start);
    if( start == -1 )
        return false;

    start = reply.indexOf("\">", start);
    start += 2;
    end = reply.indexOf('<', start);

    _chInfo.contactUrl = reply.mid(start, end - start);
    LogDialog::debug(debugPrefix + "url " + _chInfo.contactUrl);
/*
    // 接続先IPアドレスの取得
    start = reply.indexOf("<td>取得元", start);
    if( start == -1 )
        return false;
    start = reply.indexOf("<br>", start);
    start += 4;
    start = reply.indexOf(QRegExp("\\S"), start);
    end = reply.indexOf('<', start);

    QString temp = reply.mid(start, end - start);
    QString connectedIp = temp.left(temp.indexOf(':'));

    LogDialog::debug(debugPrefix + connectedIp + ' ' + _chInfo.rootIp);

#ifndef QT_NO_DEBUG_OUTPUT
    if( connectedIp == _chInfo.rootIp )
        _title = "[" + _title + "]";
#endif
*/
    // 接続状態の取得
    start = reply.indexOf("<td>状態", start);
    if( start == -1 )
        return false;

    ++start;
    start = reply.indexOf("<td>", start);
    start += 4;
    end = reply.indexOf('<', start);

    QString status = reply.mid(start, end - start);
    if( status == "RECEIVE" )
        _chInfo.status = ChannelInfo::ST_RECEIVE;
    else
    if( status == "SEARCH" )
        _chInfo.status = ChannelInfo::ST_SEARCH;
    else
    if( status == "CONNECT" )
        _chInfo.status = ChannelInfo::ST_CONNECT;
    else
    if( status == "ERROR" )
        _chInfo.status = ChannelInfo::ST_ERROR;
    else
        _chInfo.status = ChannelInfo::ST_UNKNOWN;

//  _searchingConnection = (rootIp == "0.0.0.0");

    LogDialog::debug(debugPrefix + "status " + status);

    return true;
}

bool GetChannelInfoTask::parseChannelInfoPcSt(const QString& reply)
{
    const QString debugPrefix = "GetChannelInfoTask::parseChannelInfoPcSt(): ";
    LogDialog::debug(debugPrefix + "called");

    QScriptEngine engine;
    QScriptValue  value;
//  value = engine.evaluate("(" + reply + ")");
    value = engine.evaluate("JSON.parse").call(QScriptValue(), QScriptValueList() << reply);

    if( value.isError() ) {
        LogDialog::debug(debugPrefix + value.toString(), QColor(255,0,0));
        return false;
    }

    value = value.property("result").property("info");
    if( !value.isValid() )
        return false;

    _chInfo.chName     = value.property("name").toString();
    _chInfo.contactUrl = value.property("url").toString();

    LogDialog::debug(debugPrefix + "name " + _chInfo.chName);
    LogDialog::debug(debugPrefix + "url " + _chInfo.contactUrl);

    return true;
}

bool GetChannelInfoTask::parseChannelStatusPcSt(const QString& reply)
{
    const QString debugPrefix = "GetChannelInfoTask::parseChannelStatusPcSt(): ";
    LogDialog::debug(debugPrefix + "called");

    QScriptEngine engine;
    QScriptValue  value;
//  value = engine.evaluate("(" + reply + ")");
    value = engine.evaluate("JSON.parse").call(QScriptValue(), QScriptValueList() << reply);

    if( value.isError() ) {
        LogDialog::debug(debugPrefix + value.toString(), QColor(255,0,0));
        return false;
    }

    value = value.property("result");
    if( !value.isValid() )
        return false;

    QString status = value.property("status").toString();
    if( status == "Receiving" )
        _chInfo.status = ChannelInfo::ST_RECEIVE;
    else
    if( status == "Searching" )
        _chInfo.status = ChannelInfo::ST_SEARCH;
    else
    if( status == "Connecting" )
        _chInfo.status = ChannelInfo::ST_CONNECT;
    else
    if( status == "Error" )
        _chInfo.status = ChannelInfo::ST_ERROR;
    else
        _chInfo.status = ChannelInfo::ST_UNKNOWN;

    LogDialog::debug(debugPrefix + "status " + status);

    return true;
}

// ---------------------------------------------------------------------------------------
StopChannelTask::StopChannelTask(const QString& host, ushort port, const QString& id,
        QObject* parent) : Task(parent)
{
    _host = host;
    _port = port;
    _id   = id;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));
}

void StopChannelTask::nam_finished(QNetworkReply* reply)
{
    reply->deleteLater();
    deleteLater();
}

void StopChannelTask::start()
{
    QUrl url(QString("http://%1:%2/admin?cmd=stop&id=%3").arg(_host).arg(_port).arg(_id));
    _nam.get(QNetworkRequest(url));
}

// ---------------------------------------------------------------------------------------
DisconnectChannelTask::DisconnectChannelTask(const QString& host, ushort port,
    const QString& id, Peercast::TYPE type, int startSec, QObject* parent)
        : Task(parent)
{
    _host = host;
    _port = port;
    _id   = id;
    _peercastType = type;
    _startSec = startSec;
    _listeners = -1;
    _retryGetStatus = false;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _timer.setSingleShot(true);
}

void DisconnectChannelTask::timerSingleShot_timeout()
{
    if( _peercastType == Peercast::TYPE_ST ) {
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelStatus").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _nam.post(request, data);
    }
    else { // _peercastType==Peercast::TYPE_VP || _peercastType==Peercast::TYPE_UNKNOWN
        QUrl url(QString("http://%1:%2/admin?cmd=viewxml").arg(_host).arg(_port));
        _nam.get(QNetworkRequest(url));
    }
}

void DisconnectChannelTask::nam_finished(QNetworkReply* reply)
{
    if( reply->error() == QNetworkReply::NoError ) {
        QString out = reply->readAll();

        bool result = false;
        if( _peercastType == Peercast::TYPE_ST )
            result = getChannelStatusPcSt(out);
        else
            result = getChannelStatusPcVp(out);

        qDebug("DisconnectChannelTask::nam_finished(): result: %d, listeners: %d, retry: %d", result, _listeners, _retryGetStatus);
        if( result ) {
            if( _listeners == 0 )
                Task::push(new StopChannelTask(_host, _port, _id, parent()));
            else
            if( _retryGetStatus || _timer.isActive() ) {
                qDebug("--------------");
                QTimer::singleShot(REPETITION_MSEC, this, SLOT(timerSingleShot_timeout()));

                reply->deleteLater();
                return;
            }
        }
    }
    else {
        qDebug("DisconnectChannelTask::nam_finished(): reply error");
    }

    reply->deleteLater();
    deleteLater();
}

void DisconnectChannelTask::start()
{
    _timer.start(MONITORING_MSEC);
    QTimer::singleShot(_startSec * 1000, this, SLOT(timerSingleShot_timeout()));

    qDebug("DisconnectChannelTask::start(): peercast type %d", _peercastType);
}

bool DisconnectChannelTask::getChannelStatusPcVp(const QString& reply)
{
    int overIndex = reply.indexOf("<channels_found total=");
    if( overIndex == -1 )
        return false;

    int start, end;
    start = reply.indexOf(_id);
    if( start == -1 || start > overIndex )
        return false;

    start = reply.indexOf("<relay listeners=\"", start);
    start += 18;
    end = reply.indexOf('\"', start);

    _listeners = reply.mid(start, end - start).toInt();

    start = reply.indexOf("status=\"", start);
    start += 8;
    end = reply.indexOf('\"', start);

    QString status = reply.mid(start, end - start);
    _retryGetStatus = (status=="SEARCH" || status=="CONNECT");

    return true;
}

bool DisconnectChannelTask::getChannelStatusPcSt(const QString& reply)
{
    QScriptEngine engine;
    QScriptValue  value;
    value = engine.evaluate("JSON.parse").call(QScriptValue(), QScriptValueList() << reply);

    if( value.isError() )
        return false;

    value = value.property("result");
    if( !value.isValid() )
        return false;

    _listeners = value.property("localDirects").toString().toInt();

    QString status = value.property("status").toString();
    _retryGetStatus = (status=="Searching" || status=="Connecting");

    return true;
}

