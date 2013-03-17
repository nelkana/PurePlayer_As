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
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include "configdialog.h"
#include "logdialog.h"

ConfigDialog::ConfigDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);

    _buttonBox->button(QDialogButtonBox::Apply)->setFocusPolicy(Qt::NoFocus);
    _buttonBox->button(QDialogButtonBox::Ok)->setFocusPolicy(Qt::NoFocus);
    _buttonBox->button(QDialogButtonBox::Cancel)->setFocusPolicy(Qt::NoFocus);

    connect(_buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()),
            this, SLOT(apply()));
    connect(_buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(ok()));
    connect(_buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(hide()));

    _data = NULL;

    QRegExp rx("^\t(.+)\t");
    QProcess p;

    _comboBoxVo->addItem(tr("指定無し"));
    p.start("mplayer", QStringList() << "-vo" << "help");
    if( p.waitForFinished() ) {
        QStringList out = QString(p.readAllStandardOutput()).split("\n");

        for(int i=0; i < out.size(); ++i) {
            if( rx.indexIn(out[i]) != -1 )
                _comboBoxVo->addItem(rx.cap(1));
        }
    }

    _comboBoxAo->addItem(tr("指定無し"));
    p.start("mplayer", QStringList() << "-ao" << "help");
    if( p.waitForFinished() ) {
        QStringList out = QString(p.readAllStandardOutput()).split("\n");

        for(int i=0; i < out.size(); ++i) {
            if( rx.indexIn(out[i]) != -1 )
                _comboBoxAo->addItem(rx.cap(1));
        }
    }

#ifdef Q_OS_LINUX

#ifdef QT_NO_DEBUG_OUTPUT
    _checkBoxSoftVideoEq->setEnabled(false);
#else
    connect(_checkBoxSoftVideoEq, SIGNAL(clicked(bool)),
            this,                 SLOT(checkBoxSoftVideoEq_clicked(bool)));
#endif // QT_NO_DEBUG_OUTPUT

#else
    _checkBoxSoftVideoEq->setEnabled(false);
#endif // Q_OS_LINUX

    _groupBoxScreenshotPath->setFocusPolicy(Qt::NoFocus); // qtデザイナでは効果無し、ここで指定
    _groupBoxMplayerPath->setFocusPolicy(Qt::NoFocus);
    _groupBoxContactUrlPath->setFocusPolicy(Qt::NoFocus);
    _checkBoxScreenshot->setVisible(false); // debug

    QPalette palette = _groupBoxScreenshotPath->palette();
    QColor colorText = palette.color(QPalette::Text);
    palette.setColor(QPalette::Active, QPalette::Text, palette.color(QPalette::WindowText));
    palette.setColor(QPalette::Inactive, QPalette::Text, palette.color(QPalette::WindowText));
    _groupBoxScreenshotPath->setPalette(palette);
    _groupBoxMplayerPath->setPalette(palette);
    _groupBoxContactUrlPath->setPalette(palette);
    palette.setColor(QPalette::Active, QPalette::Text, colorText);
    palette.setColor(QPalette::Inactive, QPalette::Text, colorText);
    _lineEditScreenshotPath->setPalette(palette);
    _lineEditMplayerPath->setPalette(palette);
    _lineEditContactUrlPath->setPalette(palette);
    _lineEditContactUrlArg->setPalette(palette);

    connect(_buttonScreenshotPath, SIGNAL(clicked()), this, SLOT(setScreenshotPathFromDialog()));
    connect(_buttonMplayerPath, SIGNAL(clicked()), this, SLOT(setMplayerPathFromDialog()));
    connect(_buttonContactUrlPath, SIGNAL(clicked()), this, SLOT(setContactUrlPathFromDialog()));
    connect(_buttonContactUrlArg, SIGNAL(clicked()), this, SLOT(buttonContactUrlArg_clicked()));

    // コンタクトURLパス指定部のエディットボックスの幅設定
    int w = _lineEditContactUrlArg->fontMetrics().width(ConfigData::CONTACTURL_ARG_DEFAULT) + 20;
    _lineEditContactUrlArg->setMinimumWidth(w);
    _lineEditContactUrlPath->setMinimumWidth(w);

    // コンタクトURLパス選択ボタン、引数ボタンの幅設定
    w = _buttonContactUrlPath->fontMetrics().width(_buttonContactUrlPath->text()) + 14;
    if( w < 30 )
        w = 30;

    if( w < _buttonContactUrlPath->sizeHint().width() )
        _buttonContactUrlPath->setMaximumWidth(w);

    w = _buttonContactUrlArg->fontMetrics().width(_buttonContactUrlArg->text()) + 14;
    if( w < 30 )
        w = 30;

    if( w < _buttonContactUrlArg->sizeHint().width() )
        _buttonContactUrlArg->setMaximumWidth(w);

    resize(10,10);
}

void ConfigDialog::setData(ConfigData::Data* data)
{
    if( data == NULL ) return;

    _data = data;

    int index = _comboBoxVo->findText(_data->voName);
    if( index < 0 )
        index = 0;

    _comboBoxVo->setCurrentIndex(index);

    index = _comboBoxAo->findText(_data->aoName);
    if( index < 0 )
        index = 0;

    _comboBoxAo->setCurrentIndex(index);

    _spinBoxCacheStream->setValue(_data->cacheStreamSize);
    _spinBoxVolumeMax->setValue(_data->volumeMax);
    _checkBox320x240->setChecked(_data->openIn320x240Size);
    _checkBoxSoftVideoEq->setChecked(_data->useSoftWareVideoEq);
    _checkBoxScreenshot->setChecked(_data->screenshot);
    _checkBoxDisconnectChannel->setChecked(_data->disconnectChannel);
    _groupBoxScreenshotPath->setChecked(_data->useScreenshotPath);
    _lineEditScreenshotPath->setText(_data->screenshotPath);
    _groupBoxMplayerPath->setChecked(_data->useMplayerPath);
    _lineEditMplayerPath->setText(_data->mplayerPath);
    _groupBoxContactUrlPath->setChecked(_data->useContactUrlPath);
    _lineEditContactUrlPath->setText(_data->contactUrlPath);
    _lineEditContactUrlArg->setText(_data->contactUrlArg);
}

void ConfigDialog::apply()
{
    bool restart = false;
    QString driverName;

    int index = _comboBoxVo->currentIndex();
    if( index == 0 )
        driverName = "";
    else
        driverName = _comboBoxVo->currentText();

    if( driverName != _data->voName )
        restart = true;

    _data->voName = driverName;

    index = _comboBoxAo->currentIndex();
    if( index == 0 )
        driverName = "";
    else
        driverName = _comboBoxAo->currentText();

    if( driverName != _data->aoName )
        restart = true;

    _data->aoName = driverName;

    if( _spinBoxCacheStream->value() != _data->cacheStreamSize )
        restart = true;

    _data->openIn320x240Size = _checkBox320x240->isChecked();

    if( _checkBoxSoftVideoEq->isChecked() != _data->useSoftWareVideoEq )
        restart = true;

    _data->useSoftWareVideoEq = _checkBoxSoftVideoEq->isChecked();

    if( _checkBoxScreenshot->isChecked() != _data->screenshot )
        restart = true;

    _data->screenshot = _checkBoxScreenshot->isChecked();
    _data->disconnectChannel = _checkBoxDisconnectChannel->isChecked();

    _data->volumeMax = _spinBoxVolumeMax->value();
    _data->cacheStreamSize = _spinBoxCacheStream->value();

    if( _groupBoxScreenshotPath->isChecked() != _data->useScreenshotPath )
        restart = true;

    _data->useScreenshotPath = _groupBoxScreenshotPath->isChecked();

    if( _groupBoxScreenshotPath->isChecked()
     && _lineEditScreenshotPath->text() != _data->screenshotPath )
    {
        restart = true;
    }

    _data->screenshotPath = _lineEditScreenshotPath->text();

    if( _groupBoxMplayerPath->isChecked() != _data->useMplayerPath )
        restart = true;

    _data->useMplayerPath = _groupBoxMplayerPath->isChecked();

    if( _groupBoxMplayerPath->isChecked()
     && _lineEditMplayerPath->text() != _data->mplayerPath )
    {
        restart = true;
    }

    _data->mplayerPath = _lineEditMplayerPath->text();

    _data->useContactUrlPath = _groupBoxContactUrlPath->isChecked();
    _data->contactUrlPath = _lineEditContactUrlPath->text();
    _data->contactUrlArg = _lineEditContactUrlArg->text();

    emit applied(restart);
}

void ConfigDialog::setScreenshotPathFromDialog()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("ディレクトリを選択"));
//                                                   , "", QFileDialog::ShowDirsOnly);

    if( !path.isEmpty() )
        _lineEditScreenshotPath->setText(path);
}

void ConfigDialog::setMplayerPathFromDialog()
{
    QString path = QFileDialog::getOpenFileName(this, tr("ファイルを選択"), "",
                                                tr("全てのファイル(*)"));

    if( !path.isEmpty() )
        _lineEditMplayerPath->setText(path);
}

void ConfigDialog::setContactUrlPathFromDialog()
{
    QString path = QFileDialog::getOpenFileName(this, tr("ファイルを選択"), "",
                                                tr("全てのファイル(*)"));

    if( !path.isEmpty() )
        _lineEditContactUrlPath->setText(path);
}

void ConfigDialog::showEvent(QShowEvent*)
{
//  adjustSize();
    setFixedSize(size());
    _spinBoxCacheStream->clearFocus();
    _spinBoxVolumeMax->clearFocus();
    _lineEditScreenshotPath->clearFocus();
    _lineEditMplayerPath->clearFocus();
    _lineEditContactUrlPath->clearFocus();
    _lineEditContactUrlArg->clearFocus();
}

void ConfigDialog::checkBoxSoftVideoEq_clicked(bool checked)
{
    if( !checked ) {
        QMessageBox::StandardButton ret =
            QMessageBox::warning(
                this,
                tr("警告 設定を変更し、無効にしますか？"),
                tr("この設定を無効にするとビデオ調整(明るさ等)の設定がグローバルに変更され、"
                "他のメディアプレイヤーの画面表示に影響を与えます。\n"
                "特に問題が無い限り、有効のままにして頂く事をお勧めします。\n\n"
                "設定を変更し、無効にしますか？"),
                QMessageBox::Yes|QMessageBox::No,
                QMessageBox::No);

        if( ret == QMessageBox::No )
            _checkBoxSoftVideoEq->setCheckState(Qt::Checked);
    }
}

void ConfigDialog::buttonContactUrlArg_clicked()
{
    _lineEditContactUrlArg->setText(ConfigData::CONTACTURL_ARG_DEFAULT);
}

