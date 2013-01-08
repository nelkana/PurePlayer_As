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
#include <QSettings>
#include <QDir>
#include "configdata.h"
#include "commonlib.h"
#include "logdialog.h"

ConfigData::Data ConfigData::s_data;

void ConfigData::saveData()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    s.setValue("voName",    s_data.voName);
    s.setValue("aoName",    s_data.aoName);
    s.setValue("cacheStreamSize", s_data.cacheStreamSize);
    s.setValue("volumeMax", s_data.volumeMax);
    s.setValue("openIn320x240Size",  s_data.openIn320x240Size);
    s.setValue("useSoftWareVideoEq", s_data.useSoftWareVideoEq);
    s.setValue("screenshot" , s_data.screenshot);
    s.setValue("useScreenshotPath", s_data.useScreenshotPath);
    s.setValue("screenshotPath", s_data.screenshotPath);
    s.setValue("useMplayerPath", s_data.useMplayerPath);
    s.setValue("mplayerPath", s_data.mplayerPath);
}

void ConfigData::loadData()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    s_data.voName = s.value("voName", "0").toString();

    if( s_data.voName == "0" ) {
#if defined(Q_WS_X11)
        s_data.voName = "xv";
//      s_data.voName = "xv:adaptor=0";
#elif defined(Q_OS_WIN32)
        s_data.voName = "directx";
#else
        s_data.voName = "";
#endif
    }

    s_data.aoName    = s.value("aoName", "").toString();
    s_data.cacheStreamSize = s.value("cacheStreamSize", 1000).toInt();
    s_data.volumeMax = s.value("volumeMax", 100).toInt();
    s_data.openIn320x240Size = s.value("openIn320x240Size", true).toBool();
    s_data.useSoftWareVideoEq = s.value("useSoftWareVideoEq", true).toBool();
    s_data.screenshot = s.value("screenshot", false).toBool();
    s_data.useScreenshotPath = s.value("useScreenshotPath", false).toBool();
    s_data.screenshotPath = s.value("screenshotPath", QDir::homePath()).toString();
    s_data.useMplayerPath = s.value("useMplayerPath", false).toBool();
    s_data.mplayerPath = s.value("mplayerPath", "").toString();
}

