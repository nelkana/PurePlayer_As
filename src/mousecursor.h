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
#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H

#include <QWidget>
#include <QTimer>

class QMouseEvent;

class MouseCursor : public QObject
{
    Q_OBJECT

public:
    MouseCursor(QObject* parent);
    virtual ~MouseCursor();

    void setTargetWidget(QWidget*);
    bool setVisible(bool b);
    bool isHidden() { return _widget->cursor().shape() == Qt::BlankCursor; }
    void setAutoHideTime(int msec) {_timerHideCursor.setInterval(msec); }
    void startAutoHide() { _startedAutoHide = true; _timerHideCursor.start(); }
    void stopAutoHide() { _timerHideCursor.stop(); _startedAutoHide = false; }

    void tempDisableChangeShape() { _timerDisableChangeShape.start(); }
                        // マウスカーソル非表示防止処理は必要と思われるが、内容の精査が必要

signals:
    void changedShape(Qt::CursorShape);

protected slots:
    void timerHideCursor_timeout();

protected:
    bool eventFilter(QObject*, QEvent*);

    void mousePressEvent(QWidget*, const QMouseEvent*);
    void mouseReleaseEvent(QWidget*, const QMouseEvent*);
    void mouseDoubleClickEvent(QWidget*, const QMouseEvent*);
    void mouseMoveEvent(QWidget*, const QMouseEvent*);

private:
    QWidget* _widget;
    QPoint   _pressGlobalPos;

    bool   _startedAutoHide;
    QTimer _timerHideCursor;
    QTimer _timerDisableChangeShape;
};

#endif // MOUSECURSOR_H

