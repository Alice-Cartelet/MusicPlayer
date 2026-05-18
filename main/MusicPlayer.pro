QT += core gui widgets multimedia multimediawidgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = MusicPlayer
TEMPLATE = app
app.rc
app.ico
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    playlist.cpp \
    lrcxparser.cpp \
    lyricsoverlay.cpp \
    settingsdialog.cpp

HEADERS += \
    app.rc \
    mainwindow.h \
    playlist.h \
    trackitem.h \
    lrcxparser.h \
    lyricsoverlay.h \
    settingsdialog.h
win32: LIBS += -ldwmapi
win32 {
    RC_FILE += app.rc
}
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target