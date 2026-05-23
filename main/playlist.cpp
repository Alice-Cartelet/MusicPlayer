#include "playlist.h"
#include "playlistmanager.h"
#include <QMimeData>
Playlist:: Playlist(QObject *parent) :QAbstractListModel(parent)
{
}
void Playlist:: setPlaylistManager( PlaylistManager *mgr)
{
    m_plManager=mgr;
}
int Playlist:: rowCount( const QModelIndex &parent ) const
{
    if(parent.isValid()) return 0;
    return m_tracks.size();
}
QVariant Playlist:: data( const QModelIndex &index, int role ) const
{
    if(!index.isValid() || index.row()>=m_tracks.size())
    {
        return
            {
            }
        ;
    }
    const TrackItem &t= m_tracks[index.row()];
    switch(role)
    {
    case Qt::DisplayRole: case TitleRole: return t.title;
    case ArtistRole: return t.artist.isEmpty() ? "未知艺术家" : t.artist;
    case DurationRole: return t.durationString();
    case FilePathRole: return t.filePath;
    case FavoriteRole: if(m_plManager)
        {
            return m_plManager ->isFavorite( t.filePath);
        }
        return false;
    }
    return
        {
        }
    ;
}
Qt::ItemFlags Playlist:: flags( const QModelIndex &index ) const
{
    Qt::ItemFlags f= QAbstractListModel:: flags(index);
    f|=Qt::ItemIsEnabled;
    f|=Qt::ItemIsSelectable;
    if(m_sortable)
    {
        f|=Qt::ItemIsDropEnabled;
        if(index.isValid())
        {
            f|=Qt::ItemIsDragEnabled;
        }
    }
    return f;
}
QStringList Playlist:: mimeTypes() const
{
    return
        {
            "application/x-playlist-item"
        }
    ;
}
QMimeData* Playlist:: mimeData( const QModelIndexList &indexes ) const
{
    auto *mime= new QMimeData;
    if(indexes.isEmpty())
    {
        return mime;
    }
    QByteArray arr;
    arr.setNum( indexes.first() .row());
    mime->setData( "application/x-playlist-item", arr);
    return mime;
}
Qt::DropActions Playlist:: supportedDropActions() const
{
    return Qt::MoveAction;
}
bool Playlist:: moveRows( const QModelIndex&, int sourceRow, int count, const QModelIndex&, int destinationChild )
{
    if(count!=1)
    {
        return false;
    }
    if(sourceRow== destinationChild || sourceRow+1== destinationChild)
    {
        return false;
    }
    beginMoveRows( QModelIndex(), sourceRow, sourceRow, QModelIndex(), destinationChild);
    int target= destinationChild;
    if(target> sourceRow)
    {
        target--;
    }
    m_tracks.move( sourceRow, target);
    endMoveRows();
    return true;
}
void Playlist:: addTrack( const TrackItem &track)
{
    beginInsertRows( QModelIndex(), m_tracks.size(), m_tracks.size() );
    m_tracks.append( track);
    endInsertRows();
}
void Playlist:: addTracks( const QList<TrackItem> &tracks)
{
    if(tracks.isEmpty()) return;
    int first= m_tracks.size();
    beginInsertRows( QModelIndex(), first, first+ tracks.size()-1 );
    m_tracks.append( tracks);
    endInsertRows();
}
void Playlist:: removeTrack( int idx)
{
    if(idx<0 || idx>=m_tracks.size())
    {
        return;
    }
    beginRemoveRows( QModelIndex(), idx, idx);
    m_tracks.removeAt( idx);
    endRemoveRows();
}
void Playlist:: clear()
{
    beginResetModel();
    m_tracks.clear();
    endResetModel();
}
TrackItem Playlist:: track( int index ) const
{
    if(index<0 || index>=m_tracks.size())
    {
        return
            {
            }
        ;
    }
    return m_tracks[index];
}
QStringList Playlist:: filePaths() const
{
    QStringList r;
    for(const auto &t :m_tracks)
    {
        r<<t.filePath;
    }
    return r;
}
void Playlist:: updateDuration( int index, qint64 ms )
{
    if(index<0 || index>=m_tracks.size())
    {
        return;
    }
    m_tracks[index] .duration=ms;
    QModelIndex idx= createIndex( index, 0);
    emit dataChanged( idx, idx,
                     {
                         DurationRole
                     }
                     );
}
void Playlist:: notifyFavoriteChanged( const QString &filePath)
{
    for(int i=0;
         i<m_tracks.size();
         ++i)
    {
        if( m_tracks[i] .filePath== filePath )
        {
            QModelIndex idx= createIndex( i, 0);
            emit dataChanged( idx, idx,
                             {
                                 FavoriteRole
                             }
                             );
        }
    }
}