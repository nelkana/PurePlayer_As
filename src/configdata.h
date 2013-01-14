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
#ifndef CONFIGDATA_H
#define CONFIGDATA_H

#include <QString>

class ConfigData
{
public:
    static const char* const CONTACTURL_ARG_DEFAULT;

    struct Data {
        QString voName;
        QString aoName;
        bool    openIn320x240Size;
        bool    useSoftWareVideoEq;
        bool    screenshot; // debug
        int     volumeMax;
        int     cacheStreamSize;
        bool    useScreenshotPath;
        QString screenshotPath;
        bool    useMplayerPath;
        QString mplayerPath;
        bool    useContactUrlPath;
        QString contactUrlPath;
        QString contactUrlArg;
        bool    disconnectChannel;
    };

    static Data* data() { return &s_data; }
    static void saveData();
    static void loadData();

private:
    static Data s_data;
};

#endif // CONFIGDATA_H

