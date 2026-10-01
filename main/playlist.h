#pragma once
#include <QAbstractListModel>
#include <QMimeData>
#include <QList>
#include "trackitem.h"
class PlaylistManager;
class Playlist : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        TitleRole = Qt::UserRole+1, ArtistRole, DurationRole, FilePathRole, FavoriteRole=Qt::UserRole+10
    }
    ;
    explicit Playlist( QObject *parent=nullptr);
    void setPlaylistManager( PlaylistManager *mgr);
    void setSortable(bool v)
    {
        m_sortable=v;
    }
    int rowCount( const QModelIndex &parent= QModelIndex() ) const override;
    QVariant data( const QModelIndex &index, int role= Qt::DisplayRole ) const override;
    Qt::ItemFlags flags( const QModelIndex &index ) const override;
    bool moveRows( const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild ) override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData( const QModelIndexList &indexes ) const override;
    Qt::DropActions supportedDropActions() const override;
    void addTrack( const TrackItem &track);
    void addTracks( const QList<TrackItem> &tracks);
    void removeTrack( int index);
    void clear();
    TrackItem track( int index) const;
    int count() const
    {
        return m_tracks.size();
    }
    QStringList filePaths() const;
    void updateDuration( int index, qint64 ms);
    void notifyFavoriteChanged( const QString &filePath);
private: QList<TrackItem> m_tracks;
    PlaylistManager *m_plManager=nullptr;
    bool m_sortable=false;
}
;