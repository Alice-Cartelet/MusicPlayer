#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFile>
#include <QPoint>
#include <QComboBox>
#include "desktopwallpaper.h"
class SettingsDialog : public QDialog
{
    Q_OBJECT
public: explicit SettingsDialog(QWidget *parent=nullptr);
    QString musicDir() const;
    QString lyricsDir() const;
    bool showLyrics() const;
    float volume() const;
    bool minimizeToTray() const;
    bool hideOnHover() const;
    QString lyricColorSung() const;
    QString lyricColorUnsang() const;
    int lyricFontSize() const;
    QString lyricFontFamily() const;
    bool wallpaperLyricsEnabled() const;
    LyricOrientation wallpaperOrientation() const;
    QPoint wallpaperPosition() const;
    QString wallpaperColorCurrent() const;
    int wallpaperFontSize() const;
    int wallpaperTitleFontSize() const;
    int wallpaperMaxHeightPercent() const;
    void setWallpaperMaxHeightPercent(int pct);
    void setMusicDir(const QString &d);
    void setLyricsDir(const QString &d);
    void setShowLyrics(bool v);
    void setVolume(float v);
    void setMinimizeToTray(bool v);
    void setHideOnHover(bool v);
    void setLyricColorSung(const QString &hex);
    void setLyricColorUnsang(const QString &hex);
    void setLyricFontSize(int v);
    void setLyricFontFamily(const QString &family);
    void setWallpaperLyricsEnabled(bool v);
    void setWallpaperOrientation( LyricOrientation o);
    void setWallpaperPosition( QPoint p);
    void setWallpaperColorCurrent( const QString &hex);
    void setWallpaperFontSize(int v);
    void setWallpaperTitleFontSize(int v);
    void loadSavedFonts();
signals: void musicDirChanged( const QString &dir);
    void lyricsDirChanged( const QString &dir);
    void showLyricsChanged( bool v);
    void volumeChanged( float v);
    void minimizeToTrayChanged( bool v);
    void hideOnHoverChanged( bool v);
    void lyricColorsChanged( const QString &sung, const QString &unsang);
    void miniOpacityChanged( int);
    void miniControlChanged( bool);
    void lyricFontSizeChanged( int);
    void lyricFontFamilyChanged( const QString &family);
    void wallpaperOpacityChanged( int);
    void wallpaperLyricsEnabledChanged( bool on);
    void wallpaperOrientationChanged( LyricOrientation o);
    void wallpaperPositionChanged( QPoint p);
    void wallpaperColorCurrentChanged( const QString &hex);
    void wallpaperFontSizeChanged(int px);
    void wallpaperTitleFontSizeChanged(int px);
    void wallpaperMaxHeightPercentChanged(int pct);
private: void applyStyle();
    void updateSwatch( QLabel *swatch, const QString &hex);
    QLineEdit *m_edtMusic=nullptr;
    QLineEdit *m_edtLyrics=nullptr;
    QPushButton *m_btnMusic=nullptr;
    QPushButton *m_btnLyrics=nullptr;
    QPushButton *m_btnOk=nullptr;
    QPushButton *m_btnCancel=nullptr;
    QCheckBox *m_chkLyrics=nullptr;
    QCheckBox *m_chkTray=nullptr;
    QCheckBox *m_chkMiniControl=nullptr;
    QCheckBox *m_chkHideHover=nullptr;
    QSlider *m_volSlider=nullptr;
    QSlider *m_miniOpacitySlider=nullptr;
    QSlider *m_lyricFontSlider=nullptr;
    QSlider *m_wallOpacitySlider=nullptr;
    QLineEdit *m_edtColorSung=nullptr;
    QLineEdit *m_edtColorUnsang=nullptr;
    QLabel *m_swatchSung=nullptr;
    QLabel *m_swatchUnsang=nullptr;
    QFontComboBox *m_fontCombo=nullptr;
    QPushButton *m_btnImportFont=nullptr;
    QFontComboBox *m_wallFontCombo=nullptr;
    QCheckBox *m_chkWallpaper=nullptr;
    QComboBox *m_cmbOrientation=nullptr;
    QPushButton *m_btnPickPosition=nullptr;
    QLabel      *m_lblWallPos=nullptr;
    QPoint m_wallPos=
        {
            800,500
        }
    ;
    QLineEdit *m_edtWallColorCurr=nullptr;
    QLabel *m_swatchWallCurr=nullptr;
    QSlider *m_wallFontSlider=nullptr;
    QSlider *m_wallTitleFontSlider=nullptr;
    QSlider *m_wallMaxHeightSlider=nullptr;
    QLabel  *m_lblWallMaxHeight=nullptr;
}
;