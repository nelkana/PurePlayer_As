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
#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "pureplayer.h"
#include "process.h"
#include "controlbutton.h"
#include "timeslider.h"
#include "infolabel.h"
#include "timelabel.h"
#include "opendialog.h"
#include "videoadjustdialog.h"
#include "logdialog.h"
#include "configdialog.h"
#include "configdata.h"
#include "playlistdialog.h"
#include "commonlib.h"
#include "task.h"
#include "speedspinbox.h"
#include "aboutdialog.h"

#ifdef Q_OS_WIN32
#define COLORKEY 0x040504
#endif

PurePlayer::PurePlayer(QWidget* parent) : QMainWindow(parent)
{
    const QIcon appIcon(":/icons/heart.png");

    LogDialog::initDialog();
    LogDialog::dialog()->setWindowIcon(appIcon);
    connect(LogDialog::dialog(), SIGNAL(requestCommand(const QString&)),
            this,                SLOT(mpCmd(const QString&)));

    setFont(QFont("DejaVu Sans", 9));
    setWindowIcon(appIcon);
    setAcceptDrops(true);

    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(backgroundRole(), QColor("black"));
    setPalette(p);

    setCentralWidget(new QWidget(this));
    _videoScreen = new QWidget(centralWidget());
    _videoScreen->lower();
#ifdef Q_OS_WIN32
    _videoScreen->setAutoFillBackground(true);
    p = _videoScreen->palette();
    p.setColor(_videoScreen->backgroundRole(), QColor(COLORKEY));
    _videoScreen->setPalette(p);
#endif

//  _oldTime = -1;
    _path = "";
    _audioOutput = AO_STEREO;
    _volumeFactor = VF_NORMAL;
    _aspectRatio = RATIO_VIDEO;
    _deinterlace = DI_NO_DEINTERLACE;
    _doLoop = false;
    _openedNewPath = true;
    _seekWhenStartMplayer = false;
    _reconnectWasCalled = false;
    _isMaximizedBeforeFullScreen = false;
    _muteWhenMouseRelease = false;
    _videoSize = QSize(320, 240);
    _noVideo = false;
    _isSeekable = false;
    _cursorInWindow = false;
    _disableWindowMoveFromMouse = false;
    _controlFlags = FLG_NONE;

    _port = -1;
    _id   = "";

    _nam = new QNetworkAccessManager(this);
    connect(_nam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    _mpProcess = new MplayerProcess(this);
    connect(_mpProcess, SIGNAL(outputLine(const QString&)),
            this,       SLOT(parseMplayerOutputLine(const QString&)));
    connect(_mpProcess, SIGNAL(finished()),
            this,       SLOT(mpProcessFinished()));
    connect(_mpProcess, SIGNAL(error(QProcess::ProcessError)),
            this,       SLOT(mpProcessError(QProcess::ProcessError)));
    connect(_mpProcess, SIGNAL(debugKilledCPid()),
            this,       SLOT(mpProcessDebugKilledCPid()));

    _recordingProcess = new RecordingProcess(this);
    connect(_recordingProcess, SIGNAL(outputLine(const QString&)),
            this,              SLOT(recordingOutputLine(const QString&)));
//  connect(_recordingProcess, SIGNAL(finished()),
//          this,              SLOT(recordingProcessFinished()));

    _chName = "PurePlayer*";
    setWindowTitle(_chName);

    _receivedErrorCount = 0;
    _reconnectCount = 0;
    _reconnectControlTime  = 0;
    connect(&_timerReconnect, SIGNAL(timeout()), this, SLOT(timerReconnectTimeout()));

    _openDialog        = NULL;
    _videoAdjustDialog = NULL;
    _configDialog      = NULL;
    _playListDialog    = NULL;
    _aboutDialog       = NULL;

    createStatusBar();
    createActionContextMenu();
    createToolBar();
    ConfigData::loadData();
//  _actScreenshot->setVisible(ConfigData::data()->screenshot);
//  _screenshotButton->setVisible(ConfigData::data()->screenshot);
    loadVideoSettings();
    _state = STOP;
    loadInteractiveSettings();

    setStatus(STOP);
    resizeFromVideoScreen(_videoSize);

    _menuContext->move(x()+width()*0.2, y()+height()*0.2);

    _debugFlg = false;
    _debugCount = 0;
}

PurePlayer::~PurePlayer()
{
    delete statusBar()->style();

    RenameTimerTask::waitForTasksFinished();
}

void PurePlayer::createStatusBar()
{
    _infoLabel = new InfoLabel(tr("停止 "));
    _timeLabel = new TimeLabel();
    _labelVolume = new QLabel();
    _labelFrame = new QLabel();
    _statusbarSpaceL = new QWidget();
    _statusbarSpaceR = new QWidget();

    QPalette p;
#ifndef QT_NO_DEBUG_OUTPUT
/*  p = _timeLabel->palette();
    p.setColor(_timeLabel->backgroundRole(), QColor(Qt::blue));
    _timeLabel->setPalette(p);
    p = _infoLabel->palette();
    p.setColor(_infoLabel->backgroundRole(), QColor(Qt::blue));
    _infoLabel->setPalette(p);
    p = _labelVolume->palette();
    p.setColor(_labelVolume->backgroundRole(), QColor(Qt::blue));
    _labelVolume->setPalette(p);
    p = _labelFrame->palette();
    p.setColor(_labelFrame->backgroundRole(), QColor(Qt::blue));
    _labelFrame->setPalette(p);
    _timeLabel->setAutoFillBackground(true);
    _infoLabel->setAutoFillBackground(true);
    _labelVolume->setAutoFillBackground(true);
    _labelFrame->setAutoFillBackground(true);
*/
#endif

    statusBar();
    statusBar()->setStyle(QStyleFactory::create("plastique"));
    statusBar()->setSizeGripEnabled(false);
    statusBar()->setAutoFillBackground(true);
    p = statusBar()->palette();
    p.setColor(statusBar()->foregroundRole(), QColor(Qt::white));
    p.setColor(statusBar()->backgroundRole(), QColor(Qt::black));
    statusBar()->setPalette(p);

    statusBar()->addWidget(_statusbarSpaceL);
    statusBar()->addWidget(_infoLabel);
    statusBar()->addPermanentWidget(_labelFrame);
    statusBar()->addPermanentWidget(_timeLabel);
    statusBar()->addPermanentWidget(_labelVolume);
    statusBar()->addPermanentWidget(_statusbarSpaceR);

    // QLabelにおいて等倍フォントでも微妙に横幅が変わる為、最小サイズを固定にする
    _timeLabel->initMinimumWidth();

    _labelVolume->setFrameShape(QFrame::NoFrame);
    _labelVolume->setAlignment(Qt::AlignRight);
    _labelVolume->setText(tr(" 000"));
    _labelVolume->setMinimumWidth(_labelVolume->sizeHint().width());
    _labelVolume->setText("");

    _labelFrame->setAlignment(Qt::AlignRight);
    _labelFrame->setText(" 00000000");
    _labelFrame->setMinimumWidth(_labelFrame->sizeHint().width());
    _labelFrame->setText("0");

    _statusbarSpaceL->hide();
    _statusbarSpaceR->hide();
    _infoLabel->show();
    _timeLabel->show();
    _labelVolume->show();
#ifndef QT_NO_DEBUG_OUTPUT
    _labelFrame->show();
#else
    _labelFrame->hide();
#endif
}

void PurePlayer::createToolBar()
{
    const QSize iconSize(10,10);//16,16);
    QSize buttonSize(30,17);

#ifdef Q_OS_WIN32
    buttonSize.rwidth() += 2;
    buttonSize.rheight() += 2;
#endif

    _playPauseButton = new ControlButton(this);
    _playPauseButton->setFocusPolicy(Qt::NoFocus);
    _playPauseButton->setIconSize(iconSize);
    _playPauseButton->setFixedSize(buttonSize);
    _playPauseButton->setToolTip(tr("再生"));
    connect(_playPauseButton, SIGNAL(clicked(bool)), this, SLOT(buttonPlayPauseClicked()));

    _stopButton = new ControlButton(QIcon(":/icons/stop.png"), "", this);
    _stopButton->setFocusPolicy(Qt::NoFocus);
    _stopButton->setIconSize(iconSize);
    _stopButton->setFixedSize(buttonSize);
    _stopButton->setToolTip(tr("停止"));
    connect(_stopButton, SIGNAL(clicked(bool)), this, SLOT(stop()));

    _frameAdvanceButton = new ControlButton(QIcon(":/icons/frame.png"), "", this);
    _frameAdvanceButton->setFocusPolicy(Qt::NoFocus);
    _frameAdvanceButton->setIconSize(iconSize);
    _frameAdvanceButton->setFixedSize(buttonSize);
    _frameAdvanceButton->setToolTip(tr("コマ送り"));
    connect(_frameAdvanceButton, SIGNAL(clicked(bool)), this, SLOT(frameAdvance()));

    ControlButton* prevButton = new ControlButton(QIcon(":/icons/prev.png"), "", this);
    prevButton->setFocusPolicy(Qt::NoFocus);
    prevButton->setIconSize(iconSize);
    prevButton->setFixedSize(buttonSize);
    prevButton->setToolTip("プレイリスト前へ");
    connect(prevButton, SIGNAL(clicked(bool)), this, SLOT(playPrev()));

    ControlButton* nextButton = new ControlButton(QIcon(":/icons/next.png"), "", this);
    nextButton->setFocusPolicy(Qt::NoFocus);
    nextButton->setIconSize(iconSize);
    nextButton->setFixedSize(buttonSize);
    nextButton->setToolTip("プレイリスト次へ");
    connect(nextButton, SIGNAL(clicked(bool)), this, SLOT(playNext()));

    _repeatABButton = new ControlButton(QIcon(":/icons/repeata.png"), "", this);
    _repeatABButton->setFocusPolicy(Qt::NoFocus);
    _repeatABButton->setIconSize(iconSize);
    _repeatABButton->setFixedSize(buttonSize);
    _repeatABButton->setToolTip(tr("ABリピート"));
    connect(_repeatABButton, SIGNAL(clicked(bool)), this, SLOT(repeatAB()));

    _loopButton = new ControlButton(QIcon(":/icons/loop.png"), "", this);
    _loopButton->setCheckable(true);
    _loopButton->setFocusPolicy(Qt::NoFocus);
    _loopButton->setIconSize(iconSize);
    _loopButton->setFixedSize(buttonSize);
    _loopButton->setToolTip("ループ");
    connect(_loopButton, SIGNAL(clicked(bool)), this, SLOT(setLoop(bool)));

    _screenshotButton = new ControlButton(QIcon(":/icons/screenshot.png"), "", this);
    _screenshotButton->setFocusPolicy(Qt::NoFocus);
    _screenshotButton->setIconSize(iconSize);
    _screenshotButton->setFixedSize(buttonSize);
    _screenshotButton->setToolTip("スクリーンショット");
    connect(_screenshotButton, SIGNAL(clicked(bool)), this, SLOT(screenshot()));

    _timeslider = new TimeSlider(this);
    _timeslider->setFocusPolicy(Qt::NoFocus);
    connect(_timeslider, SIGNAL(requestSeek(double, bool)), this, SLOT(seek(double, bool)));

    _speedSpinBox = new SpeedSpinBox(this);
    _speedSpinBox->setFixedWidth(_speedSpinBox->sizeHint().width());
    _speedSpinBox->setFixedHeight(17);
    _speedSpinBox->setToolTip(tr("再生速度"));
    connect(_speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)));

    _toolBar = new QToolBar("toolbar", this);
    // QToolBarのレイアウトの操作。
    // 余白指定が全部同じ値だと有効だが、異なる値だと全て0になる。これは不正な操作？
    _toolBar->layout()->setContentsMargins(0,0,0,0); 
    _toolBar->setMovable(false);
    _toolBar->setAllowedAreas(Qt::BottomToolBarArea);
    QPalette p = _toolBar->palette();
    p.setColor(QPalette::Window, QApplication::palette().color(QPalette::Window));
    _toolBar->setPalette(p);
    _toolBar->setAutoFillBackground(true);
    addToolBar(Qt::BottomToolBarArea, _toolBar);

/*  QVBoxLayout* vl = new QVBoxLayout;
    vl->setContentsMargins(3,0,3,3); // left,top,right,bottom
    vl->setSpacing(0);
*/  QHBoxLayout* hl = new QHBoxLayout;
    hl->setSpacing(0);
    hl->setContentsMargins(1,1,1,1);

    hl->addWidget(_stopButton);
    hl->addWidget(_playPauseButton);
    hl->addWidget(_frameAdvanceButton);
    hl->addWidget(prevButton);
    hl->addWidget(nextButton);
    hl->addWidget(_repeatABButton);
    hl->addWidget(_loopButton);
    hl->addWidget(_screenshotButton);
    hl->addWidget(_timeslider);
    //hl->addStretch();
    hl->addWidget(_speedSpinBox);
//  vl->addWidget(_timeslider);
//  vl->addLayout(hl);
    QWidget* widget = new QWidget(this);
    widget->setLayout(hl);
    _toolBar->addWidget(widget);

    _toolBar->hide();
}

void PurePlayer::createActionContextMenu()
{
//  QAction* actTitle = new QAction(tr("PUREPLAYER MENU"), this);
//  actTitle->setEnabled(false);

    _actMute = new QAction(tr("ミュート"), this);
    _actMute->setCheckable(true);
    _actMute->setChecked(false);
    _actMute->setShortcut(tr("m"));
    connect(_actMute, SIGNAL(triggered(bool)), this, SLOT(mute(bool)));
    addAction(_actMute); // ショートカットキーを登録する為、アクションを追加

    QAction* actVideoAdjust = new QAction(tr("ビデオ調整"), this);
    connect(actVideoAdjust, SIGNAL(triggered()), this, SLOT(showVideoAdjustDialog()));

    QAction* actOpen = new QAction(tr("開く"), this);
    connect(actOpen, SIGNAL(triggered()), this, SLOT(openFromDialog()));

    QAction* actConfig = new QAction(tr("設定"), this);
    connect(actConfig, SIGNAL(triggered()), this, SLOT(showConfigDialog()));

    QAction* actLog = new QAction(tr("ログ"), this);
    connect(actLog, SIGNAL(triggered()), this, SLOT(showLogDialog()));

    QAction* actAbout = new QAction(tr("PurePlayer*について"), this);
    connect(actAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));

    _actScreenshot = new QAction(tr("スクリーンショット"), this);
    _actScreenshot->setShortcut(tr("c"));
    _actScreenshot->setAutoRepeat(false);
    connect(_actScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
    addAction(_actScreenshot);

    _actPlayPause = new QAction(tr("再生"), this);
    _actPlayPause->setShortcut(tr("space"));
    connect(_actPlayPause, SIGNAL(triggered()), this, SLOT(buttonPlayPauseClicked()));
    addAction(_actPlayPause);

    _actStop = new QAction(tr("停止"), this);
    //_actStop->setIcon(QIcon(":/icons/stop.png"));
    _actStop->setShortcut(tr("s"));
    _actStop->setEnabled(false);
    connect(_actStop, SIGNAL(triggered()), this, SLOT(stop()));
    addAction(_actStop);

    _actOpenContactUrl = new QAction(tr("コンタクトURLを開く"), this);
    _actOpenContactUrl->setEnabled(false);
    _actOpenContactUrl->setVisible(false);
    connect(_actOpenContactUrl, SIGNAL(triggered()), this, SLOT(openContactUrl()));

    _actPlayList = new QAction(tr("プレイリスト"), this);
    _actPlayList->setShortcut(tr("l"));
    connect(_actPlayList, SIGNAL(triggered()), this, SLOT(showPlayListDialog()));
    addAction(_actPlayList);

    _actStatusBar = new QAction(tr("ステータスを常に表示"), this);
    _actStatusBar->setCheckable(true);
    _actStatusBar->setChecked(false);
    connect(_actStatusBar, SIGNAL(triggered(bool)), this, SLOT(setAlwaysShowStatusBar(bool)));

    // 音声出力メニュー
    _actGroupAudioOutput = new QActionGroup(this);
    connect(_actGroupAudioOutput, SIGNAL(triggered(QAction*)),
            this,                 SLOT(actGroupAudioOutputChanged(QAction*)));
    QAction* actStereo = new QAction(tr("ステレオ"), _actGroupAudioOutput);
    actStereo->setCheckable(true);
    actStereo->setChecked(true);
    QAction* actMonaural = new QAction(tr("モノラル"), _actGroupAudioOutput);
    actMonaural->setCheckable(true);
    QAction* actLeft = new QAction(tr("Ｌのみを両方に出力"), _actGroupAudioOutput);
    actLeft->setCheckable(true);
    QAction* actRight = new QAction(tr("Ｒのみを両方に出力"), _actGroupAudioOutput);
    actRight->setCheckable(true);

    _actGroupVolumeFactor = new QActionGroup(this);
    connect(_actGroupVolumeFactor, SIGNAL(triggered(QAction*)),
            this,                  SLOT(actGroupVolumeFactorChanged(QAction*)));
    QAction* actVFactor1 = new QAction(tr("音量を標準にする"), _actGroupVolumeFactor);
    actVFactor1->setCheckable(true);
    actVFactor1->setChecked(true);
    QAction* actVFactor2 = new QAction(tr("音量を2倍にする"), _actGroupVolumeFactor);
    actVFactor2->setCheckable(true);
    QAction* actVFactor3 = new QAction(tr("音量を3倍にする"), _actGroupVolumeFactor);
    actVFactor3->setCheckable(true);

    QMenu* menuAudioOutput = new QMenu(tr("音声出力"), this);
    menuAudioOutput->addActions(_actGroupAudioOutput->actions());
    menuAudioOutput->addSeparator();
    menuAudioOutput->addActions(_actGroupVolumeFactor->actions());

    // サイズ変更メニュー
    QAction* act320x240 = new QAction(tr("320x240"), this);
    act320x240->setShortcut(tr("0"));
    connect(act320x240, SIGNAL(triggered()), this, SLOT(resize320x240()));
    addAction(act320x240);
    QAction* act1280x720 = new QAction(tr("1280x720"), this);
    connect(act1280x720, SIGNAL(triggered()), this, SLOT(resize1280x720()));
    QAction* act25Percent = new QAction(tr("25%"), this);
    act25Percent->setShortcut(tr("1"));
    connect(act25Percent, SIGNAL(triggered()), this, SLOT(resize25Percent()));
    addAction(act25Percent);
    QAction* act50Percent = new QAction(tr("50%"), this);
    act50Percent->setShortcut(tr("2"));
    connect(act50Percent, SIGNAL(triggered()), this, SLOT(resize50Percent()));
    addAction(act50Percent);
    QAction* act75Percent = new QAction(tr("75%"), this);
    act75Percent->setShortcut(tr("3"));
    connect(act75Percent, SIGNAL(triggered()), this, SLOT(resize75Percent()));
    addAction(act75Percent);
    QAction* act100Percent = new QAction(tr("100%"), this);
    act100Percent->setShortcut(tr("4"));
    connect(act100Percent, SIGNAL(triggered()), this, SLOT(resize100Percent()));
    addAction(act100Percent);
    QAction* act125Percent = new QAction(tr("125%"), this);
    act125Percent->setShortcut(tr("5"));
    connect(act125Percent, SIGNAL(triggered()), this, SLOT(resize125Percent()));
    addAction(act125Percent);
    QAction* act150Percent = new QAction(tr("150%"), this);
    act150Percent->setShortcut(tr("6"));
    connect(act150Percent, SIGNAL(triggered()), this, SLOT(resize150Percent()));
    addAction(act150Percent);

    QMenu* menuSize = new QMenu(tr("サイズ変更"), this);
    menuSize->addAction(act320x240);
    menuSize->addAction(act1280x720);
    menuSize->addSeparator();
    menuSize->addAction(act25Percent);
    menuSize->addAction(act50Percent);
    menuSize->addAction(act75Percent);
    menuSize->addAction(act100Percent);
    menuSize->addAction(act125Percent);
    menuSize->addAction(act150Percent);

    // アスペクト比変更メニュー
    _actGroupAspect = new QActionGroup(this);
    connect(_actGroupAspect, SIGNAL(triggered(QAction*)),
            this,            SLOT(actGroupAspectChanged(QAction*)));
    QAction* actAspectVideo = new QAction(tr("ビデオの比"), _actGroupAspect);
    actAspectVideo->setCheckable(true);
    actAspectVideo->setChecked(true);
    QAction* actAspect4_3 = new QAction(tr("4:3"), _actGroupAspect);
    actAspect4_3->setCheckable(true);
    QAction* actAspect16_9 = new QAction(tr("16:9"), _actGroupAspect);
    actAspect16_9->setCheckable(true);
    QAction* actAspect16_10 = new QAction(tr("16:10"), _actGroupAspect);
    actAspect16_10->setCheckable(true);
    QAction* actAspectNoKeep = new QAction(tr("比を維持しない"), _actGroupAspect);
    actAspectNoKeep->setCheckable(true);
    QMenu* menuAspect = new QMenu(tr("アスペクト比変更"), this);
    menuAspect->addActions(_actGroupAspect->actions());

    // インターレース解除メニュー
    _actGroupDeinterlace = new QActionGroup(this);
    connect(_actGroupDeinterlace, SIGNAL(triggered(QAction*)),
            this,                 SLOT(actGroupDeinterlaceChanged(QAction*)));
    QAction* actDeinterlaceNo = new QAction(tr("無し"), _actGroupDeinterlace);
    actDeinterlaceNo->setCheckable(true);
    actDeinterlaceNo->setChecked(true);
    QAction* actDeinterlaceYadif = new QAction(tr("Yadif"), _actGroupDeinterlace);
    actDeinterlaceYadif->setCheckable(true);
    QAction* actDeinterlaceLB = new QAction(tr("リニアブレンド"), _actGroupDeinterlace);
    actDeinterlaceLB->setCheckable(true);
    QMenu* menuDeinterlace = new QMenu(tr("インターレース解除"), this);
    menuDeinterlace->addActions(_actGroupDeinterlace->actions());

    // 再接続メニュー
    _actReconnect = new QAction(tr("通常"), this);
    _actReconnect->setShortcut(tr("space"));
    _actReconnect->setAutoRepeat(false);
    _actReconnect->setVisible(false); // ショートカットキー無効化
    connect(_actReconnect, SIGNAL(triggered()), this, SLOT(reconnectFromGui()));
    addAction(_actReconnect);
    _actReconnectPly = new QAction(tr("プレイヤーのみ"), this);
    connect(_actReconnectPly, SIGNAL(triggered()), this, SLOT(reconnectPurePlayerFromGui()));
    _actReconnectPct = new QAction(tr("PeerCastのみ"), this);
    _actReconnectPct->setEnabled(false);
    connect(_actReconnectPct, SIGNAL(triggered()), this, SLOT(reconnectPeercast()));

    _menuReconnect = new QMenu(tr("再接続"), this);
    _menuReconnect->menuAction()->setVisible(false);
    _menuReconnect->addAction(_actReconnect);
    _menuReconnect->addAction(_actReconnectPly);
    _menuReconnect->addAction(_actReconnectPct);

    // コンテキストメニュー
    _menuContext = new QMenu(this);
//  _menuContext->addAction(actTitle);
//  _menuContext->addSeparator();
    _menuContext->addMenu(menuAudioOutput);
    _menuContext->addAction(_actMute);
    _menuContext->addSeparator();
    _menuContext->addMenu(menuSize);
    _menuContext->addMenu(menuAspect);
    _menuContext->addMenu(menuDeinterlace);
    _menuContext->addAction(_actScreenshot);
    _menuContext->addAction(actVideoAdjust);
    _menuContext->addSeparator();
    _menuContext->addMenu(_menuReconnect);
    _menuContext->addAction(_actPlayPause);
    _menuContext->addAction(_actStop);
    _menuContext->addAction(actOpen);
    _menuContext->addAction(_actOpenContactUrl);
    _menuContext->addAction(_actPlayList);
    _menuContext->addSeparator();
    _menuContext->addAction(_actStatusBar);
    _menuContext->addAction(actConfig);
    _menuContext->addAction(actLog);
    _menuContext->addAction(actAbout);

    // ショートカットキー単体登録
    QShortcut* fullscreen = new QShortcut(tr("f"), this);
    fullscreen->setAutoRepeat(false);
    connect(fullscreen, SIGNAL(activated()), this, SLOT(fullScreenOrWindow()));
    QShortcut* close = new QShortcut(tr("q"), this);
    connect(close, SIGNAL(activated()), this, SLOT(close()));
    QShortcut* exitfullscreen = new QShortcut(tr("esc"), this);
    connect(exitfullscreen, SIGNAL(activated()), this, SLOT(exitFullScreen()));

    //setContextMenuPolicy(Qt::ActionsContextMenu);
}

void PurePlayer::open(const QString& path)
{
    _playList.add(path);
    _playList.setCurrentIndex(_playList.itemCount()-1);
    playListDialog()->reflectDataToGui();

    openCommonProcess(path);
}

void PurePlayer::play()
{
    if( _playList.itemCount() > 0 ) {
        QString path = _playList.currentItem()->_path;
        if( path != _path )
            openCommonProcess(path);
        else
            playCommonProcess();
    }
}

void PurePlayer::playPrev()
{
    int index = _playList.currentIndex();

    _playList.downCurrentIndex();
    if( index != _playList.currentIndex() ) {
        playListDialog()->updateCurrentItemBackground();
        play();
    }
    else if( _doLoop && _playList.itemCount() > 1 ) {
        _playList.setCurrentIndex(_playList.itemCount()-1);
        playListDialog()->updateCurrentItemBackground();
        play();
    }
}

void PurePlayer::playNext()
{
    int index = _playList.currentIndex();

    _playList.upCurrentIndex();
    if( index != _playList.currentIndex() ) {
        playListDialog()->updateCurrentItemBackground();
        play();
    }
    else if( _doLoop && _playList.itemCount() > 1 ) {
        _playList.setCurrentIndex(0);
        playListDialog()->updateCurrentItemBackground();
        play();
    }
}

void PurePlayer::setCurrentDirectory()
{
    QString current;

    if( ConfigData::data()->useScreenshotPath )
        current = ConfigData::data()->screenshotPath;
    else {
        if( _path.left(8).contains("://") )
            current = QDir::homePath();
        else
            current = QFileInfo(_path).absolutePath();
    }

    if( !QFileInfo(current).isWritable() || !QDir::setCurrent(current) )
        QDir::setCurrent(QDir::homePath());
}

void PurePlayer::openCommonProcess(const QString& path)
{
    QRegExp rxPeercastUrl(
            "(?:^http|^mms|^mmsh)://(.+):(\\d+)/(?:stream|pls)/([A-F0-9]{32})");

    if( rxPeercastUrl.indexIn(path) != -1 ) {
        _host = rxPeercastUrl.cap(1);
        _port = rxPeercastUrl.cap(2).toShort();
        _id   = rxPeercastUrl.cap(3);

        QRegExp rootIp(".+\\?tip=(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})");
        if( rootIp.indexIn(path) != -1 )
            _rootIp = rootIp.cap(1);
        else
            _rootIp = "";

        _chName = "PurePlayer*";
        _reconnectCount = 0;

        _menuReconnect->menuAction()->setVisible(true);
        // _actPlayPauseとのショートカットキー切り替えの為、設定
        _actReconnect->setVisible(true);

        _actPlayPause->setVisible(false);
        _actOpenContactUrl->setVisible(true);
        _actStatusBar->setVisible(true);

        LogDialog::debug("PurePlayer::open(): peercast url detected.");
        LogDialog::debug(QString("PurePlayer::open(): port %1").arg(_port));
        LogDialog::debug(QString("PurePlayer::open(): ID %1").arg(_id));
        LogDialog::debug(QString("PurePlayer::open(): root %1").arg(_rootIp));
    }
    else {
        _host   = "";
        _port   = -1;
        _id     = "";
        _rootIp = "";
        _chName = path.split("/").last();

        _menuReconnect->menuAction()->setVisible(false);
        // _actPlayPauseとのショートカットキー切り替えの為、設定
        _actReconnect->setVisible(false);

        _actPlayPause->setVisible(true);
        _actOpenContactUrl->setVisible(false);
        _actStatusBar->setVisible(false);

        _repeatStartTime = -1;
        _repeatEndTime = -1;
        _repeatABButton->setIcon(QIcon(":/icons/repeata.png"));
    }

    _path = path;
    _openedNewPath = true;

    setWindowTitle(_chName);

    _searchingConnection = false;

    _contactUrl = "";
    _actOpenContactUrl->setEnabled(false);

    _infoLabel->clearClipInfo();

    setCurrentDirectory();

/*
//  if( ConfigData::data()->useScreenshotPath ) {
//      QFileInfo info(ConfigData::data()->screenshotPath)
//      if( !info.isWritable() || !QDir::setCurrent(ConfigData::data()->screenshotPath) )
//          QDir::setCurrent(QDir::homePath());
//  }
//  else {
        QDir dir(_path);
        if( dir.cdUp() ) {
            QFileInfo info(dir.path());
            if( !info.isWritable() || !QDir::setCurrent(dir.path()) )
                QDir::setCurrent(QDir::homePath());
        }
        else
            QDir::setCurrent(QDir::homePath());
//  }
*/
    LogDialog::debug("PurePlayer::openCommonProcess(): current dir " + QDir::currentPath());

    stop();
    playCommonProcess();
}

void PurePlayer::openFromDialog()
{
    if( _openDialog == NULL ) {
        _openDialog = new OpenDialog(this);
        connect(_openDialog, SIGNAL(openPath(const QString)), this, SLOT(open(const QString)));
    }

    _openDialog->setPath(_path);
    _openDialog->move(_menuContext->x()
                        + (_menuContext->width()-_openDialog->width())/2,
                      _menuContext->y());
    _openDialog->show();
}

void PurePlayer::stop()
{
    LogDialog::debug("PurePlayer::stop(): start-");
    setStatus(STOP);

    if( _mpProcess->state() == QProcess::NotRunning ) {
        LogDialog::debug("PurePlayer::stop(): -end. process not running.");
        return;
    }

    // mplayerプロセスを終了させる
    mpCmd("quit");
    if( !_mpProcess->waitForFinished(1000) ) {
        LogDialog::debug("PurePlayer::stop(): _mpProcess->terminate()", QColor(255,0,0));
        _mpProcess->terminate();
        if( !_mpProcess->waitForFinished(3000) ) {
            LogDialog::debug("PurePlayer::stop(): _mpProcess->kill()", QColor(255,0,0));
            _mpProcess->kill();
            _mpProcess->waitForFinished(2000);
        }
    }

    LogDialog::debug("PurePlayer::stop(): -end");
}

void PurePlayer::pauseUnPause()
{
    if( isPeercastStream() ) return;

    if( _state==PLAY || _state==PAUSE )
        mpCmd("pause");
}

void PurePlayer::frameAdvance()
{
    if( isPeercastStream() ) return;

    if( _state==PLAY || _state==PAUSE )
        mpCmd("frame_step");
}

void PurePlayer::repeatAB()
{
    if( isPeercastStream() || !(_state==PLAY || _state==PAUSE) ) return;

    if( _repeatStartTime < 0 ) {
        _repeatStartTime = _currentTime * 10;
        _repeatABButton->setIcon(QIcon(":/icons/repeatb.png"));
    }
    else
    if( _repeatEndTime < 0 ) {
        if( _repeatStartTime < _currentTime*10 ) {
            _repeatEndTime = _currentTime * 10;
            _repeatABButton->setIcon(QIcon(":/icons/repeatreset.png"));
        }
        else
            _repeatStartTime = _currentTime * 10;
    }
    else {
        _repeatStartTime = -1;
        _repeatEndTime = -1;
        _controlFlags &= ~FLG_SEEKED_REPEAT;
        _repeatABButton->setIcon(QIcon(":/icons/repeata.png"));
    }

    LogDialog::debug(QString("PurePlayer::repeatAB(): %1 %2")
                     .arg(_repeatStartTime).arg(_repeatEndTime));
}

void PurePlayer::seek(double sec, bool relative)
{
    if( !_isSeekable ) // _videoLengthが0の場合の処理が未実装
        return;

    if( _state==PLAY || _state==PAUSE ) {
        if( relative )
            mpCmd(QString().sprintf("seek %.1f 0", sec));
        else {
            if( sec < 0 )
                sec = 0;
            else if( sec > _videoLength )
                sec = _videoLength;

            double percent = sec / _videoLength * 100;
            mpCmd(QString().sprintf("seek %.1f 1", percent));   // パーセントによる指定
//          mpCmd(QString().sprintf("seek %d 2", sec));         // 時間による直接指定
                                                     // mplayer1では時間直接指定にバグあり
        }
    }
}

void PurePlayer::setSpeed(double rate)
{
    if( rate < 0.1 ) rate = 0.1; else
    if( rate > 3.0 ) rate = 3.0;

    if( _speedSpinBox->value() != rate ) {
        _speedSpinBox->setValue(rate);
        return;
    }

    if( _state == PLAY ) {
        mpCmd(QString().sprintf("speed_set %.1f", rate));
        mpCmd("osd_show_property_text 'Speed: x ${speed}'");
        //mpCmd("osd_show_text " + QString().sprintf("'Speed: x %.1f'", rate));
    }
    else
    if( _state == PAUSE )
        mpCmd(QString().sprintf("pausing_keep_force speed_set %.1f", rate));
}

void PurePlayer::reconnectPeercast()
{
    if( !isPeercastStream() ) return;

    QUrl url(QString("http://%1:%2/admin?cmd=bump&id=%3").arg(_host).arg(_port).arg(_id));
    _nam->get(QNetworkRequest(url));
    LogDialog::debug("PurePlayer::reconnectPeercast(): ");
/*
    _http->setHost(url.host(), url.port());
    _http->get("/admin?cmd=bump&id=" + id);
    LogDialog::debug("PurePlayer::reconnectPeercast(): state " +
                                                    QString("%1").arg(_http->state()));
*/
}

void PurePlayer::recordingStartStop()
{
    if( _recordingProcess->state() == QProcess::NotRunning ) {
        QString saveName = QString("%1_%2.avi").arg(_chName)
                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

        QStringList args;
        args
        << "-quiet"
        << "-ovc" << "copy"
        << "-oac" << "copy"
        << _path
        << "-ofps" << "70"
        << "-o" << CommonLib::retTheFileNameNotExists(saveName);

        _recordingProcess->start("mencoder", args, QIODevice::ReadOnly);
        _recordingProcess->waitForStarted();
        LogDialog::debug("recording");
    }
    else {
        _recordingProcess->terminate();
        _recordingProcess->waitForFinished(3000);
        LogDialog::debug("recording stop");
    }
}

void PurePlayer::mute(bool b)
{
    QString cmd, text;
    QPalette p = _labelVolume->palette();

    _isMute = b;
    if( _isMute ) {
//      cmd = "mute 1";
        cmd = "volume 0 1";
        text = "Mute";
        p.setColor(_labelVolume->foregroundRole(), QColor(80,80,80));
    }
    else {
//      cmd = "mute 0";
        cmd = QString("volume %1 1").arg(_volume);
        text = "Unmute";
        p.setColor(_labelVolume->foregroundRole(), QColor(255,255,255));
    }

    if( _state == PLAY ) { //|| _state == READY ) {
        //mpCmd("osd 0");
        mpCmd(cmd);
        //mpCmd("osd 1");
        mpCmd("osd_show_text " + text);
    }
    else
    if( _state == PAUSE )
        mpCmd("pausing_keep_force " + cmd);

    _labelVolume->setPalette(p);

    _actMute->setChecked(_isMute);
}

void PurePlayer::upVolume(int value)
{
    _volume += value;
    setVolume(_volume);
}

void PurePlayer::downVolume(int value)
{
    _volume -= value;
    setVolume(_volume);
}

void PurePlayer::setVolume(int value)
{
    if( value < 0 )   value = 0;   else
    if( value > 100 ) value = 100;

    _volume = value;

    if( !isMute() ) {
        if( _state == PLAY ) { //|| _state == READY ) {
            mpCmd(QString("volume %1 1").arg(_volume));
            if( !(_controlFlags & FLG_HIDE_DISPLAY_MESSAGE) )
                mpCmd(QString("osd_show_text %1").arg(_volume));
        }
        else
        if( _state == PAUSE )
            mpCmd(QString("pausing_keep_force volume %1 1").arg(_volume));
    }

    _labelVolume->setText(QString::number(_volume));
}

void PurePlayer::setVolumeFactor(VOLUME_FACTOR_MODE mode)
{
    if( _volumeFactor != mode ) {
        _volumeFactor = mode;

        if( !isStop() )
            restartPlay(true);

        _actGroupVolumeFactor->actions()[mode]->setChecked(true);
    }
}

void PurePlayer::setAudioOutput(AUDIO_OUTPUT_MODE mode)
{
    if( _audioOutput != mode ) {
        _audioOutput = mode;

        if( !isStop() )
            restartPlay(true);

        _actGroupAudioOutput->actions()[mode]->setChecked(true);
    }
}

void PurePlayer::setLoop(bool b)
{
    _doLoop = b;

/*  if( _state == PLAY )
        mpCmd(QString("loop %1 1").arg(_doLoop ? 0:-1));
    else
    if( _state == PAUSE )
        mpCmd(QString("pausing_keep_force loop %1 1").arg(_doLoop ? 0:-1));
*/
}

void PurePlayer::setAspectRatio(ASPECT_RATIO ratio)
{
    if( _aspectRatio != ratio ) {
        _aspectRatio = ratio;

        updateVideoScreenGeometry();

        _actGroupAspect->actions()[ratio]->setChecked(true);
    }
}

void PurePlayer::setContrast(int value)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;
    _videoSettings.contrast = value;

    if( _state == PLAY ) {
        if( _controlFlags & FLG_HIDE_DISPLAY_MESSAGE )
            mpCmd(QString("contrast %1 1").arg(_videoSettings.contrast));
        else {
            mpCmd("osd 1");
            mpCmd(QString("contrast %1 1").arg(_videoSettings.contrast));
            mpCmd("osd 0");
        }
    }
    else
    if( _state == PAUSE )
        mpCmd(QString("pausing_keep_force contrast %1 1").arg(_videoSettings.contrast));
}

void PurePlayer::setBrightness(int value)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;
    _videoSettings.brightness = value;

    if( _state == PLAY ) {
        if( _controlFlags & FLG_HIDE_DISPLAY_MESSAGE )
            mpCmd(QString("brightness %1 1").arg(_videoSettings.brightness));
        else {
            mpCmd("osd 1");
            mpCmd(QString("brightness %1 1").arg(_videoSettings.brightness));
            mpCmd("osd 0");
        }
    }
    else
    if( _state == PAUSE )
        mpCmd(QString("pausing_keep_force brightness %1 1").arg(_videoSettings.brightness));
}

void PurePlayer::setHue(int value)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;
    _videoSettings.hue = value;

    if( _state == PLAY ) {
        if( _controlFlags & FLG_HIDE_DISPLAY_MESSAGE )
            mpCmd(QString("hue %1 1").arg(_videoSettings.hue));
        else {
            mpCmd("osd 1");
            mpCmd(QString("hue %1 1").arg(_videoSettings.hue));
            mpCmd("osd 0");
        }
    }
    else
    if( _state == PAUSE )
        mpCmd(QString("pausing_keep_force hue %1 1").arg(_videoSettings.hue));
}

void PurePlayer::setSaturation(int value)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;
    _videoSettings.saturation = value;

    if( _state == PLAY ) {
        if( _controlFlags & FLG_HIDE_DISPLAY_MESSAGE )
            mpCmd(QString("saturation %1 1").arg(_videoSettings.saturation));
        else {
            mpCmd("osd 1");
            mpCmd(QString("saturation %1 1").arg(_videoSettings.saturation));
            mpCmd("osd 0");
        }
    }
    else
    if( _state == PAUSE )
        mpCmd(QString("pausing_keep_force saturation %1 1").arg(_videoSettings.saturation));
}

void PurePlayer::setGamma(int value)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;
    _videoSettings.gamma = value;

    if( _state == PLAY ) {
        if( _controlFlags & FLG_HIDE_DISPLAY_MESSAGE )
            mpCmd(QString("gamma %1 1").arg(_videoSettings.gamma));
        else {
            mpCmd("osd 1");
            mpCmd(QString("gamma %1 1").arg(_videoSettings.gamma));
            mpCmd("osd 0");
        }
    }
    else
    if( _state == PAUSE )
        mpCmd(QString("pausing_keep_force gamma %1 1").arg(_videoSettings.gamma));
}

void PurePlayer::setDeinterlace(DEINTERLACE_MODE mode)
{
    if( _deinterlace != mode ) {
        _deinterlace = mode;

        if( !isStop() )
            restartPlay(true);

        _actGroupDeinterlace->actions()[mode]->setChecked(true);
    }
}

void PurePlayer::screenshot()
{
//  if( ConfigData::data()->screenshot ) {
        if( _state == PLAY )
            mpCmd("screenshot 0");
        else
        if( _state == PAUSE ) {
            mpCmd("pausing_keep_force screenshot 0");
            frameAdvance();
        }
//  }
//  LogDialog::debug("PurePlayer::screenshot():");
}

// 返却値: リサイズ処理が行われたら、trueを返す。そうでないならfalseを返す。// 要仕様検討
bool PurePlayer::resizeFromVideoScreen(QSize size)
{
    if( isFullScreen() || isMaximized() )
        return false;

    if( isAlwaysShowStatusBar() )
        size.rheight() += statusBar()->height();

    if( _isSeekable )
        size.rheight() += _toolBar->height();

    if( size == QMainWindow::size() )
        return false;
    else {
        resize(size);
        return true;
    }
}

void PurePlayer::fullScreenOrWindow()
{
    if( isFullScreen() ) {
        if( _isMaximizedBeforeFullScreen )
            showMaximized();
        else
            showNormal();

        // ツールバー表示変更
        _toolBar->hide();
        _toolBar->setAllowedAreas(Qt::BottomToolBarArea);
        _toolBar->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);

        _statusbarSpaceL->hide();
        _statusbarSpaceR->hide();

        if( !_isSeekable )
            _actStatusBar->setEnabled(true);

        updateVisibleInterface();
#ifdef Q_OS_WIN32
        updateVideoScreenGeometry(); // windowsではshowNormal()で即リサイズされる為。
#endif // Q_OS_WIN32

        setMouseTracking(false);
        centralWidget()->setMouseTracking(false);
        _videoScreen->setMouseTracking(false);
        setCursor(QCursor(Qt::ArrowCursor));
    }
    else {
        _isMaximizedBeforeFullScreen = isMaximized();
        showFullScreen();

        QSize desktop = QApplication::desktop()->size();

        // ツールバー表示変更
        _toolBar->hide();
        _toolBar->setAllowedAreas(Qt::NoToolBarArea);
        _toolBar->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        _toolBar->resize(desktop.width() * (10 - 2*2)/10, _toolBar->height());// 2:6:2
        _toolBar->move((desktop.width()-_toolBar->width())/2,
                   desktop.height()-statusBar()->height()-_toolBar->height());

        _statusbarSpaceL->setMinimumWidth(desktop.width() / 5);
        _statusbarSpaceR->setMinimumWidth(_statusbarSpaceL->width());
        _statusbarSpaceL->show();
        _statusbarSpaceR->show();
        _actStatusBar->setEnabled(false);

        updateVisibleInterface();

        setMouseTracking(true);
        centralWidget()->setMouseTracking(true);
        _videoScreen->setMouseTracking(true);
    }

    // ウィンドウモードを切り替えた直後マウス入力したままで、
    // mouseMoveEvent()によるウィンドウ移動を無効にする。
    // (フルスクリーンからウィンドウへ切り替え直後、ウィンドウ移動で位置が飛ぶ問題対応)
    if( QApplication::mouseButtons() | Qt::LeftButton )
        _disableWindowMoveFromMouse = true;
}

void PurePlayer::setAlwaysShowStatusBar(bool b)
{
    if( _isSeekable ) return;

    _alwaysShowStatusBar = b;

    if( b ) {
        resize(width(), height() + statusBar()->size().height());
        statusBar()->show();
    }
    else {
        if( _state!=STOP && _state!=READY )
            statusBar()->hide();

        resize(width(), height() - statusBar()->size().height());
    }

    _actStatusBar->setChecked(b);
}

void PurePlayer::showVideoAdjustDialog()
{
    if( _videoAdjustDialog == NULL ) {
        _videoAdjustDialog = new VideoAdjustDialog(this);
        connect(_videoAdjustDialog, SIGNAL(clickedSaveButton()), this, SLOT(saveVideoSettings()));
        connect(_videoAdjustDialog, SIGNAL(changedContrast(int)), this, SLOT(setContrast(int)));
        connect(_videoAdjustDialog, SIGNAL(changedBrightness(int)), this, SLOT(setBrightness(int)));
        connect(_videoAdjustDialog, SIGNAL(changedSaturation(int)), this, SLOT(setSaturation(int)));
        connect(_videoAdjustDialog, SIGNAL(changedHue(int)), this, SLOT(setHue(int)));
        connect(_videoAdjustDialog, SIGNAL(changedGamma(int)), this, SLOT(setGamma(int)));

        _controlFlags |= FLG_HIDE_DISPLAY_MESSAGE;  //mpCmd("osd 0");
        _videoAdjustDialog->setValue(_videoSettings);
        _controlFlags &= ~FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 1");
        _videoAdjustDialog->updateSaveValue();
    }
    else {
        _controlFlags |= FLG_HIDE_DISPLAY_MESSAGE;  //mpCmd("osd 0");
        _videoAdjustDialog->setValue(_videoSettings);
        _controlFlags &= ~FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 1");
    }

    _videoAdjustDialog->move(_menuContext->x()
                                 + (_menuContext->width()-_videoAdjustDialog->width())/2,
                             _menuContext->y());
    _videoAdjustDialog->show();
}

void PurePlayer::showLogDialog()
{
    LogDialog::moveDialog(_menuContext->x()
                             + (_menuContext->width()-LogDialog::dialog()->width())/2,
                          _menuContext->y());
    LogDialog::showDialog();
}

void PurePlayer::showConfigDialog()
{
    if( _configDialog == NULL ) {
        _configDialog = new ConfigDialog(this);
        connect(_configDialog, SIGNAL(applied(bool)),
                this,       SLOT(appliedFromConfigDialog(bool)));
    }

    _configDialog->setData(ConfigData::data());
    _configDialog->move(_menuContext->x()
                            + (_menuContext->width()-_configDialog->width())/2,
                        _menuContext->y());
    _configDialog->show();
}

void PurePlayer::showPlayListDialog()
{
    playListDialog()->reflectDataToGui();
    _playListDialog->move(_menuContext->x()
                                + (_menuContext->width()-_playListDialog->width())/2,
                          _menuContext->y());
    playListDialog()->show();
}

void PurePlayer::showAboutDialog()
{
    if( _aboutDialog == NULL ) {
        _aboutDialog = new AboutDialog(this);
    }

    _aboutDialog->move(_menuContext->x()
                            + (_menuContext->width()-_aboutDialog->width())/2,
                       _menuContext->y());

    _aboutDialog->show();
}

void PurePlayer::updateChannelInfo()
{
    if( !isPeercastStream() ) return;

    QUrl url(QString("http://%1:%2/html/ja/relayinfo.html?id=%3")
             .arg(_host).arg(_port).arg(_id));

    _nam->get(QNetworkRequest(url));
    LogDialog::debug("PurePlayer::updateChannelInfo(): ");
}

void PurePlayer::openContactUrl()
{
    if( !_contactUrl.isEmpty() )
        QDesktopServices::openUrl(_contactUrl);
}

/*
bool PurePlayer::event(QEvent* e)
{
//  LogDialog::debug(tr("%1").arg(e->type()));

    if( e->type() == QEvent::WindowStateChange ) {
        QWindowStateChangeEvent* event = static_cast<QWindowStateChangeEvent*>(e);
        if( windowState() == Qt::WindowMaximized ) {
            LogDialog::debug(QString("PurePlayer::event(): before"));
//          showNormal();
            //showFullScreen();
            LogDialog::debug(QString("PurePlayer::event(): after"));
//          return true;
        }

        LogDialog::debug(QString("PurePlayer::event(): %1 %2")
                .arg(windowState()).arg(event->oldState()));
    }
    else
    if( e->type() == QEvent::Resize )
        LogDialog::debug(QString("PurePlayer::event(): resize"));

//  if( e->type() == QEvent::StatusTip )
//      return true;

    return QMainWindow::event(e);
}
*/
void PurePlayer::closeEvent(QCloseEvent* e)
{
    LogDialog::debug("PurePlayer::closeEvent(): start-");

    LogDialog::closeDialog();
    saveInteractiveSettings();
    hide();
    stop();
    e->accept();

    LogDialog::debug("PurePlayer::closeEvent():-end");
}

void PurePlayer::resizeEvent(QResizeEvent* )
{
    LogDialog::debug(QString().sprintf("PurePlayer::resizeEvent(): client size %dx%d",
                                     width(), height()));

    updateVideoScreenGeometry();

    // サイズ情報の出力
    QString sizeInfo;
    if( _aspectRatio == RATIO_VIDEO )
        sizeInfo = QString("%1%").arg(_videoScreen->width()*100/_videoSize.width());

    sizeInfo += QString(" %1x%2").arg(_videoScreen->width()).arg(_videoScreen->height());
    _infoLabel->setText(sizeInfo, 1000);
}

void PurePlayer::keyPressEvent(QKeyEvent* e)
{
    static QTime time;

    switch( e->key() ) {
    case Qt::Key_E: frameAdvance();         break;

#ifndef QT_NO_DEBUG_OUTPUT
//  case Qt::Key_D: showLogDialog();        break;
//  case Qt::Key_O: mpCmd("osd");           break;
    case Qt::Key_X:
        {

        break;
        }
    case Qt::Key_Z:
        {

        break;
        }
    case Qt::Key_B:
        {
        repeatAB();

        break;
        }
    case Qt::Key_N:
        {
//      recordingStartStop();
/*
        // ボリュームテキストの色を変える
        QPalette p = _labelVolume->palette();
        p.setColor(_labelVolume->foregroundRole(), QColor(106,129,198));
        _labelVolume->setPalette(p);
        LogDialog::debug(QString("%1 %2").arg(width()).arg(height()));
*/
        break;
        }
    case Qt::Key_V:
//      LogDialog::debug("mplayer pid: " + QString("%1").arg(_mpProcess->pid()));
//      LogDialog::debug("mplayer cpid: " + QString("%1").arg(_mplayerCPid));

        _debugFlg = !_debugFlg;
        LogDialog::debug(QString("blockCount: %1").arg(LogDialog::dialog()->blockCount()));

        break;
#endif // QT_NO_DEBUG_OUTPUT

    case Qt::Key_Up:
        if( e->modifiers() & Qt::ShiftModifier )
            upVolume(1);
        else
            upVolume();
        break;
    case Qt::Key_Down:
        if( e->modifiers() & Qt::ShiftModifier )
            downVolume(1);
        else
            downVolume();
        break;
    case Qt::Key_Right:
        //mpCmd("osd 0");
        seek(+3, true);
        //mpCmd("osd 1");
        break;
    case Qt::Key_Left:
        //mpCmd("osd 0");
        seek(-3, true);
        //mpCmd("osd 1");
        break;
    default:
        QMainWindow::keyPressEvent(e);
    }
}

void PurePlayer::mousePressEvent(QMouseEvent* e)
{
// 注: mouseDoubleClickEvent()と併せて実装しないと、動作漏れが起きる

/*  LogDialog::debug(QString().sprintf(
                        "PurePlayer::mousePressEvent():pos %d %d global %d %d",
                        e->pos().x(),e->pos().y(), e->globalPos().x(),e->globalPos().y()));
*/
    if( e->button() == Qt::LeftButton ) {
        _pressLocalPos.setX(e->pos().x() + geometry().x()-frameGeometry().x());
        _pressLocalPos.setY(e->pos().y() + geometry().y()-frameGeometry().y());
        if( (height()*80/100) < e->y() )
            _muteWhenMouseRelease = true;
        else
        if( isFullScreen() )
            setCursor(QCursor(Qt::BlankCursor));
    }
    else
    if( e->button() == Qt::RightButton )
        _menuContext->popup(e->globalPos());
    else
    if( e->button() == Qt::MidButton )
        middleClickResize();
}

void PurePlayer::mouseReleaseEvent(QMouseEvent* e)
{
//  LogDialog::debug("mouse release");
    if( e->button() == Qt::LeftButton ) {
        _menuContext->move(x()+width()*0.2, y()+height()*0.2);

        if( _muteWhenMouseRelease ) {
            _muteWhenMouseRelease = false;
            mute(!isMute());
        }

        _disableWindowMoveFromMouse = false;
    }
}

void PurePlayer::mouseDoubleClickEvent(QMouseEvent* e)
{
//  LogDialog::debug("mouse doubleclick");
    if( e->buttons() & Qt::LeftButton ) {
        if( (height()*80/100) < e->y() )
            _muteWhenMouseRelease = true;
        else
            fullScreenOrWindow();
    }
    else
    if( e->buttons() & Qt::MidButton )
        middleClickResize();
}

void PurePlayer::mouseMoveEvent(QMouseEvent* e)
{
//  LogDialog::debug("mouse move");

    if( isFullScreen() ) {
        setCursor(QCursor(Qt::ArrowCursor));
        updateVisibleInterface();
    }
    else
    if( !isMaximized() ) {
        if( e->buttons() & Qt::LeftButton ) {
            if( !_disableWindowMoveFromMouse )
                move(e->globalPos() - _pressLocalPos);

            _muteWhenMouseRelease = false;
        }
    }
}

void PurePlayer::wheelEvent(QWheelEvent* e)
{
//  if( isMute() )
//      mute(false);
//  else {
        int volume;
        QPoint localPoint = statusBar()->mapFromGlobal(e->globalPos());
        QRect  labelRect  = _labelVolume->geometry();
/*      LogDialog::debug(QString().sprintf("(%d %d %d %d) (%d %d)",
                        labelRect.x(),labelRect.y(), labelRect.right(),labelRect.bottom(),
                        localPoint.x(), localPoint.y()));
*/
        if( labelRect.contains(localPoint) )
            volume = 1;
        else
            volume = 5;

        if( e->delta() < 0 )
            downVolume(volume);
        else
            upVolume(volume);
//  }
}

void PurePlayer::enterEvent(QEvent*)
{
//  LogDialog::debug("enter");
    _cursorInWindow = true;
    updateVisibleInterface();
}

void PurePlayer::leaveEvent(QEvent*)
{
//  LogDialog::debug("leave");
    _cursorInWindow = false;
    updateVisibleInterface();
}

void PurePlayer::dragEnterEvent(QDragEnterEvent* e)
{
    if( e->mimeData()->hasFormat("text/uri-list") )
        e->acceptProposedAction();
}

void PurePlayer::dropEvent(QDropEvent* e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if( urls.isEmpty() )
        return;

    for(int i=0; i < urls.size(); i++) {
        QString path = urls[i].toLocalFile();
        _playList.add(path);
        LogDialog::debug("PurePlayer::dropEvent(): " + path);
    }

    // 新たに追加したアイテム群の先頭要素をカレントインデックスとする
    _playList.setCurrentIndex(_playList.itemCount() - urls.size());

    playListDialog()->reflectDataToGui();

    // 現状では必要無し:
    // プレイリストファイル内にpeercast urlがあった場合、
    // 同じストリームが開かれる可能性がある為初期化。
    //_reconnectCount = 0;

    stop();
    play();
}

/*
void PurePlayer::paintEvent(QPaintEvent* e)
{
    LogDialog::debug("PurePlayer::paintEvent():", QColor(0,255,0));
    QMainWindow::paintEvent(e);
}
*/

void PurePlayer::middleClickResize()
{
    if( isFullScreen() )
        return;
    else
    if( isMaximized() )
        showNormal();
    else
    {
        QSize videoClientSize;

        if( isAlwaysShowStatusBar() )
            videoClientSize = centralWidget()->size();
        else
            videoClientSize = size();

        QSize normalSize;
        if( _videoSize.width() < minimumSizeHint().width() ) {
            normalSize.setWidth(minimumSizeHint().width());
            normalSize.setHeight(_videoSize.height());
        }
        else
            normalSize = _videoSize;

        if( videoClientSize.width()  > normalSize.width()
         || videoClientSize.height() > normalSize.height()

         || (_videoScreen->width() == _videoSize.width()/2
         && _videoScreen->height() == _videoSize.height()/2) )
        {
            resize100Percent();
        }
        else
        {
            resize50Percent();
        }
    }
}

PlayListDialog* PurePlayer::playListDialog()
{
    if( _playListDialog == NULL ) {
        _playListDialog = new PlayListDialog(&_playList, this);
        connect(_playListDialog, SIGNAL(playItem()), this, SLOT(playFromPlayListDialog()));
        connect(_playListDialog, SIGNAL(stopItem()), this, SLOT(stop()));
    }

    return _playListDialog;
}

void PurePlayer::mpProcessFinished()
{
    LogDialog::print(QString("[%1]PurePlayer::mpProcessFinished(): start-")
                        .arg(QTime::currentTime().toString()), QColor(106,129,198));
    LogDialog::debug(QString("PurePlayer::mpProcessFinished(): state finished with %1.")
                        .arg(_state));

    if( isPeercastStream() ) {
        _elapsedTime = _timeLabel->time();
        LogDialog::debug(QString("PurePlayer::mpProcessFinished(): elapsed time %1")
                            .arg(_elapsedTime));

        if( isStop() ) {
            setStatus(STOP);
        }
        else {
            _reconnectCount++;
            LogDialog::debug(QString("PurePlayer::mpProcessFinished(): reconnectCount %1")
                             .arg(_reconnectCount));

            if( _reconnectCount <= 3 ) {
                if( _searchingConnection ) {
                    LogDialog::debug("PurePlayer::mpProcessFinished(): reconnectPurePlayer",
                                     QColor(255,0,0));
                    reconnectPurePlayer();
                }
                else {
                    LogDialog::debug("PurePlayer::mpProcessFinished(): reconnect",
                                     QColor(255,0,0));
                    reconnect();
                }
            }
            else
                setStatus(STOP);
        }
/*
         if( isPlaying() ) {
            if( _searchingConnection ) {
                LogDialog::debug("PurePlayer::mpProcessFinished(): reconnectPurePlayer",
                                 QColor(255,0,0));
                reconnectPurePlayer();
            }
            else {
                LogDialog::debug("PurePlayer::mpProcessFinished(): reconnect",
                                 QColor(255,0,0));
                reconnect();
            }
        }
        else
            setStatus(STOP);
*/
    }
    else {
        if( isPlaying() ) {
            int oldIndex = _playList.currentIndex();
            _playList.upCurrentIndex();
            playListDialog()->updateCurrentItemBackground();

            if( oldIndex != _playList.currentIndex() )
                play();
            else
            if( _doLoop ) {
                _playList.setCurrentIndex(0);
                playListDialog()->updateCurrentItemBackground();
                play();
            }
            else
                setStatus(STOP);
        }
        else
            setStatus(STOP);
    }

    LogDialog::debug(QString("PurePlayer::mpProcessFinished(): -end"));
}

void PurePlayer::mpProcessError(QProcess::ProcessError error)
{
    if( error == QProcess::FailedToStart ) {
        setStatus(STOP);
        LogDialog::debug("PurePlayer::mpProcessError(): mplayer not started.", QColor(255,0,0));
        QMessageBox::warning(this, tr("エラー"),
                tr("MPlayerを起動できませんでした。\n"
                   "起動にはMPlayerがインストールされており、\n"
                   "PATHが通っているか、起動設定が正しく指定されている必要があります。"));
    }
}

void PurePlayer::mpProcessDebugKilledCPid()
{
    _debugCount++;
    setWindowTitle(QString("%1 %2").arg(_chName).arg(_debugCount));
}

void PurePlayer::parseMplayerOutputLine(const QString& line)
{
    static QRegExp rxCacheFill("^Cache fill: *([0-9.]+)%");
//  static QRegExp rxVideoW("^ID_VIDEO_WIDTH=(\\d+)");
//  static QRegExp rxVideoH("^ID_VIDEO_HEIGHT=(\\d+)");
    static QRegExp rxVideoWH("^VO: \\[.+\\] \\d+x\\d+ => (\\d+)x(\\d+)");
    static QRegExp rxLength("^ID_LENGTH=([0-9.]+)");
    static QRegExp rxSeekable("^ID_SEEKABLE=(\\d)");
//  static QRegExp rxStartTime("^ID_START_TIME=([0-9.]+)");
    static QRegExp rxTitle("^ title: (.+)");
    static QRegExp rxAuthor("^ author: (.+)");
    static QRegExp rxCopyright("^ copyright: (.+)");
    static QRegExp rxComments("^ comments: (.+)");
    static QRegExp rxStatus("^[AV]: *([0-9.-]+)");
    static QRegExp rxFrame(".+ (\\d+)\\/ *\\d+");
    static QRegExp rxScreenshot("^\\*\\*\\* screenshot '(.+)'");
    static QRegExp rxScreenshotError(".+ Error opening .+ for writing!");

    if( rxStatus.indexIn(line) != -1 ) {
        if( _startTime == -1 ) {
            _startTime = rxStatus.cap(1).toDouble();
            _oldTime = _startTime;
            LogDialog::debug("PurePlayer::parseMplayerOutputLine(): init startTime "
                                + QString::number(_startTime));
        }

        _currentTime = rxStatus.cap(1).toDouble();

        if( _currentTime > _startTime ) {
            if( isPeercastStream() ) {
                double differenceTime = _currentTime - _oldTime;

                // 現在の取得時間が前の取得時間から大きく飛んだ場合、経過時間を無効にする
                // (再生開始時の古いキャッシュ再生による開始時間ズレの対応)
                if( differenceTime > 1 ) {
                    LogDialog::debug(
                        QString("PurePlayer::parseMplayerOutputLine(): oldElapsed %1")
                                            .arg(_elapsedTime), QColor(255,0,0));

                    _elapsedTime += _oldTime - _startTime;
                    _startTime = _currentTime;

                    LogDialog::debug(
                        QString("PurePlayer::parseMplayerOutputLine(): elapsed %1")
                                            .arg(_elapsedTime), QColor(255,0,0));
                    LogDialog::debug(
                        QString("PurePlayer::parseMplayerOutputLine(): startTime %1")
                                            .arg(_startTime), QColor(255,0,0));
                    LogDialog::debug(
                        QString("PurePlayer::parseMplayerOutputLine(): diff %1 old %2")
                                            .arg(differenceTime)
                                            .arg(_oldTime), QColor(255,0,0));
                }

                // 差分時間が特に大きい場合、再接続する
                // (差分時間が大きいとフレームがしばらく停止してしまう為)
                if( differenceTime > 10 ) {
                    LogDialog::debug(
                        "PurePlayer::parseMplayerOutputLine(): reconnect diff",
                        QColor(255,0,0));

                    reconnectPurePlayer();
                    //reconnect();
                }
            }
            else {
                // 2点間リピート処理
                if( _repeatStartTime>=0 && _repeatEndTime>=0 ) {
                    if( (_currentTime >= _repeatEndTime/10.0
                     && !(_controlFlags & FLG_SEEKED_REPEAT))
                     || _currentTime >= _repeatEndTime/10.0 + 1 )
                    {
                        //mpCmd("osd 0");
                        seek(_repeatStartTime/10.0);
                        //mpCmd("osd 1");

                        _controlFlags |= FLG_SEEKED_REPEAT;

                        LogDialog::debug(
                            QString("PurePlayer::parseMplayerOutputLine(): old %1 current %2 end %3")
                            .arg(_oldTime).arg(_currentTime).arg(_repeatEndTime/10.0));
                    }
                    else
                    if( _currentTime < _repeatEndTime/10.0
                     && _controlFlags & FLG_SEEKED_REPEAT )
                    {
                        _controlFlags -= FLG_SEEKED_REPEAT;
                    }
                }
            }

            _oldTime = _currentTime;

            double time = _currentTime - _startTime;
            _timeLabel->setTime(time + _elapsedTime);

            if( _isSeekable ) {
//              if( time != _oldTime )
                _timeslider->setPosition(time);

//              _oldTime = time;
            }
        }

        if( rxFrame.indexIn(line) != -1 ) {
            _labelFrame->setText(rxFrame.cap(1));

            _debugFrame = rxFrame.cap(1).toInt(); // debug
        }

        if( _state == PAUSE ) // ポーズが解除された場合
            setStatus(PLAY);

//      if( time < 0 ) _debugFlg = true;
        if( _debugFlg ) LogDialog::print(line);
        return;
    }

    LogDialog::print(line);

    if( (line.startsWith("Starting playback...") && _noVideo)
     || (rxVideoWH.indexIn(line) != -1) )
    {
        setStatus(PLAY);

        if( isMute() )
            mute(true);

        double roundSpeed = CommonLib::round(_speedSpinBox->value(), 1);//(int)(_speedSpinBox->value()*10+0.5) / 10.0;
        if( roundSpeed != 1.0 ) {
            setSpeed(_speedSpinBox->value());
        }
//      LogDialog::debug(QString().sprintf("%.70f", _speedSpinBox->value()));
//      LogDialog::debug(QString().sprintf("%.70f", roundSpeed));

        _controlFlags |= FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 0");

        setVolume(_volume);
        setContrast(_videoSettings.contrast);
        setBrightness(_videoSettings.brightness);
        setHue(_videoSettings.hue);
        setSaturation(_videoSettings.saturation);
        setGamma(_videoSettings.gamma);
        setLoop(_doLoop);

        _controlFlags &= ~FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 1");

        // mplayerプロセスの子プロセスIDを取得する
        _mpProcess->receiveMplayerChildProcess();

        // ビデオサイズの設定
        if( _noVideo )
            _videoSize = QSize(320, 240);
        else {
            _videoSize.setWidth(rxVideoWH.cap(1).toInt());
            if( _videoSize.width() <= 0 ) _videoSize.setWidth(320);

            _videoSize.setHeight(rxVideoWH.cap(2).toInt());
            if( _videoSize.height() <= 0 ) _videoSize.setHeight(240);
        }

        if( _openedNewPath )
        {
            _labelFrame->setText("0");
            _timeLabel->setTotalTime(_videoLength);
            _playList.currentItem()->setTime(_videoLength);
            playListDialog()->updateCurrentItemTime();
            _timeslider->setLength(_videoLength);
            //_speedSpinBox->setRange(0, _videoLength*10);

            // テキスト内容によってステータスバーの高さが変わる為、高さを固定にする
            statusBar()->setFixedHeight(statusBar()->height());

            if( _isSeekable ) {
                _toolBar->show();
                _actStatusBar->setEnabled(false);
            }
            else {
                _toolBar->hide();
                _actStatusBar->setEnabled(true);
            }

            // ウィンドウリサイズ
            QSize size;
            if( ConfigData::data()->openIn320x240Size )
                size = QSize(320, 240);
            else {
                size.setWidth(_videoSize.width() / 2);
                size.setHeight(_videoSize.height() / 2);
            }

            if( !resizeFromVideoScreen(size) )
                updateVideoScreenGeometry();

            _startTime = -1; // open()のタイミングでの初期化では、前の再生の
                             // ステータスラインを拾ってしまう為、ここで初期化。
            _elapsedTime = 0;

            _openedNewPath = false;
        }

        if( isPeercastStream() ) {
            _startTime = -1;

            // _reconnectControlTimeの比較を、再生開始時の開始時間から比較できる様に更新する
            _reconnectControlTime = _elapsedTime;
        }

        LogDialog::debug("PurePlayer::parseMplayerOutputLine(): videosize " +
                    QString("%1x%2").arg(_videoSize.width()).arg(_videoSize.height()));
    }
    else
    if( line.startsWith("ID_PAUSED") )
        setStatus(PAUSE);
    else
    if( line.startsWith("Connecting to") ) {
        if( _state != PLAY )// ネットワークストリーミングでシークした場合も受信する。一時対応
            _infoLabel->setText(tr("接続中"));
    }
    else
    if( line.startsWith("Cache size set to") ) {
        if( _reconnectWasCalled ) { // 再接続(peercast)は接続後行う
            reconnectPeercast();
            _reconnectWasCalled = false;
        }

        updateChannelInfo();
    }
    else
    if( line.startsWith("ID_EXIT=QUIT") )
        setStatus(STOP);
    else
    if( rxScreenshot.indexIn(line) != -1 ) {
        QString baseName = QString("%1_%2").arg(_chName)
                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

        new RenameTimerTask(rxScreenshot.cap(1), baseName, this);

        _infoLabel->setText("保存: スクリーンショット", 3000);
    }
    else
    if( rxScreenshotError.indexIn(line) != -1 )
        _infoLabel->setText("エラー: スクリーンショット保存", 3000);
    else
    if( rxCacheFill.indexIn(line) != -1 )
        _infoLabel->setText(QString("Cache: %1%").arg(rxCacheFill.cap(1), 5));
/*  else
    if( rxVideoW.indexIn(line) != -1 ) {
        _videoSize.setWidth(rxVideoW.cap(1).toInt());
        if( _videoSize.width() <= 0 ) _videoSize.setWidth(320);
    }
    else
    if( rxVideoH.indexIn(line) != -1 ) {
        _videoSize.setHeight(rxVideoH.cap(1).toInt());
        if( _videoSize.height() <= 0 ) _videoSize.setHeight(240);
    }
*/  else
    if( rxLength.indexIn(line) != -1 )
        _videoLength = rxLength.cap(1).toDouble();
    else
    if( rxSeekable.indexIn(line) != -1 )
        _isSeekable = rxSeekable.cap(1).toInt();
    else
    if( rxTitle.indexIn(line) != -1 )
        _infoLabel->setClipTitle(rxTitle.cap(1));
    else
    if( rxAuthor.indexIn(line) != -1 )
        _infoLabel->setClipAuthor(rxAuthor.cap(1));
    else
    if( rxCopyright.indexIn(line) != -1 )
        _infoLabel->setClipCopyright(rxCopyright.cap(1));
    else
    if( rxComments.indexIn(line) != -1 )
        _infoLabel->setClipComments(rxComments.cap(1));
    else
    if( line.startsWith("Video: no video") )
        _noVideo = true;
    else
    if( line.startsWith("Cache empty") || line.startsWith("Cache not filling")
     || line.contains("Bits overconsumption:") )
    {
        if( isPeercastStream() ) {
/*          LogDialog::debug(QString("PurePlayer::parseMplayerOutputLine(): time %1")
                                .arg(_timeLabel->time()));
            LogDialog::debug(QString("PurePlayer::parseMplayerOutputLine(): timea %1")
                                .arg(_currentTime));
            LogDialog::debug(QString("PurePlayer::parseMplayerOutputLine(): frame %1")
                                .arg(_debugFrame));
*/
//          _debugFlg = !_debugFlg;

            _receivedErrorCount++;
        }
    }
}

void PurePlayer::recordingProcessFinished()
{
    LogDialog::debug("PurePlayer::recordingProcessFinished():");
}

void PurePlayer::recordingOutputLine(const QString& line)
{
    LogDialog::print("PurePlayer::recordingOutputLine(): " + line);
}

void PurePlayer::replyFinished(QNetworkReply* reply)
{
    if( reply->error() != QNetworkReply::NoError )
        LogDialog::debug("PurePlayer::replyFinished(): error " +
                                       QString::number(reply->error()), QColor(255,0,0));

    // チャンネル情報リクエスト
    if( reply->url().path().contains("/relayinfo.html") ) {
        int i;
        QRegExp rx;
        QString out = reply->readAll();

        LogDialog::debug("PurePlayer::replyFinished(): size " + QString::number(out.size()));
//      LogDialog::debug("PurePlayer::replyFinished(): " + out);

        // チャンネル名の取得
        //rx.setPattern("<td>チャンネル名</td>.+\">(.*)</a>.+<td>ジャンル</td>.+<td>概要</td>");
        rx.setPattern("<td>チャンネル名</td>.+\">(.*)</a></td>\\s*</tr>\\s*<tr id=\"d\">");
        i = out.indexOf(rx);
        if( i == -1 )
            goto FAILED;

        _chName = rx.cap(1);
        LogDialog::debug("PurePlayer::replyFinished(): chName " + _chName);
        if( _chName.isEmpty() )
            _chName = "PurePlayer*";

        // コンタクトURLの取得
        rx.setPattern("<td>URL</td>.+\">(.*)</a>.+<td>配信者から");
        i = out.indexOf(rx, i);
        if( i == -1 )
            goto FAILED;

        _contactUrl = rx.cap(1);
        LogDialog::debug("PurePlayer::replyFinished(): contact url " + _contactUrl);

        // 接続先IPアドレスの取得
        rx.setPattern("<td>取得元</td>.+(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).+<td>受信時間</td>");
        i = out.indexOf(rx, i);
        if( i == -1 )
            goto FAILED;

        LogDialog::debug(QString("PurePlayer::replyFinished(): %1 %2")
                                                        .arg(rx.cap(1)).arg(_rootIp));

#ifndef QT_NO_DEBUG_OUTPUT
        if( rx.cap(1) == _rootIp )
            _chName = "[" + _chName + "]"; // 「peercastのみ再接続」する度にも、チャンネル情報取らなくてはいけなくなるので要検討
#endif

/*        if( rx.cap(1) == "0.0.0.0" )
            _searchingConnection = true;
        else
            _searchingConnection = false;
*/
        // peercastの状態取得
        rx.setPattern("<td>状態</td>\\s*<td>([a-zA-Z]+)</td>");
        i = out.indexOf(rx, i);
        if( i == -1 )
            goto FAILED;

        if( rx.cap(1) == "SEARCH" )
            _searchingConnection = true;
        else
            _searchingConnection = false;

        LogDialog::debug(QString("PurePlayer::replyFinished(): state %1").arg(rx.cap(1)));

FAILED:
        if( _contactUrl.isEmpty() )
            _actOpenContactUrl->setEnabled(false);
        else
            _actOpenContactUrl->setEnabled(true);

        if( _debugCount ) setWindowTitle(_chName + QString(" %1").arg(_debugCount));
        else              setWindowTitle(_chName);
    }

    LogDialog::debug(QString("PurePlayer::replyFinished(): end. request url %1")
                                                        .arg(reply->url().toString()));
    reply->deleteLater();
}

void PurePlayer::actGroupAudioOutputChanged(QAction* action)
{
    int index = _actGroupAudioOutput->actions().indexOf(action);
    setAudioOutput((AUDIO_OUTPUT_MODE)index);
}

void PurePlayer::actGroupVolumeFactorChanged(QAction* action)
{
    int index = _actGroupVolumeFactor->actions().indexOf(action);
    setVolumeFactor((VOLUME_FACTOR_MODE)index);
}

void PurePlayer::actGroupAspectChanged(QAction* action)
{
    int index = _actGroupAspect->actions().indexOf(action);
    setAspectRatio((ASPECT_RATIO)index);
}

void PurePlayer::actGroupDeinterlaceChanged(QAction* action)
{
    int index = _actGroupDeinterlace->actions().indexOf(action);
    setDeinterlace((DEINTERLACE_MODE)index);
}

void PurePlayer::timerReconnectTimeout()
{
/*  LogDialog::debug(QString("PurePlayer::timerReconnectTimeout(): time %1")
                        .arg(_timeLabel->time()));
    LogDialog::debug(QString("PurePlayer::timerReconnectTimeout(): currentTime %1")
                        .arg(_currentTime));
    LogDialog::debug(QString("PurePlayer::timerReconnectTimeout(): frame %1")
                        .arg(_debugFrame));
*/
//  LogDialog::debug("PurePlayer::timerReconnectTimeout(): " +
//                                              QString::number(_receivedErrorCount));

    if( _reconnectControlTime == _timeLabel->time() ) {
        LogDialog::debug(QString("PurePlayer::timerReconnectTimeout(): reconnect time %1")
                .arg(_reconnectControlTime), QColor(255,0,0));

        if( _searchingConnection )
            reconnectPurePlayer();
        else
            reconnect();
    }
    else
    if( _receivedErrorCount > 10 ) {
        LogDialog::debug(QString("PurePlayer::timerReconnectTimeout(): reconnect count %1")
                .arg(_receivedErrorCount), QColor(255,0,0));

        reconnect();
    }
    else
    if( _searchingConnection )
        updateChannelInfo();
    else
    if( _receivedErrorCount == 0 )
        _reconnectCount = 0;

    _reconnectControlTime = _timeLabel->time();

    _receivedErrorCount = 0;

//  _debugFlg = !_debugFlg;
}

void PurePlayer::appliedFromConfigDialog(bool restartMplayer)
{
    ConfigData::saveData();

    if( restartMplayer ) {
//      _actScreenshot->setVisible(ConfigData::data()->screenshot);
//      _screenshotButton->setVisible(ConfigData::data()->screenshot);

        setCurrentDirectory();

        if( !isStop() )
            restartPlay(true);
    }

    LogDialog::debug("PurePlayer::appliedFromConfigDialog(): end");
}

void PurePlayer::mpCmd(const QString& command)
{
    _mpProcess->command(command);
    LogDialog::debug("PurePlayer::mpCmd(): " + command);
}

void PurePlayer::buttonPlayPauseClicked()
{
    switch( _state ) {
    case PLAY:
    case PAUSE: pauseUnPause(); break;
    case STOP:  play();         break;
    default: break;
    }
}

void PurePlayer::playCommonProcess()
{
    LogDialog::debug("PurePlayer::playCommonProcess(): start");
    if( _mpProcess->state() != QProcess::NotRunning ) {
        LogDialog::debug(tr("PurePlayer::playCommonProcess(): running %1")
                                        .arg(_mpProcess->state()), QColor(255,0,0));
        return;
    }

    _noVideo = false;
    setStatus(READY);

    QStringList args;

    if( !ConfigData::data()->voName.isEmpty() ) {
#ifdef Q_WS_X11
        if( ConfigData::data()->voName == "x11" )
            args << "-vo" << ConfigData::data()->voName + ",";
        else
            args << "-vo" << ConfigData::data()->voName + ",x11,";
#else
        args << "-vo" << ConfigData::data()->voName + ",";
#endif
    }

    if( !ConfigData::data()->aoName.isEmpty() )
        args << "-ao" << ConfigData::data()->aoName + ",";

    if( ConfigData::data()->useSoftWareVideoEq )
        args << "-vf-add" << "eq2,hue";

    if( _deinterlace == DI_YADIF )
        args << "-vf-add" << "yadif";
    else
    if( _deinterlace == DI_LINEAR_BLEND )
        args << "-vf-add" << "pp=lb";

//  if( ConfigData::data()->screenshot )
        args << "-vf-add" << "screenshot";

    switch( _audioOutput ) {
    case AO_MONAURAL: args << "-af-add" << "extrastereo=0"; break;
//                    args << "-af-add" << "pan=2:1:1:1:1"; break;
//                    args << "-af-add" << "pan=2:0.5:0.5:0.5:0.5"; break;
    case AO_LEFT    : args << "-af-add" << "channels=2:2:0:0:0:1"; break;
    case AO_RIGHT   : args << "-af-add" << "channels=2:2:1:0:1:1"; break;
    default: break;
    }

    args << "-softvol-max";
    switch( _volumeFactor ) {
    case VF_DOUBLE: args << "220"; break;
    case VF_TRIPLE: args << "330"; break;
    case VF_NORMAL:
    default       : args << "110";
    }

    if( _path.left(8).contains("://") )
        args << "-cache" << QString::number(ConfigData::data()->cacheStreamSize);
    else
        args << "-nocache";

    if( _seekWhenStartMplayer ) {
        args << "-ss" << QString::number(_timeLabel->time());
        _seekWhenStartMplayer = false;
    }

    args
    << "-noquiet"
    << "-identify"
    << "-af-add" << "scaletempo"
    << "-osdlevel" << "0"
    << "-idx"
    << "-double"
    << "-nodr"          // -drの場合、OSD表示がちらつく
    << "-framedrop"
    << "-zoom"
    << "-nokeepaspect"
    << "-volume" << (isPeercastStream()||isMute() ? "0" : QString::number(_volume))
    << "-softvol"
    << "-prefer-ipv4"
    << "-nomouseinput"
    << "-input" << "nodefault-bindings:conf=/dev/null"
    << "-slave"
    << "-wid" << QString::number((unsigned)_videoScreen->winId())

#ifdef Q_WS_X11
    << "-stop-xscreensaver"
#endif

#ifdef Q_OS_WIN32
    << "-colorkey" << QString().sprintf("%#08x",
                        qRgba(qBlue(COLORKEY), qGreen(COLORKEY), qRed(COLORKEY), 0))
#endif

    << _path;

    QString mplayerPath;
    if( ConfigData::data()->useMplayerPath )
        mplayerPath = ConfigData::data()->mplayerPath;
    else
        mplayerPath = "mplayer";

    LogDialog::print(QString("[%1]PurePlayer::playCommonProcess(): processStart -------------")
                        .arg(QTime::currentTime().toString()), QColor(106,129,198));
    LogDialog::print(QString("PurePlayer: %1 %2").arg(mplayerPath).arg(args.join(" ")));

    _mpProcess->start(mplayerPath, args, QIODevice::ReadWrite);
    _mpProcess->waitForStarted();
}

void PurePlayer::saveVideoSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    s.setValue("contrast",   _videoSettings.contrast);
    s.setValue("brightness", _videoSettings.brightness);
    s.setValue("hue",        _videoSettings.hue);
    s.setValue("saturation", _videoSettings.saturation);
    s.setValue("gamma",      _videoSettings.gamma);

    LogDialog::debug("PurePlayer::saveVideoSettings(): end");
}

void PurePlayer::loadVideoSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    _videoSettings.contrast   = s.value("contrast",   0).toInt();
    _videoSettings.brightness = s.value("brightness", 0).toInt();
    _videoSettings.hue        = s.value("hue",        0).toInt();
    _videoSettings.saturation = s.value("saturation", 0).toInt();
    _videoSettings.gamma      = s.value("gamma",      0).toInt();

    LogDialog::debug("PurePlayer::loadVideoSettings(): end");
}

void PurePlayer::saveInteractiveSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    int volume;
    if( _volume > ConfigData::data()->volumeMax )
        volume = ConfigData::data()->volumeMax;
    else
        volume = _volume;

    s.setValue("volume",    volume);
    s.setValue("mute",      _isMute);
    s.setValue("statusbar", _alwaysShowStatusBar);
    s.setValue("pos",       pos());

    LogDialog::debug("PurePlayer::saveInteractiveSettings(): end");
}

void PurePlayer::loadInteractiveSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "PurePlayer", "PurePlayer");

    mute(s.value("mute", false).toBool());
    setVolume(s.value("volume", 30).toInt());
    setAlwaysShowStatusBar(s.value("statusbar", true).toBool());
    move(s.value("pos", QPoint(0,0)).toPoint());

    LogDialog::debug("PurePlayer::loadInteractiveSettings(): end");
}

void PurePlayer::updateVideoScreenGeometry()
{
//  if( !isFullScreen() && _alwaysShowStatusBar && statusBar()->isHidden() )
//      statusBar()->show();

    int h, w, x, y;
    QSize screen(centralWidget()->size());

    if( !isFullScreen() && _alwaysShowStatusBar && statusBar()->isHidden() )
    // ステータスバーを表示に切り替えた直後の場合(setAlwaysShowStatusBar())
        screen.setHeight(screen.height() - statusBar()->size().height());
    else
    if( !isAlwaysShowStatusBar() ) {
        screen.setWidth(width());
        screen.setHeight(height());
    }

    // ビデオ表示の位置,サイズ設定
    if( _aspectRatio != NO_KEEP )
    {
        int aspectWidth, aspectHeight;

        switch( _aspectRatio ) {
        case RATIO_4_3:
            aspectWidth  = 4;
            aspectHeight = 3;
            break;
        case RATIO_16_9:
            aspectWidth  = 16;
            aspectHeight = 9;
            break;
        case RATIO_16_10:
            aspectWidth  = 16;
            aspectHeight = 10;
            break;
        default:    // RATIO_VIDEO or other
            aspectWidth  = _videoSize.width();
            aspectHeight = _videoSize.height();
        }

        double tempw = screen.height() * aspectWidth / (double)aspectHeight;
        if( tempw <= screen.width() ) {
            // ビデオの比に対してウィンドウの比が横長
            h = screen.height();
            w = screen.height() * aspectWidth / aspectHeight;
            //w = screen.height() * aspectRatio + 0.1;

            x = (screen.width() - w) / 2;
            y = 0;
        }
        else {
            // ビデオの比に対してウィンドウの比が縦長
            h = screen.width() * aspectHeight / aspectWidth;
            //h = screen.width() / aspectRatio + 0.1;
            w = screen.width();

            x = 0;
            y = (screen.height() - h) / 2;
        }
    }
    else
    {
        h = screen.height();
        w = screen.width();
        x = 0;
        y = 0;
    }

    _videoScreen->setGeometry(QRect(x,y, w,h));

    LogDialog::debug(QString()
            .sprintf("PurePlayer::updateVideoScreenGeometry(): videorect(%d,%d,%dx%d)",
            x,y,w,h));
}

void PurePlayer::visibleInterface(bool b)
{
    if( b ) {
        statusBar()->show();
        if( _isSeekable )
            _toolBar->show();
    }
    else {
        statusBar()->hide();
        _toolBar->hide();
    }
}

void PurePlayer::updateVisibleInterface()
{
    if( isFullScreen() ) {
        if( (QApplication::desktop()->screen()->height()*80/100) < QCursor::pos().y() )
//      if( (height()*80/100) < QCursor::pos().y() )
            visibleInterface(true);
        else {
            if( isPlaying() )
                visibleInterface(false);
            else
                visibleInterface(true);
        }
    }
    else {
        if( _isSeekable || !isPlaying() || isAlwaysShowStatusBar() || _cursorInWindow )
            visibleInterface(true);
        else
            visibleInterface(false);
    }

    // フルスクリーンでもウィンドウ外に出てしまう場合(画面上下)がある為、
    // ウィンドウモードへ戻ってもleaveEvent()が呼ばれない場合がある。
}

void PurePlayer::setStatus(const STATE s)
{
    switch( s ) {
    case PLAY:
        _state = s;
        _infoLabel->setText(tr("再生"));
        _playPauseButton->setIcon(QIcon(":/icons/pause.png"));
        _playPauseButton->setToolTip(tr("一時停止"));
        _frameAdvanceButton->setEnabled(true);
        _screenshotButton->setEnabled(true);
        // ステータス非表示切り替え // ウィンドウ、フルスクリーン時用で再実装の必要あり
        if( !isFullScreen() && !isAlwaysShowStatusBar() ) {
            if( !_cursorInWindow )
                visibleInterface(false);
        }

        _actScreenshot->setEnabled(true);
        _actReconnectPct->setEnabled(true);
        _actPlayPause->setText(tr("一時停止"));
        _actPlayPause->setEnabled(true);
        _actStop->setEnabled(true);

        _infoLabel->startClipInfo();

        if( isPeercastStream() )
            _timerReconnect.start(6000);

#ifdef Q_WS_X11
        if( !_noVideo ) {
            _videoScreen->setAttribute(Qt::WA_NoSystemBackground);
            _videoScreen->setAttribute(Qt::WA_PaintOnScreen);
        }
#endif
        break;

    case PAUSE:
        _state = s;
        _infoLabel->setText(tr("一時停止"));
        _playPauseButton->setIcon(QIcon(":/icons/play.png"));
        _playPauseButton->setToolTip(tr("再生"));
        _actPlayPause->setText(tr("再生"));
        _infoLabel->stopClipInfo();
        break;

    case READY:
        _state = s;
        _infoLabel->setText(tr("準備中"));
        _stopButton->setEnabled(true);
        visibleInterface(true);

        _actReconnectPct->setEnabled(true);
        _actPlayPause->setEnabled(false);
        _actStop->setEnabled(true);
#ifdef Q_WS_X11
        if( !_noVideo ) {
            _videoScreen->setAttribute(Qt::WA_NoSystemBackground, false);
            _videoScreen->setAttribute(Qt::WA_PaintOnScreen, false);
        }
#endif
        break;

    case STOP:
    default:
        _state = STOP;
        _infoLabel->setText(tr("停止"));
        _playPauseButton->setIcon(QIcon(":/icons/play.png"));
        _playPauseButton->setToolTip(tr("再生"));
        _stopButton->setEnabled(false);
        _frameAdvanceButton->setEnabled(false);
        _screenshotButton->setEnabled(false);
        _timeslider->setSliderDown(false);
        _timeslider->setPosition(0);
        visibleInterface(true);

        _actScreenshot->setEnabled(false);
        if( _path.isEmpty() ) {
            _actReconnect->setEnabled(false);
            _actReconnectPly->setEnabled(false);
        }
        else {
            _actReconnect->setEnabled(true);
            _actReconnectPly->setEnabled(true);
        }
        _actReconnectPct->setEnabled(false);
        _actPlayPause->setText(tr("再生"));
        _actPlayPause->setEnabled(true);
        _actStop->setEnabled(false);

        _reconnectWasCalled = false;

        _infoLabel->stopClipInfo();
        _receivedErrorCount = 0;

        if( _timerReconnect.isActive() )
            _timerReconnect.stop();

#ifdef Q_WS_X11
        if( !_noVideo ) {
            _videoScreen->setAttribute(Qt::WA_NoSystemBackground, false);
            _videoScreen->setAttribute(Qt::WA_PaintOnScreen, false);
            _videoScreen->repaint();
        }
#endif
        break;
    }
}

