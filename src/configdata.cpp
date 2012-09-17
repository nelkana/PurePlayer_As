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
#include <QSettings>
#include <QDir>
#include "configdata.h"
#include "logdialog.h"

ConfigData::Data ConfigData::_data;

void ConfigData::saveData()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    s.setValue("voName",    _data.voName);
    s.setValue("aoName",    _data.aoName);
    s.setValue("cacheStreamSize", _data.cacheStreamSize);
    s.setValue("volumeMax", _data.volumeMax);
    s.setValue("openIn320x240Size",  _data.openIn320x240Size);
    s.setValue("useSoftWareVideoEq", _data.useSoftWareVideoEq);
    s.setValue("screenshot" , _data.screenshot);
    s.setValue("useScreenshotPath", _data.useScreenshotPath);
    s.setValue("screenshotPath", _data.screenshotPath);
    s.setValue("useMplayerPath", _data.useMplayerPath);
    s.setValue("mplayerPath", _data.mplayerPath);
}

void ConfigData::loadData()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    _data.voName = s.value("voName", "0").toString();

    if( _data.voName == "0" ) {
#if defined(Q_WS_X11)
        _data.voName = "xv";
//      _data.voName = "xv:adaptor=0";
#elif defined(Q_OS_WIN32)
        _data.voName = "directx";
#else
        _data.voName = "";
#endif
    }

    _data.aoName    = s.value("aoName", "").toString();
    _data.cacheStreamSize = s.value("cacheStreamSize", 1000).toInt();
    _data.volumeMax = s.value("volumeMax", 100).toInt();
    _data.openIn320x240Size = s.value("openIn320x240Size", true).toBool();
    _data.useSoftWareVideoEq = s.value("useSoftWareVideoEq", true).toBool();
    _data.screenshot = s.value("screenshot", false).toBool();
    _data.useScreenshotPath = s.value("useScreenshotPath", false).toBool();
    _data.screenshotPath = s.value("screenshotPath", QDir::homePath()).toString();
    _data.useMplayerPath = s.value("useMplayerPath", false).toBool();
    _data.mplayerPath = s.value("mplayerPath", "").toString();
}

