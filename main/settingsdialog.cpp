#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QSlider>
#include <QFileDialog>
#include <QApplication>
#include <QSettings>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
SettingsDialog::SettingsDialog(QWidget *parent): QDialog(parent)
{
    setWindowTitle("设置");
    setModal(true);
    setFixedSize(460, 410);
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 20, 18, 12);
    root->setSpacing(14);
    QFormLayout *form = new QFormLayout;
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QHBoxLayout *musicRow = new QHBoxLayout;
    m_edtMusic = new QLineEdit;
    m_edtMusic->setPlaceholderText("选择音乐文件夹…");
    m_edtMusic->setReadOnly(true);
    m_btnMusic = new QPushButton("浏览");
    m_btnMusic->setObjectName("browseBtn");
    m_btnMusic->setFixedSize(64, 32);
    musicRow->addWidget(m_edtMusic, 1);
    musicRow->addWidget(m_btnMusic);
    musicRow->setSpacing(12);
    form->addRow("音乐目录：", musicRow);
    QHBoxLayout *lyricsRow = new QHBoxLayout;
    m_edtLyrics = new QLineEdit;
    m_edtLyrics->setPlaceholderText("选择歌词文件夹（与音乐目录相同则留空）…");
    m_edtLyrics->setReadOnly(true);
    m_btnLyrics = new QPushButton("浏览");
    m_btnLyrics->setObjectName("browseBtn");
    m_btnLyrics->setFixedSize(64, 32);
    lyricsRow->addWidget(m_edtLyrics, 1);
    lyricsRow->addWidget(m_btnLyrics);
    lyricsRow->setSpacing(14);
    form->addRow("歌词目录：", lyricsRow);
    m_chkLyrics = new QCheckBox("启用桌面悬浮歌词");
    m_chkLyrics->setObjectName("settingsCheck");
    form->addRow("", m_chkLyrics);
    m_chkTray = new QCheckBox("最小化时最小化到系统托盘");
    m_chkTray->setObjectName("settingsCheck");
    form->addRow("", m_chkTray);
    m_chkHideHover = new QCheckBox("鼠标移到歌词上方时自动隐藏(开启后无法移动)");
    m_chkHideHover->setObjectName("settingsCheck");
    form->addRow("", m_chkHideHover);
    m_chkMiniControl = new QCheckBox("启用小控制窗");
    m_chkMiniControl->setObjectName("settingsCheck");
    form->addRow("", m_chkMiniControl);
    m_miniOpacitySlider =new QSlider(Qt::Horizontal);
    m_miniOpacitySlider->setObjectName("settingsSlider");
    m_miniOpacitySlider->setRange(20, 255);
    m_miniOpacitySlider->setValue(85);
    form->addRow("小窗透明度：",m_miniOpacitySlider);
    m_volSlider = new QSlider(Qt::Horizontal);
    m_volSlider->setObjectName("settingsSlider");
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(70);
    form->addRow("APP内音量：", m_volSlider);
    m_lyricFontSlider = new QSlider(Qt::Horizontal);
    m_lyricFontSlider->setObjectName("settingsSlider");
    m_lyricFontSlider->setRange(18, 60);
    m_lyricFontSlider->setValue(28);
    form->addRow("歌词字号：", m_lyricFontSlider);
    auto makeColorRow = [&](QLineEdit *&edt, QLabel *&swatch,const QString &defaultHex) -> QHBoxLayout* {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(6);
        row->setContentsMargins(0,0,0,0);
        edt = new QLineEdit(defaultHex);
        edt->setObjectName("colorEdit");
        edt->setMaxLength(7);
        edt->setFixedWidth(100);
        edt->setValidator(new QRegularExpressionValidator(QRegularExpression("^#[0-9A-Fa-f]{0,6}$"), edt));
        swatch = new QLabel;
        swatch->setObjectName("colorSwatch");
        swatch->setStyleSheet("margin-left:6px;");
        swatch->setFixedSize(30, 30);
        updateSwatch(swatch, defaultHex);
        row->addWidget(edt, 0, Qt::AlignVCenter);
        row->addSpacing(14);
        row->addWidget(swatch, 0, Qt::AlignVCenter);
        return row;
    };
    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->setSpacing(10);
    QLabel *lblSung = new QLabel("已读");
    lblSung->setMinimumWidth(36);
    QLabel *lblUnsang = new QLabel("未读");
    lblUnsang->setMinimumWidth(36);
    colorLayout->addWidget(lblSung);
    colorLayout->addLayout(makeColorRow( m_edtColorSung,m_swatchSung,"#E63248"));
    colorLayout->addSpacing(16);
    colorLayout->addWidget(lblUnsang);
    colorLayout->addLayout(makeColorRow(m_edtColorUnsang,m_swatchUnsang,"#F1DDDF"));
    form->addRow("歌词颜色：", colorLayout);
    root->addLayout(form);
   QLabel *aboutLabel = new QLabel( QString( "Version: %1<br>" "GitHub: github.com/Alice-Cartelet" ).arg(QApplication::applicationVersion()) );
    aboutLabel->setObjectName("aboutLabel");
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    aboutLabel->setOpenExternalLinks(false);
    aboutLabel->setTextFormat(Qt::RichText);
    root->addStretch();
    QHBoxLayout *bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0,0,0,0);
    QVBoxLayout *aboutWrap = new QVBoxLayout;
    aboutWrap->setSpacing(0);
    aboutWrap->addStretch();
    aboutWrap->addWidget(aboutLabel,0,Qt::AlignRight | Qt::AlignBottom);
    bottomRow->addLayout(aboutWrap);
    bottomRow->addStretch();
    QHBoxLayout *btns = new QHBoxLayout;
    bottomRow->addLayout(btns);
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
    root->addLayout(bottomRow);
    applyStyle();
    connect(m_btnMusic, &QPushButton::clicked, this, [this] {
        QString d = QFileDialog::getExistingDirectory(this, "选择音乐目录", m_edtMusic->text());
        if (!d.isEmpty()) m_edtMusic->setText(d);
    });
    connect(m_btnLyrics, &QPushButton::clicked, this, [this] {
        QString d = QFileDialog::getExistingDirectory(this, "选择歌词目录", m_edtLyrics->text());
        if (!d.isEmpty()) m_edtLyrics->setText(d);
    });
    connect(m_edtColorSung, &QLineEdit::textChanged, this, [this](const QString &t) {
        updateSwatch(m_swatchSung, t);
    });
    connect(m_edtColorUnsang, &QLineEdit::textChanged, this, [this](const QString &t) {
        updateSwatch(m_swatchUnsang, t);
    });
    connect(m_btnOk, &QPushButton::clicked, this, [this] {
        QSettings s("MusicPlayer", "MusicPlayer");
        QString oldMusicDir  = s.value("musicDir").toString();
        QString oldLyricsDir = s.value("lyricsDir").toString();
        bool oldShowLyrics   = s.value("showLyrics", false).toBool();
        int  oldVolume       = s.value("volume", 70).toInt();
        int oldMiniOpacity   = s.value("miniOpacity",140).toInt();
        bool oldTray         = s.value("minimizeToTray", false).toBool();
        bool oldMini         = s.value("enableMiniControl", true).toBool();
        bool oldHideHover    = s.value("hideOnHover", false).toBool();
        QString oldSung      = s.value("lyricColorSung",   "#E63248").toString();
        QString oldUnsang    = s.value("lyricColorUnsang", "#F1DDDF").toString();
        int oldFontSize = s.value("lyricFontSize", 28).toInt();
        QString newMusicDir  = m_edtMusic->text();
        QString newLyricsDir = m_edtLyrics->text();
        bool newShowLyrics   = m_chkLyrics->isChecked();
        int  newVolume       = m_volSlider->value();
        int newMiniOpacity   = m_miniOpacitySlider->value();
        bool newTray         = m_chkTray->isChecked();
        bool newMini         = m_chkMiniControl->isChecked();
        bool newHideHover    = m_chkHideHover->isChecked();
        QString newSung      = m_edtColorSung->text();
        QString newUnsang    = m_edtColorUnsang->text();
        int newFontSize = m_lyricFontSlider->value();
        if (newSung.length()   != 7) newSung   = oldSung;
        if (newUnsang.length() != 7) newUnsang = oldUnsang;
        s.setValue("musicDir",         newMusicDir);
        s.setValue("lyricsDir",        newLyricsDir);
        s.setValue("showLyrics",       newShowLyrics);
        s.setValue("volume",           newVolume);
        s.setValue("miniOpacity",      newMiniOpacity);
        s.setValue("minimizeToTray",   newTray);
        s.setValue("enableMiniControl",newMini);
        s.setValue("hideOnHover",      newHideHover);
        s.setValue("lyricColorSung",   newSung);
        s.setValue("lyricColorUnsang", newUnsang);
        s.setValue("lyricFontSize",    newFontSize);
        if (oldMusicDir   != newMusicDir)   emit musicDirChanged(newMusicDir);
        if (oldLyricsDir  != newLyricsDir)  emit lyricsDirChanged(newLyricsDir);
        if (oldShowLyrics != newShowLyrics) emit showLyricsChanged(newShowLyrics);
        if (oldVolume     != newVolume)     emit volumeChanged(newVolume / 100.f);
        if (oldMiniOpacity!= newMiniOpacity)emit miniOpacityChanged(newMiniOpacity);
        if (oldTray       != newTray)       emit minimizeToTrayChanged(newTray);
        if (oldMini       != newMini)       emit miniControlChanged(newMini);
        if (oldHideHover  != newHideHover)  emit hideOnHoverChanged(newHideHover);
        if (oldSung != newSung || oldUnsang != newUnsang)  emit lyricColorsChanged(newSung, newUnsang);
        if (oldFontSize != newFontSize)     emit lyricFontSizeChanged(newFontSize);
        accept();
    });
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    QSettings s("MusicPlayer", "MusicPlayer");
    m_edtMusic->setText(s.value("musicDir").toString());
    m_edtLyrics->setText(s.value("lyricsDir").toString());
    m_chkLyrics->setChecked(s.value("showLyrics", false).toBool());
    m_miniOpacitySlider->setValue( s.value("miniOpacity",140).toInt());
    m_volSlider->setValue(s.value("volume", 70).toInt());
    m_lyricFontSlider->setValue(s.value("lyricFontSize", 28).toInt());
    m_chkTray->setChecked(s.value("minimizeToTray", false).toBool());
    m_chkMiniControl->setChecked(s.value("enableMiniControl",true).toBool());
    m_chkHideHover->setChecked(s.value("hideOnHover", false).toBool());
    QString sungColor =s.value("lyricColorSung","#E63248").toString();
    QString unsangColor =s.value("lyricColorUnsang","#F1DDDF").toString();
    m_edtColorSung->setText(sungColor);
    m_edtColorUnsang->setText(unsangColor);
    updateSwatch(m_swatchSung,sungColor);
    updateSwatch(m_swatchUnsang,unsangColor);
}
QString SettingsDialog::musicDir()         const { return m_edtMusic->text(); }
QString SettingsDialog::lyricsDir()        const { return m_edtLyrics->text(); }
bool    SettingsDialog::showLyrics()       const { return m_chkLyrics->isChecked(); }
float   SettingsDialog::volume()           const { return m_volSlider->value() / 100.f; }
bool    SettingsDialog::minimizeToTray()   const { return m_chkTray->isChecked(); }
bool    SettingsDialog::hideOnHover()      const { return m_chkHideHover->isChecked(); }
QString SettingsDialog::lyricColorSung()   const { return m_edtColorSung->text(); }
QString SettingsDialog::lyricColorUnsang() const { return m_edtColorUnsang->text(); }
void SettingsDialog::setMusicDir(const QString &d)       { m_edtMusic->setText(d); }
void SettingsDialog::setLyricsDir(const QString &d)      { m_edtLyrics->setText(d); }
void SettingsDialog::setShowLyrics(bool v)               { m_chkLyrics->setChecked(v); }
void SettingsDialog::setVolume(float v)                  { m_volSlider->setValue(qRound(v * 100)); }
void SettingsDialog::setMinimizeToTray(bool v)           { m_chkTray->setChecked(v); }
void SettingsDialog::setHideOnHover(bool v)              { m_chkHideHover->setChecked(v); }
void SettingsDialog::setLyricColorSung(const QString &h) {m_edtColorSung->setText(h);updateSwatch(m_swatchSung,h);}
void SettingsDialog::setLyricColorUnsang(const QString &h) {m_edtColorUnsang->setText(h);updateSwatch(m_swatchUnsang,h);}
void SettingsDialog::updateSwatch(QLabel *swatch, const QString &hex) {
    QColor c(hex);
    if (c.isValid())
        swatch->setStyleSheet(QString("background:%1;border-radius:4px;border:1px solid #3a3860;").arg(hex));
    else
        swatch->setStyleSheet("background:#2a2a40;border-radius:4px;border:1px solid #3a3860;");
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
QLabel#aboutLabel{
    color: rgba(180,180,210,60);
    font-size: 9px;
    line-height: 12px;
}

QLabel#aboutLabel a{
    color: rgba(180,180,210,60);
    text-decoration:none;
}

QLabel#aboutLabel a:hover{
    color: rgba(180,180,210,100);
}
QLineEdit {
    background: #1c1f30;
    border: 1px solid #2a2e50;
    border-radius: 8px;
    color: #c0bde0;
    min-height: 28px;
    max-height: 28px;
    padding-left: 12px;
    padding-right: 12px;
    font-size: 14px;
}
QLineEdit#colorEdit {
    min-height: 24px;
    max-height: 24px;
    padding: 2px 4px 2px 2px;
    font-family: "Consolas", "Courier New", monospace;
    font-size: 12px;
    letter-spacing: 1px;
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
int SettingsDialog::lyricFontSize() const {
    return m_lyricFontSlider->value();
}

void SettingsDialog::setLyricFontSize(int v) {
    m_lyricFontSlider->setValue(v);
}