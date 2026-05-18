#pragma once
#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QListView>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include "playlist.h"
#include "lyricsoverlay.h"
#include "settingsdialog.h"
class MarqueeLabel;
class CoverLabel;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void closeEvent(QCloseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
private slots:
    void onPlayPause();
    void onPrevious();
    void onNext();
    void onPlaylistDoubleClicked(const QModelIndex &idx);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus s);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState s);
    void onDurationChanged(qint64 d);
    void onPositionChanged(qint64 p);
    void onMediaErrorOccurred(QMediaPlayer::Error error,const QString &errorString);
    void onOpenSettings();
    void onMusicDirChanged(const QString &dir);
    void onLyricsDirChanged(const QString &dir);
private:
    void setupPlayer();
    void setupUI();
    void applyStyleSheet();
    void savePlaybackState();
    void restorePlaybackState();
    void loadDir(const QString &dir);
    void loadFiles(const QStringList &paths);
    void playTrack(int index);
    int nextTrackIndex() const;
    QString formatTime(qint64 ms) const;
    QString findLyricFile(const QString &audioPath) const;
    void updateTitleBarTitle(const QString &title);
private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
    Playlist *m_playlist = nullptr;
    int m_currentIndex = -1;
    bool m_seeking = false;
    enum PlayMode {Sequential,RepeatOne,Shuffle};
    PlayMode m_playMode = Sequential;
    QString m_musicDir;
    QString m_lyricsDir;
    bool m_showLyrics = false;
    QWidget *m_bgCover = nullptr;
    QWidget    *m_titleBar   = nullptr;
    QLabel     *m_lblWinTitle = nullptr;
    QPushButton *m_btnMin    = nullptr;
    QPushButton *m_btnClose  = nullptr;
    bool   m_dragging    = false;
    QPoint m_dragOffset;
    CoverLabel *m_lblCover = nullptr;
    MarqueeLabel *m_lblTitle = nullptr;
    QLabel *m_lblArtist = nullptr;
    QLabel *m_lblTimeElapsed = nullptr;
    QLabel *m_lblTimeTotal = nullptr;
    QSlider *m_seekBar = nullptr;
    QPushButton *m_btnPrev = nullptr;
    QPushButton *m_btnPlayPause = nullptr;
    QPushButton *m_btnNext = nullptr;
    QListView *m_listView = nullptr;
    QLabel *m_lblStatus = nullptr;
    QPushButton *m_btnSettings = nullptr;
    QPushButton *m_btnShuffle = nullptr;
    LyricsOverlay *m_lyricsOverlay = nullptr;
    SettingsDialog *m_settingsDlg = nullptr;
}
;