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
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    QString musicDir()  const;
    QString lyricsDir() const;
    bool    showLyrics() const;
    float   volume() const;
    bool    minimizeToTray() const;
    bool    hideOnHover() const;
    QString lyricColorSung() const;
    QString lyricColorUnsang() const;
    int     lyricFontSize() const;
    QString lyricFontFamily() const;
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
    void loadSavedFonts();   // 启动时重新加载已导入的字体文件
signals:
    void musicDirChanged(const QString &dir);
    void lyricsDirChanged(const QString &dir);
    void showLyricsChanged(bool v);
    void volumeChanged(float v);
    void minimizeToTrayChanged(bool v);
    void hideOnHoverChanged(bool v);
    void lyricColorsChanged(const QString &sung, const QString &unsang);
    void miniOpacityChanged(int);
    void miniControlChanged(bool);
    void lyricFontSizeChanged(int);
    void lyricFontFamilyChanged(const QString &family);
private:
    void applyStyle();
    void updateSwatch(QLabel *swatch, const QString &hex);
    QLineEdit   *m_edtMusic      = nullptr;
    QLineEdit   *m_edtLyrics     = nullptr;
    QCheckBox   *m_chkLyrics     = nullptr;
    QCheckBox   *m_chkTray       = nullptr;
    QCheckBox   *m_chkMiniControl= nullptr;
    QCheckBox   *m_chkHideHover  = nullptr;
    QPushButton *m_btnMusic      = nullptr;
    QPushButton *m_btnLyrics     = nullptr;
    QPushButton *m_btnOk         = nullptr;
    QPushButton *m_btnCancel     = nullptr;
    QSlider     *m_miniOpacitySlider = nullptr;
    QSlider     *m_lyricFontSlider   = nullptr;
    QSlider     *m_volSlider         = nullptr;
    QLineEdit   *m_edtColorSung  = nullptr;
    QLineEdit   *m_edtColorUnsang= nullptr;
    QLabel      *m_swatchSung    = nullptr;
    QLabel      *m_swatchUnsang  = nullptr;
    QFontComboBox *m_fontCombo     = nullptr;
    QPushButton   *m_btnImportFont = nullptr;
};