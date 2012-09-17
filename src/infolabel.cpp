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
#include "infolabel.h"

InfoLabel::InfoLabel(QWidget* parent) : QLabel(parent)
{
    connect(&_timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(&_timerClipInfo, SIGNAL(timeout()), this, SLOT(timerClipInfoTimeout()));

    _clipinfoSeq = 0;
}

InfoLabel::InfoLabel(const QString& text, QWidget* parent) : QLabel(text, parent)
{
    connect(&_timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(&_timerClipInfo, SIGNAL(timeout()), this, SLOT(timerClipInfoTimeout()));

    _clipinfoSeq = 0;
}

void InfoLabel::setText(const QString& text, int msec)
{
    if( msec > 0 ) {
        QLabel::setText(text + " ");
        _timer.start(msec);
    }
    else {
        if( !_timer.isActive() ) 
            QLabel::setText(text + " ");

        _text = text;
    }
}

void InfoLabel::clearClipInfo()
{
    _clipinfo.title.clear();
    _clipinfo.author.clear();
    _clipinfo.copyright.clear();
    _clipinfo.comments.clear();
}

void InfoLabel::timerTimeout()
{
    _timer.stop();
    setText(_text);
}

void InfoLabel::timerClipInfoTimeout()
{
#define N 4

    QString type[N] = {tr("タイトル: "), tr("作者: "), tr("著作権: "), tr("コメント: ")};
    QString data[N] = {_clipinfo.title, _clipinfo.author, _clipinfo.copyright,
                       _clipinfo.comments};

    for(int i=0; i < N; i++) {
        if( !data[_clipinfoSeq].isEmpty() ) {
            setText(type[_clipinfoSeq] + data[_clipinfoSeq]);

            _clipinfoSeq++;
            if( _clipinfoSeq >= N )
                _clipinfoSeq = 0;

            break;
        }

        _clipinfoSeq++;
        if( _clipinfoSeq >= N )
            _clipinfoSeq = 0;
    }
}

