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
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include "clipwindow.h"
#include "windowcontroller.h"
#include "commonlib.h"

ClipWindow::ClipWindow(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint);
    setMouseTracking(true);
    setFont(QFont("DejaVu Sans", 9));
    QPalette p = palette();
    p.setColor(foregroundRole(), QColor(Qt::white));
    setPalette(p);

    _wc = new WindowController(this);
    _wc->setResizeEnabled(true);
    installEventFilter(_wc);

    setTargetWidget(parent);
    setTargetWindow(parent);

    // ラベルを初期化
//  _labelSize = new QLabel(this);
//  _labelSize->setAlignment(Qt::AlignLeft|Qt::AlignTop);
//  _labelSize->setMouseTracking(true);
    _labelDescription = new QLabel(tr("ダブルクリックで決定"), this);
    _labelDescription->setAlignment(Qt::AlignRight|Qt::AlignBottom);
    _labelDescription->setMouseTracking(true);

    _vLayout = new QVBoxLayout;
//  _vLayout->addWidget(_labelSize);
    _vLayout->addWidget(_labelDescription);
    setLayout(_vLayout);

    // コンテキストメニュー初期化
    QAction* actClose = new QAction(tr("閉じる"), this);
    connect(actClose, SIGNAL(triggered(bool)), this, SLOT(close()));
    QAction* actFit = new QAction(tr("ビデオ領域に合わせる"), this);
    connect(actFit, SIGNAL(triggered(bool)), this, SLOT(fitToTargetWidget()));
    _actTranslucentDisplay = new QAction(tr("半透明表示にする"), this);
    _actTranslucentDisplay->setCheckable(true);
    connect(_actTranslucentDisplay, SIGNAL(triggered(bool)), this, SLOT(setTranslucentDisplay(bool)));

    _menuContext = new QMenu(this);
    _menuContext->addAction(actClose);
    _menuContext->addAction(actFit);
    _menuContext->addSeparator();
    _menuContext->addAction(_actTranslucentDisplay);

    _isTranslucentDisplay = false;
    setTranslucentDisplay(true);
}

void ClipWindow::setResizeMargin(quint8 margin)
{
    _wc->setResizeMargin(margin);
    _vLayout->setContentsMargins(margin,margin,margin,margin);
    setMinimumSize(margin*2 +20, margin*2 +20);

    updateMask();
    update();
}

void ClipWindow::decideClipArea()
{
    QRect rc = CommonLib::clipRect(QRect(QPoint(0,0), _targetWidget->size()),
                                   QRect(_targetWidget->mapFromGlobal(pos()), size()));

    close();
    if( rc.x() >= 0 ) {
        emit decidedClipArea(rc);
    }
}

void ClipWindow::fitToTargetWidget()
{
    QPoint pos = _targetWidget->mapToGlobal(QPoint(0,0));
    setGeometry(QRect(pos, _targetWidget->size()));
}

void ClipWindow::setTranslucentDisplay(bool b)
{
    if( _isTranslucentDisplay != b ) {
        _isTranslucentDisplay = b;
        if( b )
            setResizeMargin(20);
        else
            setResizeMargin(10);

        _labelDescription->setVisible(b);
        _actTranslucentDisplay->setChecked(b);
        emit changedTranslucentDisplay(b);
    }
}

bool ClipWindow::event(QEvent* e)
{
    if( e->type() == QEvent::WindowActivate )
        emit windowActivate();

    return QWidget::event(e);
}

void ClipWindow::showEvent(QShowEvent* e)
{
    updateMask();
    QWidget::showEvent(e);
}

void ClipWindow::closeEvent(QCloseEvent* e)
{
    emit closed();
    QWidget::closeEvent(e);
}

void ClipWindow::paintEvent(QPaintEvent* e)
{
    QPainter p(this);

    if( _isTranslucentDisplay ) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0,0,255,75));
        p.drawRect(0,0, width(),height());
        p.setPen(QColor(255,255,255,25));
        p.setBrush(Qt::NoBrush);
        p.drawRect(_wc->resizeMargin()-1, _wc->resizeMargin()-1,
                   width()+2-_wc->resizeMargin()*2 -1, height()+2-_wc->resizeMargin()*2 -1);
    }
    else {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0,0,255,255));
        p.drawRect(0,0, width(),height());
        p.setPen(QColor(63,63,255,255));
        p.setBrush(QColor(0,0,0,255));
        p.drawRect(_wc->resizeMargin()-1, _wc->resizeMargin()-1,
                   width()+2-_wc->resizeMargin()*2 -1, height()+2-_wc->resizeMargin()*2 -1);
    }

    QWidget::paintEvent(e);
}

void ClipWindow::resizeEvent(QResizeEvent* e)
{
    updateMask();
//  _labelSize->setText(QString("%1x%2").arg(width()).arg(height()));
    QWidget::resizeEvent(e);
}

void ClipWindow::mousePressEvent(QMouseEvent* e)
{
    if( e->button() == Qt::RightButton ) {
        _menuContext->popup(e->globalPos());
    }
}

void ClipWindow::mouseDoubleClickEvent(QMouseEvent*)
{
    decideClipArea();
}

void ClipWindow::mouseMoveEvent(QMouseEvent*)
{
    updateMask();
}

void ClipWindow::updateMask()
{
    QRegion region(rect());
    if( !_isTranslucentDisplay ) {
//      QRect targetWindowClient(mapFromGlobal(_targetWindow->geometry().topLeft()), _targetWindow->size());
        QRect targetWindowClient(mapFromGlobal(_targetWidget->mapToGlobal(QPoint(0,0))),
                                 _targetWidget->size());
        int margin = _wc->resizeMargin();
        QRect subRect = CommonLib::clipRect(targetWindowClient, QRect(margin,margin, width()-margin*2,height()-margin*2));

        region = region.subtracted(subRect);
    }

    setMask(region);
    repaint();
}

