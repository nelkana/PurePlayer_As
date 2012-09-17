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
#include <stdio.h>
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "pureplayer.h"

int main(int argc, char** argv)
{
    if( argc >= 3 ) {
        printf("Usage: %s [File Path or URL]\n", argv[0]);
        exit(1);
    }

    QApplication app(argc, argv);
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));//codecForLocale());
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));//codecForLocale());

    QTranslator transQt;
    transQt.load("qt_" + QLocale::system().name(),
                 QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&transQt);

    PurePlayer main;
    main.show();
    if( argc >= 2 )
        main.open(argv[1]);

    return app.exec();
}

