/*  Copyright (C) 2012-2014 nel

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
#include "process.h"
#include "logdialog.h"

#ifdef Q_OS_WIN32
#include <QTextCodec>
#endif

CommonProcess::CommonProcess(QObject* parent) : QProcess(parent)
{
    setProcessChannelMode(QProcess::MergedChannels);

    connect(this, SIGNAL(readyReadStandardOutput()),
            this, SLOT(slot_readyReadStandardOutput()));
}

void CommonProcess::slot_readyReadStandardOutput()
{
#ifdef Q_OS_WIN32
    QByteArray ba = readAllStandardOutput();
    QString out = QTextCodec::codecForName("SJIS")->toUnicode(ba);

    out.replace("\r\n", "\n");
#else
    QString out = readAllStandardOutput();
#endif

    out = _outputTempBuff + out.replace('\r', '\n');
    QStringList lines = out.split("\n");

    if( out[out.size()-1] != '\n' )
        _outputTempBuff = lines.last();
    else
        _outputTempBuff = "";

    lines.removeLast();

    for(int i=0; i < lines.size(); ++i)
        emit outputLine(lines[i]);
}

// ---------------------------------------------------------------------------------------
MplayerProcess::MplayerProcess(QObject* parent) : CommonProcess(parent)
{
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slot_finished(int, QProcess::ExitStatus)));

    _mplayerCPid = 0;
}

void MplayerProcess::receiveMplayerChildProcess()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    const QString debugPrefix = "MplayerProcess::receiveMplayerChildProcess(): ";
    // mplayerプロセスの子プロセスIDを取得する。mplayerバグ対策用
    QProcess p;
//  p.start("pgrep", QStringList() << "-P" << QString("%1").arg(pid()));
    p.start("ps", QStringList() << "-o" << "pid" << "--no-headers"
                                << QString("--ppid=%1").arg(pid()));

    if( p.waitForFinished() ) {
        QString cpid = p.readLine();
        _mplayerCPid = cpid.toInt();
    }
    else
        _mplayerCPid = 0;

    LogDialog::debug(debugPrefix + QString("pid %1").arg(pid()));
    LogDialog::debug(debugPrefix + QString("cpid %1").arg(_mplayerCPid));
#endif
}

bool MplayerProcess::terminateWaitForFinished()
{
    const QString debugPrefix = "MplayerProcess::terminateWaitForFinished(): ";
    LogDialog::debug(debugPrefix + "called");

//  closeReadChannel(QProcess::StandardOutput);

    QProcess::ProcessState status = state();
    if( status == QProcess::Running ) {
        command("quit");
        if( waitForFinished(1000) )
            return true;
    }
    else
    if( status == QProcess::NotRunning ) {
        LogDialog::debug(debugPrefix + "process not running");
        return false;
    }
    else
        LogDialog::debug(debugPrefix + "process starting", QColor(0,255,0));

    LogDialog::debug(debugPrefix + "terminate()", QColor(255,0,0));
    terminate();
    bool ret = waitForFinished(3000);
    if( !ret ) {
        LogDialog::debug(debugPrefix + "kill()", QColor(255,0,0));
        kill();
        ret = waitForFinished(2000);
    }

    return ret;
}

void MplayerProcess::slot_finished(int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
{
    const QString debugPrefix = "MplayerProcess::slot_finished(): ";
//  LogDialog::debug(debugPrefix + "start-"));

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    // mplayer子プロセスが残ってる場合終了させる
    if( _mplayerCPid != 0 ) {
        QProcess p;
        p.start("sh", QStringList() << "-c"
                                    << QString("ps --no-headers p %1|grep -F '%2'")
                                                    .arg(_mplayerCPid).arg(_path));
        if( p.waitForFinished() ) {
            QString s = p.readAllStandardOutput();
            LogDialog::debug(debugPrefix + "ps cpid " + s);

            if( !s.isEmpty() ) { // 子プロセスが存在しているなら
                p.start("kill", QStringList() << QString("%1").arg(_mplayerCPid));
                p.waitForFinished();
                LogDialog::debug(debugPrefix + QString("terminated cpid %1 -------------")
                                                    .arg(_mplayerCPid), QColor(255,0,0));

#ifndef QT_NO_DEBUG_OUTPUT
                emit debugKilledCPid();
#endif // QT_NO_DEBUG_OUTPUT

            }
        }

        _mplayerCPid = 0;
    }
#endif // defined(Q_OS_LINUX) || defined(Q_OS_MAC)

    emit finished();

//  LogDialog::debug(debugPrefix + "-end"));
}

// ---------------------------------------------------------------------------------------
RecordingProcess::RecordingProcess(QObject* parent) : CommonProcess(parent)
{
}

