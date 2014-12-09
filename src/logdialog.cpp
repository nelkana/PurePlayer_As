/*  Copyright (C) 2012-2014 nel

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
#include "windowcontroller.h"

LogDialog* LogDialog::s_logDialog;
QWidget*   LogDialog::s_parent;
bool       LogDialog::s_calledShowFunction = false;

LogDialog::LogDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
//  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    connect(_buttonClear, SIGNAL(clicked()), this, SLOT(clear()));
    connect(_checkBoxStatusLine, SIGNAL(clicked(bool)), this, SIGNAL(requestOutputStatusLine(bool)));

    _textEdit->setTextColor(QColor(212,210,207));
#ifdef QT_NO_DEBUG_OUTPUT
    _lineEditCommand->hide();
#endif

    _outputTerminal = false;
}

LogDialog::~LogDialog()
{
//  qDebug("LogDialog::~LogDialog():");
}

void LogDialog::initDialog(QWidget* parent)
{
    delete s_logDialog;
    s_logDialog = new LogDialog();
    s_logDialog->installEventFilter(new WindowController(s_logDialog));

    s_parent = parent;
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

void LogDialog::showDialog()
{
    if( s_logDialog != NULL ) {
        // KDEでoxygenスタイルを使用した場合の、ウィンドウ又は内部Widgetのリサイズが
        // 重くなる問題への対応。
        // 1.ダイアログを一度もまだ表示させていない場合は、親Widgetをセットしない。
        // 2.最初の表示を行う際は、親Widgetをセットする(必ず行う必要は無い)。
        if( !s_calledShowFunction && s_parent != NULL ) {
            Qt::WindowFlags flags = s_logDialog->windowFlags();
            s_logDialog->setParent(s_parent);
            s_logDialog->setWindowFlags(flags);

            s_calledShowFunction = true;
//          qDebug("%08x", (uint)flags);
        }

        s_logDialog->show();
        s_logDialog->activateWindow(); // フルスクリーンされてる場合でも表示
    }
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

