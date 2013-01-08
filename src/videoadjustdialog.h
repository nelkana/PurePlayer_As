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
#ifndef VIDEOADJUSTDIALOG_H
#define VIDEOADJUSTDIALOG_H

#include "ui_videoadjustdialog.h"
#include "videosettings.h"
#include "pureplayer.h"

class VideoAdjustDialog : public QDialog, Ui::VideoAdjustDialog
{
    Q_OBJECT

public:
    VideoAdjustDialog(QWidget* parent);
    void setContrast(const int value)   { _spinBox->setValue(value);   }
    void setBrightness(const int value) { _spinBox_2->setValue(value); }
    void setSaturation(const int value) { _spinBox_3->setValue(value); }
    void setHue(const int value)        { _spinBox_4->setValue(value); }
    void setGamma(const int value)      { _spinBox_5->setValue(value); }

    void setProfiles(const QStringList& names);
    void appendProfile(const QString& name) { _comboBoxProfile->addItem(name); }
    void removeProfile(const QString& name);
    void setProfile(const QString& name);
    void setProfile(const VideoSettings::VideoProfile& profile);
    void setDefaultProfile(const QString& name);

signals:
    void windowActivate();
    void requestSave();
    void requestLoad();
    void requestCreate(const QString& name);
    void requestRemove();
    void requestDefault();
    void changedProfile(const QString& name);
    void changedContrast(int value);
    void changedBrightness(int value);
    void changedSaturation(int value);
    void changedHue(int value);
    void changedGamma(int value);

protected slots:
    void comboBoxProfileCurrentIndexChanged(const QString& name);
    void buttonSaveClicked();
    void buttonCreateClicked();
    void buttonRemoveClicked();

protected:
    bool event(QEvent*);
    void showEvent(QShowEvent*);
    void updateButtonDefaultEnableDisable(); // 本データ側がデフォルトプロファイルを
                                             // 持たないのでボタンdisable化保留。
private:
    QString _defaultProfile;
};

#endif // VIDEOADJUSTDIALOG_H

