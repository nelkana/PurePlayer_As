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

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    if( argc >= 3 ) {
        fprintf(stderr, "Usage: %s [File Path or URL]\n", argv[0]);
        exit(1);
    }

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
    if( argc >= 2 )
        main->open(argv[1]);

    main->show();

    int ret = app.exec();
    delete main;

    return ret;
}

