#pragma once
#include <QString>
#include <QByteArray>
class Id3v2Helper
{
public: static bool writeCover(const QString &mp3Path, const QByteArray &imgData, const QString &mime = "image/jpeg");
    static QByteArray readCover(const QString &mp3Path);
}
;