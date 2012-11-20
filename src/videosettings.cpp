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
#include "commonlib.h"
#include "logdialog.h"

VideoSettings* VideoSettings::s_object;
unsigned int VideoSettings::s_modifiedId = 1; // idは0にはならない

QList<VideoSettings::VideoProfile> VideoSettings::s_profiles;
QString      VideoSettings::s_modifiedIdFile;
QString      VideoSettings::s_defaultProfile;

#define INIT_PROFILE_NAME QObject::tr("profile1") // プロファイル名初期値

void VideoSettings::loadProfiles()
{
    if( !checkReload() ) return;

    convertPrevSettingsVer0_6_1ToSettingsVer0_7_0();

    QStringList profilesOrder;

    s_profiles.clear();

    // 全プロファイルデータの読み込み
    {
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

    s_modifiedIdFile = s.value("modifiedId", QString()).toString();

    if( s_modifiedIdFile.isEmpty() )
        saveModifiedIdFile(s);

    s_defaultProfile = s.value("defaultProfile", QString()).toString();
    profilesOrder    = s.value("profilesOrder", QStringList()).toStringList();

    s.beginGroup("Profiles");

//  // profilesOrderには無いプロファイルは削除する
//  QStringList removeProfile = s.childGroups();
//  foreach(QString profile, profilesOrder)
//      removeProfile.removeOne(profile);

//  foreach(QString profile, removeProfile)
//      s.remove(profile);

    // プロファイルの読み込み
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
        appendProfile(p);

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

    advanceModifiedId();

    LogDialog::debug("VideoSettings::loadProfiles(): end");
}

bool VideoSettings::checkReload()
{
    if( s_modifiedIdFile.isNull() )
        return true;

    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

    QString id = s.value("modifiedId", QString()).toString();

    return id != s_modifiedIdFile;
}

void VideoSettings::updateProfile(const VideoProfile& profile)
{
    for(int i=0; i < s_profiles.size(); i++) {
        if( s_profiles[i].name == profile.name ) {
            // メモリの該当プロファイルの内容を更新
            s_profiles[i] = profile;

            // ファイル保存
            QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

            s.beginGroup("Profiles");
            s.beginGroup(profile.name);
            s.setValue("contrast",   profile.contrast);
            s.setValue("brightness", profile.brightness);
            s.setValue("saturation", profile.saturation);
            s.setValue("hue",        profile.hue);
            s.setValue("gamma",      profile.gamma);
            s.endGroup();
            s.endGroup();

            saveModifiedIdFile(s);

            advanceModifiedId();
            LogDialog::debug("VideoSettings::saveProfile(): end");
            return;
        }
    }

    LogDialog::debug("VideoSettings::saveProfile(): no updated");
}

bool VideoSettings::appendProfile(const VideoProfile& profile)
{
    // プロファイルが作成限界数に達してる場合
    if( s_profiles.size() >= 30 )
        return false;

    // プロファイル名が空、又は文字数が規定値を超える場合
    if( profile.name.isEmpty() || profile.name.size()>15 )
        return false;

    // 前方、後方に空白文字がある場合
    if( profile.name[0].isSpace() || profile.name[profile.name.size()-1].isSpace() )
        return false;

    // 同じプロファイル名が存在しない場合
    if( !VideoSettings::profile(profile.name).isValid() ) {
        QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

        s.beginGroup("Profiles");
        s.beginGroup(profile.name);
        s.setValue("contrast",   profile.contrast);
        s.setValue("brightness", profile.brightness);
        s.setValue("saturation", profile.saturation);
        s.setValue("hue",        profile.hue);
        s.setValue("gamma",      profile.gamma);
        s.endGroup();
        s.endGroup();

        s_profiles.append(profile);
        saveProfilesOrder(s); // saveModifiedIdFile(s)

        advanceModifiedId();
//      object()->emit requestRefresh();
        LogDialog::debug("VideoSettings::appendProfile(): end");
        return true;
    }
    else {
        LogDialog::debug("VideoSettings::appendProfile(): no created");
        return false;
    }
}

void VideoSettings::saveDefaultProfile(const QString& name)
{
    for(int i=0; i < s_profiles.size(); i++) {
        if( name == s_profiles[i].name ) {
            QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

            s.setValue("defaultProfile", name);

            s_defaultProfile = name;
            saveModifiedIdFile(s);

            advanceModifiedId();
            LogDialog::debug("VideoSettings::saveDefaultProfile(): end");
            return;
        }
    }

    LogDialog::debug("VideoSettings::saveDefaultProfile(): failure");
}

void VideoSettings::removeProfile(const QString& name)
{
    QSettings* s = new QSettings(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

    s->beginGroup("Profiles");
    s->remove(name);
    s->endGroup();

    int i;
    for(i=0; i < s_profiles.size(); i++) {
        if( s_profiles[i].name == name )
            break;
    }

    if( i < s_profiles.size() ) {
        s_profiles.removeAt(i);

        if( s_profiles.size() > 0 )
            saveProfilesOrder(*s);

        delete s;

        // プロファイルが空なら初期値のプロファイルを作る
        if( s_profiles.size() <= 0 ) {
            VideoProfile p;
            p.name = INIT_PROFILE_NAME;
            p.contrast = 0;
            p.brightness = 0;
            p.saturation = 0;
            p.hue = 0;
            p.gamma = 0;

            appendProfile(p);
        }

        // 削除したプロファイルがデフォルトプロファイルなら0番をデフォルトにする
        if( name == s_defaultProfile )
            saveDefaultProfile(s_profiles[0].name);

        advanceModifiedId();
//      object()->emit requestRefresh();
    }
    else
        delete s;

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
    VideoProfile p;

    // PurePlayer.iniから旧データ読み込み、削除
    {
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    if( s.contains("contrast")
     || s.contains("brightness")
     || s.contains("saturation")
     || s.contains("hue")
     || s.contains("gamma") )
    {
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

    }

    }

    // VideoSettings.iniへ新しい形式で保存
    if( p.isValid() ) {
        QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "VideoSettings");

        QStringList profilesOrder;
        profilesOrder = s.value("profilesOrder", QStringList()).toStringList();

        // 重複プロファイルがあるか探す
        int i;
        for(i=0; i < profilesOrder.size(); i++) {
            if( profilesOrder[i] == p.name )
                break;
        }

                                  // プロファイルが最大数なら新規のプロファイル変換は諦める
        if( i < profilesOrder.size()
         || (i >= profilesOrder.size() && profilesOrder.size() < 30) )
        {
            // ビデオ値保存
            s.beginGroup("Profiles");
            s.beginGroup(p.name);
            s.setValue("contrast",   p.contrast);
            s.setValue("brightness", p.brightness);
            s.setValue("saturation", p.saturation);
            s.setValue("hue",        p.hue);
            s.setValue("gamma",      p.gamma);
            s.endGroup();
            s.endGroup();

            if( i >= profilesOrder.size() && profilesOrder.size() < 30 ) {
                // オーダー保存
                profilesOrder << p.name;
                s.setValue("profilesOrder", profilesOrder);
            }

            saveModifiedIdFile(s);
            LogDialog::debug("VideoSettings::convertPrevSettingsVer0_6_1ToSettingsVer0_7_0(): converted the old data.");
        }
    }
}

void VideoSettings::saveModifiedIdFile(QSettings& s)
{
    QString id = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    s_modifiedIdFile = id;
    s.setValue("modifiedId", id);
}

void VideoSettings::saveProfilesOrder(QSettings& s)
{
    QStringList order;
    for(int i=0; i < s_profiles.size(); i++)
        order << s_profiles[i].name;

    s.setValue("profilesOrder", order);

    saveModifiedIdFile(s);

}

void VideoSettings::advanceModifiedId()
{
    s_modifiedId++;
    if( s_modifiedId == 0 )
        s_modifiedId = 1;

//  LogDialog::debug(QString("VideoSettings::advanceModifiedId(): %1").arg(s_modifiedId));
}

