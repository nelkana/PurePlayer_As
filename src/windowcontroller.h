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
#ifndef WINDOWCONTROLLER_H
#define WINDOWCONTROLLER_H

#include <QObject>
#include <QPoint>
#include <QRect>

class QEvent;
class QMouseEvent;
class QRubberBand;

class WindowController : public QObject
{
    Q_OBJECT

public:
    enum PRESSED_EDGE_FLAGS {
        PE_NO_PRESS = 0,
        PE_TOP      = 1,
        PE_BOTTOM   = 1 << 1,
        PE_LEFT     = 1 << 2,
        PE_RIGHT    = 1 << 3,
    };
    Q_DECLARE_FLAGS(PressedEdgeFlags, PRESSED_EDGE_FLAGS)

    WindowController(QObject* parent);
    virtual ~WindowController();

    void   setResizeEnabled(bool b)    { _resizeEnabled = b; }
    void   setSimpleResizeView(bool b);
    quint8 resizeMargin()                 { return _resizeMargin; }
    void   setResizeMargin(quint8 margin) { _resizeMargin = margin; }
    void   mouseEventProcess(QWidget*, QMouseEvent*);

protected:
    bool eventFilter(QObject*, QEvent*);

    PressedEdgeFlags retPressedEdgeFlags(const QWidget*, const QPoint&);
    void mousePressEvent(QWidget*, const QMouseEvent*);
    void mouseMoveEvent(QWidget*, const QMouseEvent*);
    void mouseReleaseEvent(QWidget*, const QMouseEvent*);

private:
    QRect  _rect;
    QPoint _pressPosFromWindowTopLeft;
    QPoint _pressPosFromClientBottomRight;
    PressedEdgeFlags _pressedEdgeFlags;
    bool   _resizeEnabled;
    bool   _useRubberBand;
    quint8 _resizeMargin;

    QRubberBand* _rubberBand;
};

#endif // WINDOWCONTROLLER_H

