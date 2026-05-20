#pragma once
#include <QObject>
#include <QMap>
#include <QSet>
#include <QStringList>
class PlaylistManager : public QObject
{
    Q_OBJECT
public: static const QString FAVORITES_NAME;
    explicit PlaylistManager(QObject *parent = nullptr);
    QStringList playlistNames() const;
    bool createPlaylist(const QString &name);
    bool removePlaylist(const QString &name);
    void addToPlaylist(const QString &playlistName, const QString &filePath);
    void removeFromPlaylist(const QString &playlistName, const QString &filePath);
    bool contains(const QString &playlistName, const QString &filePath) const;
    QStringList tracks(const QString &playlistName) const;
    bool isFavorite(const QString &filePath) const;
    void toggleFavorite(const QString &filePath);
    void save();
    void load();
signals: void playlistsChanged();
    void favoriteChanged(const QString &filePath, bool isFav);
private: QMap<QString, QSet<QString>> m_data;
    QStringList m_order;
}
;