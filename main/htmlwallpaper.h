#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <memory>

// A native child of Explorer's background window. It never becomes a
// top-level Qt window and does not participate in desktop input handling.
class HtmlWallpaper : public QObject
{
    Q_OBJECT
public:
    explicit HtmlWallpaper(QObject *parent = nullptr);
    ~HtmlWallpaper() override;
    bool isEnabled() const;
    QString filePath() const;
    void setWallpaper(const QString &filePath, bool enabled);
    // Cache even while disabled/loading; replay when the selected HTML is ready.
    void updateTrack(const QJsonObject &track);
    void updatePlayback(const QJsonObject &playback);

signals:
    void ready();
    void errorOccurred(const QString &message);
    void enabledChanged(bool enabled);

private:
    class Private;
    std::unique_ptr<Private> d;
};
