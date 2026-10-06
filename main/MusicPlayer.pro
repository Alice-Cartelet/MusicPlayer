QT += core gui widgets multimedia multimediawidgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = MusicPlayer
TEMPLATE = app
RESOURCES += developer-docs.qrc
DISTFILES += HTML-WALLPAPER.md music-wallpaper.html CHARACTER-WALLPAPER.md character-wallpaper.html bg.jpg bg.png
app.rc
app.ico
SOURCES += \
    audiobeatdetector.cpp \
    id3v2helper.cpp \
    lyricseditdialog.cpp \
    lyricsformat.cpp \
    qqqrc.cpp \
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
    audiobeatdetector.h \
    app.rc \
    id3v2helper.h \
    lyricseditdialog.h \
    lyricsformat.h \
    qqqrc.h \
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
win32: LIBS += -ldwmapi -lole32 -luuid -lz -lbcrypt
win32 {
    RC_FILE += app.rc
    # WebView2 uses the installed Edge runtime and retains MinGW compatibility.
    CONFIG(debug, debug|release): wallpaperDeployDir = $$OUT_PWD/debug
    else: wallpaperDeployDir = $$OUT_PWD/release
    !isEmpty(DESTDIR): wallpaperDeployDir = $$absolute_path($$DESTDIR, $$OUT_PWD)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/webview2/WebView2Loader.dll)) $$shell_quote($$shell_path($$wallpaperDeployDir/WebView2Loader.dll))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/webview2/LICENSE.txt)) $$shell_quote($$shell_path($$wallpaperDeployDir/WebView2-LICENSE.txt))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/webview2/NOTICE.txt)) $$shell_quote($$shell_path($$wallpaperDeployDir/WebView2-NOTICE.txt))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/qqmusicdecoder-LICENSE.txt)) $$shell_quote($$shell_path($$wallpaperDeployDir/QQMusicDecoder-LICENSE.txt))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/third_party/NotoSansDevanagari-OFL.txt)) $$shell_quote($$shell_path($$wallpaperDeployDir/NotoSansDevanagari-OFL.txt))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/HTML-WALLPAPER.md)) $$shell_quote($$shell_path($$wallpaperDeployDir/HTML-WALLPAPER.md))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/music-wallpaper.html)) $$shell_quote($$shell_path($$wallpaperDeployDir/music-wallpaper.html))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/character-wallpaper.html)) $$shell_quote($$shell_path($$wallpaperDeployDir/character-wallpaper.html))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/CHARACTER-WALLPAPER.md)) $$shell_quote($$shell_path($$wallpaperDeployDir/CHARACTER-WALLPAPER.md))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/bg.jpg)) $$shell_quote($$shell_path($$wallpaperDeployDir/bg.jpg))
    QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/bg.png)) $$shell_quote($$shell_path($$wallpaperDeployDir/bg.png))
    # Optional character images supplied alongside the root HTML.
    for(characterNumber, 1 2 3 4 5 6 7 8 9 10) {
        characterImages = $$files($$PWD/../rw$${characterNumber}.*)
        for(characterImage, characterImages) {
            QMAKE_POST_LINK += $$escape_expand(\n\t) $$QMAKE_COPY $$shell_quote($$shell_path($$characterImage)) $$shell_quote($$shell_path($$wallpaperDeployDir/$$basename(characterImage)))
        }
    }
}
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
