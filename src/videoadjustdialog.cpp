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
#include <QMessageBox>
#include "videoadjustdialog.h"
#include "inputdialog.h"
#include "logdialog.h"

VideoAdjustDialog::VideoAdjustDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    connect(_buttonSave,    SIGNAL(clicked()),      this, SLOT(buttonSaveClicked()));
    connect(_buttonRestore, SIGNAL(clicked()),      this, SIGNAL(requestLoad()));
    connect(_buttonCreate,  SIGNAL(clicked()),      this, SLOT(buttonCreateClicked()));
    connect(_buttonRemove,  SIGNAL(clicked()),      this, SLOT(buttonRemoveClicked()));
    connect(_buttonDefault, SIGNAL(clicked()),      this, SIGNAL(requestDefault()));
    connect(_spinBox,    SIGNAL(valueChanged(int)), this, SIGNAL(changedContrast(int)));
    connect(_spinBox_2,  SIGNAL(valueChanged(int)), this, SIGNAL(changedBrightness(int)));
    connect(_spinBox_3,  SIGNAL(valueChanged(int)), this, SIGNAL(changedSaturation(int)));
    connect(_spinBox_4,  SIGNAL(valueChanged(int)), this, SIGNAL(changedHue(int)));
    connect(_spinBox_5,  SIGNAL(valueChanged(int)), this, SIGNAL(changedGamma(int)));
    connect(_comboBoxProfile,  SIGNAL(currentIndexChanged(const QString&)),
            this,              SLOT(comboBoxProfileCurrentIndexChanged(const QString&)));

    // プロファイル作成ボタン、削除ボタンの幅の設定
    int w = _buttonCreate->fontMetrics().width("+") + 20;
    if( w < 30 )
        w = 30;

    if( w < _buttonCreate->sizeHint().width() ) {
        _buttonCreate->setMaximumWidth(w);
        _buttonRemove->setMaximumWidth(w);
    }
}

void VideoAdjustDialog::setProfiles(const QStringList& names)
{
    disconnect(_comboBoxProfile,  SIGNAL(currentIndexChanged(const QString&)),
               this,              SLOT(comboBoxProfileCurrentIndexChanged(const QString&)));

    _comboBoxProfile->clear();
    _comboBoxProfile->addItems(names);

    connect(_comboBoxProfile,  SIGNAL(currentIndexChanged(const QString&)),
            this,              SLOT(comboBoxProfileCurrentIndexChanged(const QString&)));

    updateButtonDefaultEnableDisable();
}

void VideoAdjustDialog::removeProfile(const QString& name)
{
    int i = _comboBoxProfile->findText(name);
    if( i != -1 )
        _comboBoxProfile->removeItem(i);

    updateButtonDefaultEnableDisable();
}

void VideoAdjustDialog::setProfile(const QString& name)
{
    int i = _comboBoxProfile->findText(name);
    if( i != -1 ) {
        disconnect(_comboBoxProfile,  SIGNAL(currentIndexChanged(const QString&)),
                   this,              SLOT(comboBoxProfileCurrentIndexChanged(const QString&)));

        _comboBoxProfile->setCurrentIndex(i);
        updateButtonDefaultEnableDisable();

        connect(_comboBoxProfile,  SIGNAL(currentIndexChanged(const QString&)),
                this,              SLOT(comboBoxProfileCurrentIndexChanged(const QString&)));
    }
}

void VideoAdjustDialog::setProfile(const VideoSettings::VideoProfile& profile)
{
    setProfile(profile.name);
    setContrast(profile.contrast);
    setBrightness(profile.brightness);
    setSaturation(profile.saturation);
    setHue(profile.hue);
    setGamma(profile.gamma);
}

void VideoAdjustDialog::setDefaultProfile(const QString& name)
{
    _defaultProfile = name;
    updateButtonDefaultEnableDisable();
}

void VideoAdjustDialog::comboBoxProfileCurrentIndexChanged(const QString& name)
{
    updateButtonDefaultEnableDisable();

    emit changedProfile(name);
}

void VideoAdjustDialog::buttonSaveClicked()
{
    QMessageBox::StandardButton ret =
        QMessageBox::question(
            this,
            tr("このプロファイルを更新しますか？"),
            tr("このプロファイルを現在の内容で更新します。\nよろしいですか？"),
            QMessageBox::Yes|QMessageBox::No,
            QMessageBox::No);

    if( ret == QMessageBox::Yes )
        emit requestSave();
}

void VideoAdjustDialog::buttonCreateClicked()
{
    if( _comboBoxProfile->count() >= 30 ) {
        QMessageBox::warning(this, "エラー", "作成できるプロファイルは30個までになります");
        return;
    }

    QString name;
    QString initText;

    while( 1 )
    {
        name = InputDialog::input(this, tr("新規プロファイル"),
                                     tr("新しいプロファイル名を入力して下さい"), initText);

        // okボタン以外でウィンドウが閉じられた
        if( name.isNull() )
            return;

        // 前方、後方の空白文字を取り除く
        name.remove(QRegExp("^\\s*"));
        name.remove(QRegExp("\\s*$"));

        // 既に同じ名前のプロファイルが存在するか探す
        int i;
        for(i=0; i < _comboBoxProfile->count(); i++) {
            if( name == _comboBoxProfile->itemText(i) )
                break;
        }

        if( i < _comboBoxProfile->count() ) {
            QMessageBox::warning(this, "エラー", "既に存在するプロファイル名は指定できません");
            initText = name;
        }
        else if( name.isEmpty() ) {
            QMessageBox::warning(this, "エラー", "名前が空のプロファイルは作成できません");
            initText = "";
        }
        else if( name.size() > 15 ) {
            QMessageBox::warning(this, "エラー", "プロファイル名は15文字以内で入力して下さい");
            initText = name;
        }
        else
            break;
    }

    emit requestCreate(name);
}

void VideoAdjustDialog::buttonRemoveClicked()
{
    QMessageBox::StandardButton ret =
        QMessageBox::warning(
            this,
            tr("このプロファイルを削除しますか？"),
            tr("このプロファイルを削除してもよろしいですか？"),
            QMessageBox::Yes|QMessageBox::No,
            QMessageBox::No);

    if( ret == QMessageBox::Yes )
        emit requestRemove();
}

bool VideoAdjustDialog::event(QEvent* e)
{
    if( e->type() == QEvent::WindowActivate )
        emit windowActivate();

    return QDialog::event(e);
}

void VideoAdjustDialog::showEvent(QShowEvent*)
{
    setFixedSize(size());
// フォーカスアウトしても、フォーカスがまた移ってしまうので保留
//  _spinBox->clearFocus();
//  _spinBox_2->clearFocus();
//  _spinBox_3->clearFocus();
//  _spinBox_4->clearFocus();
//  _spinBox_5->clearFocus();
}

void VideoAdjustDialog::updateButtonDefaultEnableDisable()
{
    if( _comboBoxProfile->currentText() == _defaultProfile )
        _buttonDefault->setEnabled(false);
    else
        _buttonDefault->setEnabled(true);
}

