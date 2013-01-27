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
#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScriptEngine>
#include "pureplayer.h"
#include "process.h"
#include "controlbutton.h"
#include "timeslider.h"
#include "infolabel.h"
#include "timelabel.h"
#include "playlist.h"
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

PurePlayer::PurePlayer(QWidget* parent) : QMainWindow(parent)
{
    const QIcon appIcon(":/icons/heart.png");

    LogDialog::initDialog(this);
//  LogDialog::dialog()->setWindowIcon(appIcon);
    connect(LogDialog::dialog(), SIGNAL(windowActivate()),
            this,                SLOT(raise()));
    connect(LogDialog::dialog(), SIGNAL(requestCommand(const QString&)),
            this,                SLOT(mpCmd(const QString&)));

    setFont(QFont("DejaVu Sans", 9));
    setWindowIcon(appIcon);
    setWindowTitle("PurePlayer*");
    setAcceptDrops(true);

    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(backgroundRole(), QColor("black"));
    setPalette(p);

    setCentralWidget(new QWidget(this));
    centralWidget()->setAcceptDrops(true);
    _videoScreen = new QWidget(centralWidget());
    _videoScreen->setAcceptDrops(true);
    _videoScreen->lower();
#ifdef Q_OS_WIN32
    _videoScreen->setAutoFillBackground(true);
    p = _videoScreen->palette();
    p.setColor(_videoScreen->backgroundRole(), QColor(Qt::black));
    _videoScreen->setPalette(p);

    initColorKey();
#endif

//  _oldTime = -1;
    _path = "";
    _audioOutput = AO_STEREO;
    _volumeFactor = VF_NORMAL;
    _aspectRatio = RATIO_VIDEO;
    _deinterlace = DI_NO_DEINTERLACE;
    _videoSize = QSize(320, 240);
    _noVideo = false;
    _isSeekable = false;
    _cursorInWindow = false;
    _controlFlags = FLG_NONE;

    _port = -1;
    _id   = "";
    _chName = "";

    _nam = new QNetworkAccessManager(this);
    connect(_nam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(nam_finished(QNetworkReply*)));

    _replyChannelInfoPcVp   = NULL;
    _replyChannelInfoPcSt   = NULL;
    _replyChannelStatusPcSt = NULL;

    _mpProcess = new MplayerProcess(this);
    connect(_mpProcess, SIGNAL(outputLine(const QString&)),
            this,       SLOT(mpProcess_outputLine(const QString&)));
    connect(_mpProcess, SIGNAL(finished()),
            this,       SLOT(mpProcess_finished()));
    connect(_mpProcess, SIGNAL(error(QProcess::ProcessError)),
            this,       SLOT(mpProcess_error(QProcess::ProcessError)));
    connect(_mpProcess, SIGNAL(debugKilledCPid()),
            this,       SLOT(mpProcess_debugKilledCPid()));

    _recProcess = new RecordingProcess(this);
    connect(_recProcess, SIGNAL(outputLine(const QString&)),
            this,        SLOT(recProcess_outputLine(const QString&)));
//  connect(_recProcess, SIGNAL(finished()),
//          this,        SLOT(recProcess_finished()));

    _receivedErrorCount = 0;
    _reconnectCount = 0;
    _reconnectControlTime  = 0;
    connect(&_timerReconnect, SIGNAL(timeout()), this, SLOT(timerReconnect_timeout()));

    _fpsCount = 0;
    _oldFrame = 0;
    connect(&_timerFps, SIGNAL(timeout()), this, SLOT(timerFps_timeout()));

    _playlist = new PlaylistModel(this);
    connect(_playlist, SIGNAL(removedCurrentTrack()), this, SLOT(stop()));

    _openDialog        = NULL;
    _videoAdjustDialog = NULL;
    _configDialog      = NULL;
    _playlistDialog    = NULL;
    _aboutDialog       = NULL;

    createStatusBar();
    createActionContextMenu();
    createToolBar();
    ConfigData::loadData();
//  _actScreenshot->setVisible(ConfigData::data()->screenshot);
//  _screenshotButton->setVisible(ConfigData::data()->screenshot);
    _state = STOP;
    _videoSettingsModifiedId = 0;
    refreshVideoProfile();
    loadInteractiveSettings();

    _peercastType = PCT_UNKNOWN;
    setStatus(STOP);
    resizeFromVideoClient(_videoSize);

    _menuContext->move(x()+width()*0.2, y()+height()*0.2);

    _debugFlag = false;
    _debugCount = 0;
}

PurePlayer::~PurePlayer()
{
    delete statusBar()->style();

    Task::waitForFinished();
}

void PurePlayer::createStatusBar()
{
    _infoLabel = new InfoLabel(tr("停止 "));
    _timeLabel = new TimeLabel();
    _labelFrame = new QLabel();
    _labelFps = new QLabel();
    _labelVolume = new QLabel();
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
    statusBar()->addPermanentWidget(_labelFps);
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

    _labelFps->setFrameShape(QFrame::NoFrame);
    _labelFps->setAlignment(Qt::AlignRight);
    _labelFps->setText(tr(" 00fps"));
    _labelFps->setMinimumWidth(_labelFps->sizeHint().width());
    _labelFps->setText("0fps");

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

    _screenshotButton = new ControlButton(QIcon(":/icons/screenshot.png"), "", this);
    _screenshotButton->setFocusPolicy(Qt::NoFocus);
    _screenshotButton->setIconSize(iconSize);
    _screenshotButton->setFixedSize(buttonSize);
    _screenshotButton->setToolTip("スクリーンショット");
    connect(_screenshotButton, SIGNAL(clicked(bool)), this, SLOT(screenshot()));

    _timeSlider   = NULL;
    _speedSpinBox = NULL;
    _toolBar      = NULL;
    _timeSlider = new TimeSlider(this);
    _timeSlider->installEventFilter(this);
    _timeSlider->setFocusPolicy(Qt::NoFocus);
    connect(_timeSlider, SIGNAL(requestSeek(double, bool)), this, SLOT(seek(double, bool)));

    _speedSpinBox = new SpeedSpinBox(this);
    _speedSpinBox->installEventFilter(this);
    _speedSpinBox->setFixedWidth(_speedSpinBox->sizeHint().width());
    _speedSpinBox->setFixedHeight(17);
    _speedSpinBox->setToolTip(tr("再生速度"));
    connect(_speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)));

    _toolBar = new QToolBar("toolbar", this);
    _toolBar->installEventFilter(this);
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
    hl->addWidget(_screenshotButton);
    hl->addWidget(_timeSlider);
    //hl->addStretch();
    hl->addWidget(_speedSpinBox);
//  vl->addWidget(_timeSlider);
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

    _actScreenshot = new QAction(tr("スクリーンショット"), this);
    _actScreenshot->setShortcut(tr("c"));
    _actScreenshot->setAutoRepeat(false);
    connect(_actScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
    addAction(_actScreenshot);

    _actPlayPause = new QAction(tr("再生"), this);
    _actPlayPause->setShortcut(tr("space"));
    _actPlayPause->setAutoRepeat(false);
    connect(_actPlayPause, SIGNAL(triggered()), this, SLOT(buttonPlayPauseClicked()));
    addAction(_actPlayPause);

    _actStop = new QAction(tr("停止"), this);
    //_actStop->setIcon(QIcon(":/icons/stop.png"));
    _actStop->setShortcut(tr("s"));
    _actStop->setAutoRepeat(false);
    _actStop->setEnabled(false);
    connect(_actStop, SIGNAL(triggered()), this, SLOT(stop()));
    addAction(_actStop);

    _actOpenContactUrl = new QAction(tr("コンタクトURLを開く"), this);
    _actOpenContactUrl->setEnabled(false);
    _actOpenContactUrl->setVisible(false);
    connect(_actOpenContactUrl, SIGNAL(triggered()), this, SLOT(openContactUrl()));

    _actPlaylist = new QAction(tr("プレイリスト"), this);
    _actPlaylist->setShortcut(tr("l"));
    connect(_actPlaylist, SIGNAL(triggered()), this, SLOT(showPlaylistDialog()));
    addAction(_actPlaylist);

    _actStatusBar = new QAction(tr("ステータスを常に表示"), this);
    _actStatusBar->setCheckable(true);
    _actStatusBar->setChecked(false);
    connect(_actStatusBar, SIGNAL(triggered(bool)), this, SLOT(setAlwaysShowStatusBar(bool)));

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

    // 音声出力メニュー
    _actGroupAudioOutput = new QActionGroup(this);
    connect(_actGroupAudioOutput, SIGNAL(triggered(QAction*)),
            this,                 SLOT(actGroupAudioOutput_changed(QAction*)));
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
            this,                  SLOT(actGroupVolumeFactor_changed(QAction*)));
    QAction* actVFactor0 = new QAction(tr("音量を1/3にする"), _actGroupVolumeFactor);
    actVFactor0->setCheckable(true);
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
    QAction* actReduceSize = new QAction(tr("小さくする"), this);
    actReduceSize->setShortcut(tr("-"));
    connect(actReduceSize, SIGNAL(triggered()), this, SLOT(resizeReduce()));
    addAction(actReduceSize);
    QAction* actIncreaseSize = new QAction(tr("大きくする"), this);
    actIncreaseSize->setShortcut(tr("="));
    connect(actIncreaseSize, SIGNAL(triggered()), this, SLOT(resizeIncrease()));
    addAction(actIncreaseSize);
//  QAction* actSlightlyReduceSize = new QAction(tr("少し小さくする"), this);
//  actSlightlyReduceSize->setShortcut(tr("["));
//  connect(actSlightlyReduceSize, SIGNAL(triggered()), this, SLOT(resizeSlightlyReduce()));
//  addAction(actSlightlyReduceSize);
//  QAction* actSlightlyIncreaseSize = new QAction(tr("少し大きくする"), this);
//  actSlightlyIncreaseSize->setShortcut(tr("]"));
//  connect(actSlightlyIncreaseSize, SIGNAL(triggered()), this, SLOT(resizeSlightlyIncrease()));
//  addAction(actSlightlyIncreaseSize);
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
    menuSize->addAction(actReduceSize);
    menuSize->addAction(actIncreaseSize);
//  menuSize->addAction(actSlightlyReduceSize);
//  menuSize->addAction(actSlightlyIncreaseSize);
    menuSize->addSeparator();
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
            this,            SLOT(actGroupAspect_changed(QAction*)));
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
            this,                 SLOT(actGroupDeinterlace_changed(QAction*)));
    QAction* actDeinterlaceNo = new QAction(tr("無し"), _actGroupDeinterlace);
    actDeinterlaceNo->setCheckable(true);
    actDeinterlaceNo->setChecked(true);
    QAction* actDeinterlaceYadif = new QAction(tr("Yadif"), _actGroupDeinterlace);
    actDeinterlaceYadif->setCheckable(true);
    QAction* actDeinterlaceYadif2 = new QAction(tr("Yadif(2x)"), _actGroupDeinterlace);
    actDeinterlaceYadif2->setCheckable(true);
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
    _actReconnectPlayer = new QAction(tr("プレイヤーのみ"), this);
    connect(_actReconnectPlayer, SIGNAL(triggered()), this, SLOT(reconnectPurePlayerFromGui()));
    _actReconnectPct = new QAction(tr("PeerCastのみ"), this);
    _actReconnectPct->setEnabled(false);
    connect(_actReconnectPct, SIGNAL(triggered()), this, SLOT(reconnectPeercast()));

    _menuReconnect = new QMenu(tr("再接続"), this);
    _menuReconnect->menuAction()->setVisible(false);
    _menuReconnect->addAction(_actReconnect);
    _menuReconnect->addAction(_actReconnectPlayer);
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
    _menuContext->addAction(_actPlaylist);
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
    int rows = _playlist->appendTracks(QStringList() << path);
    if( rows ) {
        _playlist->setCurrentTrackIndex(_playlist->rowCount() - rows);

        _controlFlags |= FLG_RESIZE_WHEN_PLAYED;
        openCommonProcess(_playlist->currentTrackPath());
    }
    else
        QMessageBox::warning(this, tr("エラー"), tr("指定されたファイル又はURLが正しくありません"));
}

void PurePlayer::open(const QList<QUrl>& urls)
{
    int rows = _playlist->insertTracks(_playlist->rowCount(), urls);
    if( rows ) {
        // 新たに追加したアイテム群の先頭要素をカレントインデックスとする
        _playlist->setCurrentTrackIndex(_playlist->rowCount() - rows);

        _controlFlags |= FLG_RESIZE_WHEN_PLAYED;
        openCommonProcess(_playlist->currentTrackPath());
    }
}

void PurePlayer::play()
{
    if( _playlist->rowCount() > 0 ) {
        QString path = _playlist->currentTrackPath();
        if( path != _path )
            openCommonProcess(path);
        else
            playCommonProcess();
    }
}

bool PurePlayer::playPrev()
{
    PlaylistModel::Track* track = _playlist->currentTrack();

    _playlist->downCurrentTrackIndex();
    if( track != _playlist->currentTrack() || _playlist->loopPlay() ) {
        _controlFlags &= ~FLG_RESIZE_WHEN_PLAYED;
        play();
        return true;
    }

    return false;
}

bool PurePlayer::playNext()
{
    PlaylistModel::Track* track = _playlist->currentTrack();

    _playlist->upCurrentTrackIndex();
    if( track != _playlist->currentTrack() || _playlist->loopPlay() ) {
        _controlFlags &= ~FLG_RESIZE_WHEN_PLAYED;
        play();
        return true;
    }

    return false;
}

void PurePlayer::setCurrentDirectory()
{
    QString current;

    if( ConfigData::data()->useScreenshotPath )
        current = ConfigData::data()->screenshotPath;
    else {
        if( QFile::exists(_playlist->currentTrackPath()) )
            current = QFileInfo(_playlist->currentTrackPath()).absolutePath();
        else
            current = QDir::homePath();
    }

    if( !QFileInfo(current).isWritable() || !QDir::setCurrent(current) )
        QDir::setCurrent(QDir::homePath());
}

void PurePlayer::openCommonProcess(const QString& path)
{
    const QString debugPrefix = "PurePlayer::openCommonProcess(): ";

    stop();

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

        _chName = "";
        _reconnectCount = 0;

        _menuReconnect->menuAction()->setVisible(true);
        // _actPlayPauseとのショートカットキー切り替えの為、設定
        _actReconnect->setVisible(true);

        _actPlayPause->setVisible(false);
        _actOpenContactUrl->setVisible(true);
        _actStatusBar->setVisible(true);

        LogDialog::debug(debugPrefix + "peercast url detected.");
        LogDialog::debug(debugPrefix + QString("port %1").arg(_port));
        LogDialog::debug(debugPrefix + QString("ID %1").arg(_id));
        LogDialog::debug(debugPrefix + QString("root %1").arg(_rootIp));
    }
    else {
        _host   = "";
        _port   = -1;
        _id     = "";
        _rootIp = "";
        _chName = _playlist->currentTrackTitle(); //path.split("/").last();

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
    _controlFlags |= FLG_OPENED_PATH;

    _contactUrl = "";
    reflectChannelInfo();

    _infoLabel->clearClipInfo();

    setCurrentDirectory();
    LogDialog::debug(debugPrefix + "current dir " + QDir::currentPath());

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
                        + (_menuContext->width()-_openDialog->frameSize().width())/2,
                      _menuContext->y());

    _openDialog->show();
}

void PurePlayer::stopPeercast()
{
    if( !isPeercastStream() ) return;

    new StopChannelTask(_host, _port, _id, this);
    LogDialog::debug("PurePlayer::stopPeercast(): ");
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

void PurePlayer::reconnect()
{
    if( _peercastType == PCT_ST ) {
        bool stoped = isStop();
        stopInternal();
        if( stoped || _controlFlags.testFlag(FLG_RECONNECTED) )
            _controlFlags &= ~FLG_RECONNECT_WHEN_PLAYED;
        else
            _controlFlags |= FLG_RECONNECT_WHEN_PLAYED;
    }
    else {
        stopInternal();
        _controlFlags |= FLG_RECONNECT_WHEN_PLAYED;
    }

    play();
}

void PurePlayer::reconnectPeercast()
{
    if( !isPeercastStream() ) return;

    QUrl url(QString("http://%1:%2/admin?cmd=bump&id=%3").arg(_host).arg(_port).arg(_id));
    _nam->get(QNetworkRequest(url));
    _controlFlags |= FLG_RECONNECTED;
    LogDialog::debug("PurePlayer::reconnectPeercast(): ");
}

void PurePlayer::recordingStartStop()
{
    if( _recProcess->state() == QProcess::NotRunning ) {
        QString saveName = genDateTimeSaveFileName("avi");

        QStringList args;
        args
        << "-quiet"
        << "-ovc" << "copy"
        << "-oac" << "copy"
        << _path
        << "-ofps" << "70"
        << "-o" << CommonLib::retTheFileNameNotExists(saveName);

        _recProcess->start("mencoder", args, QIODevice::ReadOnly);
        _recProcess->waitForStarted();
        LogDialog::debug("recording");
    }
    else {
        _recProcess->terminate();
        _recProcess->waitForFinished(3000);
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
        mpCmd(cmd);
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
            if( !_controlFlags.testFlag(FLG_HIDE_DISPLAY_MESSAGE) )
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

void PurePlayer::setAspectRatio(ASPECT_RATIO ratio)
{
    if( _aspectRatio != ratio ) {
        _aspectRatio = ratio;

        updateVideoScreenGeometry();

        _actGroupAspect->actions()[ratio]->setChecked(true);
    }
}

void PurePlayer::setContrast(int value, bool alwaysSet)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;

    if( _videoProfile.contrast!=value || alwaysSet ) {
        _videoProfile.contrast = value;

        if( _state == PLAY ) {
            if( _controlFlags.testFlag(FLG_HIDE_DISPLAY_MESSAGE) )
                mpCmd(QString("contrast %1 1").arg(_videoProfile.contrast));
            else {
                mpCmd("osd 1");
                mpCmd(QString("contrast %1 1").arg(_videoProfile.contrast));
                mpCmd("osd 0");
            }
        }
        else
        if( _state == PAUSE )
            mpCmd(QString("pausing_keep_force contrast %1 1").arg(_videoProfile.contrast));
    }

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->setContrast(value);
}

void PurePlayer::setBrightness(int value, bool alwaysSet)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;

    if( _videoProfile.brightness!=value || alwaysSet ) {
        _videoProfile.brightness = value;

        if( _state == PLAY ) {
            if( _controlFlags.testFlag(FLG_HIDE_DISPLAY_MESSAGE) )
                mpCmd(QString("brightness %1 1").arg(_videoProfile.brightness));
            else {
                mpCmd("osd 1");
                mpCmd(QString("brightness %1 1").arg(_videoProfile.brightness));
                mpCmd("osd 0");
            }
        }
        else
        if( _state == PAUSE )
            mpCmd(QString("pausing_keep_force brightness %1 1").arg(_videoProfile.brightness));
    }

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->setBrightness(value);
}

void PurePlayer::setSaturation(int value, bool alwaysSet)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;

    if( _videoProfile.saturation!=value || alwaysSet ) {
        _videoProfile.saturation = value;

        if( _state == PLAY ) {
            if( _controlFlags.testFlag(FLG_HIDE_DISPLAY_MESSAGE) )
                mpCmd(QString("saturation %1 1").arg(_videoProfile.saturation));
            else {
                mpCmd("osd 1");
                mpCmd(QString("saturation %1 1").arg(_videoProfile.saturation));
                mpCmd("osd 0");
            }
        }
        else
        if( _state == PAUSE )
            mpCmd(QString("pausing_keep_force saturation %1 1").arg(_videoProfile.saturation));
    }

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->setSaturation(value);
}

void PurePlayer::setHue(int value, bool alwaysSet)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;

    if( _videoProfile.hue!=value || alwaysSet ) {
        _videoProfile.hue = value;

        if( _state == PLAY ) {
            if( _controlFlags.testFlag(FLG_HIDE_DISPLAY_MESSAGE) )
                mpCmd(QString("hue %1 1").arg(_videoProfile.hue));
            else {
                mpCmd("osd 1");
                mpCmd(QString("hue %1 1").arg(_videoProfile.hue));
                mpCmd("osd 0");
            }
        }
        else
        if( _state == PAUSE )
            mpCmd(QString("pausing_keep_force hue %1 1").arg(_videoProfile.hue));
    }

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->setHue(value);
}

void PurePlayer::setGamma(int value, bool alwaysSet)
{
    if( value < -100 ) value = -100; else
    if( value > 100 )  value = 100;

    if( _videoProfile.gamma!=value || alwaysSet ) {
        _videoProfile.gamma = value;

        if( _state == PLAY ) {
            if( _controlFlags.testFlag(FLG_HIDE_DISPLAY_MESSAGE) )
                mpCmd(QString("gamma %1 1").arg(_videoProfile.gamma));
            else {
                mpCmd("osd 1");
                mpCmd(QString("gamma %1 1").arg(_videoProfile.gamma));
                mpCmd("osd 0");
            }
        }
        else
        if( _state == PAUSE )
            mpCmd(QString("pausing_keep_force gamma %1 1").arg(_videoProfile.gamma));
    }

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->setGamma(value);
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

// ビデオクライアントサイズを指定してウィンドウをリサイズする
// 返却値:
// リサイズ処理が行われたら、trueを返す。そうでないならfalseを返す。
bool PurePlayer::resizeFromVideoClient(QSize size)
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

void PurePlayer::resizePercentageFromCurrent(int percentage)
{
    if( percentage == 0 ) return;

    // 等倍サイズとして解釈するビデオサイズを取得
    QSize videoSize = videoSize100Percent();

    // 現在のビデオサイズの割合を求める
//  int currentPercentage = _videoScreen->width()*100 / videoSize.width();
    int currentPercentage = _videoScreen->height()*100 / videoSize.height();
/*
    // 現在のウィンドウサイズの割合がしきい値の何束目に相当するか求める
    int threshold = abs(percentage);
    int temp = (double)currentPercentage/threshold + 0.5;

    // リサイズする割合を求める
    int resizePercentage = (temp + (percentage>0 ? 1:-1)) * threshold;
*/  int resizePercentage = currentPercentage + percentage; // 単純に加算する場合

    if( resizePercentage <= 0 )
        return;

    // videoSizeをスケーリングしたサイズを取得
    QSize size = calcPercentageVideoSize(videoSize, resizePercentage);

    resizeFromVideoClient(size);
}

void PurePlayer::resizeFromCurrent(int amount)
{
    if( amount == 0 ) return;

    QSize videoSize = videoSize100Percent();

    double c = hypot(videoSize.width(), videoSize.height());
    int percentage = 100 * amount / c;

    resizePercentageFromCurrent(percentage);
}

void PurePlayer::fullScreenOrWindow()
{
    if( isFullScreen() ) {
        if( _controlFlags.testFlag(FLG_MAXIMIZED_BEFORE_FULLSCREEN) )
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
        if( isMaximized() )
            _controlFlags |= FLG_MAXIMIZED_BEFORE_FULLSCREEN;
        else
            _controlFlags &= ~FLG_MAXIMIZED_BEFORE_FULLSCREEN;

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
        _controlFlags |= FLG_DISABLE_MOUSEWINDOWMOVE;
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
        connect(_videoAdjustDialog, SIGNAL(windowActivate()), this, SLOT(videoAdjustDialog_windowActivate()));
        connect(_videoAdjustDialog, SIGNAL(requestSave()), this, SLOT(updateVideoProfile()));
        connect(_videoAdjustDialog, SIGNAL(requestLoad()), this, SLOT(restoreVideoProfile()));
        connect(_videoAdjustDialog, SIGNAL(requestCreate(const QString&)), this, SLOT(createVideoProfileFromCurrent(const QString&)));
        connect(_videoAdjustDialog, SIGNAL(requestRemove()), this, SLOT(removeVideoProfile()));
        connect(_videoAdjustDialog, SIGNAL(requestDefault()), this, SLOT(saveVideoProfileToDefault()));
        connect(_videoAdjustDialog, SIGNAL(changedContrast(int)), this, SLOT(setContrast(int)));
        connect(_videoAdjustDialog, SIGNAL(changedBrightness(int)), this, SLOT(setBrightness(int)));
        connect(_videoAdjustDialog, SIGNAL(changedSaturation(int)), this, SLOT(setSaturation(int)));
        connect(_videoAdjustDialog, SIGNAL(changedHue(int)), this, SLOT(setHue(int)));
        connect(_videoAdjustDialog, SIGNAL(changedGamma(int)), this, SLOT(setGamma(int)));

        _videoAdjustDialog->setProfiles(VideoSettings::profileNames());
        _videoAdjustDialog->setDefaultProfile(VideoSettings::defaultProfileName());
        _videoAdjustDialog->setProfile(_videoProfile);

        connect(_videoAdjustDialog, SIGNAL(changedProfile(const QString&)), this, SLOT(setVideoProfile(const QString&)));
    }

    _videoAdjustDialog->move(_menuContext->x()
                                 + (_menuContext->width()-_videoAdjustDialog->width())/2,
                             _menuContext->y());

    _videoAdjustDialog->show();
}

void PurePlayer::showPlaylistDialog()
{
    playlistDialog()->show();
}

void PurePlayer::showConfigDialog()
{
    int addX = 0;
    if( _configDialog == NULL ) {
        _configDialog = new ConfigDialog(this);
        connect(_configDialog, SIGNAL(applied(bool)),
                this,          SLOT(configDialog_applied(bool)));

        addX = 170;
    }

    _configDialog->setData(ConfigData::data());
    _configDialog->move(_menuContext->x()
                            + (_menuContext->width()-_configDialog->frameSize().width())/2 - addX,
                        _menuContext->y());

//  LogDialog::debug(tr("%1 %2").arg(_configDialog->frameSize().width()).arg(_configDialog->size().width()));
    _configDialog->show();
}

void PurePlayer::showLogDialog()
{
    LogDialog::moveDialog(_menuContext->x()
                             + (_menuContext->width()-LogDialog::dialog()->frameSize().width())/2,
                          _menuContext->y());

    LogDialog::showDialog();
}

void PurePlayer::showAboutDialog()
{
    if( _aboutDialog == NULL ) {
        _aboutDialog = new AboutDialog(this);
    }

    _aboutDialog->move(_menuContext->x()
                            + (_menuContext->width()-_aboutDialog->frameSize().width())/2,
                       _menuContext->y());

    _aboutDialog->show();
}

void PurePlayer::updateChannelInfo()
{
    const QString debugPrefix = "PurePlayer::updateChannelInfo(): ";
    if( !isPeercastStream() ) return;
    if( _attemptPeercastType.isEmpty() ) {
        _replyChannelInfoPcVp   = NULL;
        _replyChannelInfoPcSt   = NULL;
        _replyChannelStatusPcSt = NULL;
        LogDialog::debug(debugPrefix + "unknown peercast", QColor(255,0,0));
        return;
    }

    if( _attemptPeercastType.at(0) == PCT_VP ) {
        QUrl url(QString("http://%1:%2/html/ja/relayinfo.html?id=%3").arg(_host).arg(_port).arg(_id));

        _replyChannelInfoPcVp = _nam->get(QNetworkRequest(url));

        _replyChannelInfoPcSt   = NULL;
        _replyChannelStatusPcSt = NULL;
    }
    else { // _attemptPeercastType.at(0) == PCT_ST
        QUrl url(QString("http://%1:%2/api/1").arg(_host).arg(_port));
        QString json("{\"jsonrpc\": \"2.0\", \"method\": \"%1\", \"params\": [\"" + _id + "\"], \"id\": 1}");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray data(json.arg("getChannelInfo").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        _replyChannelInfoPcSt = _nam->post(request, data);

        QByteArray data2(json.arg("getChannelStatus").toLatin1());
        request.setHeader(QNetworkRequest::ContentLengthHeader, data2.size());
        _replyChannelStatusPcSt = _nam->post(request, data2);

        _replyChannelInfoPcVp = NULL;
    }

    LogDialog::debug(debugPrefix + "end");
}

void PurePlayer::openContactUrl()
{
    if( !_contactUrl.isEmpty() && CommonLib::isHttpUrl(_contactUrl) ) {
        if( ConfigData::data()->useContactUrlPath ) {
            QString arg = ConfigData::data()->contactUrlArg;
            arg.replace("%{ContactUrl}", _contactUrl);
            bool b = QProcess::startDetached(QString("\"%1\" %2")
                        .arg(ConfigData::data()->contactUrlPath).arg(arg));

            if( !b )
                QDesktopServices::openUrl(_contactUrl);
        }
        else
            QDesktopServices::openUrl(_contactUrl);
    }
    else {
        QMessageBox::warning(this, tr("エラー"),
                tr("コンタクトURLが正しく無い為、開くのを中止しました。\n\n"
                   "コンタクトURL:\n%1").arg(_contactUrl));
    }
}

//bool PurePlayer::event(QEvent* e)
//{
//  LogDialog::debug(tr("%1").arg(e->type()));

/*
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
*/
//  return QMainWindow::event(e);
//}

bool PurePlayer::eventFilter(QObject* o, QEvent* e)
{
//  LogDialog::debug(QString("%1").arg(e->type()));
    if( o == _toolBar ) {
        if( e->type() == QEvent::ContextMenu )
            return true;
    }
    else
    if( o == _timeSlider ) {
        if( e->type() == QEvent::Wheel ) {
            QWheelEvent* ev = static_cast<QWheelEvent*>(e);
            if( ev->buttons() & Qt::RightButton ) {
                QApplication::sendEvent(_toolBar, ev);
                return true;
            }
        }
    }
    else
    if( o == _speedSpinBox ) {
        if( e->type() == QEvent::Wheel ) {
            QWheelEvent* ev = static_cast<QWheelEvent*>(e);
            if( ev->buttons() & Qt::RightButton ) {
                QApplication::sendEvent(_toolBar, ev);
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(o, e);
}

void PurePlayer::closeEvent(QCloseEvent* e)
{
    LogDialog::debug("PurePlayer::closeEvent(): start-");

    closeAllOtherDialog();
    saveInteractiveSettings();
    hide();
    stopInternal();
    if( isPeercastStream() && ConfigData::data()->disconnectChannel ) {
        if( _peercastType==PCT_UNKNOWN && _attemptPeercastType.size()!=0 ) { // peercast判別未確認なら
            Task::waitForFinished(
                    new GetPeercastTypeTask(_host, _port, &_peercastType, this));
        }

        new DisconnectChannelTask(_host, _port, _id, _peercastType, 5, this);
    }

    e->accept();

    LogDialog::debug("PurePlayer::closeEvent():-end");
}

void PurePlayer::resizeEvent(QResizeEvent* )
{
#ifndef QT_NO_DEBUG_OUTPUT
    QSize videoClientSize;
    if( isAlwaysShowStatusBar() ) videoClientSize = centralWidget()->size();
    else                          videoClientSize = size();

    LogDialog::debug(
            QString("PurePlayer::resizeEvent(): clientSize %1x%2, videoClientSize %3x%4")
                .arg(width()).arg(height())
                .arg(videoClientSize.width()).arg(videoClientSize.height()));
#endif

    updateVideoScreenGeometry();
    _mousePressPos.setX(INT_MIN);
    _mousePressPos.setY(INT_MIN);

    // サイズ情報の出力
    QString sizeInfo;
    if( _aspectRatio == RATIO_VIDEO )
        sizeInfo = QString("%1%").arg(_videoScreen->height()*100/_videoSize.height());

    sizeInfo += QString(" %1x%2").arg(_videoScreen->width()).arg(_videoScreen->height());
    _infoLabel->setText(sizeInfo, 1000);
}

void PurePlayer::keyPressEvent(QKeyEvent* e)
{
    switch( e->key() ) {
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
        seek(+3, true);
        break;
    case Qt::Key_Left:
        seek(-3, true);
        break;

    case Qt::Key_E:
        frameAdvance();
        break;

#ifndef QT_NO_DEBUG_OUTPUT
    case Qt::Key_B:
    {
        repeatAB();
        break;
    }
    case Qt::Key_Z:
    {
        break;
    }
    case Qt::Key_X:
    {
        break;
    }
    case Qt::Key_N:
    {
//      recordingStartStop();
        break;
    }
    case Qt::Key_V:
        if( e->modifiers() & Qt::AltModifier
         && e->modifiers() & Qt::ShiftModifier )
        {
            _debugFlag = !_debugFlag;
            LogDialog::debug(QString("blockCount: %1").arg(LogDialog::dialog()->blockCount()));
        }

        break;
#endif // QT_NO_DEBUG_OUTPUT

    default:
        QMainWindow::keyPressEvent(e);
    }
}

void PurePlayer::mousePressEvent(QMouseEvent* e)
{
// 注: mouseDoubleClickEvent()と併せて実装しないと、動作漏れが起きる

//  LogDialog::debug(QString().sprintf(
//                      "PurePlayer::mousePressEvent():pos %d %d global %d %d",
//                      e->pos().x(),e->pos().y(), e->globalPos().x(),e->globalPos().y()));

    if( e->button() == Qt::LeftButton ) {
        _mousePressLocalPos.setX(e->pos().x() + (geometry().x()-frameGeometry().x()));
        _mousePressLocalPos.setY(e->pos().y() + (geometry().y()-frameGeometry().y()));

        if( whetherMuteArea(e->y()) )
            _mousePressPos = e->globalPos();
        else
        if( isFullScreen() )
            setCursor(QCursor(Qt::BlankCursor));
    }
    else
    if( e->button() == Qt::MidButton )
        middleClickResize();
}

void PurePlayer::mouseReleaseEvent(QMouseEvent* e)
{
//  LogDialog::debug("mouse release");
    if( e->button() == Qt::LeftButton ) {
        _menuContext->move(x()+width()*0.2, y()+height()*0.2);

        if( whetherMuteArea(e->y()) ) {
            if( _mousePressPos == e->globalPos() )
                mute(!isMute());
        }

        _controlFlags &= ~FLG_DISABLE_MOUSEWINDOWMOVE;
    }
    else
    if( e->button() == Qt::RightButton ) {
        if( !_controlFlags.testFlag(FLG_WHEEL_RESIZED)
         && geometry().contains(e->globalPos()) )
        {
            _menuContext->popup(e->globalPos());
        }

        _controlFlags &= ~FLG_WHEEL_RESIZED;
    }
}

void PurePlayer::mouseDoubleClickEvent(QMouseEvent* e)
{
//  LogDialog::debug("mouse doubleclick");
    if( e->buttons() & Qt::LeftButton ) {
        if( !whetherMuteArea(e->y()) )
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
            if( !_controlFlags.testFlag(FLG_DISABLE_MOUSEWINDOWMOVE) )
                move(e->globalPos() - _mousePressLocalPos);
        }
    }
}

void PurePlayer::wheelEvent(QWheelEvent* e)
{
    if( e->buttons() & Qt::RightButton ) {
        if( e->delta() < 0 )
            resizeFromCurrent(-50);
        else
            resizeFromCurrent(+50);

        _controlFlags |= FLG_WHEEL_RESIZED;
    }
    else {
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
    }
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
    if( e->mimeData()->hasFormat("text/uri-list") ) {
        QList<QUrl> urls = e->mimeData()->urls();
        open(urls);

        // プレイリストファイル内にpeercast urlがあった場合、
        // 同じストリームが開かれる可能性がある為初期化。
        //_reconnectCount = 0;                  // 現状では必要無し
    }
    else
        QMainWindow::dropEvent(e);
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

        QSize normalVideoClientSize = calcPercentageVideoSize(100);
        if( normalVideoClientSize.width() < minimumSizeHint().width() )
            normalVideoClientSize.setWidth(minimumSizeHint().width());

        QSize halfVideoSize = calcPercentageVideoSize(50);

        if( videoClientSize.width()  > normalVideoClientSize.width()
         || videoClientSize.height() > normalVideoClientSize.height()

         || (_videoScreen->width() == halfVideoSize.width()
         && _videoScreen->height() == halfVideoSize.height()) )
        {
            resize100Percent();
        }
        else
        {
            resize50Percent();
        }
    }
}

void PurePlayer::closeAllOtherDialog()
{
    LogDialog::closeDialog();

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->close();

    if( _openDialog != NULL )
        _openDialog->close();

    if( _playlistDialog != NULL )
        _playlistDialog->close();

    if( _configDialog != NULL )
        _configDialog->close();

    if( _aboutDialog != NULL )
        _aboutDialog->close();
}

PlaylistDialog* PurePlayer::playlistDialog()
{
    if( _playlistDialog == NULL ) {
        _playlistDialog = new PlaylistDialog(_playlist, this);
        connect(_playlistDialog, SIGNAL(playStopCurrentTrack()), this, SLOT(playlist_playStopCurrentTrack()));
        connect(_playlistDialog, SIGNAL(playPrev()), this, SLOT(playPrev()));
        connect(_playlistDialog, SIGNAL(playNext()), this, SLOT(playNext()));

        QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");
        _playlistDialog->move(s.value("playlist_pos", QPoint(x()+width()/2, y()+height()/2)).toPoint());
        _playlistDialog->resize(s.value("playlist_size", _playlistDialog->size()).toSize());
    }

    return _playlistDialog;
}

void PurePlayer::mpProcess_finished()
{
    const QString debugPrefix = "PurePlayer::mpProcess_finished(): ";
    LogDialog::print(QString("[%1]PurePlayer: mplayer process finished")
                        .arg(QTime::currentTime().toString()), QColor(106,129,198));
    LogDialog::debug(debugPrefix + QString("state finished with %1.").arg(_state));

    if( isPeercastStream() )
    {
        _elapsedTime = _timeLabel->time();
        LogDialog::debug(debugPrefix + QString("elapsed time %1").arg(_elapsedTime));

        if( isStop() ) {
//          setStatus(STOP);
            if( ConfigData::data()->disconnectChannel
             && _controlFlags.testFlag(FLG_EXPLICITLY_STOPPED) )
            {
                new DisconnectChannelTask(_host, _port, _id, _peercastType, 15, this);
            }
        }
        else {
            _reconnectCount++;
            LogDialog::debug(debugPrefix + QString("reconnectCount %1").arg(_reconnectCount));

            if( _reconnectCount <= 3 ) {
                if( _channelStatus == CS_SEARCH ) {
                    LogDialog::debug(debugPrefix + "reconnectPurePlayer", QColor(255,0,0));
                    reconnectPurePlayer();
                }
                else {
                    LogDialog::debug(debugPrefix + "reconnect", QColor(255,0,0));
                    reconnect();
                }
            }
            else {
                setStatus(STOP);
                if( ConfigData::data()->disconnectChannel )
                    new DisconnectChannelTask(_host, _port, _id, _peercastType, 15, this);
            }
        }
    }
    else {
        if( !_controlFlags.testFlag(FLG_EOF) || !playNext() )
            setStatus(STOP);
    }

    LogDialog::debug(debugPrefix + "end");
}

void PurePlayer::mpProcess_error(QProcess::ProcessError error)
{
    if( error == QProcess::FailedToStart ) {
        setStatus(STOP);
        LogDialog::debug("PurePlayer::mpProcess_error(): mplayer not started.", QColor(255,0,0));
        QMessageBox::warning(this, tr("エラー"),
                tr("MPlayerを起動できませんでした。\n"
                   "起動にはMPlayerがインストールされており、\n"
                   "PATHが通っているか、起動設定が正しく指定されている必要があります。"));
    }
}

void PurePlayer::mpProcess_debugKilledCPid()
{
    _debugCount++;
    setWindowTitle(QString("%1 %2").arg(_chName).arg(_debugCount));
}

void PurePlayer::mpProcess_outputLine(const QString& line)
{
    const QString debugPrefix = "PurePlayer::mpProcess_outputLine(): ";
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

    if( isStop() )
    {
        if( rxStatus.indexIn(line) != -1 )
            return;

        LogDialog::print(line);
    }
    else
    {
        if( rxStatus.indexIn(line) != -1 )
        {
            if( _startTime == -1 ) {
                _startTime = rxStatus.cap(1).toDouble();
                _oldTime = _startTime;
                LogDialog::debug(debugPrefix + QString("init startTime %1").arg(_startTime));
            }

            _currentTime = rxStatus.cap(1).toDouble();

            if( _currentTime > _startTime ) {
                if( isPeercastStream() ) {
                    double differenceTime = _currentTime - _oldTime;

                    // 現在の取得時間が前の取得時間から大きく飛んだ場合、経過時間を無効にする
                    // (再生開始時の古いキャッシュ再生による開始時間ズレの対応)
                    if( differenceTime > 1 ) {
                        LogDialog::debug(debugPrefix + QString("oldElapsed %1")
                                            .arg(_elapsedTime), QColor(255,0,0));

                        _elapsedTime += _oldTime - _startTime;
                        _startTime = _currentTime;

                        LogDialog::debug(debugPrefix + QString("elapsed %1").arg(_elapsedTime), QColor(255,0,0));
                        LogDialog::debug(debugPrefix + QString("startTime %1").arg(_startTime), QColor(255,0,0));
                        LogDialog::debug(debugPrefix + QString("diff %1 old %2").arg(differenceTime).arg(_oldTime), QColor(255,0,0));
                    }

                    // 差分時間が特に大きい場合、再接続する
                    // (差分時間が大きいとフレームがしばらく停止してしまう為)
                    if( differenceTime > 10 ) {
                        LogDialog::debug(debugPrefix + "reconnect diff", QColor(255,0,0));

                        reconnectPurePlayer();
                        //reconnect();
                    }
                }
                else {
                    // 2点間リピート処理
                    if( _repeatStartTime>=0 && _repeatEndTime>=0 ) {
                        if( (_currentTime >= _repeatEndTime/10.0
                            && !_controlFlags.testFlag(FLG_SEEKED_REPEAT))
                         || _currentTime >= _repeatEndTime/10.0 + 1 )
                        {
                            seek(_repeatStartTime/10.0);

                            _controlFlags |= FLG_SEEKED_REPEAT;

                            LogDialog::debug(debugPrefix + QString("old %1 current %2 end %3")
                                .arg(_oldTime).arg(_currentTime).arg(_repeatEndTime/10.0));
                        }
                        else
                        if( _currentTime < _repeatEndTime/10.0
                         && _controlFlags.testFlag(FLG_SEEKED_REPEAT) )
                        {
                            _controlFlags &= ~FLG_SEEKED_REPEAT;
                        }
                    }
                }

                _oldTime = _currentTime;

                double time = _currentTime - _startTime;
                _timeLabel->setTime(time + _elapsedTime);

                if( _isSeekable ) {
//                  if( time != _oldTime )
                    _timeSlider->setPosition(time);

//                  _oldTime = time;
                }
            }

            if( rxFrame.indexIn(line) != -1 ) {
                _labelFrame->setText(rxFrame.cap(1));

                uint currentFrame = rxFrame.cap(1).toInt();
                if( currentFrame==0 || currentFrame!=_oldFrame ) {
                    _fpsCount++;
                    _oldFrame = currentFrame;
                }
            }

            if( _state == PAUSE ) // ポーズが解除された場合
                setStatus(PLAY);

//          if( time < 0 ) _debugFlag = true;
            if( _debugFlag ) LogDialog::print(line + QString::number(_fpsCount));
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
//          LogDialog::debug(QString().sprintf("%.70f", _speedSpinBox->value()));
//          LogDialog::debug(QString().sprintf("%.70f", roundSpeed));

            _controlFlags |= FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 0");

            setVolume(_volume);
            setContrast(_videoProfile.contrast, true);
            setBrightness(_videoProfile.brightness, true);
            setSaturation(_videoProfile.saturation, true);
            setHue(_videoProfile.hue, true);
            setGamma(_videoProfile.gamma, true);

            _controlFlags &= ~FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 1");

            // mplayerプロセスの子プロセスIDを取得する
            _mpProcess->receiveMplayerChildProcess();

            _timeLabel->setTotalTime(_videoLength);
            _timeSlider->setLength(_videoLength);
            _playlist->setCurrentTrackTime(_videoLength);
            //_speedSpinBox->setRange(0, _videoLength*10);

            if( _controlFlags.testFlag(FLG_OPENED_PATH) )
            {
                _labelFrame->setText("0");
                _labelFps->setText("0fps");

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

                _startTime = -1; // open()のタイミングでの初期化では、
                                 // 前の再生のステータスラインを拾ってしまう為ここで初期化。
                _elapsedTime = 0;

                _controlFlags &= ~FLG_OPENED_PATH;
            }

            if( isPeercastStream() ) {
                _startTime = -1;

                // _reconnectControlTimeの比較を、再生開始時の開始時間から比較できる様に更新する
                _reconnectControlTime = _elapsedTime;

                if( _channelStatus == CS_SEARCH )
                    updateChannelInfo();
            }

            // ビデオサイズの設定
            if( _noVideo )
                _videoSize = QSize(320, 240);
            else {
                _videoSize.setWidth(rxVideoWH.cap(1).toInt());
                if( _videoSize.width() <= 0 ) _videoSize.setWidth(320);

                _videoSize.setHeight(rxVideoWH.cap(2).toInt());
                if( _videoSize.height() <= 0 ) _videoSize.setHeight(240);
            }

            // ウィンドウリサイズ
            if( _controlFlags.testFlag(FLG_RESIZE_WHEN_PLAYED) ) {
                QSize size;
                if( ConfigData::data()->openIn320x240Size )
                    size = QSize(320, 240);
                else
                    size = QSize(_videoSize.width()/2, _videoSize.height()/2);

                if( !resizeFromVideoClient(size) )
                    updateVideoScreenGeometry();

                _controlFlags &= ~FLG_RESIZE_WHEN_PLAYED;
            }
            else
                updateVideoScreenGeometry();

            LogDialog::debug(debugPrefix + QString("videosize %1x%2").arg(_videoSize.width()).arg(_videoSize.height()));
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
            if( _controlFlags.testFlag(FLG_RECONNECT_WHEN_PLAYED) ) { // 再接続(peercast)は接続後行う
                reconnectPeercast();
                _controlFlags &= ~FLG_RECONNECT_WHEN_PLAYED;
            }

            updateChannelInfo();
        }
        else
        if( rxCacheFill.indexIn(line) != -1 )
            _infoLabel->setText(QString("Cache: %1%").arg(rxCacheFill.cap(1), 5));
        else
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
    /*          LogDialog::debug(debugPrefix + QString("time %1").arg(_timeLabel->time()));
                LogDialog::debug(debugPrefix + QString("timea %1").arg(_currentTime));
                LogDialog::debug(debugPrefix + QString("frame %1").arg(_currentFrame));
    */
                _receivedErrorCount++;
            }
        }
    }

//  if( line.startsWith("ID_EXIT=QUIT") )
//      ;
//  else
    if( line.startsWith("ID_EXIT=EOF") )
        _controlFlags |= FLG_EOF;
    else
    if( rxScreenshot.indexIn(line) != -1 ) {
        QString file = rxScreenshot.cap(1);
        QString newName = genDateTimeSaveFileName(QFileInfo(file).suffix());

        new RenameFileTask(file, newName, this);

        _infoLabel->setText(tr("保存: スクリーンショット"), 3000);
    }
    else
    if( rxScreenshotError.indexIn(line) != -1 )
        _infoLabel->setText(tr("エラー: スクリーンショット保存"), 3000);
}

void PurePlayer::recProcess_finished()
{
    LogDialog::debug("PurePlayer::recProcess_finished():");
}

void PurePlayer::recProcess_outputLine(const QString& line)
{
    LogDialog::print("PurePlayer::recProcess_outputLine(): " + line);
}

bool PurePlayer::updateChannelInfoPcVp(const QString& reply)
{
    const QString debugPrefix = "PurePlayer::updateChannelInfoPcVp(): ";
    LogDialog::debug(debugPrefix + "called");

    int start, end;

    // チャンネル名の取得
    start = reply.indexOf("<td>チャンネル名");
    if( start == -1 )
        return false;

    start = reply.indexOf("\">", start);
    start += 2;
    end = reply.indexOf('<', start);

    _chName = reply.mid(start, end - start);
    QTextDocument txt;
    txt.setHtml(_chName);
    _chName = txt.toPlainText();
    LogDialog::debug(debugPrefix + "name " + _chName);

    // コンタクトURLの取得
    start = reply.indexOf("<td>URL", start);
    if( start == -1 ) {
        reflectChannelInfo();
        return false;
    }
    start = reply.indexOf("\">", start);
    start += 2;
    end = reply.indexOf('<', start);

    _contactUrl = reply.mid(start, end - start);
    LogDialog::debug(debugPrefix + "url " + _contactUrl);
/*
    // 接続先IPアドレスの取得
    start = reply.indexOf("<td>取得元", start);
    if( start == -1 ) {
        reflectChannelInfo();
        return false;
    }
    start = reply.indexOf("<br>", start);
    start += 4;
    start = reply.indexOf(QRegExp("\\S"), start);
    end = reply.indexOf('<', start);

    QString temp = reply.mid(start, end - start);
    QString rootIp = temp.left(temp.indexOf(':'));

    LogDialog::debug(debugPrefix + rootIp + ' ' + _rootIp);

#ifndef QT_NO_DEBUG_OUTPUT
    if( rootIp == _rootIp )
        _chName = "[" + _chName + "]";
#endif
*/
    // 接続状態の取得
    start = reply.indexOf("<td>状態", start);
    if( start == -1 ) {
        reflectChannelInfo();
        return false;
    }
    start++;
    start = reply.indexOf("<td>", start);
    start += 4;
    end = reply.indexOf('<', start);

    QString status = reply.mid(start, end - start);
    if( status == "RECEIVE" )
        _channelStatus = CS_RECEIVE;
    else
    if( status == "SEARCH" )
        _channelStatus = CS_SEARCH;
    else
    if( status == "CONNECT" )
        _channelStatus = CS_CONNECT;
    else
    if( status == "ERROR" )
        _channelStatus = CS_ERROR;
    else
        _channelStatus = CS_UNKNOWN;

//  _searchingConnection = (rootIp == "0.0.0.0");

    LogDialog::debug(debugPrefix + "status " + status);

    reflectChannelInfo();
    return true;
}

bool PurePlayer::updateChannelInfoPcSt(const QString& reply)
{
    const QString debugPrefix = "PurePlayer::updateChannelInfoPcSt(): ";
    LogDialog::debug(debugPrefix + "called");

    QScriptEngine engine;
    QScriptValue  value;
//  value = engine.evaluate("(" + reply + ")");
    value = engine.evaluate("JSON.parse").call(QScriptValue(), QScriptValueList() << reply);

    if( value.isError() ) {
        LogDialog::debug(debugPrefix + value.toString(), QColor(255,0,0));
        return false;
    }

    value = value.property("result").property("info");
    if( !value.isValid() )
        return false;

    _chName     = value.property("name").toString();
    _contactUrl = value.property("url").toString();

    reflectChannelInfo();

    LogDialog::debug(debugPrefix + "name " + _chName);
    LogDialog::debug(debugPrefix + "url " + _contactUrl);

    return true;
}

bool PurePlayer::updateChannelStatusPcSt(const QString& reply)
{
    const QString debugPrefix = "PurePlayer::updateChannelStatusPcSt(): ";
    LogDialog::debug(debugPrefix + "called");

    QScriptEngine engine;
    QScriptValue  value;
//  value = engine.evaluate("(" + reply + ")");
    value = engine.evaluate("JSON.parse").call(QScriptValue(), QScriptValueList() << reply);

    if( value.isError() ) {
        LogDialog::debug(debugPrefix + value.toString(), QColor(255,0,0));
        return false;
    }

    value = value.property("result");
    if( !value.isValid() )
        return false;

    QString status = value.property("status").toString();
    if( status == "Receiving" )
        _channelStatus = CS_RECEIVE;
    else
    if( status == "Searching" )
        _channelStatus = CS_SEARCH;
    else
    if( status == "Connecting" )
        _channelStatus = CS_CONNECT;
    else
    if( status == "Error" )
        _channelStatus = CS_ERROR;
    else
        _channelStatus = CS_UNKNOWN;

    LogDialog::debug(debugPrefix + "status " + status);

    return true;
}

void PurePlayer::reflectChannelInfo()
{
    if( _contactUrl.isEmpty() )
        _actOpenContactUrl->setEnabled(false);
    else
        _actOpenContactUrl->setEnabled(true);

    QString title;
    if( _chName.isEmpty() ) {
        title = "PurePlayer*";
    }
    else {
        title = _chName;
        if( isPeercastStream() )
            _playlist->setCurrentTrackTitle(_chName);
    }

    if( _debugCount )
        setWindowTitle(title + QString(" %1").arg(_debugCount));
    else
        setWindowTitle(title);

    LogDialog::dialog()->setWindowTitle(title + " - PureLog");
}

QString PurePlayer::genDateTimeSaveFileName(const QString& suffix)
{
    QString title;
    if( _chName.isEmpty() || (!isPeercastStream() && !QFile::exists(_path)) )
        title = "snap";
    else
        title = _chName;

    QDateTime dateTime = QDateTime::currentDateTime();
    QString name = QString("%1_%2")
                        .arg(title)
                        .arg(dateTime.toString("yyyyMMdd_")
                            + CommonLib::dayOfWeek(dateTime.date().dayOfWeek()-1)
                            + dateTime.toString("_hhmm_ss"));

    if( !suffix.isEmpty() )
        name += '.' + suffix;

    name = CommonLib::convertStringForFileName(name);
    return name;
}

void PurePlayer::nam_finished(QNetworkReply* reply)
{
    const QString debugPrefix = "PurePlayer::nam_finished(): ";
    if( reply->error() != QNetworkReply::NoError )
        LogDialog::debug(debugPrefix + "error " + QString::number(reply->error()), QColor(255,0,0));

    if( reply == _replyChannelInfoPcVp ) {
        _replyChannelInfoPcVp = NULL;

        bool error = !(reply->error() == QNetworkReply::NoError);
        if( !error ) {
            QString out = reply->readAll();
            error = !updateChannelInfoPcVp(out);
        }

        if( error ) {
            Q_ASSERT( _attemptPeercastType.size() > 0 );
            Q_ASSERT( _attemptPeercastType.at(0) == PCT_VP );
            _attemptPeercastType.pop_front();
            if( _attemptPeercastType.size() == 0 )
                _peercastType = PCT_UNKNOWN;

            updateChannelInfo();
        }
        else
            _peercastType = PCT_VP;
    }
    else
    if( reply == _replyChannelInfoPcSt ) {
        _replyChannelInfoPcSt = NULL;

        bool error = !(reply->error() == QNetworkReply::NoError);
        if( !error ) {
            QString out = reply->readAll();
            error = !updateChannelInfoPcSt(out);
        }

        if( error ) {
            Q_ASSERT( _attemptPeercastType.size() > 0 );
            Q_ASSERT( _attemptPeercastType.at(0) == PCT_ST );
            _attemptPeercastType.pop_front();
            if( _attemptPeercastType.size() == 0 )
                _peercastType = PCT_UNKNOWN;

            updateChannelInfo();
        }
        else
            _peercastType = PCT_ST;
    }
    else
    if( reply == _replyChannelStatusPcSt ) {
        _replyChannelStatusPcSt = NULL;

        bool error = !(reply->error() == QNetworkReply::NoError);
        if( !error ) {
            QString out = reply->readAll();
            error = !updateChannelStatusPcSt(out);
            if( !error ) {
                if( _channelStatus == CS_ERROR )
                    stopPeercast();
            }
        }
    }

    LogDialog::debug(QString("%1end. %2, %3")
        .arg(debugPrefix).arg(reply->url().toString()).arg(QString().sprintf("%p", reply)));

    reply->deleteLater();
}

void PurePlayer::actGroupAudioOutput_changed(QAction* action)
{
    int index = _actGroupAudioOutput->actions().indexOf(action);
    setAudioOutput((AUDIO_OUTPUT_MODE)index);
}

void PurePlayer::actGroupVolumeFactor_changed(QAction* action)
{
    int index = _actGroupVolumeFactor->actions().indexOf(action);
    setVolumeFactor((VOLUME_FACTOR_MODE)index);
}

void PurePlayer::actGroupAspect_changed(QAction* action)
{
    int index = _actGroupAspect->actions().indexOf(action);
    setAspectRatio((ASPECT_RATIO)index);
}

void PurePlayer::actGroupDeinterlace_changed(QAction* action)
{
    int index = _actGroupDeinterlace->actions().indexOf(action);
    setDeinterlace((DEINTERLACE_MODE)index);
}

void PurePlayer::timerReconnect_timeout()
{
    const QString debugPrefix = "PurePlayer::timerReconnect_timeout(): ";
//  LogDialog::debug(QString(debugPrefix + "time %1").arg(_timeLabel->time()));
//  LogDialog::debug(QString(debugPrefix + "currentTime %1").arg(_currentTime));
//  LogDialog::debug(QString(debugPrefix + "frame %1").arg(_currentFrame));

//  LogDialog::debug(debugPrefix + QString::number(_receivedErrorCount));

    if( _reconnectControlTime == _timeLabel->time() ) {
        LogDialog::debug(QString(debugPrefix + "reconnect time %1")
                .arg(_reconnectControlTime), QColor(255,0,0));

        if( _channelStatus == CS_SEARCH )
            reconnectPurePlayer();
        else
            reconnect();
    }
    else
    if( _receivedErrorCount > 10 ) {
        LogDialog::debug(QString(debugPrefix + "reconnect count %1")
                .arg(_receivedErrorCount), QColor(255,0,0));

        reconnect();
    }
    else
    if( _channelStatus == CS_SEARCH )
        updateChannelInfo();
    else
    if( _receivedErrorCount == 0 )
        _reconnectCount = 0;

    _reconnectControlTime = _timeLabel->time();

    _receivedErrorCount = 0;

//  _debugFlag = !_debugFlag;
}

void PurePlayer::timerFps_timeout()
{
/*  static QTime time;
    LogDialog::debug(QString("PurePlayer::timerFps_timeout(): elapsed %1 fps %2")
                                                   .arg(time.elapsed()).arg(_fpsCount));
    time.restart();
*/
    _labelFps->setText(QString("%1fps").arg(_fpsCount));
    _fpsCount = 0;
}

void PurePlayer::configDialog_applied(bool restartMplayer)
{
    ConfigData::saveData();

    if( restartMplayer ) {
//      _actScreenshot->setVisible(ConfigData::data()->screenshot);
//      _screenshotButton->setVisible(ConfigData::data()->screenshot);

        setCurrentDirectory();

        if( !isStop() )
            restartPlay(true);
    }

    LogDialog::debug("PurePlayer::configDialog_applied(): end");
}

void PurePlayer::mpCmd(const QString& command)
{
    _mpProcess->command(command);
    LogDialog::debug("PurePlayer::mpCmd(): " + command);
}

void PurePlayer::stopInternal()
{
    const QString debugPrefix = "PurePlayer::stopInternal(): ";
    LogDialog::debug(debugPrefix + "start-");

//  if( _mpProcess->state() != QProcess::Running )
        setStatus(STOP);

    _mpProcess->terminateWaitForFinished();

    LogDialog::debug(debugPrefix + "-end");
}

void PurePlayer::playlist_playStopCurrentTrack()
{
    if( _playlist->isCurrentTrack(_path) && isPlaying() )
            stop();
    else {
        _controlFlags &= ~FLG_RESIZE_WHEN_PLAYED;
        _controlFlags |= FLG_EXPLICITLY_STOPPED;
        _reconnectCount=0;
        restartPlay();
    }
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

    if( _attemptPeercastType.isEmpty() || _attemptPeercastType.at(0) == PCT_VP ) {
        _attemptPeercastType.clear();
        _attemptPeercastType.push_back(PCT_VP);
        _attemptPeercastType.push_back(PCT_ST);
    }
    else {
        _attemptPeercastType.clear();
        _attemptPeercastType.push_back(PCT_ST);
        _attemptPeercastType.push_back(PCT_VP);
    }
    _replyChannelInfoPcVp   = NULL;
    _replyChannelInfoPcSt   = NULL;
    _replyChannelStatusPcSt = NULL;
    _channelStatus = CS_UNKNOWN;

    _noVideo = false;
    _controlFlags &= ~FLG_EOF;
    _controlFlags &= ~FLG_EXPLICITLY_STOPPED;
    _controlFlags &= ~FLG_RECONNECTED;
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
    if( _deinterlace == DI_YADIF_DOUBLE )
        args << "-vf-add" << "yadif=1";
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
    case VF_ONE_THIRD: args << "36.7"; break;
    case VF_DOUBLE   : args << "220"; break;
    case VF_TRIPLE   : args << "330"; break;
    case VF_NORMAL   :
    default          : args << "110";
    }

    if( _path.left(8).contains("://") )
        args << "-cache" << QString::number(ConfigData::data()->cacheStreamSize);
    else
        args << "-nocache";

    if( _controlFlags.testFlag(FLG_SEEK_WHEN_PLAYED) ) {
        args << "-ss" << QString::number(_timeLabel->time());
        _controlFlags &= ~FLG_SEEK_WHEN_PLAYED;
    }

    args
    << "-noquiet"
    << "-identify"
    << "-af-add" << "scaletempo"
    << "-osdlevel" << "0"
    << "-ass"   // windowsでは'-vf-add eq2,hue'単体では多重再生不可。このオプションで改善
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
    << "-wid" << QString::number((uint)_videoScreen->winId())

#ifdef Q_WS_X11
    << "-stop-xscreensaver"
#endif

#ifdef Q_OS_WIN32
    << "-colorkey" << QString().sprintf("%#08x",
                        qRgba(qBlue(_colorKey), qGreen(_colorKey), qRed(_colorKey), 0))
#endif

    << "-idx";

    if( isPeercastStream() )
        args << QString(_path).replace("/pls/", "/stream/");
    else
        args << _path;

    QString mplayerPath;
    if( ConfigData::data()->useMplayerPath )
        mplayerPath = ConfigData::data()->mplayerPath;
    else
        mplayerPath = "mplayer";

    LogDialog::print(QString("[%1]PurePlayer: mplayer process start -------------")
                        .arg(QTime::currentTime().toString()), QColor(106,129,198));
    LogDialog::print(QString("PurePlayer: %1 %2").arg(mplayerPath).arg(args.join(" ")));

    _mpProcess->start(mplayerPath, args, QIODevice::ReadWrite);
    _mpProcess->waitForStarted();
}

void PurePlayer::saveVideoProfileToDefault()
{
    VideoSettings::saveDefaultProfile(_videoProfile.name);

    if( _videoAdjustDialog != NULL )
        _videoAdjustDialog->setDefaultProfile(_videoProfile.name);

    LogDialog::debug("PurePlayer::saveDefaultVideoProfile(): end");
}

void PurePlayer::setVideoProfile(const QString& profileName, bool setVideoValue)
{
    LogDialog::debug(QString("PurePlayer::setVideoProfile(): %1").arg(profileName));
    VideoSettings::VideoProfile profile = VideoSettings::profile(profileName);

    if( profile.isValid() ) {
        _videoProfile.name = profile.name;

        if( setVideoValue ) {
            _controlFlags |= FLG_HIDE_DISPLAY_MESSAGE;  //mpCmd("osd 0");
            setContrast(profile.contrast);
            setBrightness(profile.brightness);
            setSaturation(profile.saturation);
            setHue(profile.hue);
            setGamma(profile.gamma);
            _controlFlags &= ~FLG_HIDE_DISPLAY_MESSAGE; //mpCmd("osd 1");
        }

        if( _videoAdjustDialog != NULL )
            _videoAdjustDialog->setProfile(_videoProfile.name);

        LogDialog::debug("PurePlayer::setVideoProfile(): end");
        return;
    }

    LogDialog::debug("PurePlayer::setVideoProfile(): failure");
}

void PurePlayer::createVideoProfileFromCurrent(QString profileName)
{
    VideoSettings::VideoProfile profile = _videoProfile;

    profileName = CommonLib::removeSpaceBeforeAfter(profileName);
    profile.name = profileName;

    if( VideoSettings::appendProfile(profile) ) {
        refreshVideoProfile(false);
        setVideoProfile(profile.name, false);
    }

    LogDialog::debug("PurePlayer::createVideoProfileFromCurrent(): end");
}

void PurePlayer::updateVideoProfile()
{
    VideoSettings::updateProfile(_videoProfile);

    LogDialog::debug("PurePlayer::updateVideoProfile(): end");
}

void PurePlayer::restoreVideoProfile()
{
    setVideoProfile(_videoProfile.name);

    LogDialog::debug("PurePlayer::restoreVideoProfile(): end");
}

void PurePlayer::removeVideoProfile()
{
    VideoSettings::removeProfile(_videoProfile.name);
    refreshVideoProfile();

    LogDialog::debug("PurePlayer::removeVideoProfile(): end");
}

void PurePlayer::refreshVideoProfile(bool restoreVideoValue, bool warning)
{
    if( !warning )
    {
        VideoSettings::loadProfiles();

        if( _videoSettingsModifiedId != VideoSettings::modifiedId() )
        {
            if( _videoAdjustDialog != NULL ) {
                _videoAdjustDialog->setProfiles(VideoSettings::profileNames());
                _videoAdjustDialog->setDefaultProfile(VideoSettings::defaultProfileName());

                if( !VideoSettings::profile(_videoProfile.name).isValid() )
                    setVideoProfile(VideoSettings::defaultProfileName(), restoreVideoValue);
                else
                    setVideoProfile(_videoProfile.name, false);
            }
            else
            if( !VideoSettings::profile(_videoProfile.name).isValid() )
                setVideoProfile(VideoSettings::defaultProfileName(), restoreVideoValue);

            _videoSettingsModifiedId = VideoSettings::modifiedId();
//          LogDialog::debug("PurePlayer::refreshVideoProfile(): refresh");
        }
    }
    else
    {
        QString profileName = _videoProfile.name;

        refreshVideoProfile(restoreVideoValue, false);

        if( !VideoSettings::profile(profileName).isValid() ) {
            QWidget* parent = _videoAdjustDialog;
            if( parent == NULL )
                parent = this;

            QMessageBox::warning(parent, "プロファイルは削除されました",
                QString("現在選択していたプロファイル「%1」は外部から削除されました。\n"
                        "デフォルトのプロファイルを選択します。").arg(profileName));
        }
    }

//  LogDialog::debug("PurePlayer::refreshVideoProfile(): end");
}

void PurePlayer::saveInteractiveSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    int volume;
    if( _volume > ConfigData::data()->volumeMax )
        volume = ConfigData::data()->volumeMax;
    else
        volume = _volume;

    s.setValue("volume",    volume);
    s.setValue("mute",      _isMute);
    s.setValue("statusbar", _alwaysShowStatusBar);
    s.setValue("pos",       pos());

    if( _playlistDialog != NULL ) {
        s.setValue("playlist_pos", _playlistDialog->pos());
        s.setValue("playlist_size", _playlistDialog->size());
    }

    LogDialog::debug("PurePlayer::saveInteractiveSettings(): end");
}

void PurePlayer::loadInteractiveSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    mute(s.value("mute", false).toBool());
    setVolume(s.value("volume", 30).toInt());
    setAlwaysShowStatusBar(s.value("statusbar", true).toBool());
    move(s.value("pos", QPoint(0,0)).toPoint());

    LogDialog::debug("PurePlayer::loadInteractiveSettings(): end");
}

// アスペクト比設定を考慮に入れた等倍サイズとして解釈するビデオサイズを返す
QSize PurePlayer::videoSize100Percent()
{
    QSize videoSize;
    if( _aspectRatio == RATIO_VIDEO )
        videoSize = _videoSize;
    else {
        QSize aspect;
        switch( _aspectRatio ) {
        case RATIO_4_3:
            aspect.setWidth(4);
            aspect.setHeight(3);
            break;
        case RATIO_16_9:
            aspect.setWidth(16);
            aspect.setHeight(9);
            break;
        case RATIO_16_10:
            aspect.setWidth(16);
            aspect.setHeight(10);
            break;
        case NO_KEEP:
        default:
            aspect = _videoScreen->size();
        }

        QRect rc;
        rc = CommonLib::scaleRectOnRect(_videoSize, aspect);
        videoSize = rc.size();
    }

    return videoSize;
}

// toSizeを有効な値に修正したサイズを返す
QSize PurePlayer::correctToValidVideoSize(QSize toSize, const QSize& videoSize)
{
    if( videoSize.width() <= 0 || videoSize.height() <= 0 )
        return QSize(0, 0);

    // 最大最小サイズを超える場合は、有効なサイズへ修正する
    // (比を維持しない設定の場合は、超えたサイズでリサイズすると比崩れが起きる為必須)
    // 最大サイズを求める
    QRect rc = QApplication::desktop()->availableGeometry();
    int maxW = rc.width()  - (frameGeometry().width()  - width());
    int maxH = rc.height() - (frameGeometry().height() - height());

    if( isAlwaysShowStatusBar() )
        maxH -= statusBar()->height();

    if( _isSeekable )
        maxH -=_toolBar->height();

    // 最小サイズを求める
    int minW = minimumSizeHint().width();

    // サイズを修正する
    if( toSize.width() > maxW || toSize.height() > maxH ) {
        QRect rc = CommonLib::scaleRectOnRect(QSize(maxW, maxH), videoSize);
        toSize = rc.size();
    }
    else
    if( toSize.width() < minW ) { // 注意: 最小値の修正は比を維持しない場合のみ行う
                     //       (比を維持している場合は小さいサイズでも表示可能な為)
        if( _aspectRatio == NO_KEEP ) {
            toSize.setWidth(minW);
            // セントラルwidgetのサイズがそのままビデオサイズになるので適当に四捨五入
            toSize.setHeight(minW * videoSize.height() / (double)videoSize.width() + 0.5);
        }
    }

//  LogDialog::debug(
//      QString("PurePlayer::correctToValidVideoSize(): %1 %2 maxWH:%3 %4 minW: %5")
//      .arg(toSize.width()).arg(toSize.height()).arg(maxW).arg(maxH).arg(minW));

    return toSize;
}

// videoSizeに対してスケーリングを計算し、有効なサイズを返す
QSize PurePlayer::calcPercentageVideoSize(const QSize& videoSize, const int percentage)
{
    if( percentage <= 0 ) return QSize(0, 0); // マイナス分は最後に-1を掛ければ
                                              // 対応できるが特に対応しない。

    // サイズを求める
    // サイズの%表示,ビデオ矩形の表示算出処理が高さを元に判定している為、高さ起点で求める
    int w, h;
    h = videoSize.height() * percentage / 100;
    if( videoSize.height() * percentage % 100 )
        h++;

    w = h * videoSize.width() / videoSize.height();
    if( h * videoSize.width() % videoSize.height() )
        w++;

//  LogDialog::debug(QString("PurePlayer::calcPercentageVideoSize(): %1 %2 %3%")
//                                                   .arg(w).arg(h).arg(percentage));

    return correctToValidVideoSize(QSize(w, h), videoSize);
}

// ビデオに対してスケーリングを計算し、有効なサイズを返す
QSize PurePlayer::calcPercentageVideoSize(const int percentage)
{
    // 等倍サイズとして解釈するビデオサイズを取得
    QSize videoSize = videoSize100Percent();

    // videoSizeをスケーリングしたサイズを取得
    QSize size = calcPercentageVideoSize(videoSize, percentage);

    return size;
}

#ifdef Q_OS_WIN32
void PurePlayer::initColorKey()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, CommonLib::QSETTINGS_ORGNAME, "PurePlayer");

    int step = s.value("WinColorKeyStep", 0).toInt();

    s.setValue("WinColorKeyStep", (step+1) % 50);

    _colorKey = genColorKey(step, 0x040506);

    LogDialog::debug(QString("PurePlayer::initColorKey(): %1 %2 %3 %4")
                    .arg(qRed(_colorKey)).arg(qGreen(_colorKey)).arg(qBlue(_colorKey))
                    .arg(step));
}

QRgb PurePlayer::genColorKey(int step, QRgb baseColorKey)
{
    step = step % 100;

    int r = qRed(baseColorKey);
    int g = qGreen(baseColorKey);
    int b = qBlue(baseColorKey);

    r = (r + step/3 + (step%3 + 2)/3) % 256;
    g = (g + step/3 + (step%3 + 1)/3) % 256;
    b = (b + step/3) % 256;

    return (r << 16) + (g << 8) + b;
}
#endif // Q_OS_WIN32

void PurePlayer::updateVideoScreenGeometry()
{
//  if( !isFullScreen() && _alwaysShowStatusBar && statusBar()->isHidden() )
//      statusBar()->show();

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
    QRect rect;
    if( _aspectRatio != NO_KEEP )
    {
        QSize aspect;

        switch( _aspectRatio ) {
        case RATIO_4_3:
            aspect.setWidth(4);
            aspect.setHeight(3);
            break;
        case RATIO_16_9:
            aspect.setWidth(16);
            aspect.setHeight(9);
            break;
        case RATIO_16_10:
            aspect.setWidth(16);
            aspect.setHeight(10);
            break;
        case RATIO_VIDEO:
        default:
            aspect.setWidth(_videoSize.width());
            aspect.setHeight(_videoSize.height());
        }

        rect = CommonLib::scaleRectOnRect(screen, aspect);
    }
    else
        rect.setRect(0,0, screen.width(),screen.height());

    _videoScreen->setGeometry(rect);

    LogDialog::debug(QString()
            .sprintf("PurePlayer::updateVideoScreenGeometry(): videorect(%d,%d,%dx%d)",
            rect.x(),rect.y(),rect.width(),rect.height()));
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
        if( QCursor::pos().y() >= (QApplication::desktop()->screen()->height()*80/100) )
//      if( QCursor::pos().y() >= (height()*80/100) )
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

bool PurePlayer::whetherMuteArea(int mouseLocalY)
{
    int threshold;
    if( isFullScreen() )
        threshold = height() * 80/100;
    else
        threshold = height() - statusBar()->height();

    return (mouseLocalY >= threshold);
}
/*
bool PurePlayer::whetherMuteArea(QPoint mousePos)
{
    QRect rc;
    if( isFullScreen() ) {
        QPoint pos = geometry().topLeft();
        int localY = height() * 80/100;
        rc.setRect(pos.x(), pos.y() + localY, width(), height() - localY);
    }
    else
        rc = QRect(mapToGlobal(statusBar()->pos()), statusBar()->size());

    return rc.contains(mousePos);
}
*/
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

        if( !_noVideo ) {
            _timerFps.start(1000);
            _fpsCount = 0;
            _oldFrame = 0;
        }

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
        _timeSlider->setSliderDown(false);
        _timeSlider->setPosition(0);
        visibleInterface(true);

        _actScreenshot->setEnabled(false);
        if( _playlist->rowCount() > 0 ) {
            _actReconnect->setEnabled(true);
            _actReconnectPlayer->setEnabled(true);
        }
        else {
            _actReconnect->setEnabled(false);
            _actReconnectPlayer->setEnabled(false);
        }
        _actReconnectPct->setEnabled(false);
        _actPlayPause->setText(tr("再生"));
        _actPlayPause->setEnabled(true);
        _actStop->setEnabled(false);

        _controlFlags &= ~FLG_RECONNECT_WHEN_PLAYED;

        _infoLabel->stopClipInfo();
        _receivedErrorCount = 0;

        _timerReconnect.stop();
        _timerFps.stop();
        _labelFps->setText("0fps");

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
/*
void PurePlayer::resize(const QSize& size)
{
    qDebug("%d,%d %d,%d", width(),height(), size.width(),size.height());
    if( geometry().x() < 0 || geometry().y() < 0 ) {
        const int threshold = 50;

        // リサイズするサイズ決定
        QSize s = size;
        if( s.width() < minimumSizeHint().width() )
            s.setWidth(minimumSizeHint().width());

        // 右xがしきい値より大きい場合で外に出る場合は、右xをしきい値としてリサイズ。
        // 右xがしきい値より小さい場合で外に出る場合は、右xは現在のままでリサイズ。
        // xの位置決定(クライアント位置)
        int x = geometry().x();
        qDebug("x: %d", x);
        if( (geometry().x() + s.width()-1) < 0 ) { // デスクトップ外へ完全に出たなら
            int rx = geometry().x() + width()-1;
            if( rx >= threshold )
                x = threshold - s.width();
            else
                x = rx - (s.width()-1);

            if( x > 0 )
                x = 0;
        }

        // yの位置決定(クライアント位置)
        int y = geometry().y();
        if( (geometry().y() + s.height()-1) < 0 ) { // デスクトップ外へ完全に出たなら
            int by = geometry().y() + height()-1;
            if( by >= threshold )
                y = threshold - s.height();
            else
                y = by - (s.height()-1);

            if( y > 0 )
                y = 0;
        }

        setGeometry(QRect(QPoint(x, y), s));
        qDebug("p: %d,%d s: %d,%d", x,y, width(), height());
    }
    else
        QMainWindow::resize(size);
}
*/

