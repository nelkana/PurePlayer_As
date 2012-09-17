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
#ifndef INFOLABEL_H
#define INFOLABEL_H

#include <QLabel>
#include <QString>
#include <QTimer>

class InfoLabel : public QLabel
{
    Q_OBJECT
public:
    InfoLabel(QWidget* parent=0);
    InfoLabel(const QString& text, QWidget* parent=0);

    void setText(const QString& text, int msec=0);
    void startClipInfo() { _clipinfoSeq = 0; _timerClipInfo.start(7000); }
    void stopClipInfo()  { _timerClipInfo.stop(); }

    void setClipTitle(const QString& data)     { _clipinfo.title = data; }
    void setClipAuthor(const QString& data)    { _clipinfo.author = data; }
    void setClipCopyright(const QString& data) { _clipinfo.copyright = data; }
    void setClipComments(const QString& data)  { _clipinfo.comments = data; }
    void clearClipInfo();

protected slots:
    void timerTimeout();
    void timerClipInfoTimeout();

private:
    QString _text;
    QTimer  _timer;
    QTimer  _timerClipInfo;

    struct {
        QString title;
        QString author;
        QString copyright;
        QString comments;
    }       _clipinfo;
    quint8  _clipinfoSeq;
};

#endif // INFOLABEL_H

