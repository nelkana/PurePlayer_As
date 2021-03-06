/*  Copyright (C) 2012-2015 nel

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
#ifndef CONFIGDATA_H
#define CONFIGDATA_H

#include <QString>
#include <QSize>

class ConfigData
{
public:
    static const char* const VONAME_DEFAULT;
    static const char* const VONAME_FOR_CLIPPING_DEFAULT;
    static const char* const AONAME_DEFAULT;
    static const char* const CONTACTURL_ARG_DEFAULT;

    struct Data {
        QString voName;
        QString voNameForClipping;
        QString aoName;
        bool    useSoftWareVideoEq;
        bool    autoHideMouseCursor;
        bool    reverseWheelSeek;
        int     volumeMax;
        QSize   initSize;
        bool    suitableResize;
        int     suitableResizeValue;
        bool    useCacheSize;
        int     cacheStreamSize;
        bool    useScreenshotPath;
        QString screenshotPath;
        bool    useMplayerPath;
        QString mplayerPath;
        bool    limitLogLine;
        int     logLineMax;
        bool    useContactUrlPath;
        QString contactUrlPath;
        QString contactUrlArg;
        bool    disconnectChannel;
    };

    static Data* data() { return &s_data; }
    static void setData(const Data& data) { s_data = data; }
    static void saveData();
    static void loadData();

private:
    static Data s_data;
};

#endif // CONFIGDATA_H

