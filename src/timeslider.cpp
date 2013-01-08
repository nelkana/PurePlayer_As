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
#include <QMouseEvent>
#include "timeslider.h"
#include "logdialog.h"

#define REQUEST_SEEK_CYCLE_TIME 100 // msec
#define UNABLE_SETPOSITION_TIME 400 // msec

TimeSlider::TimeSlider(QWidget* parent) : QSlider(parent)
{
    setLength(100);
    setOrientation(Qt::Horizontal);
    setPageStep(0); // 0に設定するとwheelイベントはそのまま親へ伝搬される

    connect(this, SIGNAL(sliderPressed()),  this, SLOT(timeSliderPressed()));
    connect(this, SIGNAL(sliderReleased()), this, SLOT(timeSliderReleased()));
    connect(this, SIGNAL(sliderMoved(int)), this, SLOT(timeSliderMoved(int)));

    _movedPos = -1;
    _unableSetPosition = false;

    connect(&_timerRequestSeek, SIGNAL(timeout()),
            this,               SLOT(timerRequestSeekTimeout()));

    _timerUnableSetPosition.setSingleShot(true);
    connect(&_timerUnableSetPosition, SIGNAL(timeout()),
            this,                     SLOT(timerUnableSetPositionTimeout()));
}

void TimeSlider::setPosition(double sec)
{
    if( !_unableSetPosition ) {
        setValue(sec * 10);
//      LogDialog::debug("TimeSlider::setPosition(): sec " + QString::number(sec));
    }
}

bool TimeSlider::event(QEvent* e)
{
//  LogDialog::debug(tr("TimeSlider::event(): %1").arg(e->type()));

    if( e->type() == QEvent::MouseButtonPress
     || e->type() == QEvent::MouseButtonDblClick )
    {
        QMouseEvent* event = static_cast<QMouseEvent*>(e);
        if( event->button() == Qt::RightButton )
            return false;
        else
        if( event->button() == Qt::LeftButton ) {
            if( pointOutsideHandle(event->pos()) ) {
                QMouseEvent mevent(QEvent::MouseButtonPress,
                                   event->pos(),
                                   Qt::MidButton, Qt::MidButton, Qt::NoModifier);

                return QSlider::event(&mevent);
            }
        }
    }
    else
    if( e->type() == QEvent::MouseMove ) {
        if( !isSliderDown() )
            return true;
    }

    bool b = QSlider::event(e);
//  LogDialog::debug(tr("%1").arg(b));
    return b;
}
/*
void TimeSlider::mousePressEvent(QMouseEvent* e)
{
    LogDialog::debug("press");
    QSlider::mousePressEvent(e);
}

void TimeSlider::mouseMoveEvent(QMouseEvent* e)
{
    LogDialog::debug("move");
    QSlider::mouseMoveEvent(e);
}
*/

void TimeSlider::wheelEvent(QWheelEvent* e)
{
    if( e->delta() < 0 )
        emit requestSeek(-3, true);
    else
        emit requestSeek(+3, true);
}

bool TimeSlider::pointOutsideHandle(const QPoint& pos)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderHandle, this);

    bool b = !handleRect.contains(pos);

/*  LogDialog::debug(QString().sprintf(
                    "TimerSlider::pointOutsideHandle(): handle %d %d %dx%d %s",
                    handleRect.x(),handleRect.y(),handleRect.width(),handleRect.height(),
                    (b ? "true" : "false")));
*/
    return b;
}

void TimeSlider::timeSliderPressed()
{
    _unableSetPosition = true;
//  LogDialog::debug("TimeSlider::timeSliderPressed():");
}

void TimeSlider::timeSliderReleased()
{
//  LogDialog::debug("TimeSlider::timeSliderReleased():");
//    _unableSetPosition = true;
    timeSliderMoved(value());
}

void TimeSlider::timeSliderMoved(int pos)
{
    if( !_timerRequestSeek.isActive() )
        _timerRequestSeek.start(REQUEST_SEEK_CYCLE_TIME);

    _movedPos = pos;

//  LogDialog::debug("TimeSlider::timeSliderMoved(): pos " + QString::number(pos));
}
/*
void TimeSlider::timerRequestSeekTimeout()
{
    if( !isSliderDown() && _movedPos < 0 ) {
        if( _movedPos == -1 ) {
            _unableSetPosition = false;
            _timerRequestSeek.stop();
        }
        else
            _movedPos++;
    }

    if( _movedPos >= 0 ) {
//      emit requestSeek(_movedPos*100/(double)maximum(), false); // パーセントによる指定
        emit requestSeek((double)_movedPos/10, false);            // 時間による直接指定
        _movedPos = -UNABLE_SETPOSITION_TIME / REQUEST_SEEK_CYCLE_TIME;
    }

//  LogDialog::debug("timer " + QString::number(_movedPos));
}
*/
void TimeSlider::timerRequestSeekTimeout()
{
    if( _movedPos >= 0 ) {
//      emit requestSeek(_movedPos*100/(double)maximum(), false); // パーセントによる指定
        emit requestSeek((double)_movedPos/10, false);            // 時間による直接指定
        _movedPos = -1;

        _timerUnableSetPosition.start(UNABLE_SETPOSITION_TIME);
    }

//    LogDialog::debug("TimeSlider::timerRequestSeekTimeout(): movedPos "
//                        + QString::number(_movedPos));
}

void TimeSlider::timerUnableSetPositionTimeout()
{
    if( !isSliderDown() && _movedPos < 0 ) {
        _unableSetPosition = false;
        _timerRequestSeek.stop();
    }

//    LogDialog::debug("TimeSlider::timerUnableSetPositionTimeout():");
}

