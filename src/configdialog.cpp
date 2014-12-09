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
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include "configdialog.h"
#include "pureplayer.h"
#include "logdialog.h"

ConfigDialog::ConfigDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);

    _buttonBox->button(QDialogButtonBox::Apply)->setFocusPolicy(Qt::NoFocus);
    _buttonBox->button(QDialogButtonBox::Ok)->setFocusPolicy(Qt::NoFocus);
    _buttonBox->button(QDialogButtonBox::Cancel)->setFocusPolicy(Qt::NoFocus);
    connect(_buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()),
            this, SIGNAL(applied()));
    connect(_buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(ok()));
    connect(_buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(hide()));

    _comboBoxVo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    _comboBoxVoClipping->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    _comboBoxAo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    QString voToolTip = tr(
            "使用するビデオドライバになります。\n"
            "初期設定は「%1」になります。");
    QString voNameDefault = ConfigData::VONAME_DEFAULT;
    if( voNameDefault.isEmpty() )
        voNameDefault = tr("指定無し");

    _comboBoxVo->setToolTip(voToolTip.arg(voNameDefault));

    QString voClippingToolTip = tr(
            "クリッピング機能使用時に自動で切り替えるビデオドライバになります。\n"
            "クリッピングが正常に機能するかは、選択するビデオドライバに依存します。\n"
            "初期設定は「%1」になります。");
    QString voNameForClippingDefault = ConfigData::VONAME_FOR_CLIPPING_DEFAULT;
    if( voNameForClippingDefault.isEmpty() )
        voNameForClippingDefault = tr("指定無し");

    _comboBoxVoClipping->setToolTip(voClippingToolTip.arg(voNameForClippingDefault));

    QString aoToolTip = tr(
            "使用するオーディオドライバになります。\n"
            "初期設定は「%1」になります。");
    QString aoNameDefault = ConfigData::AONAME_DEFAULT;
    if( aoNameDefault.isEmpty() )
        aoNameDefault = tr("指定無し");

    _comboBoxAo->setToolTip(aoToolTip.arg(aoNameDefault));

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

    _groupBoxCacheSize->setFocusPolicy(Qt::NoFocus);    // qtデザイナでは効果無し、ここで指定
    connect(_groupBoxCacheSize, SIGNAL(toggled(bool)),  // checkBoxを切り替えた時のspinBoxの選択状態を解除する
            this,               SLOT(groupBoxCacheSize_toggled(bool)));
    _groupBoxScreenshotPath->setFocusPolicy(Qt::NoFocus);
    _groupBoxMplayerPath->setFocusPolicy(Qt::NoFocus);
    _groupBoxLimitLogLine->setFocusPolicy(Qt::NoFocus);
    connect(_groupBoxLimitLogLine, SIGNAL(toggled(bool)),
            this,                  SLOT(groupBoxLimitLogLine_toggled(bool)));
    _groupBoxContactUrlPath->setFocusPolicy(Qt::NoFocus);

    QPalette palette = _groupBoxCacheSize->palette();
    QColor colorText = palette.color(QPalette::Text);
    palette.setColor(QPalette::Active, QPalette::Text, palette.color(QPalette::WindowText));
    palette.setColor(QPalette::Inactive, QPalette::Text, palette.color(QPalette::WindowText));
    _groupBoxCacheSize->setPalette(palette);
    _groupBoxScreenshotPath->setPalette(palette);
    _groupBoxMplayerPath->setPalette(palette);
    _groupBoxContactUrlPath->setPalette(palette);
    palette.setColor(QPalette::Active, QPalette::Text, colorText);
    palette.setColor(QPalette::Inactive, QPalette::Text, colorText);
    _spinBoxCacheStream->setPalette(palette);
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

void ConfigDialog::setData(const ConfigData::Data& data)
{
    _checkBoxSoftVideoEq->setChecked(data.useSoftWareVideoEq);
    _checkBox320x240->setChecked(data.openIn320x240Size);
    _checkBoxReverseWheelSeek->setChecked(data.reverseWheelSeek);
    _spinBoxVolumeMax->setValue(data.volumeMax);
    _groupBoxCacheSize->setChecked(data.useCacheSize);
    _spinBoxCacheStream->setValue(data.cacheStreamSize);
    _groupBoxScreenshotPath->setChecked(data.useScreenshotPath);
    _lineEditScreenshotPath->setText(data.screenshotPath);
    _groupBoxMplayerPath->setChecked(data.useMplayerPath);
    _lineEditMplayerPath->setText(data.mplayerPath);
    _groupBoxLimitLogLine->setChecked(data.limitLogLine);
    _spinBoxLimitLogLine->setValue(data.logLineMax);
    _checkBoxDisconnectChannel->setChecked(data.disconnectChannel);
    _groupBoxContactUrlPath->setChecked(data.useContactUrlPath);
    _lineEditContactUrlPath->setText(data.contactUrlPath);
    _lineEditContactUrlArg->setText(data.contactUrlArg);

    resetComboBoxVoAoItem(data);
}

void ConfigDialog::getData(ConfigData::Data* data)
{
    if( data == NULL ) return;

    data->voName = _comboBoxVo->itemData(_comboBoxVo->currentIndex()).toString();
    data->voNameForClipping = _comboBoxVoClipping->itemData(_comboBoxVoClipping->currentIndex()).toString();
    data->aoName = _comboBoxAo->itemData(_comboBoxAo->currentIndex()).toString();
    data->useSoftWareVideoEq = _checkBoxSoftVideoEq->isChecked();
    data->openIn320x240Size = _checkBox320x240->isChecked();
    data->reverseWheelSeek = _checkBoxReverseWheelSeek->isChecked();
    data->volumeMax = _spinBoxVolumeMax->value();
    data->useCacheSize = _groupBoxCacheSize->isChecked();
    data->cacheStreamSize = _spinBoxCacheStream->value();
    data->useScreenshotPath = _groupBoxScreenshotPath->isChecked();
    data->screenshotPath = _lineEditScreenshotPath->text();
    data->useMplayerPath = _groupBoxMplayerPath->isChecked();
    data->mplayerPath = _lineEditMplayerPath->text();
    data->limitLogLine = _groupBoxLimitLogLine->isChecked();
    data->logLineMax = _spinBoxLimitLogLine->value();
    data->disconnectChannel = _checkBoxDisconnectChannel->isChecked();
    data->useContactUrlPath = _groupBoxContactUrlPath->isChecked();
    data->contactUrlPath = _lineEditContactUrlPath->text();
    data->contactUrlArg = _lineEditContactUrlArg->text();
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
/*
#include <QMouseEvent>
bool ConfigDialog::eventFilter(QObject* o, QEvent* e)
{
    if( o == _tabWidget ) {
        if( e->type() == QEvent::MouseButtonPress
         || e->type() == QEvent::MouseMove
         || e->type() == QEvent::MouseButtonRelease )
        {
            QMouseEvent* ev = static_cast<QMouseEvent*>(e);
            QMouseEvent event(ev->type(),
                              _tabWidget->mapTo(this, ev->pos()),
                              _tabWidget->mapToGlobal(ev->pos()),
                              ev->button(), ev->buttons(), ev->modifiers());

            _wc->mouseEventProcess(this, &event);
        }
    }

    return QDialog::eventFilter(o, e);
}
*/
void ConfigDialog::showEvent(QShowEvent*)
{
//  adjustSize();
    setFixedSize(size());
    _spinBoxVolumeMax->clearFocus();
    _spinBoxCacheStream->clearFocus();
    _lineEditScreenshotPath->clearFocus();
    _lineEditMplayerPath->clearFocus();
    _spinBoxLimitLogLine->clearFocus();
    _lineEditContactUrlPath->clearFocus();
    _lineEditContactUrlArg->clearFocus();
}

void ConfigDialog::resetComboBoxVoAoItem(const ConfigData::Data& data)
{
    // 使用出来るドライバ項目を取得、初期化する
    QRegExp rx("^\t(.+)\t");
    QProcess p;

    QString mplayerPath;
    if( data.useMplayerPath )
        mplayerPath = data.mplayerPath;
    else
        mplayerPath = "mplayer";

    _comboBoxVo->clear();
    _comboBoxVo->addItem(tr("指定無し"), "");
    _comboBoxVoClipping->clear();
    _comboBoxVoClipping->addItem(tr("指定無し"), "");
    p.start(mplayerPath, QStringList() << "-vo" << "help");
    if( p.waitForFinished() ) {
        QStringList out = QString(p.readAllStandardOutput()).split("\n");

        for(int i=0; i < out.size(); ++i) {
            if( rx.indexIn(out[i]) != -1 ) {
                _comboBoxVo->addItem(rx.cap(1), rx.cap(1));
                _comboBoxVoClipping->addItem(rx.cap(1), rx.cap(1));
            }
        }
    }

    _comboBoxAo->clear();
    _comboBoxAo->addItem(tr("指定無し"), "");
    p.start(mplayerPath, QStringList() << "-ao" << "help");
    if( p.waitForFinished() ) {
        QStringList out = QString(p.readAllStandardOutput()).split("\n");

        for(int i=0; i < out.size(); ++i) {
            if( rx.indexIn(out[i]) != -1 )
                _comboBoxAo->addItem(rx.cap(1), rx.cap(1));
        }
    }

    // 項目を選択する
    int index;
    index = _comboBoxVo->findData(data.voName);
    if( index < 0 ) {
        _comboBoxVo->addItem(data.voName + tr("(使用不可)"), data.voName);
        index = _comboBoxVo->count() - 1;
    }
    _comboBoxVo->setCurrentIndex(index);

    index = _comboBoxVoClipping->findData(data.voNameForClipping);
    if( index < 0 ) {
        _comboBoxVoClipping->addItem(data.voNameForClipping + tr("(使用不可)"), data.voNameForClipping);
        index = _comboBoxVoClipping->count() - 1;
    }
    _comboBoxVoClipping->setCurrentIndex(index);

    index = _comboBoxAo->findData(data.aoName);
    if( index < 0 ) {
        _comboBoxAo->addItem(data.aoName + tr("(使用不可)"), data.aoName);
        index = _comboBoxAo->count() - 1;
    }
    _comboBoxAo->setCurrentIndex(index);
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

