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
#include <QScriptEngine>
#include <QDebug>
#include "task.h"
#include "commonlib.h"

QList<Task*> Task::s_tasks;

Task::Task(QObject* parent) : QObject(parent)
{
    s_tasks.append(this);

    _pFinished = NULL;
}

void Task::waitForFinished()
{
    while( s_tasks.size() ) {
        QCoreApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        CommonLib::msleep(1);
    }
}

void Task::waitForFinished(Task* task)
{
    bool finished = false;
    task->_pFinished = &finished;

    while( !finished ) {
        QCoreApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        CommonLib::msleep(1);
    }
}

void Task::deleteObject()
{
    s_tasks.removeOne(this);
    if( _pFinished != NULL )
        *_pFinished = true;

    deleteLater();
}

// ---------------------------------------------------------------------------------------
RenameFileTask::RenameFileTask(const QString& file, const QString& newName,
        QObject* parent) : Task(parent)
{
    _file = file;
    _newName = newName;

    QTimer::singleShot(1500, this, SLOT(timerSingleShot_timeout()));
}

void RenameFileTask::timerSingleShot_timeout()
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
GetPeercastTypeTask::GetPeercastTypeTask(const QString& host, short port,
        PurePlayer::PEERCAST_TYPE* type, QObject* parent) : Task(parent)
{
    _host = host;
    _port = port;
    _type = type;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _attemptType = PurePlayer::PCT_VP;
    queryPeercastType();
}

void GetPeercastTypeTask::nam_finished(QNetworkReply* reply)
{
    bool error = !(reply->error() == QNetworkReply::NoError);

    if( !error ) {
        QString out = reply->readAll();
        if( _attemptType == PurePlayer::PCT_VP )
            error = !whetherPcVp(out);
        else
            error = !whetherPcSt(out);

        if( !error )
            *_type = _attemptType;
    }

    if( error ) {
        if( _attemptType == PurePlayer::PCT_VP ) {
            _attemptType = PurePlayer::PCT_ST;
            queryPeercastType();
            reply->deleteLater();
            return;
        }
        else
            *_type = PurePlayer::PCT_UNKNOWN;
    }

    reply->deleteLater();
    deleteObject();
}

void GetPeercastTypeTask::queryPeercastType()
{
    if( _attemptType == PurePlayer::PCT_VP ) {
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
StopChannelTask::StopChannelTask(const QString& host, short port, const QString& id,
        QObject* parent) : Task(parent)
{
    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    QUrl url(QString("http://%1:%2/admin?cmd=stop&id=%3").arg(host).arg(port).arg(id));
    _nam.get(QNetworkRequest(url));
}

void StopChannelTask::nam_finished(QNetworkReply* reply)
{
    reply->deleteLater();
    deleteObject();
}

// ---------------------------------------------------------------------------------------
DisconnectChannelTask::DisconnectChannelTask(const QString& host, short port,
    const QString& id, PurePlayer::PEERCAST_TYPE type, int startSec, QObject* parent)
        : Task(parent)
{
    _host = host;
    _port = port;
    _id   = id;
    _peercastType = type;
    _listeners = -1;
    _retryGetStatus = false;

    connect(&_nam, SIGNAL(finished(QNetworkReply*)),
            this,  SLOT(nam_finished(QNetworkReply*)));

    _timer.setSingleShot(true);
    _timer.start(MONITORING_MSEC);

    QTimer::singleShot(startSec * 1000, this, SLOT(timerSingleShot_timeout()));

    qDebug("DisconnectChannelTask::DisconnectChannelTask(): peercast type %d", _peercastType);
}

void DisconnectChannelTask::timerSingleShot_timeout()
{
    if( _peercastType == PurePlayer::PCT_ST ) {
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelStatus").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _nam.post(request, data);
    }
    else { // _peercastType==PurePlayer::PCT_VP || _peercastType==PurePlayer::PCT_UNKNOWN
        QUrl url(QString("http://%1:%2/admin?cmd=viewxml").arg(_host).arg(_port));
        _nam.get(QNetworkRequest(url));
    }
}

void DisconnectChannelTask::nam_finished(QNetworkReply* reply)
{
    if( reply->error() == QNetworkReply::NoError ) {
        QString out = reply->readAll();

        bool result = false;
        if( _peercastType == PurePlayer::PCT_ST )
            result = getChannelStatusPcSt(out);
        else
            result = getChannelStatusPcVp(out);

        qDebug("DisconnectChannelTask::nam_finished(): result: %d, listeners: %d, retry: %d", result, _listeners, _retryGetStatus);
        if( result ) {
            if( _listeners == 0 )
                new StopChannelTask(_host, _port, _id, parent());
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
    deleteObject();
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

