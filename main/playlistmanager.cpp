#include "playlistmanager.h"
#include <QSettings>
const QString PlaylistManager::FAVORITES_NAME="喜欢";
PlaylistManager:: PlaylistManager(QObject *parent) : QObject(parent)
{
    m_data[FAVORITES_NAME]=
        {
        }
    ;
    load();
}
QStringList PlaylistManager:: playlistNames() const
{
    QStringList r;
    r<<FAVORITES_NAME;
    for(const auto& n:m_order)
    {
        if(n!=FAVORITES_NAME) r<<n;
    }
    return r;
}
bool PlaylistManager:: createPlaylist(const QString &name)
{
    if(name.isEmpty()|| m_data.contains(name)) return false;
    m_data[name]=
        {
        }
    ;
    m_order<<name;
    save();
    emit playlistsChanged();
    return true;
}
bool PlaylistManager:: removePlaylist(const QString &name)
{
    if(name==FAVORITES_NAME) return false;
    if(!m_data.contains(name)) return false;
    m_data.remove(name);
    m_order.removeAll(name);
    save();
    emit playlistsChanged();
    return true;
}
bool PlaylistManager:: renamePlaylist( const QString &oldName, const QString &newName)
{
    if(oldName==FAVORITES_NAME) return false;
    if(newName.isEmpty()) return false;
    if(m_data.contains(newName)) return false;
    if(!m_data.contains(oldName)) return false;
    m_data[newName]=m_data.take(oldName);
    int idx=m_order.indexOf(oldName);
    if(idx>=0) m_order[idx]=newName;
    save();
    emit playlistsChanged();
    return true;
}
void PlaylistManager:: addToPlaylist( const QString &playlistName, const QString &filePath)
{
    if(!m_data.contains( playlistName)) return;
    auto &list= m_data[playlistName];
    if(list.contains(filePath)) return;
    list.append(filePath);
    save();
    if(playlistName== FAVORITES_NAME)
    {
        emit favoriteChanged( filePath,true);
    }
    emit playlistsChanged();
}
void PlaylistManager:: removeFromPlaylist( const QString &playlistName, const QString &filePath)
{
    if(!m_data.contains( playlistName)) return;
    m_data[ playlistName] .removeAll(filePath);
    save();
    if(playlistName== FAVORITES_NAME)
    {
        emit favoriteChanged( filePath,false);
    }
    emit playlistsChanged();
}
bool PlaylistManager:: contains( const QString &playlistName, const QString &filePath) const
{
    return m_data .value(playlistName) .contains(filePath);
}
QStringList PlaylistManager:: tracks( const QString &playlistName) const
{
    return m_data.value( playlistName);
}
bool PlaylistManager:: isFavorite( const QString &filePath) const
{
    return contains( FAVORITES_NAME, filePath);
}
void PlaylistManager:: toggleFavorite( const QString &filePath)
{
    if(isFavorite(filePath))
    {
        removeFromPlaylist( FAVORITES_NAME, filePath);
    }
    else
    {
        addToPlaylist( FAVORITES_NAME, filePath);
    }
}
void PlaylistManager:: setTracks( const QString& name, const QStringList& tracks)
{
    m_data[name]=tracks;
    save();
    emit playlistsChanged();
}
void PlaylistManager:: save()
{
    QSettings s( "MusicPlayer", "MusicPlayer");
    s.beginGroup( "UserPlaylists");
    s.remove("");
    s.setValue( "order", m_order);
    for(auto it= m_data.begin();
         it!=m_data.end();
         ++it)
    {
        s.setValue( "pl_"+it.key(), it.value());
    }
    s.endGroup();
}
void PlaylistManager:: load()
{
    QSettings s( "MusicPlayer", "MusicPlayer");
    s.beginGroup( "UserPlaylists");
    m_order= s.value( "order") .toStringList();
    m_order.removeAll( FAVORITES_NAME);
    QStringList fav= s.value( "pl_喜欢") .toStringList();
    m_data[ FAVORITES_NAME ]=fav;
    for(auto &name:m_order)
    {
        m_data[name]= s.value( "pl_"+name) .toStringList();
    }
    s.endGroup();
}