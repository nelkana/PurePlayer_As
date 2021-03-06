/*  Copyright (C) 2012-2015 nel

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
#ifndef __COMMONLIB_H
#define __COMMONLIB_H

#include <QRect>

namespace CommonLib
{
extern const char* const QSETTINGS_ORGNAME;
extern const char* const MEDIA_FORMATS;

double round(double value, int precision);
int    digit(int value);
int    rand(int min, int max);
void   msleep(ulong msec);

QString dayOfWeek(int day);
void    secondTimeToHourMinSec(int sec, int* h, int* m, int* s);
QString secondTimeToString(int sec);
QString removeSpaceBeforeAfter(QString str);
QRect   clipRect(const QRect& target, QRect clip);
QRect   scaleRectOnRect(const QSize& baseRect, const QSize& placeRect);
QString convertStringForFileName(QString name);
QString retTheFileNameNotExists(const QString& requestFileName);
QString getOpenFileNameDialog(QWidget* parent=0, const QString& caption=QString());
QStringList getOpenFileNamesDialog(QWidget* parent=0, const QString& caption=QString());
QString getExistingDirectoryDialog(QWidget* parent=0, const QString& caption=QString());
bool    isPeercastUrl(const QString& url);
bool    isHttpUrl(const QString& url);
//QSize   widgetFrameSize(QWidget* const);

// QListから重複する項目を削除する
template <class T>
void removeDuplicateQList(T& list)
{
    if( list.isEmpty() ) return;

    for(int i=0; i < list.count()-1; ++i) {
        int index = i + 1;
        while( 1 ) {
            index = list.indexOf(list[i], index);
            if( index == -1 )
                break;

            list.removeAt(index);
        }
    }
}

class EmitDeterFlag
{
public:
    EmitDeterFlag() { _state = END; }

    void begin() { _state = BEGIN; }
    void end()   { _state |= END; }
    bool toEmit() { _state |= EMITTED; return (_state & END); }

    bool emitted() { return (_state & EMITTED); }

private:
    enum { BEGIN = 0x0, END = 0x1, EMITTED = 0x2 };
    qint8 _state;
};

};

#endif // __COMMONLIB_H

