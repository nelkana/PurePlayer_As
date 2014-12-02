/*  Copyright (C) 2013-2014 nel

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
class QLabel;
class QMenu;
class WindowController;

class ClipWindow : public QWidget
{
    Q_OBJECT

public:
    ClipWindow(QWidget* parent);
    void setTargetWidget(QWidget* w) { _targetWidget = w; }
    void setTargetWindow(QWidget* w) { _targetWindow = w; }
    void setFitToWidgetTriggerShow(bool b) { _fitToWidgetWhenTriggerShow = b; }
    void setResizeMargin(quint8 margin);
    void repaintWindow() { updateMask(); }

public slots:
    void triggerShow() { if( _fitToWidgetWhenTriggerShow ) fitToTargetWidget(); show(); emit triggeredShow(); }
    void decideClipArea();
    void fitToTargetWidget();
    void setTranslucentDisplay(bool);

signals:
    void triggeredShow();
    void closed();
    void windowActivate();
    void decidedClipArea(QRect);
    void changedTranslucentDisplay(bool);

protected:
    bool event(QEvent*);
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent*);
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);

    void updateMask();

private:
    QWidget* _targetWidget;
    QWidget* _targetWindow;
    bool     _fitToWidgetWhenTriggerShow;
    bool     _isTranslucentDisplay;

    WindowController* _wc;
    QVBoxLayout* _vLayout;
//  QLabel*  _labelSize;
    QLabel*  _labelDescription;
    QAction* _actTranslucentDisplay;
    QMenu*   _menuContext;
};

#endif // CLIPWINDOW_H

