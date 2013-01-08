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
#ifndef INPUTDIALOGs_H
#define INPUTDIALOGs_H

#include "ui_inputdialog.h"

class InputDialog : public QDialog, Ui::InputDialog
{
    Q_OBJECT

public:
    InputDialog(QWidget* parent);

    static QString input(QWidget* parent, const QString& title, const QString& text, const QString& initText=QString());

protected:
    void showEvent(QShowEvent*);
};

#endif // INPUTDIALOGs_H

