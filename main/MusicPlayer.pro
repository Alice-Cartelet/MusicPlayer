QT += core gui widgets multimedia multimediawidgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = MusicPlayer
TEMPLATE = app
app.rc
app.ico
SOURCES += \
    id3v2helper.cpp \
    main.cpp \
    mainwindow.cpp \
    miniControlWindow.cpp \
    desktopwallpaper.cpp \
    playlistmanager.cpp \
    miniconbutton.cpp \
    playlist.cpp \
    lrcxparser.cpp \
    lyricsoverlay.cpp \
    positionpicker.cpp \
    settingsdialog.cpp

HEADERS += \
    app.rc \
    id3v2helper.h \
    desktopwallpaper.h \
    mainwindow.h \
    playlistmanager.h \
    miniconbutton.h \
    minicontrolwindow.h \
    playlist.h \
    positionpicker.h \
    trackitem.h \
    lrcxparser.h \
    lyricsoverlay.h \
    settingsdialog.h \
    version.h
win32: LIBS += -ldwmapi
win32 {
    RC_FILE += app.rc
}
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target