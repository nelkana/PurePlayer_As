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
#include <QSettings>
#include <QDir>
#include "configdata.h"
#include "commonlib.h"
#include "logdialog.h"

#if defined(Q_WS_X11)
const char* const ConfigData::VONAME_DEFAULT = "xv";
//const char* const VONAME_DEFAULT = "xv:adaptor=0";
const char* const ConfigData::VONAME_FOR_CLIPPING_DEFAULT = "x11";
#elif defined(Q_OS_WIN32)
const char* const ConfigData::VONAME_DEFAULT = "direct3d";
const char* const ConfigData::VONAME_FOR_CLIPPING_DEFAULT = "direct3d";
#else
const char* const ConfigData::VONAME_DEFAULT = "";
const char* const ConfigData::VONAME_FOR_CLIPPING_DEFAULT = "";
#endif

const char* const ConfigData::AONAME_DEFAULT = "";
const char* const ConfigData::CONTACTURL_ARG_DEFAULT = "\"%{ContactUrl}\"";
ConfigData::Data ConfigData::s_data;

void ConfigData::saveData()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    s.setValue("voName", s_data.voName);
    s.setValue("voNameForClipping", s_data.voNameForClipping);
    s.setValue("aoName", s_data.aoName);
    s.setValue("useSoftWareVideoEq", s_data.useSoftWareVideoEq);
    s.setValue("openIn320x240Size",  s_data.openIn320x240Size);
    s.setValue("autoHideMouseCursor", s_data.autoHideMouseCursor);
    s.setValue("reverseWheelSeek", s_data.reverseWheelSeek);
    s.setValue("volumeMax", s_data.volumeMax);
    s.setValue("useCacheSize", s_data.useCacheSize);
    s.setValue("cacheStreamSize", s_data.cacheStreamSize);
    s.setValue("useScreenshotPath", s_data.useScreenshotPath);
    s.setValue("screenshotPath", s_data.screenshotPath);
    s.setValue("useMplayerPath", s_data.useMplayerPath);
    s.setValue("mplayerPath", s_data.mplayerPath);
    s.setValue("limitLogLine", s_data.limitLogLine);
    s.setValue("logLineMax", s_data.logLineMax);
    s.setValue("useContactUrlPath", s_data.useContactUrlPath);
    s.setValue("contactUrlPath", s_data.contactUrlPath);
    s.setValue("contactUrlArg", s_data.contactUrlArg);
    s.setValue("disconnectChannel", s_data.disconnectChannel);
}

void ConfigData::loadData()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    s_data.voName = s.value("voName", VONAME_DEFAULT).toString();
    s_data.voNameForClipping = s.value("voNameForClipping", VONAME_FOR_CLIPPING_DEFAULT).toString();
    s_data.aoName = s.value("aoName", AONAME_DEFAULT).toString();
    s_data.useSoftWareVideoEq = s.value("useSoftWareVideoEq", true).toBool();
    s_data.openIn320x240Size = s.value("openIn320x240Size", true).toBool();
    s_data.autoHideMouseCursor = s.value("autoHideMouseCursor", false).toBool();
    s_data.reverseWheelSeek = s.value("reverseWheelSeek", false).toBool();
    s_data.volumeMax = s.value("volumeMax", 100).toInt();
    s_data.useCacheSize = s.value("useCacheSize", false).toBool();
    s_data.cacheStreamSize = s.value("cacheStreamSize", 3000).toInt();
    s_data.useScreenshotPath = s.value("useScreenshotPath", false).toBool();
    s_data.screenshotPath = s.value("screenshotPath", QDir::homePath()).toString();
    s_data.useMplayerPath = s.value("useMplayerPath", false).toBool();
    s_data.mplayerPath = s.value("mplayerPath", "").toString();
    s_data.limitLogLine = s.value("limitLogLine", true).toBool();
    s_data.logLineMax = s.value("logLineMax", 1000).toInt();
    s_data.useContactUrlPath = s.value("useContactUrlPath", false).toBool();
    s_data.contactUrlPath = s.value("contactUrlPath", "").toString();
    s_data.contactUrlArg = s.value("contactUrlArg", CONTACTURL_ARG_DEFAULT).toString();
    s_data.disconnectChannel = s.value("disconnectChannel", false).toBool();
}

