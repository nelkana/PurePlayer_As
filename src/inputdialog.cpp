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
#include <QPushButton> // QDialogButtonBox::button()から返されるQPushButtonの為に必要
#include "inputdialog.h"
#include "logdialog.h"

InputDialog::InputDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
                                // _lineEditで改行するとokボタンが押されてる様、後で調べる
    connect(_buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(_buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(reject()));
}

// okボタンが押される以外でウィンドウが閉じられた場合、nullのQStringが返される
QString InputDialog::input(QWidget* parent, const QString& title, const QString& text, const QString& initText)
{
    InputDialog dialog(parent);

    dialog.setModal(true);
    dialog.setWindowTitle(title);
    dialog._label->setText(text);
    dialog._lineEdit->setText(initText);

    dialog.show();

#ifdef Q_OS_WIN32
    if( parent != NULL ) {
        dialog.move(
            parent->x() + (parent->frameSize().width()-dialog.frameSize().width())/2,
            parent->y() + (parent->frameSize().height()-dialog.frameSize().height())/2);
    }
#endif

    QString ret;
    if( dialog.exec() )
        ret = dialog._lineEdit->text();
    else
        ret = QString();

    return ret;
}

void InputDialog::showEvent(QShowEvent*)
{
    setFixedHeight(height());
}

