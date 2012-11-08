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
#include <QDateTime>
#include "videosettings.h"
#include "logdialog.h"

QList<VideoSettings::VideoProfile> VideoSettings::s_profiles;
QString      VideoSettings::s_modifiedTime;
QString      VideoSettings::s_defaultProfile;

#define INIT_PROFILE_NAME QObject::tr("profile1") // プロファイル名初期値

void VideoSettings::loadProfiles()
{
    convertPrevSettingsVer0_6_1ToSettingsVer0_7_0();

    QStringList profilesOrder;

    s_profiles.clear();

    // 全プロファイルデータの読み込み
    {
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

    s_modifiedTime   = s.value("modifiedTime", QString()).toString();
    s_defaultProfile = s.value("defaultProfile", "").toString();
    profilesOrder    = s.value("profilesOrder", QStringList()).toStringList();
        // メモ: profilesOrderに無いProfilesのデータは、設定ファイルの
        //       profilesOrder行を削除しない限り、Profilesに無駄なデータが残り続ける。

    s.beginGroup("Profiles");

    // 保留: プロファイル同期ずれを考慮してプロファイル削除、オーダー決定

    if( profilesOrder.size() <= 0 )
        profilesOrder = s.childGroups();

    for(int i=0; i < profilesOrder.size(); i++) {
        s.beginGroup(profilesOrder[i]);

        VideoProfile v;
        v.name = profilesOrder[i];
        v.contrast   = s.value("contrast",   0).toInt();
        v.brightness = s.value("brightness", 0).toInt();
        v.saturation = s.value("saturation", 0).toInt();
        v.hue        = s.value("hue",        0).toInt();
        v.gamma      = s.value("gamma",      0).toInt();
        s_profiles.append(v);

        s.endGroup();
    }

    s.endGroup();

    }

    if( s_profiles.size() <= 0 ) {
        // プロファイルが1つもない場合は作成する
        VideoProfile p;
        p.name = INIT_PROFILE_NAME;
        p.contrast = 0;
        p.brightness = 0;
        p.saturation = 0;
        p.hue = 0;
        p.gamma = 0;
        saveProfile(p);

        s_defaultProfile = p.name;
    }
    else {
        // デフォルトプロファイルが存在しないなら、デフォルトプロファイルを0番にする
        int i;
        for(i=0; i < s_profiles.size(); i++) {
            if( s_profiles[i].name == s_defaultProfile )
                break;
        }

        if( i >= s_profiles.size() )
            s_defaultProfile = s_profiles[0].name;
    }

    LogDialog::debug("VideoSettings::loadProfiles(): end");
}

bool VideoSettings::checkReload()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

    QString time = s.value("modifiedTime", QString()).toString();
    return time != s_modifiedTime;
}

void VideoSettings::saveProfile(const VideoProfile& profile)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

    s.beginGroup("Profiles");
    s.beginGroup(profile.name);
    s.setValue("contrast",   profile.contrast);
    s.setValue("brightness", profile.brightness);
    s.setValue("saturation", profile.saturation);
    s.setValue("hue",        profile.hue);
    s.setValue("gamma",      profile.gamma);
    s.endGroup();
    s.endGroup();

    // メモリの該当プロファイルの内容を更新
    int i;
    for(i=0; i < s_profiles.size(); i++) {
        if( s_profiles[i].name == profile.name ) {
            s_profiles[i] = profile;
            break;
        }
    }

    if( i >= s_profiles.size() ) {
        // 該当プロファイルが無い場合、追加する
        s_profiles.append(profile);
        saveProfilesOrder(s); // saveModifiedTime(s)
    }
    else
        saveModifiedTime(s);

    LogDialog::debug("VideoSettings::saveProfile(): end");
}

void VideoSettings::saveDefaultProfile(const QString& name)
{
    for(int i=0; i < s_profiles.size(); i++) {
        if( name == s_profiles[i].name ) {
            QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

            s.setValue("defaultProfile", name);

            saveModifiedTime(s);

            LogDialog::debug("VideoSettings::saveDefaultProfile(): end");
            return;
        }
    }

    LogDialog::debug("VideoSettings::saveDefaultProfile(): failure");
}

void VideoSettings::removeProfile(const QString& name)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

    s.beginGroup("Profiles");
    s.remove(name);
    s.endGroup();

    for(int i=0; i < s_profiles.size(); i++) {
        if( s_profiles[i].name == name ) {
            s_profiles.removeAt(i);
            saveProfilesOrder(s);
            break;
        }
    }

    LogDialog::debug("VideoSettings::removeProfile(): end");
}

VideoSettings::VideoProfile VideoSettings::profile(const QString& name)
{
    for(int i=0; i < s_profiles.size(); i++) {
        if( s_profiles[i].name == name )
            return s_profiles[i];
    }

    return VideoProfile();
}

VideoSettings::VideoProfile VideoSettings::profile(int index)
{
    if( index <= s_profiles.size() )
        return s_profiles[index];
    else
        return VideoProfile();
}

QStringList VideoSettings::profileNames()
{
    QStringList names;
    for(int i=0; i < s_profiles.size(); i++)
        names << s_profiles[i].name;

    return names;
}

// 設定ファイルにversion 0.6.1以前のビデオ調整データがあった場合、新しい形式で保存
void VideoSettings::convertPrevSettingsVer0_6_1ToSettingsVer0_7_0()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    if( s.contains("contrast")
     || s.contains("brightness")
     || s.contains("saturation")
     || s.contains("hue")
     || s.contains("gamma") )
    {
        VideoProfile p;

        p.name = INIT_PROFILE_NAME; // 同じ名前のプロファイルがあった場合、上書きされる
        p.contrast   = s.value("contrast",   0).toInt();
        p.brightness = s.value("brightness", 0).toInt();
        p.saturation = s.value("saturation", 0).toInt();
        p.hue        = s.value("hue",        0).toInt();
        p.gamma      = s.value("gamma",      0).toInt();
        s.remove("contrast");
        s.remove("brightness");
        s.remove("saturation");
        s.remove("hue");
        s.remove("gamma");

        saveProfile(p);

        LogDialog::debug("VideoSettings::convertPrevSettingsVer0_6_1ToSettingsVer0_7_0(): converted the old data.");
    }
}

void VideoSettings::saveModifiedTime(QSettings& s)
{
//  QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

    QString time = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    s_modifiedTime = time;
    s.setValue("modifiedTime", time);
}

void VideoSettings::saveProfilesOrder(QSettings& s)
{
//  QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "VideoSettings");

    QStringList order;
    for(int i=0; i < s_profiles.size(); i++)
        order << s_profiles[i].name;

    s.setValue("profilesOrder", order);

    saveModifiedTime(s);
}

