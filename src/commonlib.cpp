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
#include <math.h>
#include <QThread>
#include <QFileInfo>    // retTheFileNameNotExists()
#include <QFileDialog>  // getOpenFileNameDialog()
#include "commonlib.h"

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

void CommonLib::msleep(unsigned long msec)
{
    MyThread::msleep(msec);
}

// 要求したファイル名に基づいて、現在のディレクトリにおける重複しないファイル名を返す。
// ファイルが既に有る場合、ファイル名に連番が付加される。
QString CommonLib::retTheFileNameNotExists(const QString& requestFileName)
{
    QFileInfo file(requestFileName);
    QString baseName = file.baseName();
    QString suffix   = file.suffix();
    if( !suffix.isEmpty() )
        suffix = "." + suffix;

    unsigned long num = 1;
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
    QString formats =
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

    QString file = QFileDialog::getOpenFileName(parent, caption, "",
                    QObject::tr("メディアファイル(%1);;全てのファイル(*)").arg(formats));

    return file;
}

