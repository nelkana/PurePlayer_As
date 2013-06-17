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

    _wc = new WindowController(this);
    _wc->setResizeEnabled(true);
    _wc->setResizeMargin(20);
    installEventFilter(_wc);

    setMinimumSize(_wc->resizeMargin()*2 +20, _wc->resizeMargin()*2 +20);

    setTargetWidget(parent);

    // ラベルを初期化
//  _labelSize = new QLabel(this);
//  _labelSize->setAlignment(Qt::AlignLeft|Qt::AlignTop);
//  _labelSize->setMouseTracking(true);
    QLabel* label2 = new QLabel(tr("ダブルクリックで決定"), this);
    label2->setAlignment(Qt::AlignRight|Qt::AlignBottom);
    label2->setMouseTracking(true);

    QVBoxLayout* vl = new QVBoxLayout;
    vl->setContentsMargins(_wc->resizeMargin(),_wc->resizeMargin(),
                           _wc->resizeMargin(),_wc->resizeMargin());
//  vl->addWidget(_labelSize);
    vl->addWidget(label2);
    setLayout(vl);

    // コンテキストメニュー初期化
    QAction* actClose = new QAction(tr("閉じる"), this);
    connect(actClose, SIGNAL(triggered(bool)), this, SLOT(close()));
    QAction* actFit = new QAction(tr("ビデオ領域に合わせる"), this);
    connect(actFit, SIGNAL(triggered(bool)), this, SLOT(fitToTargetWidget()));

    _menuContext = new QMenu(this);
    _menuContext->addAction(actClose);
    _menuContext->addAction(actFit);
}

void ClipWindow::fitToTargetWidget()
{
    QPoint pos = _targetWidget->mapToGlobal(QPoint(0,0));
    setGeometry(QRect(pos, _targetWidget->size()));
}

bool ClipWindow::event(QEvent* e)
{
    if( e->type() == QEvent::WindowActivate )
        emit windowActivate();

    return QWidget::event(e);
}

void ClipWindow::paintEvent(QPaintEvent* e)
{
    QPainter p(this);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,255,75));
    p.drawRect(0,0, width(),height());
    p.setPen(QColor(255,255,255,25));
    p.setBrush(Qt::NoBrush);
    p.drawRect(_wc->resizeMargin()-1, _wc->resizeMargin()-1,
               width()+2-_wc->resizeMargin()*2 -1, height()+2-_wc->resizeMargin()*2 -1);

    QWidget::paintEvent(e);
}
/*
void ClipWindow::resizeEvent(QResizeEvent* e)
{
    _labelSize->setText(QString("%1x%2").arg(width()).arg(height()));
    QWidget::resizeEvent(e);
}
*/
void ClipWindow::mousePressEvent(QMouseEvent* e)
{
    if( e->button() == Qt::RightButton ) {
        _menuContext->popup(e->globalPos());
    }
}

void ClipWindow::mouseDoubleClickEvent(QMouseEvent*)
{
    QRect rc = CommonLib::clipRect(QRect(QPoint(0,0), _targetWidget->size()),
                                   QRect(_targetWidget->mapFromGlobal(pos()), size()));

    close();
    if( rc.x() >= 0 ) {
        emit decidedClipArea(rc);
    }
}

