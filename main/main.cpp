#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QMessageBox>
#include <QLocalServer>
#include <QLocalSocket>
#include <windows.h>
#include "mainwindow.h"
#include "version.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("本地音乐播放器");
    app.setOrganizationName("MusicPlayer");
    app.setApplicationVersion(APP_VERSION);
    const int devanagariFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/NotoSansDevanagari.ttf"));
    const QStringList devanagariFamilies =
        QFontDatabase::applicationFontFamilies(devanagariFontId);
    if (!devanagariFamilies.isEmpty()) {
        QFontDatabase::addApplicationFallbackFontFamily(
            QChar::Script_Devanagari, devanagariFamilies.first());
    }
    const QString serverName = "LocalMusicPlayer_Unique_Instance";
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(300))
    {
        MessageBoxW(nullptr,L"本程序已在运行。",L"MusicPlayer出现错误",MB_OK | MB_ICONERROR);
        return 0;
    }
    QLocalServer::removeServer(serverName);
    QLocalServer server;
    server.listen(serverName);
    MainWindow w;
    w.show();
    return app.exec();
}
