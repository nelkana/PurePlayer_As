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
#include <stdio.h>
#include <QApplication>
#include <QTextCodec>
#include <QTextStream>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "pureplayer.h"
#include "aboutdialog.h"

#define HELP_STRING \
    "PurePlayer* %1\n" \
    "Usage: %2 [Options] [File|URL]...\n" \
    "\n" \
    "Options:\n" \
    "  -h, --help            ヘルプ\n" \
    "  -t, --title name      ウィンドウタイトル設定\n" \
    "  -v, --version         バージョン\n"

bool parseArgs(PurePlayer* player, int argc, char** argv)
{
    QStringList paths;

    for(int i=1; i < argc; ++i) {
        if( !strcmp(argv[i], "-h")
         || !strcmp(argv[i], "--help") )
        {
            QString help = QObject::tr(HELP_STRING).arg(PUREPLAYER_VERSION).arg(argv[0]);

            QTextStream(stdout) << help << flush;
            return false;
        }
        else
        if( !strcmp(argv[i], "-t")
         || !strcmp(argv[i], "--title") )
        {
            ++i;
            if( i >= argc ) {
                QTextStream(stderr) << QObject::tr("error: '%1'の引数が足りません。\n")
                                       .arg(argv[i-1]) << flush;

                return false;
            }
            else {
                player->setTitleOption(QTextCodec::codecForLocale()->toUnicode(argv[i]));
            }
        }
        else
        if( !strcmp(argv[i], "-v")
         || !strcmp(argv[i], "--version") )
        {
            QTextStream(stdout) << QObject::tr("PurePlayer* %1\n").arg(PUREPLAYER_VERSION) << flush;
            return false;
        }
        else {
            paths << QTextCodec::codecForLocale()->toUnicode(argv[i]);
        }
    }

    if( paths.count() > 0 )
        player->open(paths, true);

    return true;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));//codecForLocale());
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));//codecForLocale());

    QTranslator transQt;
    transQt.load("qt_" + QLocale::system().name(),
                 QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&transQt);

#ifdef Q_OS_WIN32
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QApplication::applicationDirPath());
#endif

    PurePlayer* main = new PurePlayer();
    if( !parseArgs(main, argc, argv) )
        exit(1);

    main->show();

    int ret = app.exec();
    delete main;

    return ret;
}

