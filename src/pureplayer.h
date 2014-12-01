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
#ifndef PUREPLAYER_H
#define PUREPLAYER_H

#include <QMainWindow>
#include <QSize>
#include <QPoint>
#include <QProcess>
#include <QTimer>
#include <QLabel>
#include "videosettings.h"
#include "configdata.h"
#include "peercast.h"

class QWidget;
class QActionGroup;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;

class MplayerProcess;
class RecordingProcess;
class ControlButton;
class TimeSlider;
class InfoLabel;
class TimeLabel;
class PlaylistModel;
class CommonMenu;
class OpenDialog;
class VideoAdjustDialog;
class ConfigDialog;
class PlaylistDialog;
class AboutDialog;
class ClipWindow;
class LogDialog;

class PurePlayer : public QMainWindow
{
    Q_OBJECT

public:
    enum ASPECT_RATIO { AR_VIDEO, AR_4_3, AR_16_9, AR_16_10, AR_NO_KEEP };
    enum DEINTERLACE_MODE { DI_NO_DEINTERLACE, DI_YADIF, DI_YADIF_DOUBLE, DI_LINEAR_BLEND };
    enum AUDIO_OUTPUT_MODE { AO_STEREO, AO_MONAURAL, AO_LEFT, AO_RIGHT };
    enum VOLUME_FACTOR_MODE { VF_ONE_THIRD, VF_NORMAL, VF_DOUBLE, VF_TRIPLE };

    PurePlayer(QWidget* parent = 0);
    virtual ~PurePlayer();
    bool isMute()    { return _isMute; }
    bool isPlaying() { return _state == ST_PLAY; }
    bool isStop()    { return _state == ST_STOP; }
    bool isClipping() { return _clipRect != QRect(0,0,_videoSize.width(),_videoSize.height()); }
    bool isAlwaysShowStatusBar() { return !isFullScreen()
                                        && (_alwaysShowStatusBar || !isPeercastStream()); }
    bool isPeercastStream() { return _peercast.port() != 0; }

//  void resize(const QSize&);
//  void resize(int w, int h) { resize(QSize(w, h)); }

public slots:
    void open(const QString& path, bool doResize=false) { open(QStringList() << path, doResize); }
    void open(const QStringList& paths, bool doResize=false);
    void open(const QList<QUrl>& urls, bool doResize=false);
    void openFromDialog();
    void play();
    bool playPrev(bool forceLoop=false);
    bool playNext(bool forceLoop=false);
    void stop() { _controlFlags |= FLG_EXPLICITLY_STOPPED; stopInternal(); }
    void stopPeercast();
    void pauseUnPause();
    void frameAdvance();
    void repeatAB();
    void seek(double sec, bool relative=false);
    void upSpeedRate(double value=0.1);
    void downSpeedRate(double value=0.1);
    void setSpeedRate(double value);
    void reconnect();
    void reconnectPurePlayer() { restartPlay(); }
    void reconnectPeercast();
    void setPlayNoSound(bool);
    void recordingStartStop();
    void mute(bool);
    void upVolume(int value=5);
    void downVolume(int value=5);
    void setVolume(int value);
    void setVolumeFactor(VOLUME_FACTOR_MODE);
    void setAudioOutput(AUDIO_OUTPUT_MODE);
    void setAspectRatio(ASPECT_RATIO);
    QSize aspectRatioSize();
    void setContrast(int value, bool alwaysSet=false);
    void setBrightness(int value, bool alwaysSet=false);
    void setSaturation(int value, bool alwaysSet=false);
    void setHue(int value, bool alwaysSet=false);
    void setGamma(int value, bool alwaysSet=false);
    void setDeinterlace(DEINTERLACE_MODE);
    void clipVideoViewArea(const QRect&);
    void releaseClipping();
    void screenshot();
    void resizeReduce()     { resizeFromCurrent(-300); }
    void resizeIncrease()   { resizeFromCurrent(+300); }
    void resize320x240()    { resizeFromVideoClient(QSize(320,240)); }
    void resize1280x720()   { resizeFromVideoClient(QSize(1280,720)); }
    void resize25Percent()  { resizeFromVideoClient(calcVideoViewSizeForResize(25)); }
    void resize50Percent()  { resizeFromVideoClient(calcVideoViewSizeForResize(50)); }
    void resize75Percent()  { resizeFromVideoClient(calcVideoViewSizeForResize(75)); }
    void resize100Percent() { resizeFromVideoClient(calcVideoViewSizeForResize(100)); }
    void resize125Percent() { resizeFromVideoClient(calcVideoViewSizeForResize(125)); }
    void resize150Percent() { resizeFromVideoClient(calcVideoViewSizeForResize(150)); }
    bool resizeFromVideoClient(QSize size);
    void resizePercentFromCurrent(int percent);
    void resizeFromCurrent(int amount);
    void toggleFullScreenOrWindow();
    void setAlwaysShowStatusBar(bool);

    void saveVideoProfileToDefault();
    void setVideoProfile(const QString& profileName, bool setVideoValue=true);
    void createVideoProfileFromCurrent(QString profileName);
    void updateVideoProfile();
    void restoreVideoProfile();
    void removeVideoProfile();

    void showVideoAdjustDialog();
    void showPlaylistDialog();
    void showConfigDialog();
    void showLogDialog();
    void showAboutDialog();
    void showClipWindow();

    void updateChannelInfo() { _peercast.getChannelInfo(); }
    void openContactUrl();

protected slots:
    void mpCmd(const QString& command);
    void stopInternal();
    void reconnectFromGui() { _reconnectCount=0; reconnect(); }
    void reconnectPurePlayerFromGui() { _reconnectCount=0; reconnectPurePlayer(); }
    void playlist_playStopCurrentTrack();
    void buttonPlayPauseClicked();
    void nextButton_clicked() { playNext(true); }
    void prevButton_clicked() { playPrev(true); }
    void exitFullScreen() { if( isFullScreen() ) toggleFullScreenOrWindow(); }
    void restartPlay(bool keepSeekPos=false) { if(keepSeekPos && _isSeekable) _controlFlags |= FLG_SEEK_WHEN_PLAYED; stopInternal(); play(); }

    void refreshVideoProfile(bool restoreVideoValue=true, bool warning=false);

protected:
    enum STATE { ST_STOP, ST_PAUSE, ST_READY, ST_PLAY };
    enum CONTROL_FLAG {
        FLG_NONE                        = 0,
        FLG_CURSOR_IN_WINDOW            = 1,       // ウィンドウの中にカーソル
        FLG_HIDE_DISPLAY_MESSAGE        = 1 << 1,  // ディスプレイメッセージを非表示
        FLG_SEEKED_REPEAT               = 1 << 2,  // ABリピートでseek()した
        FLG_WHEEL_RESIZED               = 1 << 3,  // ホイールリサイズした
        FLG_EOF                         = 1 << 4,  // 再生が最後まで到達した
        FLG_OPENED_PATH                 = 1 << 5,  // パスを開いた
        FLG_RESIZE_WHEN_PLAYED          = 1 << 6,  // 再生した時リサイズする
        FLG_MUTE_WHEN_MOUSE_RELEASE     = 1 << 7,  // マウスリリースした時にミュートする
        FLG_DISABLE_MOUSEWINDOWMOVE     = 1 << 8,  // マウスによるウィンドウ移動を無効にする
        FLG_SEEK_WHEN_PLAYED            = 1 << 9,  // 再生した時、前回の位置へシークする
        FLG_RECONNECT_WHEN_PLAYED       = 1 << 10, // 再生した時、再接続(peercast)する
        FLG_MAXIMIZED_BEFORE_FULLSCREEN = 1 << 11, // フルスクリーンの前は最大化
        FLG_RECONNECTED                 = 1 << 12, // 再接続した
        FLG_EXPLICITLY_STOPPED          = 1 << 13, // 明示的に停止した
        FLG_NO_CHANGE_VDRIVER_WHEN_CLIPPING= 1 << 14, // クリッピングした時、ビデオドライバを切り替えない
        FLG_MOUSE_PRESSED_CLIPWINDOW    = 1 << 15, // クリップウィンドウ内をマウス押下した
    };
    Q_DECLARE_FLAGS(ControlFlags, CONTROL_FLAG)

    bool event(QEvent*);
    bool eventFilter(QObject*, QEvent*);
    void closeEvent(QCloseEvent*);
    void moveEvent(QMoveEvent*);
    void resizeEvent(QResizeEvent*);
    void keyPressEvent(QKeyEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);

    void setMouseTrackingClient(bool);
    void hideMouseCursor(bool);
    bool isHideMouseCursor() { return centralWidget()->cursor().shape() == Qt::BlankCursor; }
    void middleClickResize();
    void setCurrentDirectory();
    void openCommonProcess(const QString& path);
    void playCommonProcess();
    void saveInteractiveSettings();
    void loadInteractiveSettings();
    bool checkRestartFromConfigData(const ConfigData::Data& oldData, const ConfigData::Data& newData);

    QSize windowVideoClientSize();
    QSize videoViewSize100Percent();
    QSize correctToValidVideoSize(QSize toSize, const QSize& videoSize);
    QSize calcVideoViewSizeForResize(const QSize& viewSize, int percent);
    QSize calcVideoViewSizeForResize(int percent);
    QSize calcFullVideoSizeFromVideoViewSize(QSize viewSize);

    bool containsInClipWindow(const QPoint& pos);

    void reflectChannelInfo();
    QString genDateTimeSaveFileName(const QString& suffix=QString());

    void closeAllOtherDialog();

private slots:
    void mpProcess_finished();
    void mpProcess_error(QProcess::ProcessError);
    void mpProcess_debugKilledCPid();                    // debug
    void mpProcess_outputLine(const QString& line);
    void recProcess_finished();
    void recProcess_outputLine(const QString& line);
    void peercast_gotChannelInfo(const ChannelInfo&);
    void actGroupAudioOutput_changed(QAction*);
    void actGroupVolumeFactor_changed(QAction*);
    void actGroupAspect_changed(QAction*);
    void actGroupDeinterlace_changed(QAction*);
    void timerReconnect_timeout();
    void timerFps_timeout();
    void clipWindow_changedTranslucentDisplay(bool);
    void clipWindow_triggeredShow();
    void clipWindow_closed();
    void configDialog_applied();
    void videoAdjustDialog_windowActivate() { refreshVideoProfile(false, true); }

#ifdef Q_OS_WIN32
    void menuContext_aboutToHide();
#endif

private:
    void createStatusBar();
    void createToolBar();
    void createActionContextMenu();
    void updateVideoScreenGeometry();
    void showInterface(bool);
    void updateShowInterface();
    bool whetherMuteArea(int mouseLocalY);
//  bool whetherMuteArea(QPoint mousePos);
    void setStatus(const STATE);

#ifdef Q_OS_WIN32
    void initColorKey();
    QRgb genColorKey(int step, QRgb baseColorKey=0x000000);
#endif

private:
    MplayerProcess*   _mpProcess;
    RecordingProcess* _recProcess;
#ifdef Q_OS_WIN32
    QRgb _colorKey;
#endif

    STATE           _state;
    QString         _path;

    ChannelInfo _channelInfo;
    Peercast _peercast;

    QString         _usingVideoDriver;

    QSize           _videoSize;
    double          _videoLength;
    bool            _isSeekable;
    bool            _noVideo;
    QRect           _clipRect;

    double          _currentTime;
    double          _startTime;
    double          _oldTime;
    int             _elapsedTime;
    int             _repeatStartTime;
    int             _repeatEndTime;

    qint8           _volume;
    double          _speedRate;

    QTimer          _timerFps;
    quint16         _fpsCount;
    uint            _oldFrame;

    VideoSettings::VideoProfile _videoProfile;
    uint            _videoSettingsModifiedId;

    AUDIO_OUTPUT_MODE  _audioOutput;
    VOLUME_FACTOR_MODE _volumeFactor;
    ASPECT_RATIO       _aspectRatio;
    DEINTERLACE_MODE   _deinterlace;
    bool            _isMute;
    bool            _alwaysShowStatusBar;
    bool            _playNoSound;
    ControlFlags    _controlFlags;
    QPoint          _mousePressLocalPos;
    QPoint          _mousePressPos;
    QTimer          _timerBlockCursorHide;

    QTimer          _timerReconnect;
    quint8          _reconnectCount;
    quint16         _reconnectScore;
    int             _reconnectControlTime;

    PlaylistModel*  _playlist;

    QWidget*        _clipScreen;
    QWidget*        _videoScreen;
    QToolBar*       _toolBar;
    ControlButton*  _playPauseButton;
    ControlButton*  _stopButton;
    ControlButton*  _frameAdvanceButton;
    ControlButton*  _repeatABButton;
    ControlButton*  _loopButton;
    ControlButton*  _screenshotButton;
    TimeSlider*     _timeSlider;
    TimeLabel*      _timeLabel;
    QLabel*         _labelFrame;
    QLabel*         _labelFps;
    QLabel*         _labelVolume;
    QLabel*         _labelSpeedRate;
    InfoLabel*      _infoLabel;
    QWidget*        _statusbarSpaceL;
    QWidget*        _statusbarSpaceR;
    CommonMenu*     _menuContext;
    QMenu*          _menuReconnect;

    QAction*        _actScreenshot;
    QAction*        _actReconnect;
    QAction*        _actReconnectPlayer;
    QAction*        _actReconnectPct;
    QAction*        _actPlayNoSound;
    QAction*        _actPlayPause;
    QAction*        _actStop;
    QAction*        _actMute;
    QAction*        _actShowClipWindow;
    QAction*        _actReleaseClipping;
    QAction*        _actOpenContactUrl;
    QAction*        _actPlaylist;
    QAction*        _actVideoAdjust;
    QAction*        _actOpen;
    QAction*        _actConfig;
    QAction*        _actLog;
    QAction*        _actAbout;
    QAction*        _actStatusBar;
    QActionGroup*   _actGroupAudioOutput;
    QActionGroup*   _actGroupVolumeFactor;
    QActionGroup*   _actGroupAudioVolume;
    QActionGroup*   _actGroupAspect;
    QActionGroup*   _actGroupDeinterlace;

    OpenDialog*        _openDialog;
    VideoAdjustDialog* _videoAdjustDialog;
    ConfigDialog*      _configDialog;
    PlaylistDialog*    _playlistDialog;
    AboutDialog*       _aboutDialog;
    ClipWindow*        _clipWindow;
    QList<QWidget*>    _hiddenWindowList;

    bool _debugFlag;
    int  _debugCount;
};

#endif // PUREPLAYER_H

