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
#ifndef TIMESLIDER_H
#define TIMESLIDER_H

#include <QSlider>
#include <QTimer>

class TimeSlider : public QSlider
{
    Q_OBJECT

public:
    TimeSlider(QWidget* parent);

    void setLength(double sec) { setRange(0, sec*10); }
    void setPosition(double sec);

signals:
    void requestSeek(double pos, bool relative);

protected:
    bool event(QEvent*);
//    void mousePressEvent(QMouseEvent*);
//    void mouseMoveEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    bool pointOutsideHandle(const QPoint&);

private slots:
    void timeSliderPressed();
    void timeSliderReleased();
    void timeSliderMoved(int);
    void timerRequestSeekTimeout();
    void timerUnableSetPositionTimeout();

private:
    QTimer _timerRequestSeek;
    QTimer _timerUnableSetPosition;
    int    _movedPos;
    bool   _unableSetPosition;
};

#endif // TIMESLIDER_H

