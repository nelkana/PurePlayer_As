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
#include <stdio.h>
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "pureplayer.h"
#include "aboutdialog.h"

bool parseArgs(PurePlayer* player, int argc, char** argv)
{
    QStringList paths;

    for(int i=1; i < argc; i++) {
        if( !strcmp(argv[i], "--help")
         || !strcmp(argv[i], "-h") )
        {
            QString help = QObject::tr(
                    "PurePlayer* %1\n"
                    "Usage: %2 [Options] [File|URL]...\n"
                    "\n"
                    "Options:\n"
                    "  --help|-h            ヘルプ\n")
                    .arg(PUREPLAYER_VERSION)
                    .arg(argv[0]);

            fprintf(stderr, help.toAscii().data());

            return false;
        }
        else {
            paths << argv[i];
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

