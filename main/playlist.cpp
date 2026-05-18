#include "playlist.h"
#include <QFileInfo>
Playlist::Playlist(QObject *parent)
    : QAbstractListModel(parent) {
}
int Playlist::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_tracks.size();
}
QVariant Playlist::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_tracks.size())
        return {
        }
        ;
    const TrackItem &t = m_tracks[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:    return t.title;
    case ArtistRole:   return t.artist.isEmpty() ? QStringLiteral("未知艺术家") : t.artist;
    case DurationRole: return t.durationString();
    case FilePathRole: return t.filePath;
    default:           return {
        }
        ;
    }
}
void Playlist::addTrack(const TrackItem &track) {
    beginInsertRows( {
                    }
                    , m_tracks.size(), m_tracks.size());
    m_tracks.append(track);
    endInsertRows();
}
void Playlist::addTracks(const QList<TrackItem> &tracks) {
    if (tracks.isEmpty()) return;
    int first = m_tracks.size();
    beginInsertRows( {
                    }
                    , first, first + tracks.size() - 1);
    m_tracks.append(tracks);
    endInsertRows();
}
void Playlist::removeTrack(int idx) {
    if (idx < 0 || idx >= m_tracks.size()) return;
    beginRemoveRows( {
                    }
                    , idx, idx);
    m_tracks.removeAt(idx);
    endRemoveRows();
}
void Playlist::clear() {
    beginResetModel();
    m_tracks.clear();
    endResetModel();
}
TrackItem Playlist::track(int index) const {
    if (index < 0 || index >= m_tracks.size()) return {
        }
        ;
    return m_tracks[index];
}
void Playlist::updateDuration(int index, qint64 ms) {
    if (index < 0 || index >= m_tracks.size()) return;
    m_tracks[index].duration = ms;
    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {
                                   DurationRole
                               }
                     );
}