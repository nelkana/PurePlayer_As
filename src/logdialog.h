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
#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include "ui_logdialog.h"

class QKeyEvent;

class LogDialog : public QDialog, Ui::LogDialog
{
    Q_OBJECT

public:
    LogDialog(QWidget* parent=0);
    ~LogDialog();

    void   setOutputTerminal(bool b) { _outputTerminal = b; }
    void   setMaximumBlockCount(int count) { _textEdit->document()->setMaximumBlockCount(count); }
    int    blockCount() { return _textEdit->document()->blockCount(); }
    QColor TextColor() { return _textEdit->textColor(); }
    void   setTextColor(const QColor& c) { _textEdit->setTextColor(c); }

    static void initDialog(QWidget* parent=0);
    static LogDialog* dialog();
    static bool isVisibleDialog();

public slots:
    void setWindowTitle(const QString& title) { QDialog::setWindowTitle(title + " - PURE LOG"); }
    void printOut(const QString& text);
    void printOut(const QString& text, const QColor& color);
    void debugOut(const QString& text);
    void debugOut(const QString& text, const QColor& color);
    void clear() { _textEdit->clear(); }

    static void moveDialog(int x, int y);
    static void showDialog();
    static void closeDialog();
    static void debug(const QString& text);
    static void debug(const QString& text, const QColor& color);
    static void print(const QString& text) { s_logDialog->printOut(text); }
    static void print(const QString& text, const QColor& color)
                                           { s_logDialog->printOut(text, color); }

signals:
    void windowActivate();
    void requestCommand(const QString& command);
    void requestOutputStatusLine(bool);

protected:
    bool event(QEvent*);
    void keyPressEvent(QKeyEvent*);

private:
    static LogDialog* s_logDialog;
    static QWidget*   s_parent;
    static bool       s_calledShowFunction;

    bool _outputTerminal;
};

inline LogDialog* LogDialog::dialog()
{
    if( s_logDialog == NULL )
        initDialog();

    return s_logDialog;
}

inline bool LogDialog::isVisibleDialog()
{
    if( s_logDialog == NULL )
        return false;

    return s_logDialog->isVisible();
}

inline void LogDialog::printOut(const QString& text)
{
    _textEdit->append(text);

    if( _outputTerminal )
        qDebug(text.toAscii().constData());
}

inline void LogDialog::debugOut(const QString& text)
{
#ifdef QT_NO_DEBUG_OUTPUT
    Q_UNUSED(text);
#else
    QColor old = _textEdit->textColor();

    _textEdit->setTextColor(QColor(128,128,128));
    _textEdit->append(text);
    _textEdit->setTextColor(old);

    if( _outputTerminal )
        qDebug(text.toAscii().constData());
#endif
}

inline void LogDialog::debugOut(const QString& text, const QColor& color)
{
#ifdef QT_NO_DEBUG_OUTPUT
    Q_UNUSED(text);
    Q_UNUSED(color);
#else
    QColor old = _textEdit->textColor();

    _textEdit->setTextColor(color);
    _textEdit->append(text);
    _textEdit->setTextColor(old);

    if( _outputTerminal )
        qDebug(text.toAscii().constData());
#endif
}

inline void LogDialog::moveDialog(int x, int y)
{
    if( s_logDialog != NULL )
        s_logDialog->move(x, y);
}

inline void LogDialog::closeDialog()
{
    if( s_logDialog != NULL )
        s_logDialog->close();
}

inline void LogDialog::debug(const QString& text)
{
#ifdef QT_NO_DEBUG_OUTPUT
    Q_UNUSED(text);
#else
    s_logDialog->debugOut(text);
#endif
}

inline void LogDialog::debug(const QString& text, const QColor& color)
{
#ifdef QT_NO_DEBUG_OUTPUT
    Q_UNUSED(text);
    Q_UNUSED(color);
#else
    s_logDialog->debugOut(text, color);
#endif
}

#endif // define LOGDIALOG_H

