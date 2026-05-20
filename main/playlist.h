#pragma once
#include <QAbstractListModel>
#include <QList>
#include "trackitem.h"
class PlaylistManager;
class Playlist : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        TitleRole = Qt::UserRole + 1, ArtistRole, DurationRole, FilePathRole, FavoriteRole = Qt::UserRole + 10
    }
    ;
    explicit Playlist(QObject *parent = nullptr);
    void setPlaylistManager(PlaylistManager *mgr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void addTrack(const TrackItem &track);
    void addTracks(const QList<TrackItem> &tracks);
    void removeTrack(int index);
    void clear();
    TrackItem track(int index) const;
    int count() const
    {
        return m_tracks.size();
    }
    void updateDuration(int index, qint64 ms);
    void notifyFavoriteChanged(const QString &filePath);
private: QList<TrackItem> m_tracks;
    PlaylistManager *m_plManager = nullptr;
}
;