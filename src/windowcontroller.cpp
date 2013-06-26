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
#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QRubberBand>
#include <QMouseEvent>
#include <QDebug>
#include "windowcontroller.h"

WindowController::WindowController(QObject* parent) : QObject(parent)
{
    _pressPosFromWindowTopLeft = QPoint(INT_MIN, INT_MIN);
    _pressedEdgeFlags = PE_NO_PRESS;
    _resizeEnabled = false;
    _useRubberBand = false;
    _resizeMargin = 4;

    _rubberBand = NULL;
}

WindowController::~WindowController()
{
    delete _rubberBand;
}

void WindowController::setSimpleResizeView(bool b)
{
    _useRubberBand = b;
    if( _useRubberBand && !_rubberBand )
        _rubberBand = new QRubberBand(QRubberBand::Rectangle);
}

void WindowController::mousePressEvent(QWidget* w, const QMouseEvent* e)
{
    if( e->button() == Qt::LeftButton ) {
        QPoint pressLocalPos = e->pos();
        QSize  frameSize(w->geometry().x() - w->frameGeometry().x(),
                         w->geometry().y() - w->frameGeometry().y());

        // ウィンドウ左上座標からのプレス位置を保持。
            // ウィンドウ位置の値は信頼性が低い為、
            // フレームサイズに基づく計算で求める(e->globalPos()-w->pos()で求めない)。
        _pressPosFromWindowTopLeft.setX(pressLocalPos.x() + frameSize.width());
        _pressPosFromWindowTopLeft.setY(pressLocalPos.y() + frameSize.height());

        if( _resizeEnabled ) {
            // クライアント右下座標からのプレス位置を保持
            _pressPosFromClientBottomRight.setX(w->width()-1 - pressLocalPos.x());
            _pressPosFromClientBottomRight.setY(w->height()-1 - pressLocalPos.y());

            // クライアント矩形情報を保持する。
            // mouseMoveEvent()内で逐一ジオメトリを取得して実装したら、何故か左側上側から
            // リサイズした時ウィンドウの位置がずれる現象が起きた為、情報を自前で保持する。
            _rect = w->geometry();

            // リサイズエリアのどこをプレスしたか取得
            _pressedEdgeFlags = retPressedEdgeFlags(w, pressLocalPos);

            if( _useRubberBand && !_pressedEdgeFlags.testFlag(PE_NO_PRESS) ) {
                _rubberBand->setGeometry(_rect);
                _rubberBand->show();
            }

//          qDebug() << w->frameGeometry();
//          qDebug() << _pressPosFromWindowTopLeft << _pressPosFromClientBottomRight << _pressedEdgeFlags;
        }
        else {
            _pressedEdgeFlags = PE_NO_PRESS;
        }

//      w->setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
    }
}

void WindowController::mouseMoveEvent(QWidget* w, const QMouseEvent* e)
{
    if( _resizeEnabled )
    {
        if( e->buttons() & Qt::LeftButton
            && _pressPosFromWindowTopLeft != QPoint(INT_MIN, INT_MIN) )
        {
            if( _pressedEdgeFlags.testFlag(PE_NO_PRESS) ) {
                QPoint pos = e->globalPos() - _pressPosFromWindowTopLeft;
#ifdef Q_WS_X11
//              if( pos.x() < 0 )
//                  w->setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
#endif
                w->move(pos);
            }
            else {
//              qDebug() << w->geometry() << w->pos().x() << w->pos().y()
//                  << w->pos().x()+w->width()-1 << w->pos().y()+w->height()-1;

                QRect availableDesktopRect = QApplication::desktop()->availableGeometry();

                if( _pressedEdgeFlags.testFlag(PE_TOP) ) {
                    int minimumY = availableDesktopRect.top();
                    int maximumY = _rect.bottom() - (w->minimumHeight()-1);

                    int y = e->globalPos().y() - _pressPosFromWindowTopLeft.y();
                    if( y < minimumY ) y = minimumY; else
                    if( y > maximumY ) y = maximumY;

                    _rect.setTop(y);
                }
                else
                if( _pressedEdgeFlags.testFlag(PE_BOTTOM) ) {
                    int minimumY = _rect.top() + (w->minimumHeight()-1);
                    int maximumY = availableDesktopRect.bottom();

                    int y = e->globalPos().y() + _pressPosFromClientBottomRight.y();
                    if( y < minimumY ) y = minimumY; else
                    if( y > maximumY ) y = maximumY;

                    _rect.setBottom(y);
                }

                if( _pressedEdgeFlags.testFlag(PE_LEFT) ) {
                    int minimumX = availableDesktopRect.left();
                    int maximumX = _rect.right() - (w->minimumWidth()-1);

                    int x = e->globalPos().x() - _pressPosFromWindowTopLeft.x();
                    if( x < minimumX ) x = minimumX; else
                    if( x > maximumX ) x = maximumX;

                    _rect.setLeft(x);
                }
                else
                if( _pressedEdgeFlags.testFlag(PE_RIGHT) ) {
                    int minimumX = _rect.left() + (w->minimumWidth()-1);
                    int maximumX = availableDesktopRect.right();

                    int x = e->globalPos().x() + _pressPosFromClientBottomRight.x();
                    if( x < minimumX ) x = minimumX; else
                    if( x > maximumX ) x = maximumX;

                    _rect.setRight(x);
                }
#ifdef Q_WS_X11
//              if( _rect.x() < 0 )
//                  w->setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
#endif
                // wの親の座標系にジオメトリを変換
                // w->parent!=NULLならw->parentの座標系にマッピングする処理をここに書く

                if( _useRubberBand )
                    _rubberBand->setGeometry(_rect);
                else
                    w->setGeometry(_rect);

//              qDebug() << _rect;
//              qDebug() << w->geometry() << w->pos().x() << w->pos().y()
//                  << w->pos().x()+w->width()-1 << w->pos().y()+w->height()-1;
            }
        }
        else
        if( e->buttons() == Qt::NoButton ) {
            // マウスカーソルの形状設定
            int flags = retPressedEdgeFlags(w, e->pos());
            switch( flags ) {
            case PE_TOP:
            case PE_BOTTOM:
                w->setCursor(Qt::SizeVerCursor);
                break;
            case PE_LEFT:
            case PE_RIGHT:
                w->setCursor(Qt::SizeHorCursor);
                break;
            case PE_TOP|PE_LEFT:
            case PE_BOTTOM|PE_RIGHT:
                w->setCursor(Qt::SizeFDiagCursor);
                break;
            case PE_TOP|PE_RIGHT:
            case PE_BOTTOM|PE_LEFT:
                w->setCursor(Qt::SizeBDiagCursor);
                break;
            default:
                w->setCursor(Qt::ArrowCursor);
            }
        }
    }
    else {
        if( e->buttons() & Qt::LeftButton
            && _pressPosFromWindowTopLeft != QPoint(INT_MIN, INT_MIN) )
        {
            QPoint pos = e->globalPos() - _pressPosFromWindowTopLeft;
#ifdef Q_WS_X11
//          if( pos.x() < 0 )
//              w->setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
#endif
            w->move(pos);
        }
    }
}

void WindowController::mouseReleaseEvent(QWidget* w, const QMouseEvent* e)
{
    if( e->button() == Qt::LeftButton ) {
        if( _resizeEnabled ) {
            if( _useRubberBand && !_pressedEdgeFlags.testFlag(PE_NO_PRESS) ) {
                w->setGeometry(_rect);
                _rubberBand->hide();
            }
#ifdef Q_WS_X11
//          w->setAttribute(Qt::WA_X11NetWmWindowTypeDock, false);
#endif
        }

        _pressPosFromWindowTopLeft = QPoint(INT_MIN, INT_MIN);
    }
}

void WindowController::mouseEventProcess(QWidget* w, QMouseEvent* e)
{
    if( e->type() == QEvent::MouseButtonPress )
        mousePressEvent(w, e);
    else
    if( e->type() == QEvent::MouseMove )
        mouseMoveEvent(w, e);
    else
    if( e->type() == QEvent::MouseButtonRelease )
        mouseReleaseEvent(w, e);
}

bool WindowController::eventFilter(QObject* o, QEvent* e)
{
//  if( e->spontaneous() ) {
        mouseEventProcess((QWidget*)o, (QMouseEvent*)e);
//  }

    return QObject::eventFilter(o, e);
}

WindowController::PressedEdgeFlags WindowController::retPressedEdgeFlags(const QWidget* w, const QPoint& pos)
{
    if( w == NULL ) return PE_NO_PRESS;

    const QRect rect = w->rect();
    const int topMargin    = _resizeMargin - 1;
    const int bottomMargin = _resizeMargin - 1;
    const int leftMargin   = _resizeMargin - 1;
    const int rightMargin  = _resizeMargin - 1;

    PressedEdgeFlags flags = PE_NO_PRESS;
    if( rect.top()<=pos.y() && pos.y()<=rect.top()+topMargin )
        flags |= PE_TOP;

    if( rect.bottom()-bottomMargin<=pos.y() && pos.y()<=rect.bottom() )
        flags |= PE_BOTTOM;

    if( rect.left()<=pos.x() && pos.x()<=rect.left()+leftMargin )
        flags |= PE_LEFT;

    if( rect.right()-rightMargin<=pos.x() && pos.x()<=rect.right() )
        flags |= PE_RIGHT;

    return flags;
}

