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
#include <math.h>
#include <QThread>
#include <QFileInfo>    // retTheFileNameNotExists()
#include "commonlib.h"

class MyThread : public QThread
{
    Q_OBJECT;

public:
    static void msleep(unsigned long msec) { QThread::msleep(msec); }
};

// valueを四捨五入する。
// 小数点以下precision桁の値を返す。
double CommonLib::round(double value, int precision)
{
	return (int)(value * pow(10, precision) + 0.5+(value<0 ? -1:0)) / pow(10, precision);
}

void CommonLib::msleep(unsigned long msec)
{
    MyThread::msleep(msec);
}

// 要求したファイル名に基づいて、現在のディレクトリにおける重複しないファイル名を返す。
// ファイルが既に有る場合、ファイル名に連番が付加される。
QString CommonLib::retTheFileNameNotExists(const QString& requestFileName)
{
    QFileInfo file(requestFileName);
    QString baseName = file.baseName();
    QString suffix   = file.suffix();
    if( !suffix.isEmpty() )
        suffix = "." + suffix;

    unsigned long num = 1;
    while( file.exists() || file.isSymLink() ) {
        num++;
//      if( num > limitNum ) break;

        file.setFile(QString("%1_%2%3").arg(baseName).arg(num).arg(suffix));
    }

//  if( num > limitNum )
//      return QString();
//  else
        return file.fileName();
}

