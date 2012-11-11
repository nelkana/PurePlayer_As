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
#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "ui_aboutdialog.h"
#include "logdialog.h"

#define PUREPLAYER_VERSION "0.7.2"

class AboutDialog : public QDialog, Ui::AboutDialog
{
    Q_OBJECT

public:
    AboutDialog(QWidget* parent);

protected:
    void showEvent(QShowEvent*);
};

inline AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);

    QString text = 
        tr("<h3>PurePlayer* %1</h3>"
        "PurePlayer*は、動画プレイヤー及びPeerCast視聴プレイヤーです。<br>"
        "<br>"
        "バックエンドにMPlayer"
            "(<a href=\"http://www.mplayerhq.hu/\">MPlayer</a>または"
            "<a href=\"http://www.mplayer2.org/\">MPlayer2</a>)を使用しています。<br>"
        "<a href=\"http://qt-project.org/\">Qtフレームワーク</a>で開発されています。<br>"
        "<br>"
        "Qtバージョン(コンパイル): %2<br>"
        "Qtバージョン(使用中): %3<br>"
        "ライセンス: GNU GPLv3<br>"
        "<br>"
        "Copyright (C) 2012 nel<br>"
        ).arg(PUREPLAYER_VERSION).arg(QT_VERSION_STR).arg(qVersion());

    _labelText->setText(text);
}

inline void AboutDialog::showEvent(QShowEvent*)
{
    setFixedSize(size());
}

#endif // ABOUTDIALOG_H

