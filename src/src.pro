TEMPLATE = app
TARGET = pureplayer
DESTDIR = ..
DEPENDPATH += .
INCLUDEPATH += .
QT += network script
CONFIG += release
#CONFIG += debug

HEADERS += \
    pureplayer.h \
    peercast.h \
    process.h \
    controlbutton.h \
    speedspinbox.h \
    timeslider.h \
    infolabel.h \
    timelabel.h \
    configdata.h \
    videosettings.h \
    playlist.h \
    commonlib.h \
    task.h \
    windowcontroller.h \
    \
    logdialog.h \
    commonspinbox.h \
    configdialog.h \
    opendialog.h \
    videoadjustdialog.h \
    playlistdialog.h \
    inputdialog.h \
    aboutdialog.h \
    clipwindow.h

SOURCES += \
    main.cpp \
    pureplayer.cpp \
    peercast.cpp \
    process.cpp \
    speedspinbox.cpp \
    timeslider.cpp \
    infolabel.cpp \
    timelabel.cpp \
    configdata.cpp \
    videosettings.cpp \
    playlist.cpp \
    commonlib.cpp \
    task.cpp \
    windowcontroller.cpp \
    \
    logdialog.cpp \
    configdialog.cpp \
    videoadjustdialog.cpp \
    playlistdialog.cpp \
    inputdialog.cpp \
    clipwindow.cpp

FORMS += \
    logdialog.ui \
    configdialog.ui \
    opendialog.ui \
    videoadjustdialog.ui \
    playlistdialog.ui \
    inputdialog.ui \
    aboutdialog.ui

RESOURCES += resource.qrc

win32 {
    CONFIG(debug, debug|release) {
        CONFIG += console
    }
}

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

