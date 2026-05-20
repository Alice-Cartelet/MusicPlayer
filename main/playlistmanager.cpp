#include "playlistmanager.h"
#include <QSettings>
const QString PlaylistManager::FAVORITES_NAME = QStringLiteral("喜欢");
PlaylistManager::PlaylistManager(QObject *parent) : QObject(parent)
{
    m_data[FAVORITES_NAME] =
        {
        }
    ;
    load();
}
QStringList PlaylistManager::playlistNames() const
{
    QStringList result;
    result << FAVORITES_NAME;
    for (const QString &name : m_order)
    {
        if (name != FAVORITES_NAME) result << name;
    }
    return result;
}
bool PlaylistManager::createPlaylist(const QString &name)
{
    if (name.isEmpty() || m_data.contains(name)) return false;
    m_data[name] =
        {
        }
    ;
    m_order << name;
    save();
    emit playlistsChanged();
    return true;
}
bool PlaylistManager::removePlaylist(const QString &name)
{
    if (name == FAVORITES_NAME || !m_data.contains(name)) return false;
    m_data.remove(name);
    m_order.removeAll(name);
    save();
    emit playlistsChanged();
    return true;
}
void PlaylistManager::addToPlaylist(const QString &playlistName, const QString &filePath)
{
    if (!m_data.contains(playlistName)) return;
    m_data[playlistName].insert(filePath);
    save();
    if (playlistName == FAVORITES_NAME) emit favoriteChanged(filePath, true);
    emit playlistsChanged();
}
void PlaylistManager::removeFromPlaylist(const QString &playlistName, const QString &filePath)
{
    if (!m_data.contains(playlistName)) return;
    m_data[playlistName].remove(filePath);
    save();
    if (playlistName == FAVORITES_NAME) emit favoriteChanged(filePath, false);
    emit playlistsChanged();
}
bool PlaylistManager::contains(const QString &playlistName, const QString &filePath) const
{
    auto it = m_data.find(playlistName);
    if (it == m_data.end()) return false;
    return it->contains(filePath);
}
QStringList PlaylistManager::tracks(const QString &playlistName) const
{
    auto it = m_data.find(playlistName);
    if (it == m_data.end()) return
            {
            }
        ;
    return QStringList(it->begin(), it->end());
}
bool PlaylistManager::isFavorite(const QString &filePath) const
{
    return contains(FAVORITES_NAME, filePath);
}
void PlaylistManager::toggleFavorite(const QString &filePath)
{
    if (isFavorite(filePath)) removeFromPlaylist(FAVORITES_NAME, filePath);
    else addToPlaylist(FAVORITES_NAME, filePath);
}
void PlaylistManager::save()
{
    QSettings s("MusicPlayer", "MusicPlayer");
    s.beginGroup("UserPlaylists");
    s.remove("");
    s.setValue("order", m_order);
    for (auto it = m_data.begin(); it != m_data.end(); ++it)
    {
        QString key = "pl_" + it.key();
        s.setValue(key, QStringList(it->begin(), it->end()));
    }
    s.endGroup();
}
void PlaylistManager::load()
{
    QSettings s("MusicPlayer", "MusicPlayer");
    s.beginGroup("UserPlaylists");
    m_order = s.value("order").toStringList();
    m_order.removeAll(FAVORITES_NAME);
    QStringList favTracks = s.value("pl_" + FAVORITES_NAME).toStringList();
    m_data[FAVORITES_NAME] = QSet<QString>(favTracks.begin(), favTracks.end());
    for (const QString &name : m_order)
    {
        QStringList trks = s.value("pl_" + name).toStringList();
        m_data[name] = QSet<QString>(trks.begin(), trks.end());
    }
    s.endGroup();
}