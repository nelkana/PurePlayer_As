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
#ifndef VIDEOADJUSTDIALOG_H
#define VIDEOADJUSTDIALOG_H

#include <QMessageBox>
#include "ui_videoadjustdialog.h"
#include "pureplayer.h"

class VideoAdjustDialog : public QDialog, Ui::VideoAdjustDialog
{
    Q_OBJECT

public:
    VideoAdjustDialog(QWidget* parent);
    void setSettings(const PurePlayer::VideoSettings&);
    void setContrast(const int value)   { _spinBox->setValue(value);   }
    void setBrightness(const int value) { _spinBox_2->setValue(value); }
    void setSaturation(const int value) { _spinBox_3->setValue(value); }
    void setHue(const int value)        { _spinBox_4->setValue(value); }
    void setGamma(const int value)      { _spinBox_5->setValue(value); }

signals:
    void requestSave();
    void requestLoad();
    void changedContrast(int value);
    void changedBrightness(int value);
    void changedSaturation(int value);
    void changedHue(int value);
    void changedGamma(int value);

protected slots:
    void buttonSaveClicked();

protected:
    void showEvent(QShowEvent*);
};

inline VideoAdjustDialog::VideoAdjustDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    connect(_buttonSave, SIGNAL(clicked()),         this, SLOT(buttonSaveClicked()));
    connect(_buttonLoad, SIGNAL(clicked()),         this, SIGNAL(requestLoad()));
    connect(_spinBox,    SIGNAL(valueChanged(int)), this, SIGNAL(changedContrast(int)));
    connect(_spinBox_2,  SIGNAL(valueChanged(int)), this, SIGNAL(changedBrightness(int)));
    connect(_spinBox_3,  SIGNAL(valueChanged(int)), this, SIGNAL(changedSaturation(int)));
    connect(_spinBox_4,  SIGNAL(valueChanged(int)), this, SIGNAL(changedHue(int)));
    connect(_spinBox_5,  SIGNAL(valueChanged(int)), this, SIGNAL(changedGamma(int)));
}

inline void VideoAdjustDialog::setSettings(const PurePlayer::VideoSettings& v)
{
    _spinBox->setValue(v.contrast);
    _spinBox_2->setValue(v.brightness);
    _spinBox_3->setValue(v.saturation);
    _spinBox_4->setValue(v.hue);
    _spinBox_5->setValue(v.gamma);
}

inline void VideoAdjustDialog::buttonSaveClicked()
{
    QMessageBox::StandardButton ret =
        QMessageBox::question(
            this,
            "設定を保存しますか？",
            "設定を保存してもよろしいですか？",
            QMessageBox::Yes|QMessageBox::No,
            QMessageBox::No);

    if( ret == QMessageBox::Yes )
        emit requestSave();
}

inline void VideoAdjustDialog::showEvent(QShowEvent*)
{
    setFixedSize(size());
}

#endif // VIDEOADJUSTDIALOG_H

