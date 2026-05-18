#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QSlider>
#include <QFileDialog>
#include <QSettings>
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("设置");
    setModal(true);
    setFixedSize(480, 300);
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 20);
    root->setSpacing(16);
    QLabel *title = new QLabel("设置");
    title->setObjectName("dlgTitle");
    root->addWidget(title);
    QFormLayout *form = new QFormLayout;
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QHBoxLayout *musicRow = new QHBoxLayout;
    m_edtMusic  = new QLineEdit;
    m_edtMusic->setPlaceholderText("选择音乐文件夹…");
    m_edtMusic->setReadOnly(true);
    m_btnMusic  = new QPushButton("浏览");
    m_btnMusic->setObjectName("browseBtn");
    m_btnMusic->setFixedWidth(60);
    musicRow->addWidget(m_edtMusic, 1);
    musicRow->addWidget(m_btnMusic);
    form->addRow("音乐目录：", musicRow);
    QHBoxLayout *lyricsRow = new QHBoxLayout;
    m_edtLyrics  = new QLineEdit;
    m_edtLyrics->setPlaceholderText("选择歌词文件夹（与音乐目录相同则留空）…");
    m_edtLyrics->setReadOnly(true);
    m_btnLyrics  = new QPushButton("浏览");
    m_btnLyrics->setObjectName("browseBtn");
    m_btnLyrics->setFixedWidth(60);
    lyricsRow->addWidget(m_edtLyrics, 1);
    lyricsRow->addWidget(m_btnLyrics);
    form->addRow("歌词目录：", lyricsRow);
    m_chkLyrics = new QCheckBox("启用桌面悬浮歌词");
    m_chkLyrics->setObjectName("settingsCheck");
    form->addRow("", m_chkLyrics);
    m_volSlider = new QSlider(Qt::Horizontal);
    m_volSlider->setObjectName("settingsSlider");
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(70);
    form->addRow("默认音量：", m_volSlider);
    root->addLayout(form);
    root->addStretch();
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    m_btnCancel = new QPushButton("取消");
    m_btnOk     = new QPushButton("确定");
    m_btnCancel->setObjectName("dlgBtn");
    m_btnOk->setObjectName("dlgBtnPrimary");
    m_btnCancel->setFixedSize(80, 32);
    m_btnOk->setFixedSize(80, 32);
    btns->addWidget(m_btnCancel);
    btns->addSpacing(8);
    btns->addWidget(m_btnOk);
    root->addLayout(btns);
    applyStyle();
    connect(m_btnMusic, &QPushButton::clicked, this, [this] {
        QString d = QFileDialog::getExistingDirectory(this, "选择音乐目录", m_edtMusic->text());
        if (!d.isEmpty()) m_edtMusic->setText(d);
    }
            );
    connect(m_btnLyrics, &QPushButton::clicked, this, [this] {
        QString d = QFileDialog::getExistingDirectory(this, "选择歌词目录", m_edtLyrics->text());
        if (!d.isEmpty()) m_edtLyrics->setText(d);
    }
            );
    connect(m_btnOk, &QPushButton::clicked, this, [this] {
        QSettings s("MusicPlayer", "MusicPlayer");
        QString oldMusicDir  = s.value("musicDir").toString();
        QString oldLyricsDir = s.value("lyricsDir").toString();
        bool oldShowLyrics   = s.value("showLyrics", false).toBool();
        int oldVolume        = s.value("volume", 70).toInt();
        QString newMusicDir  = m_edtMusic->text();
        QString newLyricsDir = m_edtLyrics->text();
        bool newShowLyrics   = m_chkLyrics->isChecked();
        int newVolume        = m_volSlider->value();
        s.setValue("musicDir",   newMusicDir);
        s.setValue("lyricsDir",  newLyricsDir);
        s.setValue("showLyrics", newShowLyrics);
        s.setValue("volume",     newVolume);
        if(oldMusicDir != newMusicDir)
            emit musicDirChanged(newMusicDir);
        if(oldLyricsDir != newLyricsDir)
            emit lyricsDirChanged(newLyricsDir);
        if(oldShowLyrics != newShowLyrics)
            emit showLyricsChanged(newShowLyrics);
        if(oldVolume != newVolume)
            emit volumeChanged(newVolume / 100.0f);
        accept();
    }
            );
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    QSettings s("MusicPlayer", "MusicPlayer");
    m_edtMusic->setText(s.value("musicDir").toString());
    m_edtLyrics->setText(s.value("lyricsDir").toString());
    m_chkLyrics->setChecked(s.value("showLyrics", false).toBool());
    m_volSlider->setValue(s.value("volume", 70).toInt());
}
QString SettingsDialog::musicDir()  const {
    return m_edtMusic->text();
}
QString SettingsDialog::lyricsDir() const {
    return m_edtLyrics->text();
}
bool    SettingsDialog::showLyrics() const {
    return m_chkLyrics->isChecked();
}
float   SettingsDialog::volume()    const {
    return m_volSlider->value() / 100.0f;
}
void SettingsDialog::setMusicDir(const QString &d) {
    m_edtMusic->setText(d);
}
void SettingsDialog::setLyricsDir(const QString &d) {
    m_edtLyrics->setText(d);
}
void SettingsDialog::setShowLyrics(bool v) {
    m_chkLyrics->setChecked(v);
}
void SettingsDialog::setVolume(float v) {
    m_volSlider->setValue(qRound(v * 100));
}
void SettingsDialog::applyStyle() {
    setStyleSheet(R"(
QDialog {
    background: #13151f;
    color: #c8c5e0;
    font-family: "Microsoft YaHei", "PingFang SC", sans-serif;
}
QLabel#dlgTitle {
    font-size: 18px;
    font-weight: bold;
    color: #e0ddf8;
    margin-bottom: 4px;
}
QLabel {
    color: #9890c0;
    font-size: 13px;
}
QLineEdit {
    background: #1c1f30;
    border: 1px solid #2a2e50;
    border-radius: 6px;
    color: #c0bde0;
    padding: 5px 10px;
    font-size: 12px;
}
QLineEdit:focus { border-color: #5a50b0; }
QPushButton#browseBtn {
    background: #22263a;
    border: 1px solid #2e3258;
    border-radius: 6px;
    color: #9090c0;
    font-size: 12px;
    padding: 5px;
}
QPushButton#browseBtn:hover { background: #2a2f4a; color: #c0b8f0; }
QCheckBox#settingsCheck {
    color: #a8a0d0;
    font-size: 13px;
    spacing: 8px;
}
QCheckBox#settingsCheck::indicator {
    width: 16px; height: 16px;
    border: 1px solid #3a3860;
    border-radius: 4px;
    background: #1c1f30;
}
QCheckBox#settingsCheck::indicator:checked {
    background: #6050c0;
    border-color: #7060d8;
}
QSlider#settingsSlider::groove:horizontal {
    height: 4px; background: #252840; border-radius: 2px;
}
QSlider#settingsSlider::sub-page:horizontal {
    background: #5a50c0; border-radius: 2px;
}
QSlider#settingsSlider::handle:horizontal {
    width: 12px; height: 12px; margin: -4px 0;
    background: #a090f0; border-radius: 6px;
}
QPushButton#dlgBtn {
    background: #1e2138;
    border: 1px solid #2e3258;
    border-radius: 8px;
    color: #9090c0;
    font-size: 13px;
}
QPushButton#dlgBtn:hover { background: #262a42; color: #c0b8f0; }
QPushButton#dlgBtnPrimary {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
        stop:0 #6050c8, stop:1 #4838a0);
    border: none;
    border-radius: 8px;
    color: white;
    font-size: 13px;
    font-weight: bold;
}
QPushButton#dlgBtnPrimary:hover { background: #7060d8; }
)");
}