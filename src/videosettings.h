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
#ifndef VIDEOSETTINGS_H
#define VIDEOSETTINGS_H

#include <QApplication>
#include <QSettings>

class VideoSettings : public QObject
{
    Q_OBJECT

public:
    class VideoProfile
    {
    public:
        QString name;
        qint8 contrast;
        qint8 brightness;
        qint8 hue;
        qint8 saturation;
        qint8 gamma;

        bool isValid() { return !name.isNull(); }
    };

    VideoSettings(QObject* parent) : QObject(parent) {}
    static VideoSettings* object();
    static unsigned int modifiedId() { return s_modifiedId; }

    static void loadProfiles();
    static bool checkReload();
    static void updateProfile(const VideoProfile&);
    static bool appendProfile(const VideoProfile&);
    static void saveDefaultProfile(const QString& name);
    static void removeProfile(const QString& name);

    static VideoProfile profile(const QString& name);
    static VideoProfile profile(int index);
    static int profilesCount()          { return s_profiles.size(); }
    static QStringList profileNames();
    static QString defaultProfileName() { return s_defaultProfile; }
//  static QList<VideoProfile>& profiles() { return s_profiles; }

//signals:
//  void requestRefresh();

protected:
    static void convertPrevSettingsVer0_6_1ToSettingsVer0_7_0();
    static void saveModifiedIdFile(QSettings&);
    static void saveProfilesOrder(QSettings&);
    static void advanceModifiedId();

private:
    static VideoSettings* s_object;
    static unsigned int s_modifiedId;

    static QList<VideoProfile> s_profiles;
    static QString s_modifiedIdFile;
    static QString s_defaultProfile;
};

inline VideoSettings* VideoSettings::object()
{
    if( s_object == NULL )
        s_object = new VideoSettings(qApp);

    return s_object;
}

#endif // VIDEOSETTINGS_H

