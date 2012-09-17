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

#include "ui_videoadjustdialog.h"
#include "pureplayer.h"

class VideoAdjustDialog : public QDialog, Ui::VideoAdjustDialog
{
    Q_OBJECT

public:
    VideoAdjustDialog(QWidget* parent);
    ~VideoAdjustDialog() {}
    void setValue(const PurePlayer::VideoSettings&);

public slots:
    void updateSaveValue();
    void load();

signals:
    void clickedSaveButton();
    void changedContrast(int value);
    void changedBrightness(int value);
    void changedSaturation(int value);
    void changedHue(int value);
    void changedGamma(int value);

protected:
    void showEvent(QShowEvent*);

private slots:
    void updateSaveButton();

private:
    int  _saveContrast;
    int  _saveBrightness;
    int  _saveSaturation;
    int  _saveHue;
    int  _saveGamma;
};

inline VideoAdjustDialog::VideoAdjustDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    connect(_buttonSave, SIGNAL(clicked()),         this, SIGNAL(clickedSaveButton()));
    connect(_buttonSave, SIGNAL(clicked()),         this, SLOT(updateSaveValue()));
    connect(_buttonLoad, SIGNAL(clicked()),         this, SLOT(load()));
    connect(_spinBox,    SIGNAL(valueChanged(int)), this, SIGNAL(changedContrast(int)));
    connect(_spinBox,    SIGNAL(valueChanged(int)), this, SLOT(updateSaveButton()));
    connect(_spinBox_2,  SIGNAL(valueChanged(int)), this, SIGNAL(changedBrightness(int)));
    connect(_spinBox_2,  SIGNAL(valueChanged(int)), this, SLOT(updateSaveButton()));
    connect(_spinBox_3,  SIGNAL(valueChanged(int)), this, SIGNAL(changedSaturation(int)));
    connect(_spinBox_3,  SIGNAL(valueChanged(int)), this, SLOT(updateSaveButton()));
    connect(_spinBox_4,  SIGNAL(valueChanged(int)), this, SIGNAL(changedHue(int)));
    connect(_spinBox_4,  SIGNAL(valueChanged(int)), this, SLOT(updateSaveButton()));
    connect(_spinBox_5,  SIGNAL(valueChanged(int)), this, SIGNAL(changedGamma(int)));
    connect(_spinBox_5,  SIGNAL(valueChanged(int)), this, SLOT(updateSaveButton()));

    updateSaveValue();
}

inline void VideoAdjustDialog::setValue(const PurePlayer::VideoSettings& v)
{
    _spinBox->setValue(v.contrast);
    _spinBox_2->setValue(v.brightness);
    _spinBox_3->setValue(v.saturation);
    _spinBox_4->setValue(v.hue);
    _spinBox_5->setValue(v.gamma);
}

inline void VideoAdjustDialog::updateSaveValue()
{
    _saveContrast   = _spinBox->value();
    _saveBrightness = _spinBox_2->value();
    _saveSaturation = _spinBox_3->value();
    _saveHue        = _spinBox_4->value();
    _saveGamma      = _spinBox_5->value();

    _buttonSave->setEnabled(false);
}

inline void VideoAdjustDialog::load()
{
    PurePlayer::VideoSettings v;
    v.contrast   = _saveContrast;
    v.brightness = _saveBrightness;
    v.saturation = _saveSaturation;
    v.hue        = _saveHue;
    v.gamma      = _saveGamma;

    setValue(v);
}

inline void VideoAdjustDialog::showEvent(QShowEvent*)
{
    setFixedSize(size());
}

inline void VideoAdjustDialog::updateSaveButton()
{
    if(_saveContrast   != _spinBox->value()   ||
       _saveBrightness != _spinBox_2->value() ||
       _saveSaturation != _spinBox_3->value() ||
       _saveHue        != _spinBox_4->value() ||
       _saveGamma      != _spinBox_5->value() )
    {
        _buttonSave->setEnabled(true);
    }
    else
        _buttonSave->setEnabled(false);
}

#endif // VIDEOADJUSTDIALOG_H

