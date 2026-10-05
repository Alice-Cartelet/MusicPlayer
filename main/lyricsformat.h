#pragma once

#include <QByteArray>
#include <QString>

struct LyricsContent
{
    QString lines;
    QString words;
    QString translation;
};

namespace LyricsFormat {
LyricsContent decodeKrc(const QByteArray &data);
LyricsContent decodeNetease(const QString &yrc, const QString &lrc, const QString &translation);
LyricsContent decodeQqCloud(const QString &encryptedLyric, const QString &encryptedTranslation);
QString render(const LyricsContent &content, bool wordByWord, bool includeTranslation,
               int offsetMs, double speed, QString *error = nullptr);
}
