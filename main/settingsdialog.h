#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    QString musicDir()  const;
    QString lyricsDir() const;
    bool    showLyrics() const;
    float   volume() const;
    void setMusicDir(const QString &d);
    void setLyricsDir(const QString &d);
    void setShowLyrics(bool v);
    void setVolume(float v);
signals:
    void musicDirChanged(const QString &dir);
    void lyricsDirChanged(const QString &dir);
    void showLyricsChanged(bool v);
    void volumeChanged(float v);
private:
    QLineEdit   *m_edtMusic   = nullptr;
    QLineEdit   *m_edtLyrics  = nullptr;
    QCheckBox   *m_chkLyrics  = nullptr;
    QPushButton *m_btnMusic   = nullptr;
    QPushButton *m_btnLyrics  = nullptr;
    QPushButton *m_btnOk      = nullptr;
    QPushButton *m_btnCancel  = nullptr;
    class QSlider *m_volSlider = nullptr;
    void applyStyle();
}
;