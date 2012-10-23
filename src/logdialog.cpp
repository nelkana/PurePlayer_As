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
#include <QKeyEvent>
#include "logdialog.h"

LogDialog* LogDialog::s_logDialog;

LogDialog::LogDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
//  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    connect(_buttonClear, SIGNAL(clicked()), this, SLOT(clear()));

    _textEdit->setTextColor(QColor(212,210,207));
#ifdef QT_NO_DEBUG_OUTPUT
    _textEdit->document()->setMaximumBlockCount(300);
    _lineEditCommand->hide();
#endif

    _outputTerminal = false;
}

LogDialog::~LogDialog()
{
    qDebug("LogDialog::~LogDialog():");
}

void LogDialog::printOut(const QString& text, const QColor& color)
{
    QColor old = _textEdit->textColor();

    _textEdit->setTextColor(color);
    _textEdit->append(text);
    _textEdit->setTextColor(old);

    if( _outputTerminal )
        qDebug(text.toAscii().constData());
}

bool LogDialog::event(QEvent* e)
{
    if( e->type() == QEvent::WindowActivate )
        emit windowActivate();

    return QDialog::event(e);
}

void LogDialog::keyPressEvent(QKeyEvent* e)
{
    if( e->key() == Qt::Key_Return ) {
        if( _lineEditCommand->hasFocus() )
            emit requestCommand(_lineEditCommand->text());
    }
}

