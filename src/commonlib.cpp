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
#include <math.h>
#include <QThread>
#include <QFileInfo>    // retTheFileNameNotExists()
#include <QFileDialog>  // getOpenFileNameDialog()
#include "commonlib.h"

#ifdef Q_OS_WIN32
const char* const CommonLib::QSETTINGS_ORGNAME = "Settings";
#else
const char* const CommonLib::QSETTINGS_ORGNAME = "PurePlayer";
#endif // Q_OS_WIN32

const char* const CommonLib::MEDIA_FORMATS =
    // (S)VCD (Super Video CD)
    // CDRwin's .bin image file
    "*.bin"
    // DVD, including encrypted DVD
    // MPEG-1/2 (ES/PS/PES/VOB)
    " *.mpg *.mpeg *.mp1 *.mp2 *.mp3 *.m1v *.m1a *.mpa *.mpv"
    " *.m2v *.m2a *.m2s"
    " *.ps *.m2p"
    " *.ts *.m2t *.m2ts"
    " *.vob"
        //" *.vro"
    " *.mod"
    // AVI file format
    " *.avi"
    // ASF/WMV/WMA format
    " *.asf *.wmv *.wma"
    // QT/MOV/MP4 format
    " *.qt *.mov"
    " *.mp4 *.m4a *.m4p *.m4b *.m4r *.m4 *.m4v"
    " *.3gp *.3g2"
    " *.aac"
    // RealAudio/RealVideo format
    " *.rm *.rmvb *.ra *.ram"
    // Ogg/OGM files
    " *.ogv *.oga *.ogx *.ogg *.spx *.ogm"
    // Matroska
    " *.mkv *.mka *.mks *.mk3d"
    // NUT
    " *.nut"
    // NSV (Nullsoft Streaming Video)
    " *.nsv"
    // VIVO format
    " *.viv"
    // FLI format
    " *.fli" //*.flc"
    // NuppelVideo format
    " *.nuv"
    // yuv4mpeg format
    " *.y4m"
    // FILM (.cpk) format // SEGA FILM
    " *.cpk" //*.cak *.film"
    // RoQ format // Id Software Game Video
    " *.roq"
    // PVA format
    " *.pva"
    // streaming via HTTP/FTP, RTP/RTSP, MMS/MMST, MPST, SDP
    // TV grabbing

    // Bink Video
    //" *.bik"
    // Flash Video
    " *.flv *.f4v *.f4p *.f4a *.f4b";

class MyThread : public QThread
{
    Q_OBJECT;

public:
    static void msleep(unsigned long msec) { QThread::msleep(msec); }
};

// valueを四捨五入する。
// 小数点以下precision桁の値を返す。
double CommonLib::round(double value, int precision)
{
	return (int)(value * pow(10, precision) + 0.5+(value<0 ? -1:0)) / pow(10, precision);
}

int CommonLib::digit(int value)
{
    int i = 0;
    while( value /= 10 )
        i++;

    return i + 1;
}

void CommonLib::msleep(unsigned long msec)
{
    MyThread::msleep(msec);
}

// 受け取った曜日の値(0 ~ 6)に対応する曜日の文字列を返す
QString CommonLib::dayOfWeek(int day)
{
    if( day < 0 || day > 6 ) return QString();

    const char* d[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

    return QString(d[day]);
}

QString CommonLib::secondTimeToString(int sec)
{
    if( sec < 0 ) return QString();

    int h, m, s;
    h = sec / 3600;
    sec %= 3600;
    m = sec / 60;
    s = sec % 60;

    if( h == 0 )
        return QString().sprintf("%02d:%02d", m, s);
    else
        return QString().sprintf("%d:%02d:%02d", h, m, s);
}

// 前方、後方の空白文字を取り除いた文字列を返す
QString CommonLib::removeSpaceBeforeAfter(QString str)
{
    str.remove(QRegExp("^\\s*"));
    str.remove(QRegExp("\\s*$"));
    return str;
}

// 矩形上で、比を維持してスケーリングを行った矩形を返す
QRect CommonLib::scaleRectOnRect(const QSize& baseRect, const QSize& placeRect)
{
    int h, w, x, y;
    int aspectWidth, aspectHeight;

    aspectWidth  = placeRect.width();
    aspectHeight = placeRect.height();

    double tempw = baseRect.height() * aspectWidth / (double)aspectHeight;
    if( tempw <= baseRect.width() ) {
        // 乗っている矩形の比に対して下の矩形の比が横長
        h = baseRect.height();
        w = baseRect.height() * aspectWidth / (double)aspectHeight + 0.5;
        //w = baseRect.height() * aspectRatio + 0.1;

        x = (baseRect.width() - w) / 2;
        y = 0;
    }
    else {
        // 乗っている矩形の比に対して下の矩形の比が縦長
        h = baseRect.width() * aspectHeight / (double)aspectWidth + 0.5;
        //h = baseRect.width() / aspectRatio + 0.1;
        w = baseRect.width();

        x = 0;
        y = (baseRect.height() - h) / 2;
    }

    return QRect(x,y, w,h);
}

// 要求したファイル名に基づいて、現在のディレクトリにおける重複しないファイル名を返す。
// ファイルが既に有る場合、ファイル名に連番が付加される。
QString CommonLib::retTheFileNameNotExists(const QString& requestFileName)
{
    QFileInfo file(requestFileName);
    QString baseName = file.completeBaseName();
    QString suffix   = file.suffix();
    if( !suffix.isEmpty() )
        suffix = "." + suffix;

    ulong num = 1;
    while( file.exists() || file.isSymLink() ) {
        num++;
//      if( num > limitNum ) break;

        file.setFile(QString("%1_%2%3").arg(baseName).arg(num).arg(suffix));
    }

//  if( num > limitNum )
//      return QString();
//  else
        return file.fileName();
}

// ファイルを選択するダイアログを開く。
QString CommonLib::getOpenFileNameDialog(QWidget* parent, const QString& caption)
{
    return QFileDialog::getOpenFileName(parent, caption, QDir::homePath(),
                QObject::tr("メディアファイル(%1);;全てのファイル(*)").arg(MEDIA_FORMATS));
}

QStringList CommonLib::getOpenFileNamesDialog(QWidget* parent, const QString& caption)
{
    return QFileDialog::getOpenFileNames(parent, caption, QDir::homePath(),
                QObject::tr("メディアファイル(%1);;全てのファイル(*)").arg(MEDIA_FORMATS));
}

QString CommonLib::getExistingDirectoryDialog(QWidget* parent, const QString& caption)
{
    return QFileDialog::getExistingDirectory(parent, caption, QDir::homePath());
}

bool CommonLib::isPeercastUrl(const QString& url)
{
    return QRegExp("(?:^http|^mms|^mmsh)://.+:\\d+/(?:stream|pls)/[A-F0-9]{32}")
            .indexIn(url) != -1;
}
/*
QSize CommonLib::widgetFrameSize(QWidget* const w)
{
    return QSize(w->frameSize().width() - w->width(),
                 w->frameSize().height() - w->height());
}
*/

