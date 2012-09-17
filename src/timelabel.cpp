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
#include "timelabel.h"

TimeLabel::TimeLabel(QWidget* parent) : QLabel(parent)
{
    setAlignment(Qt::AlignRight);
    setTotalTime(0);
    setTime(0);
}

void TimeLabel::initMinimumWidth()
{
    QString s;

    if( _visibleHour )
        s = " 00:00:00";
    else
        s = " 00:00";

    s += _totalTime;
//  s.append(" "); // 末尾スペース追加

    setMinimumWidth(fontMetrics().width(s));
}

void TimeLabel::setTotalTime(int sec)
{
    if( sec <= 0 ) {
        _totalTime.clear();
        _visibleHour = false;
    }
    else {
        int h, m, s;
        h = sec / 3600;
        sec %= 3600;
        m = sec / 60;
        s = sec % 60;

        _visibleHour = (bool)h;
        if( _visibleHour )
            _totalTime.sprintf("/%02d:%02d:%02d", h, m, s);
        else
            _totalTime.sprintf("/%02d:%02d", m, s);
    }

    initMinimumWidth();
}

void TimeLabel::updateText()
{
    int sec = _sec;
    int h, m, s;
    h = sec / 3600;
    sec %= 3600;
    m = sec / 60;
    s = sec % 60;

    if( h != _visibleHour ) {
        _visibleHour = (bool)h;
        initMinimumWidth();
    }

    if( _visibleHour )
        setText(QString().sprintf("%02d:%02d:%02d", h, m, s) + _totalTime);
    else
        setText(QString().sprintf("%02d:%02d", m, s) + _totalTime);
}

