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
#include "timelabel.h"
#include "commonlib.h"

TimeLabel::TimeLabel(QWidget* parent) : QLabel(parent)
{
    setAlignment(Qt::AlignRight);
    setTotalTime(0);
    setTime(0);
}

void TimeLabel::initMinimumWidth()
{
    QString s;

    if( _digitForHour )
        s.sprintf("%0*d:", _digitForHour, 0);

    s += "00:00";
    s += _totalTime;

    setMinimumWidth(fontMetrics().width(s) + 1);
}

void TimeLabel::setTotalTime(int sec)
{
    if( sec <= 0 ) {
        _totalTime.clear();
        _digitForHour = 0;
    }
    else {
        int h, m, s;
        CommonLib::secondTimeToHourMinSec(sec, &h, &m, &s);

        if( h )
            _totalTime.sprintf("/%d:%02d:%02d", h, m, s);
        else
            _totalTime.sprintf("/%02d:%02d", m, s);
    }

    initMinimumWidth();
}

void TimeLabel::updateText()
{
    int h, m, s;
    CommonLib::secondTimeToHourMinSec(_sec, &h, &m, &s);

    int digit;
    if( h )
        digit = CommonLib::digit(h);
    else
        digit = 0;

    if( digit != _digitForHour ) {
        _digitForHour = digit;
        initMinimumWidth();
    }

    if( _digitForHour )
        setText(QString().sprintf("%d:%02d:%02d", h, m, s) + _totalTime);
    else
        setText(QString().sprintf("%02d:%02d", m, s) + _totalTime);
}

