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
#ifndef __COMMONLIB_H
#define __COMMONLIB_H

#include <QRect>

namespace CommonLib
{
extern const char* const QSETTINGS_ORGNAME;

double round(double value, int precision);
void   msleep(unsigned long msec);

QString dayOfWeek(int day);
QString secondTimeToString(int sec);
QString removeSpaceBeforeAfter(QString str);
QRect   scaleRectOnRect(const QSize& baseRect, const QSize& placeRect);
QString retTheFileNameNotExists(const QString& requestFileName);
QString getOpenFileNameDialog(QWidget* parent=0, const QString& caption=QString());
//QSize   widgetFrameSize(QWidget* const);

};

#endif // __COMMONLIB_H

