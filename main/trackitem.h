#pragma once
#include <QString>
struct TrackItem
{
    QString filePath;
    QString title;
    QString artist;
    QString album;
    qint64 duration;
    TrackItem() : duration(0)
    {
    }
    TrackItem(const QString &path, const QString &title, const QString &artist = QString(), const QString &album = QString(), qint64 duration = 0) : filePath(path), title(title), artist(artist), album(album), duration(duration)
    {
    }
    QString durationString() const
    {
        if (duration <= 0) return "--:--";
        int secs = duration / 1000;
        int mins = secs / 60;
        secs %= 60;
        return QString("%1:%2").arg(mins, 2, 10, QChar('0')) .arg(secs, 2, 10, QChar('0'));
    }
}
;