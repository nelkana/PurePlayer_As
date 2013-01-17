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
#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>
#include <QString>

class CommonProcess : public QProcess
{
    Q_OBJECT

public:
    CommonProcess(QObject* parent);
    virtual ~CommonProcess() {}

signals:
    void outputLine(const QString& line);

private slots:
    void slot_readyReadStandardOutput();

private:
    QString _outputTempBuff;
};

class MplayerProcess : public CommonProcess
{
    Q_OBJECT

public:
    MplayerProcess(QObject* parent);
    void start(const QString& program, const QStringList& arguments, OpenMode mode=ReadWrite);
    void receiveMplayerChildProcess();
    void command(const QString& command) { write(command.toLocal8Bit() + "\n"); }

    bool terminateWaitForFinished();

signals:
    void finished();
    void debugKilledCPid();     // debug

private slots:
    void slot_finished(int, QProcess::ExitStatus);

private:
    QString _path;
    Q_PID   _mplayerCPid;
};

inline void MplayerProcess::start(const QString& program, const QStringList& arguments,
                                  OpenMode mode)
{
    _path = program;
    CommonProcess::start(program, arguments, mode);
}

class RecordingProcess : public CommonProcess
{
    Q_OBJECT

public:
    RecordingProcess(QObject* parent);
};

#endif // PROCESS_H

