/*  Copyright (C) 2015 nel

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
#include "mousecursor.h"
#include "logdialog.h"

MouseCursor::MouseCursor(QObject* parent) : QObject(parent)
{
    _widget = NULL;
    _pressGlobalPos = QPoint(INT_MIN, INT_MIN);

    _timerDisableChangeShape.setSingleShot(true);
    _timerDisableChangeShape.setInterval(500);

    _startedAutoHide = false;
    _timerHideCursor.setInterval(500);
    connect(&_timerHideCursor, SIGNAL(timeout()), this, SLOT(timerHideCursor_timeout()));
}

MouseCursor::~MouseCursor()
{
}

void MouseCursor::setTargetWidget(QWidget* w)
{
    if( w == NULL || (_widget != NULL && _widget != w) ) {
        _widget->removeEventFilter(this);
    }

    _widget = w;
    if( _widget != NULL )
        _widget->installEventFilter(this);
}

bool MouseCursor::setVisible(bool b)
{
    if( _timerDisableChangeShape.isActive() ) {
        _timerDisableChangeShape.stop();
        return false;
    }

    if( _timerHideCursor.isActive() )
        _timerHideCursor.start();

    Qt::CursorShape old = _widget->cursor().shape();

    if( b )
        _widget->unsetCursor();
    else
        _widget->setCursor(QCursor(Qt::BlankCursor));

    if( _widget->cursor().shape() != old ) {
        _pressGlobalPos = QPoint(INT_MIN, INT_MIN);   // この関数の任意呼出し後のmouseEvent処理を無効化
        emit changedShape(_widget->cursor().shape());

        return true;
    }

    return false;
/*
    if( ( b && _widget->cursor().shape() == Qt::BlankCursor)
     || (!b && _widget->cursor().shape() != Qt::BlankCursor) )
    {
        if( b )
            _widget->unsetCursor();
        else
            _widget->setCursor(QCursor(Qt::BlankCursor));

        _pressGlobalPos = QPoint(INT_MIN, INT_MIN);   // この関数の任意呼出し後のmouseEvent処理を無効化
        emit changedShape(_widget->cursor().shape());

        return true;
    }

    return false;
*/
}

void MouseCursor::timerHideCursor_timeout()
{
    QPoint pos = _widget->mapFromGlobal(QCursor::pos());
    if( _widget->rect().contains(pos) )
        setVisible(false);

    _timerHideCursor.stop();
}

void MouseCursor::mousePressEvent(QWidget*, const QMouseEvent* e)
{
    _timerHideCursor.stop();

    if( e->button() == Qt::LeftButton ) {
        _pressGlobalPos = e->globalPos();
    }
    else
    if( e->button() == Qt::MidButton ) {
        setVisible(true);
    }
}

void MouseCursor::mouseReleaseEvent(QWidget*, const QMouseEvent* e)
{
    if( _startedAutoHide ) _timerHideCursor.start();

    if( e->button() == Qt::LeftButton ) {
        if( _pressGlobalPos == e->globalPos() ) {
            if( setVisible(isHidden()) )
                _timerHideCursor.stop();
        }
    }
    else
    if( e->button() == Qt::RightButton ) {
        setVisible(true);
    }
}

void MouseCursor::mouseDoubleClickEvent(QWidget*, const QMouseEvent*)
{
    _timerHideCursor.stop();
}

void MouseCursor::mouseMoveEvent(QWidget*, const QMouseEvent* e)
{
    if( _startedAutoHide && e->buttons() == Qt::NoButton )
        _timerHideCursor.start();

    if( _widget->window()->isFullScreen() ) {
        if( _pressGlobalPos != e->globalPos() )
            setVisible(true);
    }
    else {
        setVisible(true);
    }
}

bool MouseCursor::eventFilter(QObject* o, QEvent* e)
{
    if( o == _widget ) {
        if( e->type() == QEvent::MouseButtonPress )
            mousePressEvent((QWidget*)o, (QMouseEvent*)e);
        else
        if( e->type() == QEvent::MouseButtonRelease )
            mouseReleaseEvent((QWidget*)o, (QMouseEvent*)e);
        else
        if( e->type() == QEvent::MouseButtonDblClick )
            mouseDoubleClickEvent((QWidget*)o, (QMouseEvent*)e);
        else
        if( e->type() == QEvent::MouseMove )
            mouseMoveEvent((QWidget*)o, (QMouseEvent*)e);
    }

    return QObject::eventFilter(o, e);
}

