TEMPLATE = app
TARGET = pureplayer
DESTDIR = ..
DEPENDPATH += .
INCLUDEPATH += .
QT += network
#CONFIG += release
CONFIG += debug

HEADERS += \
    pureplayer.h \
    process.h \
    controlbutton.h \
    speedspinbox.h \
    timeslider.h \
    infolabel.h \
    timelabel.h \
    configdata.h \
    playlist.h \
    commonlib.h \
    task.h \
    \
    logdialog.h \
    configdialog.h \
    opendialog.h \
    videoadjustdialog.h \
    playlistdialog.h \
    aboutdialog.h

SOURCES += \
    main.cpp \
    pureplayer.cpp \
    process.cpp \
    speedspinbox.cpp \
    timeslider.cpp \
    infolabel.cpp \
    timelabel.cpp \
    configdata.cpp \
    playlist.cpp \
    commonlib.cpp \
    task.cpp \
    \
    logdialog.cpp \
    configdialog.cpp \
    playlistdialog.cpp

FORMS += \
    logdialog.ui \
    configdialog.ui \
    opendialog.ui \
    videoadjustdialog.ui \
    playlistdialog.ui \
    aboutdialog.ui

RESOURCES += resource.qrc

win32:debug {
    CONFIG += console
}

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

