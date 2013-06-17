/*  Copyright (C) 2013 nel

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
#ifndef CLIPWINDOW_H
#define CLIPWINDOW_H

#include <QWidget>

class QMenu;
class WindowController;

class ClipWindow : public QWidget
{
    Q_OBJECT

public:
    ClipWindow(QWidget* parent);
    void setTargetWidget(QWidget* w) { _targetWidget = w; }

public slots:
    void fitToTargetWidget();

signals:
    void windowActivate();
    void decidedClipArea(QRect);

protected:
    bool event(QEvent*);
    void paintEvent(QPaintEvent*);
//  void resizeEvent(QResizeEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);

private:
    QWidget* _targetWidget;
//  QLabel*  _labelSize;
    QMenu*   _menuContext;

    WindowController* _wc;
};

#endif // CLIPWINDOW_H

