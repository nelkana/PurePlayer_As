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
#include "process.h"
#include "logdialog.h"

#ifdef Q_OS_WIN32
#include <QTextCodec>
#endif

CommonProcess::CommonProcess(QObject* parent) : QProcess(parent)
{
    setProcessChannelMode(QProcess::MergedChannels);

    connect(this, SIGNAL(readyReadStandardOutput()),
            this, SLOT(slotReadyReadStandardOutput()));
}

void CommonProcess::slotReadyReadStandardOutput()
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

    for(int i=0; i < lines.size(); i++)
        emit outputLine(lines[i]);
}

// ---------------------------------------------------------------------------------------
MplayerProcess::MplayerProcess(QObject* parent) : CommonProcess(parent)
{
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotFinished(int, QProcess::ExitStatus)));

    _mplayerCPid = 0;
}

void MplayerProcess::receiveMplayerChildProcess()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
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

    LogDialog::debug("MplayerProcess::receiveMplayerChildProcess(): pid " +
                                                        QString("%1 ").arg(pid()));
    LogDialog::debug("MplayerProcess::receiveMplayerChildProcess(): cpid " +
                                                        QString("%1 ").arg(_mplayerCPid));
#endif
}

void MplayerProcess::slotFinished(int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
{
//  LogDialog::debug(QString("MplayerProcess::slotFinished(): start-"));

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    // mplayer子プロセスが残ってる場合終了させる
    if( _mplayerCPid != 0 ) {
        QProcess p;
        p.start("sh", QStringList() << "-c"
                                    << QString("ps --no-headers p %1|grep mplayer")
                                                                    .arg(_mplayerCPid));
        if( p.waitForFinished() ) {
            QString s = p.readAllStandardOutput();
            LogDialog::debug("MplayerProcess::slotFinished(): ps cpid " + s);

            if( !s.isEmpty() ) { // 子プロセスが存在しているなら
                p.start("kill", QStringList() << QString("%1").arg(_mplayerCPid));
                p.waitForFinished();
                LogDialog::debug("MplayerProcess::slotFinished(): terminated cpid " +
                                       QString("%1").arg(_mplayerCPid), QColor(255,0,0));

#ifndef QT_NO_DEBUG_OUTPUT
                emit debugKilledCPid();
#endif // QT_NO_DEBUG_OUTPUT

            }
        }

        _mplayerCPid = 0;
    }
#endif // defined(Q_OS_LINUX) || defined(Q_OS_MAC)

    emit finished();

//  LogDialog::debug(QString("MplayerProcess::slotFinished(): -end"));
}

// ---------------------------------------------------------------------------------------
RecordingProcess::RecordingProcess(QObject* parent) : CommonProcess(parent)
{
}

