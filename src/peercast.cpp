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
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScriptEngine>
#include <QXmlStreamReader>
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
    Task* task = new GetChannelInfoTask(_host, _port, _id, _type, this);
    connect(task, SIGNAL(finished(const ChannelInfo&)),
            this, SIGNAL(gotChannelInfo(const ChannelInfo&)));
    Task::push(task);
}

void Peercast::disconnectChannel_GetPeercastTypeTask_finished()
{
    Task::push(new DisconnectChannelTask(_host, _port, _id, _type, _disconnectStartSec, this));
}

void Peercast::nam_finished(QNetworkReply* reply)
{
    reply->deleteLater();
}

// ---------------------------------------------------------------------------------------
GetPeercastTypeTask::GetPeercastTypeTask(const QString& host, ushort port,
        Peercast::TYPE* pType, QObject* parent) : Task(parent)
{
    _host = host;
    _port = port;
    _pType = pType;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _attemptTypes << Peercast::TYPE_VP << Peercast::TYPE_ST;
    int i = _attemptTypes.indexOf(*_pType);
    if( i > 0 )
        _attemptTypes.move(i, 0);
}

void GetPeercastTypeTask::nam_finished(QNetworkReply* reply)
{
    bool failed = !(reply->error() == QNetworkReply::NoError);

    Q_ASSERT( !_attemptTypes.isEmpty() );
//  LogDialog::debug(QString("peercast type %1").arg(_attemptTypes.first()));

    if( !failed ) {
        QString out = reply->readAll();
        if( _attemptTypes.first() == Peercast::TYPE_ST )
            failed = !whetherPcSt(out);
        else
            failed = !whetherPcVp(out);

        if( !failed )
            *_pType = _attemptTypes.first();
    }

    if( failed ) {
        _attemptTypes.pop_front();
        if( _attemptTypes.isEmpty() )
            *_pType = Peercast::TYPE_UNKNOWN;
        else {
            queryPeercastType();
            reply->deleteLater();
            return;
        }
    }

    LogDialog::debug(QString("GetPeercastTypeTask::nam_finished(): peercast type %1").arg(*_pType));

    reply->deleteLater();
    deleteLater();
}

void GetPeercastTypeTask::start()
{
    queryPeercastType();
}

void GetPeercastTypeTask::queryPeercastType()
{
    if( _attemptTypes.isEmpty() ) return;

    if( _attemptTypes.first() == Peercast::TYPE_VP ) {
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

        // PeercastIMではviewxml返却時のヘッダに誤りがある為、その場合のエラーは通過させる
    if( reply->error() == QNetworkReply::NoError
     || (reply->error()==QNetworkReply::RemoteHostClosedError && _type!=Peercast::TYPE_ST) )
    {
        QString out = reply->readAll();

        if( reply == _replyChannelInfo ) {
            _replyChannelInfo = NULL;

            if( _type == Peercast::TYPE_ST ) {
                if( parseChannelInfoPcSt(out) ) {
                    getChannelStatusPcSt();
                    reply->deleteLater();
                    return;
                }
            }
            else { // _type==Peercast::TYPE_VP || _type==Peercast::TYPE_UNKNOWN
                if( parseChannelInfoPcVp(out) ) {
                    debugChannelInfo();
                    emit finished(_chInfo);
                }
            }
        }
        else
        if( reply == _replyChannelStatusPcSt ) {
            _replyChannelStatusPcSt = NULL;

            if( parseChannelStatusPcSt(out) ) {
                debugChannelInfo();
                emit finished(_chInfo);
            }
        }
    }
    else
        LogDialog::debug(debugPrefix + "reply error " + QString::number(reply->error()), QColor(255,0,0));

    reply->deleteLater();
    deleteLater();
}

void GetChannelInfoTask::start()
{
//  if( _type == Peercast::TYPE_UNKNOWN ) {
//      deleteLater();
//      return;
//  }

    if( _type == Peercast::TYPE_ST ) {
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelInfo").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _replyChannelInfo = _nam.post(request, data);
    }
    else { // _type==Peercast::TYPE_VP || _type==Peercast::TYPE_UNKNOWN
        QUrl url(QString("http://%1:%2/admin?cmd=viewxml").arg(_host).arg(_port));
        _replyChannelInfo = _nam.get(QNetworkRequest(url));
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

    QXmlStreamReader xml(reply);
    QString status;

    while( !xml.atEnd() ) {
        xml.readNext();
//      LogDialog::debug(QString("%1/%2").arg(xml.isStartElement()).arg(xml.name().toString()));

        if( xml.isStartElement() ) {
            if( xml.name() == "peercast" )
                ;
            else
            if( xml.name() == "channels_relayed" )
                ;
            else
            if( xml.name() == "channel" && xml.attributes().value("id") == _id ) {
                _chInfo.chName = xml.attributes().value("name").toString();
                _chInfo.contactUrl = xml.attributes().value("url").toString();
                _chInfo.bitrate = xml.attributes().value("bitrate").toString().toInt();

                // 不要
//              QTextDocument txt;
//              txt.setHtml(_chInfo.chName);
//              _chInfo.chName = txt.toPlainText();
            }
            else
            if( xml.name() == "relay" ) {
                status = xml.attributes().value("status").toString();
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

                break;
            }
            else
                xml.skipCurrentElement();
        }
    }

    if( xml.hasError() ) {
        LogDialog::debug(debugPrefix + xml.errorString());
        return false;
    }

    return !status.isNull();
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
    _chInfo.bitrate    = value.property("bitrate").toInt32();

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

    return true;
}

void GetChannelInfoTask::debugChannelInfo()
{
    const QString debugPrefix = "GetChannelInfoTask::debugChannelInfo(): ";

    LogDialog::debug(debugPrefix + "name " + _chInfo.chName);
    LogDialog::debug(debugPrefix + "url " + _chInfo.contactUrl);
    LogDialog::debug(debugPrefix + "bitrate " + QString::number(_chInfo.bitrate));
    LogDialog::debug(debugPrefix + "status " + _chInfo.statusString());
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
    _type = type;
    _startSec = startSec;
    _listeners = -1;
    _retryGetStatus = false;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _timer.setSingleShot(true);
}

void DisconnectChannelTask::timerSingleShot_timeout()
{
    if( _type == Peercast::TYPE_ST ) {
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelStatus").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _nam.post(request, data);
    }
    else { // _type==Peercast::TYPE_VP || _type==Peercast::TYPE_UNKNOWN
        QUrl url(QString("http://%1:%2/admin?cmd=viewxml").arg(_host).arg(_port));
        _nam.get(QNetworkRequest(url));
    }
}

void DisconnectChannelTask::nam_finished(QNetworkReply* reply)
{
        // PeercastIMではviewxml返却時のヘッダに誤りがある為、その場合のエラーは通過させる
    if( reply->error() == QNetworkReply::NoError
     || (reply->error()==QNetworkReply::RemoteHostClosedError && _type!=Peercast::TYPE_ST) )
    {
        QString out = reply->readAll();

        bool result = false;
        if( _type == Peercast::TYPE_ST )
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

    qDebug("DisconnectChannelTask::start(): peercast type %d", _type);
}

bool DisconnectChannelTask::getChannelStatusPcVp(const QString& reply)
{
    QXmlStreamReader xml(reply);
    _listeners = -1;

    while( !xml.atEnd() ) {
        xml.readNext();
//      qDebug() << QString("%1/%2").arg(xml.isStartElement()).arg(xml.name().toString());

        if( xml.isStartElement() ) {
            if( xml.name() == "peercast" )
                ;
            else
            if( xml.name() == "channels_relayed" )
                ;
            else
            if( xml.name() == "channel" && xml.attributes().value("id") == _id )
                ;
            else
            if( xml.name() == "relay" ) {
                _listeners = xml.attributes().value("listeners").toString().toInt();
                QStringRef status = xml.attributes().value("status");
                _retryGetStatus = (status=="SEARCH" || status=="CONNECT");
                break;
            }
            else
                xml.skipCurrentElement();
        }
    }

    if( xml.hasError() ) {
        qDebug() << "DisconnectChannelTask::getChannelStatusPcVp():" << xml.errorString();
        return false;
    }

    return _listeners != -1;
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

