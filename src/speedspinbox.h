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
#ifndef __SPEEDSPINBOX_H
#define __SPEEDSPINBOX_H

#include <QDoubleSpinBox>
#include <QLineEdit>

class SpeedSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    SpeedSpinBox(QWidget* parent);
    void stepBy(int steps);

protected:
    bool event(QEvent* e);
};

class LineEditForSpeedSpinBox : public QLineEdit
{
    Q_OBJECT

public:
    LineEditForSpeedSpinBox(QWidget* parent);

protected:
    bool event(QEvent* e);
};

#endif //__SPEEDSPINBOX_H

