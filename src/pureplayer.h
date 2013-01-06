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
#ifndef PUREPLAYER_H
#define PUREPLAYER_H

#include <QMainWindow>
#include <QSize>
#include <QPoint>
#include <QProcess>
#include <QTimer>
#include <QLabel>
#include "videosettings.h"

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
class SpeedSpinBox;
class TimeSlider;
class InfoLabel;
class TimeLabel;
class PlaylistModel;
class OpenDialog;
class VideoAdjustDialog;
class ConfigDialog;
class PlaylistDialog;
class AboutDialog;
class LogDialog;

class PurePlayer : public QMainWindow
{
    Q_OBJECT

public:
    enum ASPECT_RATIO { RATIO_VIDEO, RATIO_4_3, RATIO_16_9, RATIO_16_10, NO_KEEP };
    enum DEINTERLACE_MODE { DI_NO_DEINTERLACE, DI_YADIF, DI_YADIF_DOUBLE, DI_LINEAR_BLEND };
    enum AUDIO_OUTPUT_MODE { AO_STEREO, AO_MONAURAL, AO_LEFT, AO_RIGHT };
    enum VOLUME_FACTOR_MODE { VF_ONE_THIRD, VF_NORMAL, VF_DOUBLE, VF_TRIPLE };

    PurePlayer(QWidget* parent = 0);
    virtual ~PurePlayer();
    bool isMute()    { return _isMute; }
    bool isPlaying() { return _state == PLAY; }
    bool isStop()    { return _state == STOP; }
    bool isAlwaysShowStatusBar() { return !isFullScreen() && (_alwaysShowStatusBar
                                                                        || _isSeekable); }
    bool isPeercastStream() { return _port != -1; }

//  void resize(const QSize&);
//  void resize(int w, int h) { resize(QSize(w, h)); }

public slots:
    void open(const QString& path);
    void open(const QList<QUrl>& urls);
    void openFromDialog();
    void play();
    bool playPrev();
    bool playNext();
    void stop();
    void stopPeercast();
    void pauseUnPause();
    void frameAdvance();
    void repeatAB();
    void seek(double sec, bool relative=false);
    void setSpeed(double rate);
    void reconnect();
    void reconnectPurePlayer() { restartPlay(); }
    void reconnectPeercast();
    void recordingStartStop();
    void mute(bool);
    void upVolume(int value=5);
    void downVolume(int value=5);
    void setVolume(int value);
    void setVolumeFactor(VOLUME_FACTOR_MODE);
    void setAudioOutput(AUDIO_OUTPUT_MODE);
    void setAspectRatio(ASPECT_RATIO);
    void setContrast(int value, bool alwaysSet=false);
    void setBrightness(int value, bool alwaysSet=false);
    void setSaturation(int value, bool alwaysSet=false);
    void setHue(int value, bool alwaysSet=false);
    void setGamma(int value, bool alwaysSet=false);
    void setDeinterlace(DEINTERLACE_MODE);
    void screenshot();
    void resizeReduce()           { resizeFromCurrent(-300); }
    void resizeIncrease()         { resizeFromCurrent(+300); }
//  void resizeSlightlyReduce()   { resizePercentageFromCurrent(-10); }
//  void resizeSlightlyIncrease() { resizePercentageFromCurrent(+10); }
    void resize320x240()    { resizeFromVideoClient(QSize(320,240)); }
    void resize1280x720()   { resizeFromVideoClient(QSize(1280,720)); }
    void resize25Percent()  { resizeFromVideoClient(calcPercentageVideoSize(25)); }
    void resize50Percent()  { resizeFromVideoClient(calcPercentageVideoSize(50)); }
    void resize75Percent()  { resizeFromVideoClient(calcPercentageVideoSize(75)); }
    void resize100Percent() { resizeFromVideoClient(calcPercentageVideoSize(100)); }
    void resize125Percent() { resizeFromVideoClient(calcPercentageVideoSize(125)); }
    void resize150Percent() { resizeFromVideoClient(calcPercentageVideoSize(150)); }
    bool resizeFromVideoClient(QSize size);
    void resizePercentageFromCurrent(int percentage);
    void resizeFromCurrent(int amount);
    void fullScreenOrWindow();
    void setAlwaysShowStatusBar(bool);

    void saveVideoProfileToDefault();
    void setVideoProfile(const QString& profileName, bool setVideoValue=true);
    void createVideoProfileFromCurrent(QString profileName);
    void updateVideoProfile();
    void restoreVideoProfile();
    void removeVideoProfile();

    void showVideoAdjustDialog();
    void showLogDialog();
    void showAboutDialog();
    void showConfigDialog();
    void showPlaylistDialog();

    void updateChannelInfo();
    void openContactUrl();

protected slots:
    void mpCmd(const QString& command);
    void stopFromGui()      { _controlFlags |= FLG_EXPLICITLY_STOPPED; stop(); }
    void reconnectFromGui() { _reconnectCount=0; reconnect(); }
    void reconnectPurePlayerFromGui() { _reconnectCount=0; reconnectPurePlayer(); }
    void playlist_playStopCurrentTrack();
    void buttonPlayPauseClicked();
    void exitFullScreen() { if( isFullScreen() ) fullScreenOrWindow(); }
    void restartPlay(bool keepSeekPos=false) { if(keepSeekPos && _isSeekable) _controlFlags |= FLG_SEEK_WHEN_PLAYED; stop(); play(); }

    void refreshVideoProfile(bool restoreVideoValue=true, bool warning=false);

protected:
    enum STATE { STOP, PAUSE, READY, PLAY };
    enum PEERCAST_TYPE { PCT_UNKNOWN, PCT_VP, PCT_ST };
    enum CONTROL_FLAG {
        FLG_NONE                    = 0x00000000,
        FLG_HIDE_DISPLAY_MESSAGE    = 0x00000001, // ディスプレイメッセージを非表示
        FLG_SEEKED_REPEAT           = 0x00000002, // ABリピートでseek()した
        FLG_WHEEL_RESIZED           = 0x00000004, // ホイールリサイズした
        FLG_EOF                     = 0x00000008, // 再生が最後まで到達した
        FLG_OPENED_PATH             = 0x00000010, // パスを開いた
        FLG_RESIZE_WHEN_PLAYED      = 0x00000020, // 再生した時リサイズする
        FLG_MUTE_WHEN_MOUSE_RELEASE = 0x00000040, // マウスリリースした時にミュートする
        FLG_DISABLE_MOUSEWINDOWMOVE = 0x00000080, // マウスによるウィンドウ移動を無効にする
        FLG_SEEK_WHEN_PLAYED        = 0x00000100, // 再生した時、前回の位置へシークする
        FLG_RECONNECT_WHEN_PLAYED   = 0x00000200, // 再生した時、再接続(peercast)する
        FLG_MAXIMIZED_BEFORE_FULLSCREEN = 0x00000400, // フルスクリーンの前は最大化
        FLG_RECONNECTED             = 0x00000800, // 再接続した
        FLG_EXPLICITLY_STOPPED      = 0x00001000, // 明示的に停止した
    };

//  bool event(QEvent*);
    bool eventFilter(QObject*, QEvent*);
    void closeEvent(QCloseEvent*);
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
//  void paintEvent(QPaintEvent*);

    void middleClickResize();
    void setCurrentDirectory();
    void openCommonProcess(const QString& path);
    void playCommonProcess();
    void saveInteractiveSettings();
    void loadInteractiveSettings();

    QSize videoSize100Percent();
    QSize correctToValidVideoSize(QSize toSize, const QSize& videoSize);
    QSize calcPercentageVideoSize(const QSize& videoSize, const int percentage);
    QSize calcPercentageVideoSize(const int percentage);

    bool updateChannelInfoPcVp(const QString& reply);
    bool updateChannelInfoPcSt(const QString& reply);
    bool updateChannelStatusPcSt(const QString& reply);
    void reflectChannelInfo();

    PlaylistDialog* playlistDialog();

private slots:
    void mpProcessFinished();
    void mpProcessError(QProcess::ProcessError);
    void mpProcessDebugKilledCPid();                    // debug
    void parseMplayerOutputLine(const QString& line);
    void recordingProcessFinished();
    void recordingOutputLine(const QString& line);
    void replyFinished(QNetworkReply*);
    void actGroupAudioOutputChanged(QAction*);
    void actGroupVolumeFactorChanged(QAction*);
    void actGroupAspectChanged(QAction*);
    void actGroupDeinterlaceChanged(QAction*);
    void timerReconnectTimeout();
    void timerFpsTimeout();
    void appliedFromConfigDialog(bool restartMplayer);
    void videoAdjustDialogWindowActivate() { refreshVideoProfile(false, true); }

private:
    void createStatusBar();
    void createToolBar();
    void createActionContextMenu();
    void updateVideoScreenGeometry();
    void visibleInterface(bool);
    void updateVisibleInterface();
    bool whetherMuteArea(int y);
    void setStatus(const STATE);

#ifdef Q_OS_WIN32
    void initColorKey();
    QRgb genColorKey(int step, QRgb baseColorKey=0x000000);
#endif

private:
    STATE           _state;
    PEERCAST_TYPE   _peercastType;
    QWidget*        _videoScreen;
    QToolBar*       _toolBar;
    ControlButton*  _playPauseButton;
    ControlButton*  _stopButton;
    ControlButton*  _frameAdvanceButton;
    ControlButton*  _repeatABButton;
    ControlButton*  _loopButton;
    ControlButton*  _screenshotButton;
    TimeSlider*     _timeSlider;
    SpeedSpinBox*   _speedSpinBox;
    TimeLabel*      _timeLabel;
    QLabel*         _labelFrame;
    QLabel*         _labelFps;
    QLabel*         _labelVolume;
    InfoLabel*      _infoLabel;
    QWidget*        _statusbarSpaceL;
    QWidget*        _statusbarSpaceR;

    QNetworkAccessManager* _nam;
    QNetworkReply*         _replyChannelInfoPcVp;
    QNetworkReply*         _replyChannelInfoPcSt;
    QNetworkReply*         _replyChannelStatusPcSt;
    QVector<PEERCAST_TYPE> _attemptPeercastType;
    MplayerProcess*   _mpProcess;
    RecordingProcess* _recordingProcess;
#ifdef Q_OS_WIN32
    QRgb _colorKey;
#endif

    QString         _path;

    QString         _host;
    short           _port;
    QString         _id;
    QString         _rootIp;
    QString         _chName;
    QString         _contactUrl;
    QString         _connectedIP;
    bool            _searchingConnection;

    QSize           _videoSize;
    double          _videoLength;
    bool            _noVideo;
    bool            _isSeekable;
    double          _currentTime;
    double          _startTime;
    double          _oldTime;
    int             _elapsedTime;
    int             _repeatStartTime;
    int             _repeatEndTime;

    qint8           _volume;

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
    bool            _cursorInWindow;
    bool            _alwaysShowStatusBar;
    quint32         _controlFlags;
    QPoint          _mousePressLocalPos;
    QPoint          _mousePressWindowPos;

    QMenu*          _menuContext;
    QMenu*          _menuReconnect;
    QAction*        _actScreenshot;
    QAction*        _actReconnect;
    QAction*        _actReconnectPly;
    QAction*        _actReconnectPct;
    QAction*        _actPlayPause;
    QAction*        _actStop;
    QAction*        _actMute;
    QAction*        _actOpenContactUrl;
    QAction*        _actPlaylist;
    QAction*        _actStatusBar;
    QActionGroup*   _actGroupAudioOutput;
    QActionGroup*   _actGroupVolumeFactor;
    QActionGroup*   _actGroupAudioVolume;
    QActionGroup*   _actGroupAspect;
    QActionGroup*   _actGroupDeinterlace;

    QTimer          _timerReconnect;
    quint16         _receivedErrorCount;
    qint8           _reconnectCount;
    int             _reconnectControlTime;

    PlaylistModel*  _playlist;

    OpenDialog*        _openDialog;
    VideoAdjustDialog* _videoAdjustDialog;
    ConfigDialog*      _configDialog;
    PlaylistDialog*    _playlistDialog;
    AboutDialog*       _aboutDialog;

    bool _debugFlg;
    int  _debugCount;
};

#endif // PUREPLAYER_H

