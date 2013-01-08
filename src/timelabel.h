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
#ifndef TIMELABEL_H
#define TIMELABEL_H

#include <QLabel>

class TimeLabel : public QLabel
{
    Q_OBJECT

public:
    TimeLabel(QWidget* parent=0);

    void initMinimumWidth();
    void setTotalTime(int sec);
    void setTime(int sec) { _sec = sec; updateText(); }
    int  time()           { return _sec; }
    QString& totalTimeText() { return QString(_totalTime).remove(0,1); }

protected:
    void updateText();

private:
    int     _sec;
    QString _totalTime;
    bool    _visibleHour;
};

#endif // TIMELABEL_H

