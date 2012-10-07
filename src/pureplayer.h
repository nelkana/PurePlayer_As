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
#include "playlist.h"

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
class OpenDialog;
class VideoAdjustDialog;
class ConfigDialog;
class PlayListDialog;
class AboutDialog;
class LogDialog;

class PurePlayer : public QMainWindow
{
    Q_OBJECT

public:
    enum ASPECT_RATIO { RATIO_VIDEO, RATIO_4_3, RATIO_16_9, RATIO_16_10, NO_KEEP };
    enum DEINTERLACE_MODE { DI_NO_DEINTERLACE, DI_YADIF, DI_LINEAR_BLEND };
    enum AUDIO_OUTPUT_MODE { AO_STEREO, AO_MONAURAL, AO_LEFT, AO_RIGHT };
    enum VOLUME_FACTOR_MODE { VF_NORMAL, VF_DOUBLE, VF_TRIPLE };

    struct VideoSettings {
        int contrast;
        int brightness;
        int hue;
        int saturation;
        int gamma;
    };

    PurePlayer(QWidget* parent = 0);
    virtual ~PurePlayer();
    bool isMute()    { return _isMute; }
    bool isPlaying() { return _state == PLAY; }
    bool isStop()    { return _state == STOP; }
    bool isAlwaysShowStatusBar() { return !isFullScreen() && (_alwaysShowStatusBar
                                                                        || _isSeekable); }
    bool isPeercastStream() { return _port != -1; }

public slots:
    void open(const QString& path);
    void openFromDialog();
    void play();
    void playPrev();
    void playNext();
    void stop();
    void pauseUnPause();
    void frameAdvance();
    void repeatAB();
    void seek(double sec, bool relative=false);
    void setSpeed(double rate);
    void reconnect()           { stop(); _reconnectWasCalled=true; play(); }
    void reconnectPurePlayer() { restartPlay(); }
    void reconnectPeercast();
    void recordingStartStop();
    void mute(bool);
    void upVolume(int value=5);
    void downVolume(int value=5);
    void setVolume(int value);
    void setVolumeFactor(VOLUME_FACTOR_MODE);
    void setAudioOutput(AUDIO_OUTPUT_MODE);
    void setLoop(bool);
    void setAspectRatio(ASPECT_RATIO);
    void setContrast(int value);
    void setBrightness(int value);
    void setHue(int value);
    void setSaturation(int value);
    void setGamma(int value);
    void setDeinterlace(DEINTERLACE_MODE);
    void screenshot();
    void resize320x240()    { resizeFromVideoScreen(QSize(320,240)); }
    void resize1280x720()   { resizeFromVideoScreen(QSize(1280,720)); }
    void resize25Percent()  { resizeFromVideoScreen(QSize(_videoSize.width()/4,_videoSize.height()/4)); }
    void resize50Percent()  { resizeFromVideoScreen(QSize(_videoSize.width()/2,_videoSize.height()/2)); }
    void resize75Percent()  { resizeFromVideoScreen(QSize(_videoSize.width()*3/4,_videoSize.height()*3/4)); }
    void resize100Percent() { resizeFromVideoScreen(_videoSize); }
    void resize125Percent() { resizeFromVideoScreen(QSize(_videoSize.width()*5/4,_videoSize.height()*5/4)); }
    void resize150Percent() { resizeFromVideoScreen(QSize(_videoSize.width()*3/2,_videoSize.height()*3/2)); }
    bool resizeFromVideoScreen(QSize size);
    void fullScreenOrWindow();
    void setAlwaysShowStatusBar(bool);
    void showVideoAdjustDialog();
    void saveVideoSettings();
    void loadVideoSettings();
    void showLogDialog();
    void showAboutDialog();
    void showConfigDialog();
    void showPlayListDialog();

    void updateChannelInfo();
    void openContactUrl();

protected slots:
    void mpCmd(const QString& command);
    void reconnectFromGui() { _reconnectCount=0; reconnect(); }
    void reconnectPurePlayerFromGui() { _reconnectCount=0; reconnectPurePlayer(); }
    void playFromPlayListDialog() { _reconnectCount=0; restartPlay(); }
    void buttonPlayPauseClicked();
    void exitFullScreen() { if( isFullScreen() ) fullScreenOrWindow(); }
    void restartPlay(bool keepSeekPos=false) { if(keepSeekPos && _isSeekable) _seekWhenStartMplayer=true; stop(); play(); }

protected:
    enum STATE { STOP, PAUSE, READY, PLAY };
    enum CONTROL_FLAG {
        FLG_NONE                 = 0x00000000,
        FLG_HIDE_DISPLAY_MESSAGE = 0x00000001, // ディスプレイメッセージを非表示
        FLG_SEEKED_REPEAT        = 0x00000002, // ABリピートでseek()したか
    };

//  bool event(QEvent*);
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

    PlayListDialog* playListDialog();

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

private:
    void createStatusBar();
    void createToolBar();
    void createActionContextMenu();
    void updateVideoScreenGeometry();
    void visibleInterface(bool);
    void updateVisibleInterface();
    void setStatus(const STATE);

private:
    STATE           _state;
    QWidget*        _videoScreen;
    QToolBar*       _toolBar;
    ControlButton*  _playPauseButton;
    ControlButton*  _stopButton;
    ControlButton*  _frameAdvanceButton;
    ControlButton*  _repeatABButton;
    ControlButton*  _loopButton;
    ControlButton*  _screenshotButton;
    TimeSlider*     _timeslider;
    SpeedSpinBox*   _speedSpinBox;
    TimeLabel*      _timeLabel;
    QLabel*         _labelFrame;
    QLabel*         _labelFps;
    QLabel*         _labelVolume;
    InfoLabel*      _infoLabel;
    QWidget*        _statusbarSpaceL;
    QWidget*        _statusbarSpaceR;

    QNetworkAccessManager* _nam;
    MplayerProcess* _mpProcess;
    RecordingProcess* _recordingProcess;

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

    VideoSettings   _videoSettings;

    qint8           _volume;

    AUDIO_OUTPUT_MODE  _audioOutput;
    VOLUME_FACTOR_MODE _volumeFactor;
    ASPECT_RATIO       _aspectRatio;
    DEINTERLACE_MODE   _deinterlace;
    bool            _isMute;
    bool            _doLoop;
    bool            _openedNewPath;
    bool            _seekWhenStartMplayer;
    bool            _reconnectWasCalled;
    bool            _isMaximizedBeforeFullScreen;
    bool            _muteWhenMouseRelease;
    bool            _cursorInWindow;
    bool            _disableWindowMoveFromMouse;
    bool            _alwaysShowStatusBar;
    quint32         _controlFlags;
    QPoint          _pressLocalPos;

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
    QAction*        _actPlayList;
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

    QTimer          _timerFps;
    int             _fpsCount;

    PlayList        _playList;

    OpenDialog*        _openDialog;
    VideoAdjustDialog* _videoAdjustDialog;
    ConfigDialog*      _configDialog;
    PlayListDialog*    _playListDialog;
    AboutDialog*       _aboutDialog;

    bool _debugFlg;
    int  _debugCount;
};

#endif // PUREPLAYER_H

