#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include "positionpicker.h"
#include <QSettings>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QFileDialog>
#include <QApplication>
#include <QSettings>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QMessageBox>
#include <QScrollArea>
#include <QWheelEvent>
#include <QComboBox>
#include <QFontComboBox>

namespace {
class NoWheelSlider : public QSlider
{
public:
    using QSlider::QSlider;

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore();
    }
};

class NoWheelFontComboBox : public QFontComboBox
{
public:
    using QFontComboBox::QFontComboBox;

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore();
    }
};

class NoWheelComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore();
    }
};
}

SettingsDialog::SettingsDialog(QWidget *parent): QDialog(parent)
{
    setWindowTitle("设置");
    setModal(true);
    resize(597,430);
    //setMinimumSize(760,680);

    // ── 滚动容器 ──────────────────────────────────────────────
    QVBoxLayout *rootOuter = new QVBoxLayout(this);
    rootOuter->setContentsMargins(0, 0, 0, 0);
    rootOuter->setSpacing(0);
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rootOuter->addWidget(scroll);
    QWidget *inner = new QWidget;
    scroll->setWidget(inner);
    QVBoxLayout *root = new QVBoxLayout(inner);
    root->setContentsMargins(18, 20, 18, 12);
    root->setSpacing(16);

    // ── 颜色行辅助 lambda ─────────────────────────────────────
    auto makeColorRow = [&](QLineEdit *&edt, QLabel *&swatch, const QString &defaultHex) -> QHBoxLayout*
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(6);
        row->setContentsMargins(0,0,0,0);
        edt = new QLineEdit(defaultHex);
        edt->setObjectName("colorEdit");
        edt->setMaxLength(30);
        edt->setFixedWidth(120);
        edt->setValidator(nullptr);
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

    // ══════════════════════════════════════════════════════════
    // 分组一：常规
    // ══════════════════════════════════════════════════════════
    QGroupBox *grpGeneral = new QGroupBox("⚙️  常规");
    grpGeneral->setObjectName("settingsGroup");
    QFormLayout *genForm = new QFormLayout(grpGeneral);
    genForm->setSpacing(12);
    genForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 音乐目录
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
    genForm->addRow("音乐目录：", musicRow);

    // 歌词目录
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
    genForm->addRow("歌词目录：", lyricsRow);

    // 音量
    m_volSlider = new NoWheelSlider(Qt::Horizontal);
    m_volSlider->setObjectName("settingsSlider");
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(70);
    genForm->addRow("APP 内音量：", m_volSlider);

    // 行为开关
    m_chkTray = new QCheckBox("最小化时缩到系统托盘");
    m_chkTray->setObjectName("settingsCheck");
    genForm->addRow("", m_chkTray);

    m_chkMiniControl = new QCheckBox("启用小控制窗");
    m_chkMiniControl->setObjectName("settingsCheck");
    genForm->addRow("", m_chkMiniControl);

    m_miniOpacitySlider = new NoWheelSlider(Qt::Horizontal);
    m_miniOpacitySlider->setObjectName("settingsSlider");
    m_miniOpacitySlider->setRange(20, 255);
    m_miniOpacitySlider->setValue(85);
    genForm->addRow("小窗透明度：", m_miniOpacitySlider);

    root->addWidget(grpGeneral);

    // ══════════════════════════════════════════════════════════
    // 分组二：悬浮歌词
    // ══════════════════════════════════════════════════════════
    QGroupBox *grpLyric = new QGroupBox("🎵  悬浮歌词");
    grpLyric->setObjectName("settingsGroup");
    QFormLayout *lyricForm = new QFormLayout(grpLyric);
    lyricForm->setSpacing(12);
    lyricForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_chkLyrics = new QCheckBox("启用桌面悬浮歌词");
    m_chkLyrics->setObjectName("settingsCheck");
    lyricForm->addRow("", m_chkLyrics);

    m_chkHideHover = new QCheckBox("鼠标悬停时自动隐藏（开启后无法拖动）");
    m_chkHideHover->setObjectName("settingsCheck");
    lyricForm->addRow("", m_chkHideHover);

    // 歌词字号
    m_lyricFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_lyricFontSlider->setObjectName("settingsSlider");
    m_lyricFontSlider->setRange(18, 60);
    m_lyricFontSlider->setValue(28);
    lyricForm->addRow("歌词字号：", m_lyricFontSlider);

    // 歌词字体
    loadSavedFonts();
    QHBoxLayout *fontRow = new QHBoxLayout;
    fontRow->setSpacing(8);
    fontRow->setContentsMargins(0,0,0,0);
    m_fontCombo = new NoWheelFontComboBox;
    m_fontCombo->setObjectName("fontCombo");
    m_fontCombo->setEditable(false);
    m_fontCombo->setCurrentFont(QFont("Microsoft YaHei"));
    {
        QString naikaiPath = QCoreApplication::applicationDirPath() + "/naikai.ttf";
        if (QFile::exists(naikaiPath))
        {
            int id = QFontDatabase::addApplicationFont(naikaiPath);
            QStringList fams = QFontDatabase::applicationFontFamilies(id);
            if (!fams.isEmpty()) m_fontCombo->setCurrentFont(QFont(fams.first()));
        }
    }
    m_btnImportFont = new QPushButton("导入字体");
    m_btnImportFont->setObjectName("browseBtn");
    m_btnImportFont->setFixedSize(72, 32);
    fontRow->addWidget(m_fontCombo, 1);
    fontRow->addWidget(m_btnImportFont);
    lyricForm->addRow("歌词字体：", fontRow);

    // 歌词颜色（已读 + 未读并排）
    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->setSpacing(10);
    QLabel *lblSung = new QLabel("已读");
    lblSung->setMinimumWidth(36);
    QLabel *lblUnsang = new QLabel("未读");
    lblUnsang->setMinimumWidth(36);
    colorLayout->addWidget(lblSung);
    colorLayout->addLayout(makeColorRow(m_edtColorSung, m_swatchSung, "#E63248"));
    colorLayout->addSpacing(16);
    colorLayout->addWidget(lblUnsang);
    colorLayout->addLayout(makeColorRow(m_edtColorUnsang, m_swatchUnsang, "#F1DDDF"));
    lyricForm->addRow("歌词颜色：", colorLayout);

    root->addWidget(grpLyric);

    // ══════════════════════════════════════════════════════════
    // 分组三：壁纸歌词
    // ══════════════════════════════════════════════════════════
    QGroupBox *grpWall = new QGroupBox("🖼  壁纸歌词");
    grpWall->setObjectName("wallGroup");
    QFormLayout *wallForm = new QFormLayout(grpWall);
    wallForm->setSpacing(10);
    wallForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_chkWallpaper = new QCheckBox("启用壁纸歌词（将歌词显示于桌面壁纸）");
    m_chkWallpaper->setObjectName("settingsCheck");
    wallForm->addRow("", m_chkWallpaper);
    m_cmbOrientation = new NoWheelComboBox;
    m_cmbOrientation->setObjectName("wallCombo");
    m_cmbOrientation->addItem("横排", static_cast<int>(LyricOrientation::Horizontal));
    m_cmbOrientation->addItem("竖排", static_cast<int>(LyricOrientation::Vertical));
    wallForm->addRow("排列方式：", m_cmbOrientation);
    m_btnPickPosition = new QPushButton("选择桌面位置");
    m_btnPickPosition->setObjectName("browseBtn");
    m_lblWallPos = new QLabel(
        QString("X:%1  Y:%2").arg(m_wallPos.x()).arg(m_wallPos.y()));
    m_lblWallPos->setObjectName("wallPosLabel");
    {
        QHBoxLayout *posRow = new QHBoxLayout;
        posRow->setSpacing(10);
        posRow->addWidget(m_btnPickPosition);
        posRow->addWidget(m_lblWallPos);
        posRow->addStretch();
        wallForm->addRow("显示位置：", posRow);
    }
    m_wallFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_wallFontSlider->setObjectName("settingsSlider");
    m_wallFontSlider->setRange(18, 80);
    m_wallFontSlider->setValue(36);
    wallForm->addRow("歌词字号：", m_wallFontSlider);
    m_wallTitleFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_wallTitleFontSlider->setObjectName("settingsSlider");
    m_wallTitleFontSlider->setRange(10, 60);
    m_wallTitleFontSlider->setValue(22);
    wallForm->addRow("歌名字号：", m_wallTitleFontSlider);
    m_wallOpacitySlider = new NoWheelSlider(Qt::Horizontal);
    m_wallOpacitySlider ->setObjectName("settingsSlider");
    m_wallOpacitySlider ->setRange(20,255);
    m_wallOpacitySlider ->setValue(255);
    wallForm->addRow( "歌词透明度：", m_wallOpacitySlider );
    wallForm->addRow("歌词颜色：", makeColorRow(m_edtWallColorCurr, m_swatchWallCurr, "#FFFFFF"));
    root->addWidget(grpWall);

    QLabel *aboutLabel = new QLabel(QString("Version: %1<br>GitHub: github.com/Alice-Cartelet")
                                        .arg(QApplication::applicationVersion()));
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
    aboutWrap->addWidget(aboutLabel, 0, Qt::AlignRight | Qt::AlignBottom);
    bottomRow->addLayout(aboutWrap);
    bottomRow->addStretch();

    QHBoxLayout *btns = new QHBoxLayout;
    bottomRow->addLayout(btns);
    btns->addStretch();

    m_btnCancel = new QPushButton("取消");
    m_btnOk = new QPushButton("确定");
    m_btnCancel->setObjectName("dlgBtn");
    m_btnOk->setObjectName("dlgBtnPrimary");
    m_btnCancel->setFixedSize(80, 32);
    m_btnOk->setFixedSize(80, 32);
    btns->addWidget(m_btnCancel);
    btns->addSpacing(8);
    btns->addWidget(m_btnOk);
    root->addLayout(bottomRow);

    applyStyle();

    connect(m_btnMusic, &QPushButton::clicked, this, [this]
            {
                QString d = QFileDialog::getExistingDirectory(this, "选择音乐目录", m_edtMusic->text());
                if (!d.isEmpty()) m_edtMusic->setText(d);
            });
    connect(m_btnLyrics, &QPushButton::clicked, this, [this]
            {
                QString d = QFileDialog::getExistingDirectory(this, "选择歌词目录", m_edtLyrics->text());
                if (!d.isEmpty()) m_edtLyrics->setText(d);
            });
    connect(m_edtColorSung, &QLineEdit::textChanged, this, [this](const QString &t)
            {
                updateSwatch(m_swatchSung, t);
            });
    connect(m_edtColorUnsang, &QLineEdit::textChanged, this, [this](const QString &t)
            {
                updateSwatch(m_swatchUnsang, t);
            });
    connect(m_btnImportFont, &QPushButton::clicked, this, [this]
            {
                QStringList files = QFileDialog::getOpenFileNames(
                    this, "选择字体文件", QString(), "字体文件 (*.ttf *.otf *.TTF *.OTF)");
                if (files.isEmpty()) return;
                QSettings s("MusicPlayer", "MusicPlayer");
                QStringList saved = s.value("importedFontPaths").toStringList();
                QString lastFamily;
                for (const QString &path : files)
                {
                    if (saved.contains(path))
                    {
                        int id = QFontDatabase::addApplicationFont(path);
                        QStringList families = QFontDatabase::applicationFontFamilies(id);
                        if (!families.isEmpty()) lastFamily = families.first();
                        continue;
                    }
                    int id = QFontDatabase::addApplicationFont(path);
                    if (id == -1)
                    {
                        QMessageBox::warning(this, "导入失败", QString("无法加载字体文件：\n%1").arg(path));
                        continue;
                    }
                    QStringList families = QFontDatabase::applicationFontFamilies(id);
                    if (families.isEmpty()) continue;
                    saved << path;
                    lastFamily = families.first();
                }
                s.setValue("importedFontPaths", saved);
                if (!lastFamily.isEmpty()) m_fontCombo->setCurrentFont(QFont(lastFamily));
            });
    connect(m_btnPickPosition, &QPushButton::clicked, this, [this]
            {
                LyricOrientation orient = static_cast<LyricOrientation>(
                    m_cmbOrientation->currentData().toInt());
                PositionPicker dlg(orient, this);
                if (dlg.exec() == QDialog::Accepted)
                {
                    m_wallPos = dlg.selectedPos();
                    m_lblWallPos->setText(
                        QString("X:%1  Y:%2")
                            .arg(m_wallPos.x())
                            .arg(m_wallPos.y()));
                    emit wallpaperPositionChanged(m_wallPos);
                }
            });
    connect(m_edtWallColorCurr, &QLineEdit::textChanged, this, [this](const QString &t)
            {
                updateSwatch(m_swatchWallCurr, t);
            });
    connect(m_btnOk, &QPushButton::clicked, this, [this]
            {
                QSettings s("MusicPlayer", "MusicPlayer");
                QString oldMusicDir = s.value("musicDir").toString();
                QString oldLyricsDir = s.value("lyricsDir").toString();
                bool oldShowLyrics = s.value("showLyrics", false).toBool();
                int oldVolume = s.value("volume", 70).toInt();
                int oldMiniOpacity = s.value("miniOpacity", 140).toInt();
                bool oldTray = s.value("minimizeToTray", false).toBool();
                bool oldMini = s.value("enableMiniControl", true).toBool();
                bool oldHideHover = s.value("hideOnHover", false).toBool();
                QString oldSung = s.value("lyricColorSung", "#E63248").toString();
                QString oldUnsang = s.value("lyricColorUnsang", "#F1DDDF").toString();
                int oldFontSize = s.value("lyricFontSize", 28).toInt();
                QString oldFontFamily = s.value("lyricFontFamily", "Microsoft YaHei").toString();
                bool oldWallEnabled = s.value("wallpaperLyricsEnabled", false).toBool();
                int oldWallOrient = s.value("wallpaperOrientation", 0).toInt();
                int oldWallPos = s.value("wallPosX", 7).toInt();
                QString oldWallColorC = s.value("wallpaperColorCurrent", "#FFFFFF").toString();
                QString oldWallColorO = s.value("wallpaperColorOther", "rgba(255,255,255,0.55)").toString();
                bool oldWallShadow = s.value("wallpaperTextShadow", true).toBool();
                int oldWallFont = s.value("wallpaperFontSize", 36).toInt();
                int oldWallTitleFont= s.value("wallpaperTitleFontSize", 22).toInt();
                int oldWallOpacity = s.value("wallpaperOpacity", 255).toInt();

                QString newMusicDir = m_edtMusic->text();
                QString newLyricsDir = m_edtLyrics->text();
                bool newShowLyrics = m_chkLyrics->isChecked();
                int newVolume = m_volSlider->value();
                int newMiniOpacity = m_miniOpacitySlider->value();
                bool newTray = m_chkTray->isChecked();
                bool newMini = m_chkMiniControl->isChecked();
                bool newHideHover = m_chkHideHover->isChecked();
                QString newSung = m_edtColorSung->text();
                QString newUnsang = m_edtColorUnsang->text();
                int newFontSize = m_lyricFontSlider->value();
                QString newFontFamily = m_fontCombo->currentFont().family();

                if (newSung.length() != 7) newSung = oldSung;
                if (newUnsang.length() != 7) newUnsang = oldUnsang;

                bool newWallEnabled = m_chkWallpaper->isChecked();
                int newWallOrient = m_cmbOrientation->currentIndex();
                s.setValue("wallPosX", m_wallPos.x());
                s.setValue("wallPosY", m_wallPos.y());
                QString newWallColorC = m_edtWallColorCurr->text();
                int newWallFont = m_wallFontSlider->value();
                int newWallTitleFont= m_wallTitleFontSlider->value();
                int newWallOpacity = m_wallOpacitySlider->value();

                s.setValue("musicDir", newMusicDir);
                s.setValue("lyricsDir", newLyricsDir);
                s.setValue("showLyrics", newShowLyrics);
                s.setValue("volume", newVolume);
                s.setValue("miniOpacity", newMiniOpacity);
                s.setValue("minimizeToTray", newTray);
                s.setValue("enableMiniControl", newMini);
                s.setValue("hideOnHover", newHideHover);
                s.setValue("lyricColorSung", newSung);
                s.setValue("lyricColorUnsang", newUnsang);
                s.setValue("lyricFontSize", newFontSize);
                s.setValue("lyricFontFamily", newFontFamily);
                s.setValue("wallpaperOpacity", newWallOpacity);

                if (oldWallOpacity != newWallOpacity) emit wallpaperOpacityChanged(newWallOpacity);

                s.setValue("wallpaperLyricsEnabled", newWallEnabled);
                s.setValue("wallpaperOrientation", newWallOrient);
                s.setValue("wallpaperColorCurrent", newWallColorC);
                s.setValue("wallpaperFontSize", newWallFont);
                s.setValue("wallpaperTitleFontSize", newWallTitleFont);

                if (oldMusicDir != newMusicDir) emit musicDirChanged(newMusicDir);
                if (oldLyricsDir != newLyricsDir) emit lyricsDirChanged(newLyricsDir);
                if (oldShowLyrics != newShowLyrics) emit showLyricsChanged(newShowLyrics);
                if (oldVolume != newVolume) emit volumeChanged(newVolume / 100.f);
                if (oldMiniOpacity != newMiniOpacity) emit miniOpacityChanged(newMiniOpacity);
                if (oldTray != newTray) emit minimizeToTrayChanged(newTray);
                if (oldMini != newMini) emit miniControlChanged(newMini);
                if (oldHideHover != newHideHover) emit hideOnHoverChanged(newHideHover);
                if (oldSung != newSung || oldUnsang != newUnsang) emit lyricColorsChanged(newSung, newUnsang);
                if (oldFontSize != newFontSize) emit lyricFontSizeChanged(newFontSize);
                if (oldFontFamily != newFontFamily) emit lyricFontFamilyChanged(newFontFamily);
                if (oldWallEnabled != newWallEnabled) emit wallpaperLyricsEnabledChanged(newWallEnabled);
                if (oldWallOrient != newWallOrient) emit wallpaperOrientationChanged(static_cast<LyricOrientation>(m_cmbOrientation->currentData().toInt()));
                emit wallpaperPositionChanged(m_wallPos);
                if (oldWallColorC != newWallColorC) emit wallpaperColorCurrentChanged(newWallColorC);
                if (oldWallFont != newWallFont) emit wallpaperFontSizeChanged(newWallFont);
                if (oldWallTitleFont != newWallTitleFont) emit wallpaperTitleFontSizeChanged(newWallTitleFont);

                accept();
            });
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QSettings s("MusicPlayer", "MusicPlayer");
    m_edtMusic->setText(s.value("musicDir").toString());
    m_edtLyrics->setText(s.value("lyricsDir").toString());
    m_chkLyrics->setChecked(s.value("showLyrics", false).toBool());
    m_miniOpacitySlider->setValue(s.value("miniOpacity", 140).toInt());
    m_volSlider->setValue(s.value("volume", 70).toInt());
    m_lyricFontSlider->setValue(s.value("lyricFontSize", 28).toInt());
    m_wallOpacitySlider->setValue(s.value("wallpaperOpacity", 255).toInt());
    m_chkTray->setChecked(s.value("minimizeToTray", false).toBool());
    m_chkMiniControl->setChecked(s.value("enableMiniControl", true).toBool());
    m_chkHideHover->setChecked(s.value("hideOnHover", false).toBool());

    QString sungColor = s.value("lyricColorSung", "#E63248").toString();
    QString unsangColor = s.value("lyricColorUnsang", "#F1DDDF").toString();
    m_edtColorSung->setText(sungColor);
    m_edtColorUnsang->setText(unsangColor);
    updateSwatch(m_swatchSung, sungColor);
    updateSwatch(m_swatchUnsang, unsangColor);

    QString savedFamily = s.value("lyricFontFamily", "Microsoft YaHei").toString();
    m_fontCombo->setCurrentFont(QFont(savedFamily));

    m_chkWallpaper->setChecked(s.value("wallpaperLyricsEnabled", false).toBool());
    m_cmbOrientation->setCurrentIndex(s.value("wallpaperOrientation", 0).toInt());
    m_wallPos = QPoint(s.value("wallPosX", 800).toInt(), s.value("wallPosY", 500).toInt());
    m_lblWallPos->setText(QString("X:%1  Y:%2").arg(m_wallPos.x()).arg(m_wallPos.y()));

    m_wallFontSlider->setValue(s.value("wallpaperFontSize", 36).toInt());
    m_wallTitleFontSlider->setValue(s.value("wallpaperTitleFontSize", 22).toInt());

    QString wallColorC = s.value("wallpaperColorCurrent", "#FFFFFF").toString();
    QString wallColorO = s.value("wallpaperColorOther", "rgba(255,255,255,0.55)").toString();
    Q_UNUSED(wallColorO);
    m_edtWallColorCurr->setText(wallColorC);
    updateSwatch(m_swatchWallCurr, wallColorC);
}

QString SettingsDialog::musicDir() const
{
    return m_edtMusic->text();
}
QString SettingsDialog::lyricsDir() const
{
    return m_edtLyrics->text();
}
bool SettingsDialog::showLyrics() const
{
    return m_chkLyrics->isChecked();
}
float SettingsDialog::volume() const
{
    return m_volSlider->value() / 100.f;
}
bool SettingsDialog::minimizeToTray() const
{
    return m_chkTray->isChecked();
}
bool SettingsDialog::hideOnHover() const
{
    return m_chkHideHover->isChecked();
}
QString SettingsDialog::lyricColorSung() const
{
    return m_edtColorSung->text();
}
QString SettingsDialog::lyricColorUnsang() const
{
    return m_edtColorUnsang->text();
}
int SettingsDialog::lyricFontSize() const
{
    return m_lyricFontSlider->value();
}
QString SettingsDialog::lyricFontFamily() const
{
    return m_fontCombo->currentFont().family();
}
bool SettingsDialog::wallpaperLyricsEnabled() const
{
    return m_chkWallpaper->isChecked();
}
LyricOrientation SettingsDialog::wallpaperOrientation() const
{
    return static_cast<LyricOrientation>(m_cmbOrientation->currentData().toInt());
}
QPoint SettingsDialog::wallpaperPosition() const
{
    return m_wallPos;
}
QString SettingsDialog::wallpaperColorCurrent() const
{
    return m_edtWallColorCurr->text();
}
int SettingsDialog::wallpaperFontSize() const
{
    return m_wallFontSlider->value();
}
int SettingsDialog::wallpaperTitleFontSize() const
{
    return m_wallTitleFontSlider->value();
}
void SettingsDialog::setMusicDir(const QString &d)
{
    m_edtMusic->setText(d);
}
void SettingsDialog::setLyricsDir(const QString &d)
{
    m_edtLyrics->setText(d);
}
void SettingsDialog::setShowLyrics(bool v)
{
    m_chkLyrics->setChecked(v);
}
void SettingsDialog::setVolume(float v)
{
    m_volSlider->setValue(qRound(v * 100));
}
void SettingsDialog::setMinimizeToTray(bool v)
{
    m_chkTray->setChecked(v);
}
void SettingsDialog::setHideOnHover(bool v)
{
    m_chkHideHover->setChecked(v);
}
void SettingsDialog::setLyricFontSize(int v)
{
    m_lyricFontSlider->setValue(v);
}
void SettingsDialog::setLyricFontFamily(const QString &f)
{
    m_fontCombo->setCurrentFont(QFont(f));
}
void SettingsDialog::setLyricColorSung(const QString &h)
{
    m_edtColorSung->setText(h);
    updateSwatch(m_swatchSung, h);
}
void SettingsDialog::setLyricColorUnsang(const QString &h)
{
    m_edtColorUnsang->setText(h);
    updateSwatch(m_swatchUnsang, h);
}
void SettingsDialog::setWallpaperLyricsEnabled(bool v)
{
    m_chkWallpaper->setChecked(v);
}
void SettingsDialog::setWallpaperOrientation(LyricOrientation o)
{
    for (int i = 0; i < m_cmbOrientation->count(); ++i)
    {
        if (m_cmbOrientation->itemData(i).toInt() == static_cast<int>(o))
        {
            m_cmbOrientation->setCurrentIndex(i);
            break;
        }
    }
}
void SettingsDialog::setWallpaperPosition(QPoint p)
{
    m_wallPos = p;
    if (m_lblWallPos)
        m_lblWallPos->setText(QString("X:%1  Y:%2").arg(p.x()).arg(p.y()));
}
void SettingsDialog::setWallpaperColorCurrent(const QString &h)
{
    m_edtWallColorCurr->setText(h);
    updateSwatch(m_swatchWallCurr, h);
}
void SettingsDialog::setWallpaperFontSize(int v)
{
    m_wallFontSlider->setValue(v);
}
void SettingsDialog::setWallpaperTitleFontSize(int v)
{
    m_wallTitleFontSlider->setValue(v);
}
void SettingsDialog::loadSavedFonts()
{
    QString naikaiPath = QCoreApplication::applicationDirPath() + "/naikai.ttf";
    if (QFile::exists(naikaiPath)) QFontDatabase::addApplicationFont(naikaiPath);
    QSettings s("MusicPlayer", "MusicPlayer");
    const QStringList paths = s.value("importedFontPaths").toStringList();
    QStringList valid;
    for (const QString &path : paths)
    {
        if (!QFile::exists(path)) continue;
        QFontDatabase::addApplicationFont(path);
        valid << path;
    }
    if (valid.size() != paths.size()) s.setValue("importedFontPaths", valid);
}
void SettingsDialog::updateSwatch(QLabel *swatch, const QString &hex)
{
    QColor c(hex);
    if (c.isValid()) swatch->setStyleSheet(QString("background:%1;border-radius:4px;border:1px solid #3a3860;").arg(hex));
    else swatch->setStyleSheet("background:#2a2a40;border-radius:4px;border:1px solid #3a3860;");
}
void SettingsDialog::applyStyle()
{
    setStyleSheet(R"( QDialog
    {
        background:qlineargradient( x1:0,y1:0,x2:1,y2:1, stop:0 #10131d, stop:0.5 #161b29, stop:1 #1b1d35 );
        color:#E5E8FF;
        font-family:"Microsoft YaHei";
    }
    QScrollArea
    {
        border:none;
        background:transparent;
    }
    QScrollArea>QWidget>QWidget
    {
        background:transparent;
    }
    QLabel
    {
        color:#BFC5F7;
        font-size:13px;
    }
    QLineEdit
    {
        background:rgba(35,40,65,0.85);
        border:1px solid rgba(90,100,180,0.4);
        border-radius:12px;
        color:white;
        min-height:34px;
        padding-left:14px;
        padding-right:14px;
        font-size:13px;
    }
    QLineEdit:hover
    {
        border:1px solid rgba(110,120,220,0.8);
    }
    QLineEdit:focus
    {
        border:1px solid #7A6BFF;
    }
    QFontComboBox, QComboBox#wallCombo
    {
        background:rgba(35,40,65,0.85);
        border:1px solid rgba(90,100,180,0.4);
        border-radius:12px;
        color:white;
        min-height:34px;
        padding-left:10px;
    }
    QFontComboBox:hover, QComboBox#wallCombo:hover
    {
        border:1px solid #7A6BFF;
    }
    QComboBox QAbstractItemView, QFontComboBox QAbstractItemView
    {
        background:#20253a;
        border:1px solid #303860;
        color:white;
        selection-background-color:#6658F0;
    }
    QPushButton#browseBtn
    {
        background:qlineargradient( x1:0,y1:0, x2:1,y2:1, stop:0 #2F3555, stop:1 #242944 );
        border:none;
        border-radius:10px;
        color:#D8DCFF;
        font-size:12px;
        padding:6px 12px;
    }
    QPushButton#browseBtn:hover
    {
        background:qlineargradient( x1:0,y1:0, x2:1,y2:1, stop:0 #4A56AA, stop:1 #6559F4 );
    }
    QPushButton#browseBtn:pressed
    {
        padding-top:8px;
    }
    QPushButton#dlgBtn
    {
        background:#232741;
        border:none;
        border-radius:12px;
        color:#C7CBF5;
    }
    QPushButton#dlgBtn:hover
    {
        background:#30375D;
    }
    QPushButton#dlgBtnPrimary
    {
        background:qlineargradient( x1:0,y1:0, x2:1,y2:1, stop:0 #7568FF, stop:1 #9B5CFF );
        border:none;
        border-radius:12px;
        font-weight:bold;
        color:white;
    }
    QPushButton#dlgBtnPrimary:hover
    {
        background:qlineargradient( x1:0,y1:0, x2:1,y2:1, stop:0 #8A7FFF, stop:1 #AD73FF );
    }
    QCheckBox
    {
        color:#D2D6FF;
        spacing:8px;
    }
    QCheckBox::indicator
    {
        width:18px;
        height:18px;
        border-radius:6px;
        border:1px solid #555C88;
        background:#232741;
    }
    QCheckBox::indicator:checked
    {
        background:#7B6DFF;
        border:none;
    }
    QSlider::groove:horizontal
    {
        background:#242942;
        height:6px;
        border-radius:3px;
    }
    QSlider::sub-page:horizontal
    {
        background:qlineargradient( x1:0,y1:0, x2:1,y2:0, stop:0 #7164FF, stop:1 #A46EFF );
        border-radius:3px;
    }
    QSlider::handle:horizontal
    {
        width:18px;
        margin:-6px 0;
        border-radius:9px;
        background:white;
    }
   QGroupBox#wallGroup, QGroupBox#settingsGroup
    {
        background:rgba(30,35,60,0.55);
        border:1px solid rgba(110,120,200,0.35);
        border-radius:18px;
        margin-top:14px;
        padding-top:14px;
    }
    QGroupBox#wallGroup::title, QGroupBox#settingsGroup::title
    {
        subcontrol-origin:margin;
        left:16px;
        padding:0px 10px;
        color:#D7D9FF;
        font-size:14px;
        font-weight:bold;
    }
    QLabel#colorSwatch
    {
        border-radius:8px;
        border:2px solid rgba(255,255,255,0.2);
    }
    QLabel#wallPosLabel
    {
        color:#9EA6FF;
        font-size:12px;
        font-family:"Microsoft YaHei";
        background:rgba(30,35,60,0.7);
        border:1px solid rgba(90,100,180,0.35);
        border-radius:8px;
        padding:3px 10px;
    }
    QScrollBar:vertical
    {
        width:8px;
        background:transparent;
    }
    QScrollBar::handle:vertical
    {
        background:#5358A8;
        border-radius:4px;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical
    {
        height:0;
    }
    )");
}