QT += core gui widgets multimedia multimediawidgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = MusicPlayer
TEMPLATE = app
RESOURCES += developer-docs.qrc
DISTFILES += HTML-WALLPAPER.md music-wallpaper.html
app.rc
app.ico
SOURCES += \
    id3v2helper.cpp \
    main.cpp \
    mainwindow.cpp \
    miniControlWindow.cpp \
    desktopwallpaper.cpp \
    htmlwallpaper.cpp \
    musicwallpaperdata.cpp \
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
    htmlwallpaper.h \
    musicwallpaperdata.h \
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
win32: LIBS += -ldwmapi -lole32 -luuid
win32 {
    RC_FILE += app.rc
    # WebView2 uses the installed Edge runtime and retains MinGW compatibility.
    CONFIG(debug, debug|release): wallpaperDeployDir = $$OUT_PWD/debug
    else: wallpaperDeployDir = $$OUT_PWD/release
    !isEmpty(DESTDIR): wallpaperDeployDir = $$absolute_path($$DESTDIR, $$OUT_PWD)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/webview2/WebView2Loader.dll)) $$shell_quote($$shell_path($$wallpaperDeployDir/WebView2Loader.dll))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/webview2/LICENSE.txt)) $$shell_quote($$shell_path($$wallpaperDeployDir/WebView2-LICENSE.txt))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/webview2/NOTICE.txt)) $$shell_quote($$shell_path($$wallpaperDeployDir/WebView2-NOTICE.txt))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/HTML-WALLPAPER.md)) $$shell_quote($$shell_path($$wallpaperDeployDir/HTML-WALLPAPER.md))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/music-wallpaper.html)) $$shell_quote($$shell_path($$wallpaperDeployDir/music-wallpaper.html))
}
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
