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
#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include "ui_opendialog.h"
#include "commonlib.h"

class OpenDialog : public QDialog, Ui::OpenDialog
{
    Q_OBJECT

public:
    OpenDialog(QWidget* parent);
    void setPath(const QString& path) { _lineEdit->setText(path); }

public slots:
    void show() { _lineEdit->selectAll(); QDialog::show(); }
    void open() { close(); emit openPath(_lineEdit->text()); }
    void selectFile();
    void paste() { _lineEdit->clear(); _lineEdit->paste(); _lineEdit->selectAll(); }
    void copy()  { _lineEdit->selectAll(); _lineEdit->copy(); }

signals:
    void openPath(const QString path);

protected:
    void showEvent(QShowEvent*);
};

inline OpenDialog::OpenDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    connect(_buttonOpen, SIGNAL(clicked()), this, SLOT(open()));
    connect(_buttonFile, SIGNAL(clicked()), this, SLOT(selectFile()));
    connect(_buttonPaste,SIGNAL(clicked()), this, SLOT(paste()));
    connect(_buttonCopy, SIGNAL(clicked()), this, SLOT(copy()));
}

inline void OpenDialog::selectFile()
{
    QString file = CommonLib::getOpenFileNameDialog(this, tr("ファイルを選択"));

    if( !file.isEmpty() )
        _lineEdit->setText(file);
}

inline void OpenDialog::showEvent(QShowEvent* )
{
    adjustSize();
    setFixedHeight(height());
}

#endif // OPENDIALOG_H

