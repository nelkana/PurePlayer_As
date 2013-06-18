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

class QVBoxLayout;
class QMenu;
class WindowController;

class ClipWindow : public QWidget
{
    Q_OBJECT

public:
    ClipWindow(QWidget* parent);
    void setTargetWidget(QWidget* w) { _targetWidget = w; }
    void setResizeMargin(quint8 margin);

public slots:
    void decideClipArea();
    void fitToTargetWidget();
    void setTranslucentDisplay(bool);

signals:
    void windowActivate();
    void decidedClipArea(QRect);
    void changedTranslucentDisplay(bool);

protected:
    bool event(QEvent*);
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);

    void updateMask();

private:
    QWidget* _targetWidget;
    bool     _isTranslucentDisplay;

    WindowController* _wc;
    QVBoxLayout* _vLayout;
//  QLabel*  _labelSize;
    QAction* _actTranslucentDisplay;
    QMenu*   _menuContext;
};

#endif // CLIPWINDOW_H

