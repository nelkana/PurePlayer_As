/*  Copyright (C) 2012 nel

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
#include <QEvent>
#include <QMouseEvent>
#include "speedspinbox.h"

SpeedSpinBox::SpeedSpinBox(QWidget* parent) : QDoubleSpinBox(parent)
{
    setLineEdit(new LineEditForSpeedSpinBox(this));

    setRange(0.1, 3.0);
    setSingleStep(0.1);
    setDecimals(1);
    setValue(1.0);
    setPrefix("x");
    setFocusPolicy(Qt::NoFocus);
    setKeyboardTracking(false);
    setContextMenuPolicy(Qt::NoContextMenu);

    //lineEdit()->setReadOnly(true);
    //lineEdit()->setContextMenuPolicy(Qt::NoContextMenu);
}

void SpeedSpinBox::stepBy(int steps)
{
    QDoubleSpinBox::stepBy(steps);
    lineEdit()->deselect();
}

bool SpeedSpinBox::event(QEvent* e)
{
    if( e->type() == QEvent::MouseButtonPress ) {
        QMouseEvent* event = static_cast<QMouseEvent*>(e);
        if( event->button() == Qt::RightButton ) {
            return false;
        }
    }

    return QDoubleSpinBox::event(e);
}

// ------------------------------------------------------------------------------------
LineEditForSpeedSpinBox::LineEditForSpeedSpinBox(QWidget* parent) : QLineEdit(parent)
{
    setCursor(QCursor(Qt::ArrowCursor));
}

bool LineEditForSpeedSpinBox::event(QEvent* e)
{
    if( e->type() == QEvent::MouseButtonPress
     || e->type() == QEvent::MouseButtonDblClick
     || e->type() == QEvent::MouseMove )
    {
        return false;
    }

    return QLineEdit::event(e);
}

