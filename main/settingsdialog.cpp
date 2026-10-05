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
#include <QColorDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QFrame>
#include <QWheelEvent>
#include <QComboBox>
#include <QFontComboBox>
#include <QFileInfo>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QTextDocument>
#include <QUrl>
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
    static const unsigned char kData1[] = {0x0C, 0x3F, 0x28, 0x29, 0x33, 0x35, 0x34, 0x60,0x7A, 0x7F, 0x6B, 0x66, 0x38, 0x28, 0x64, 0x1D};
    setModal(true);
    resize(820, 540);
    setMinimumSize(720, 480);
    QVBoxLayout *rootOuter = new QVBoxLayout(this);
    rootOuter->setContentsMargins(18, 16, 18, 14);
    rootOuter->setSpacing(12);
    auto *title = new QLabel(QStringLiteral("设置"), this);
    title->setObjectName("settingsTitle");
    rootOuter->addWidget(title);

    auto *body = new QFrame(this);
    body->setObjectName("settingsBody");
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    auto *navigation = new QListWidget(body);
    navigation->setObjectName("settingsNavigation");
    navigation->setFixedWidth(156);
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setFocusPolicy(Qt::NoFocus);
    for (const QString &name : {QStringLiteral("常规"), QStringLiteral("悬浮歌词"),
                                QStringLiteral("壁纸歌词"), QStringLiteral("HTML 背景")}) {
        auto *item = new QListWidgetItem(name, navigation);
        item->setSizeHint(QSize(136, 44));
    }
    auto *pages = new QStackedWidget(body);
    pages->setObjectName("settingsPages");
    bodyLayout->addWidget(navigation);
    bodyLayout->addWidget(pages, 1);
    rootOuter->addWidget(body, 1);

    auto makePage = [pages](const QString &name, const QString &description) {
        auto *scroll = new QScrollArea(pages);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *page = new QWidget;
        page->setObjectName("settingsPage");
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 18, 20, 16);
        layout->setSpacing(12);
        auto *heading = new QLabel(name, page);
        heading->setObjectName("pageTitle");
        layout->addWidget(heading);
        auto *hint = new QLabel(description, page);
        hint->setObjectName("pageDescription");
        hint->setWordWrap(true);
        layout->addWidget(hint);
        scroll->setWidget(page);
        pages->addWidget(scroll);
        return layout;
    };
    QVBoxLayout *generalPage = makePage(QStringLiteral("常规"), QStringLiteral("管理音乐目录、播放音量和窗口行为。"));
    QVBoxLayout *lyricPage = makePage(QStringLiteral("悬浮歌词"), QStringLiteral("调整桌面悬浮歌词的字体、颜色和交互。"));
    QVBoxLayout *wallPage = makePage(QStringLiteral("壁纸歌词"), QStringLiteral("控制壁纸歌词的位置、排版和显示内容。"));
    QVBoxLayout *htmlPage = makePage(QStringLiteral("HTML 背景"), QStringLiteral("使用本地网页作为可交互的桌面背景。"));
    connect(navigation, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    navigation->setCurrentRow(0);
    auto makeColorRow = [&](QLineEdit *&edt, QLabel *&swatch, const QString &defaultHex) -> QHBoxLayout*
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(6);
        row->setContentsMargins(0,0,0,0);
        edt = new QLineEdit(defaultHex);
        edt->setObjectName("colorEdit");
        edt->setMaxLength(7);
        edt->setFixedWidth(102);
        edt->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral(R"(#[0-9A-Fa-f]{0,6})")), edt));
        swatch = new QLabel;
        swatch->setObjectName("colorSwatch");
        swatch->setFixedSize(24, 24);
        updateSwatch(swatch, defaultHex);
        auto *pick = new QPushButton(QStringLiteral("选色"));
        pick->setObjectName("colorPickBtn");
        pick->setFixedWidth(52);
        connect(pick, &QPushButton::clicked, this, [this, edt] {
            const QColor initial(edt->text());
            const QColor chosen = QColorDialog::getColor(initial.isValid() ? initial : Qt::white,
                this, QStringLiteral("选择歌词颜色"));
            if (chosen.isValid()) edt->setText(chosen.name(QColor::HexRgb));
        });
        row->addWidget(edt, 0, Qt::AlignVCenter);
        row->addWidget(swatch, 0, Qt::AlignVCenter);
        row->addWidget(pick, 0, Qt::AlignVCenter);
        return row;
    };
    auto makeSliderRow = [&](QSlider *slider, const QString &unit) -> QHBoxLayout*
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(6);
        row->setContentsMargins(0,0,0,0);
        QLabel *valLbl = new QLabel;
        valLbl->setFixedWidth(42);
        valLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto updateText = [valLbl, unit](int v)
        {
            if(unit == "opacity")
                valLbl->setText(QString("%1%").arg(qRound(v / 255.0 * 100)));
            else if(unit == "%")
                valLbl->setText(QString("%1%").arg(v));
            else
                valLbl->setText(QString("%1px").arg(v));
        };
        updateText(slider->value());
        QObject::connect(slider, &QSlider::valueChanged, valLbl, [updateText](int v){ updateText(v); });
        row->addWidget(slider, 1);
        row->addWidget(valLbl);
        return row;
    };
    QGroupBox *grpGeneral = new QGroupBox("媒体资料库");
    static const unsigned char kData2[] = {0x33, 0x2E, 0x12, 0x2F, 0x38, 0x60, 0x7A, 0x3D, 0x33, 0x2E, 0x32, 0x2F, 0x38, 0x74, 0x39, 0x35};
    grpGeneral->setObjectName("settingsGroup");
    QFormLayout *genForm = new QFormLayout(grpGeneral);
    genForm->setSpacing(12);
    genForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
    QGroupBox *grpPlayback = new QGroupBox("播放与窗口");
    grpPlayback->setObjectName("settingsGroup");
    auto *playbackForm = new QFormLayout(grpPlayback);
    playbackForm->setSpacing(12);
    playbackForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_volSlider = new NoWheelSlider(Qt::Horizontal);
    m_volSlider->setObjectName("settingsSlider");
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(70);
    playbackForm->addRow("默认音量：", makeSliderRow(m_volSlider, "%"));
    m_chkTray = new QCheckBox("最小化时缩到系统托盘");
    m_chkTray->setObjectName("settingsCheck");
    playbackForm->addRow("", m_chkTray);
    m_chkMiniControl = new QCheckBox("启用小控制窗");
    m_chkMiniControl->setObjectName("settingsCheck");
    playbackForm->addRow("", m_chkMiniControl);
    m_miniOpacitySlider = new NoWheelSlider(Qt::Horizontal);
    m_miniOpacitySlider->setObjectName("settingsSlider");
    m_miniOpacitySlider->setRange(20, 255);
    m_miniOpacitySlider->setValue(85);
    playbackForm->addRow("小窗透明度：", makeSliderRow(m_miniOpacitySlider, "opacity"));
    generalPage->addWidget(grpGeneral);
    generalPage->addWidget(grpPlayback);
    generalPage->addStretch();
    QGroupBox *grpLyricBehavior = new QGroupBox("显示行为");
    grpLyricBehavior->setObjectName("settingsGroup");
    auto *lyricBehaviorForm = new QFormLayout(grpLyricBehavior);
    lyricBehaviorForm->setSpacing(10);
    QGroupBox *grpLyric = new QGroupBox("字体与颜色");
    static const unsigned char kData3[] = {0x37, 0x75, 0x1B, 0x36, 0x33, 0x39, 0x3F, 0x77,0x19, 0x3B, 0x28, 0x2E, 0x3F, 0x36, 0x3F, 0x2E};
    grpLyric->setObjectName("settingsGroup");
    QFormLayout *lyricForm = new QFormLayout(grpLyric);
    lyricForm->setSpacing(12);
    lyricForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_chkLyrics = new QCheckBox("启用桌面悬浮歌词");
    m_chkLyrics->setObjectName("settingsCheck");
    lyricBehaviorForm->addRow("", m_chkLyrics);
    m_chkHideHover = new QCheckBox("鼠标悬停时自动隐藏（开启后无法拖动）");
    m_chkHideHover->setObjectName("settingsCheck");
    QByteArray d_fontCombo;
    lyricBehaviorForm->addRow("", m_chkHideHover);
    m_lyricFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_lyricFontSlider->setObjectName("settingsSlider");
    m_lyricFontSlider->setRange(18, 60);
    m_lyricFontSlider->setValue(28);
    lyricForm->addRow("歌词字号：", makeSliderRow(m_lyricFontSlider, "px"));
    loadSavedFonts();
    QHBoxLayout *fontRow = new QHBoxLayout;
    fontRow->setSpacing(8);
    fontRow->setContentsMargins(0,0,0,0);
    d_fontCombo.append(reinterpret_cast<const char*>(kData1), sizeof(kData1));
    m_fontCombo = new NoWheelFontComboBox;
    m_fontCombo->setObjectName("fontCombo");
    m_fontCombo->setEditable(false);
    m_fontCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_fontCombo->setMinimumContentsLength(10);
    m_fontCombo->setMinimumWidth(90);
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
    d_fontCombo.append(reinterpret_cast<const char*>(kData2), sizeof(kData2));
    fontRow->addWidget(m_fontCombo, 1);
    fontRow->addWidget(m_btnImportFont);
    lyricForm->addRow("歌词字体：", fontRow);
    QVBoxLayout *colorLayout = new QVBoxLayout;
    colorLayout->setSpacing(5);
    QLabel *lblSung = new QLabel("已读");
    lblSung->setMinimumWidth(36);
    QLabel *lblUnsang = new QLabel("未读");
    lblUnsang->setMinimumWidth(36);
    auto *sungRow = new QHBoxLayout;
    sungRow->addWidget(lblSung);
    sungRow->addLayout(makeColorRow(m_edtColorSung, m_swatchSung, "#E63248"));
    sungRow->addStretch();
    colorLayout->addLayout(sungRow);
    auto *unsangRow = new QHBoxLayout;
    unsangRow->addWidget(lblUnsang);
    unsangRow->addLayout(makeColorRow(m_edtColorUnsang, m_swatchUnsang, "#F1DDDF"));
    unsangRow->addStretch();
    colorLayout->addLayout(unsangRow);
    lyricForm->addRow("歌词颜色：", colorLayout);
    lyricPage->addWidget(grpLyricBehavior);
    lyricPage->addWidget(grpLyric);
    lyricPage->addStretch();
    QGroupBox *grpHtml = new QGroupBox("HTML 桌面背景");
    grpHtml->setObjectName("settingsGroup");
    auto *htmlForm = new QFormLayout(grpHtml);
    htmlForm->setSpacing(12);
    htmlForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_chkHtmlWallpaper = new QCheckBox("使用 HTML 作为桌面背景");
    m_chkHtmlWallpaper->setObjectName("settingsCheck");
    htmlForm->addRow("", m_chkHtmlWallpaper);
    auto *htmlRow = new QHBoxLayout;
    htmlRow->setSpacing(12);
    m_edtHtmlWallpaper = new QLineEdit;
    m_edtHtmlWallpaper->setReadOnly(true);
    m_edtHtmlWallpaper->setPlaceholderText("选择本地 HTML 文件…");
    auto *htmlBrowse = new QPushButton("浏览");
    htmlBrowse->setObjectName("browseBtn");
    htmlBrowse->setFixedSize(64, 32);
    htmlRow->addWidget(m_edtHtmlWallpaper, 1);
    htmlRow->addWidget(htmlBrowse);
    htmlForm->addRow("HTML 文件：", htmlRow);
    auto *htmlHint = new QLabel("网页铺满桌面背景，桌面图标和任务栏照常使用。关闭后恢复系统壁纸。");
    htmlHint->setWordWrap(true);
    htmlForm->addRow("", htmlHint);
    auto *htmlDocs = new QPushButton("歌词接口文档");
    htmlDocs->setObjectName("htmlWallpaperDocsBtn");
    htmlDocs->setToolTip("查看 HTML 歌词与封面接口的 Markdown 开发文档");
    auto *docsRow = new QHBoxLayout;
    docsRow->addWidget(htmlDocs);
    docsRow->addStretch();
    htmlForm->addRow("开发文档：", docsRow);
    connect(htmlDocs, &QPushButton::clicked, this, [this] {
        const QString documentPath = QCoreApplication::applicationDirPath() + "/HTML-WALLPAPER.md";
        QFile document(documentPath);
        if (!document.open(QIODevice::ReadOnly)) {
            document.setFileName(":/docs/HTML-WALLPAPER.md");
            if (!document.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "歌词接口文档", "无法读取 HTML-WALLPAPER.md 开发文档。");
                return;
            }
        }
        auto *viewer = new QDialog(this);
        viewer->setAttribute(Qt::WA_DeleteOnClose);
        viewer->setWindowTitle("HTML 歌词与封面接口 · HTML-WALLPAPER.md");
        viewer->setWindowModality(Qt::WindowModal);
        viewer->resize(880, 680);
        viewer->setMinimumSize(600, 420);
        auto *layout = new QVBoxLayout(viewer);
        auto *browser = new QTextBrowser(viewer);
        browser->setObjectName("htmlWallpaperDocsView");
        browser->setStyleSheet("QTextBrowser { background: #ffffff; color: #253047; border: none; padding: 18px; }");
        browser->setOpenExternalLinks(true);
        browser->document()->setDefaultFont(QFont("Microsoft YaHei", 11));
        browser->document()->setBaseUrl(QUrl::fromLocalFile(documentPath));
        browser->setMarkdown(QString::fromUtf8(document.readAll()));
        layout->addWidget(browser);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, viewer);
        buttons->button(QDialogButtonBox::Close)->setText("关闭");
        connect(buttons, &QDialogButtonBox::rejected, viewer, &QDialog::reject);
        layout->addWidget(buttons);
        viewer->show();
    });
    htmlPage->addWidget(grpHtml);
    htmlPage->addStretch();
    connect(htmlBrowse, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, "选择 HTML 桌面背景",
            m_edtHtmlWallpaper->text(), "HTML 文件 (*.html *.htm)");
        if (!path.isEmpty()) m_edtHtmlWallpaper->setText(path);
    });
    QGroupBox *grpWall = new QGroupBox("显示与位置");
    grpWall->setObjectName("wallGroup");
    QFormLayout *wallForm = new QFormLayout(grpWall);
    wallForm->setSpacing(10);
    wallForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QGroupBox *grpWallStyle = new QGroupBox("字体与内容");
    grpWallStyle->setObjectName("wallGroup");
    grpWall->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    grpWallStyle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QFormLayout *wallStyleForm = new QFormLayout(grpWallStyle);
    wallStyleForm->setSpacing(10);
    wallStyleForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_chkWallpaper = new QCheckBox("启用壁纸歌词");
    m_chkWallpaper->setObjectName("settingsCheck");
    wallForm->addRow("", m_chkWallpaper);
    m_cmbOrientation = new NoWheelComboBox;
    m_cmbOrientation->setObjectName("wallCombo");
    m_cmbOrientation->addItem("横排", static_cast<int>(LyricOrientation::Horizontal));
    m_cmbOrientation->addItem("竖排", static_cast<int>(LyricOrientation::Vertical));
    wallForm->addRow("排列方式：", m_cmbOrientation);
    m_btnPickPosition = new QPushButton("选位置");
    m_btnPickPosition->setObjectName("browseBtn");
    m_lblWallPos = new QLabel(QString("X:%1  Y:%2").arg(m_wallPos.x()).arg(m_wallPos.y()));
    m_lblWallPos->setObjectName("wallPosLabel");
    {
        QHBoxLayout *posRow = new QHBoxLayout;
        posRow->setSpacing(10);
        posRow->addWidget(m_btnPickPosition);
        posRow->addWidget(m_lblWallPos);
        posRow->addStretch();
        wallForm->addRow("显示位置：", posRow);
    }
    {
        m_wallFontCombo = new NoWheelFontComboBox;
        m_wallFontCombo->setObjectName("fontCombo");
        m_wallFontCombo->setEditable(false);
        m_wallFontCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_wallFontCombo->setMinimumContentsLength(8);
        m_wallFontCombo->setMinimumWidth(90);
        m_wallFontCombo->setMaximumWidth(170);
        m_wallFontCombo->setCurrentFont(QFont("Microsoft YaHei"));
        wallStyleForm->addRow("歌词字体：", m_wallFontCombo);
    }
    m_wallFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_wallFontSlider->setObjectName("settingsSlider");
    m_wallFontSlider->setRange(18, 80);
    m_wallFontSlider->setValue(36);
    d_fontCombo.append(reinterpret_cast<const char*>(kData3), sizeof(kData3));
    wallStyleForm->addRow("歌词字号：", makeSliderRow(m_wallFontSlider, "px"));
    m_cmbWallExtraMode = new NoWheelComboBox;
    m_cmbWallExtraMode->setObjectName("wallCombo");
    m_cmbWallExtraMode->clear();
    m_cmbWallExtraMode->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_cmbWallExtraMode->setMinimumContentsLength(7);
    m_cmbWallExtraMode->setMaximumWidth(170);
    m_cmbWallExtraMode->setToolTip("附加内容");
    m_cmbWallExtraMode->addItem("无", static_cast<int>(WallpaperExtraLyricsMode::None));
    m_cmbWallExtraMode->addItem("译文歌词", static_cast<int>(WallpaperExtraLyricsMode::Translation));
    m_cmbWallExtraMode->addItem("下一句歌词", static_cast<int>(WallpaperExtraLyricsMode::NextLine));
    wallStyleForm->addRow("附加内容：", m_cmbWallExtraMode);
    m_wallExtraFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_wallExtraFontSlider->setObjectName("settingsSlider");
    m_wallExtraFontSlider->setRange(10, 60);
    m_wallExtraFontSlider->setValue(24);
    m_wallExtraFontSlider->setToolTip("附加字号");
    wallStyleForm->addRow("附加字号：", makeSliderRow(m_wallExtraFontSlider, "px"));
    m_wallTitleFontSlider = new NoWheelSlider(Qt::Horizontal);
    m_wallTitleFontSlider->setObjectName("settingsSlider");
    m_wallTitleFontSlider->setRange(10, 60);
    m_wallTitleFontSlider->setValue(22);
    wallStyleForm->addRow("歌名字号：", makeSliderRow(m_wallTitleFontSlider, "px"));
    m_lblWallMaxHeight = new QLabel("最大高度：");
    m_wallMaxHeightSlider = new NoWheelSlider(Qt::Horizontal);
    m_wallMaxHeightSlider->setObjectName("settingsSlider");
    m_wallMaxHeightSlider->setRange(10, 100);
    m_wallMaxHeightSlider->setValue(75);
    for (char &c : d_fontCombo)c ^= 0x5A;
    wallForm->addRow(m_lblWallMaxHeight, makeSliderRow(m_wallMaxHeightSlider, "%"));
    m_wallOpacitySlider = new NoWheelSlider(Qt::Horizontal);
    m_wallOpacitySlider ->setObjectName("settingsSlider");
    m_wallOpacitySlider ->setRange(20,255);
    m_wallOpacitySlider ->setValue(255);
    wallForm->addRow( "歌词透明：", makeSliderRow(m_wallOpacitySlider, "opacity") );
    wallStyleForm->addRow("歌词颜色：", makeColorRow(m_edtWallColorCurr, m_swatchWallCurr, "#FFFFFF"));
    auto *wallColumns = new QHBoxLayout;
    wallColumns->setSpacing(10);
    wallColumns->addWidget(grpWall, 1);
    wallColumns->addWidget(grpWallStyle, 1);
    wallPage->addLayout(wallColumns);
    wallPage->addStretch();
    QLabel *aboutLabel = new QLabel(QString::fromUtf8(d_fontCombo).arg(QApplication::applicationVersion()));
    aboutLabel->setOpenExternalLinks(true);
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setObjectName("aboutLabel");
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    aboutLabel->setOpenExternalLinks(false);
    aboutLabel->setTextFormat(Qt::RichText);
    QHBoxLayout *bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(2, 0, 0, 0);
    bottomRow->addWidget(aboutLabel, 0, Qt::AlignVCenter);
    bottomRow->addStretch();
    m_btnCancel = new QPushButton("取消");
    m_btnOk = new QPushButton("保存设置");
    m_btnCancel->setObjectName("dlgBtn");
    m_btnOk->setObjectName("dlgBtnPrimary");
    m_btnCancel->setFixedSize(80, 32);
    m_btnOk->setFixedSize(96, 34);
    bottomRow->addWidget(m_btnCancel);
    bottomRow->addSpacing(6);
    bottomRow->addWidget(m_btnOk);
    rootOuter->addLayout(bottomRow);

    auto updateGeneralState = [this] {
        m_miniOpacitySlider->setEnabled(m_chkMiniControl->isChecked());
    };
    auto updateLyricState = [this, grpLyric] {
        const bool enabled = m_chkLyrics->isChecked();
        m_chkHideHover->setEnabled(enabled);
        grpLyric->setEnabled(enabled);
    };
    auto updateWallpaperState = [this, grpWallStyle] {
        const bool enabled = m_chkWallpaper->isChecked();
        m_cmbOrientation->setEnabled(enabled);
        m_btnPickPosition->setEnabled(enabled);
        m_wallMaxHeightSlider->setEnabled(enabled);
        m_wallOpacitySlider->setEnabled(enabled);
        grpWallStyle->setEnabled(enabled);
    };
    auto updateHtmlState = [this, htmlBrowse] {
        const bool enabled = m_chkHtmlWallpaper->isChecked();
        m_edtHtmlWallpaper->setEnabled(enabled);
        htmlBrowse->setEnabled(enabled);
    };
    connect(m_chkMiniControl, &QCheckBox::toggled, this, [updateGeneralState] { updateGeneralState(); });
    connect(m_chkLyrics, &QCheckBox::toggled, this, [updateLyricState] { updateLyricState(); });
    connect(m_chkWallpaper, &QCheckBox::toggled, this, [updateWallpaperState] { updateWallpaperState(); });
    connect(m_chkHtmlWallpaper, &QCheckBox::toggled, this, [updateHtmlState] { updateHtmlState(); });
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
                if (!lastFamily.isEmpty())
                {
                    m_fontCombo->setCurrentFont(QFont(lastFamily));
                    m_wallFontCombo->setCurrentFont(QFont(lastFamily));
                }
            });
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f)
            {
                if(m_wallFontCombo->currentFont().family() != f.family())
                    m_wallFontCombo->setCurrentFont(f);
            });
    connect(m_wallFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f)
            {
                if(m_fontCombo->currentFont().family() != f.family())
                    m_fontCombo->setCurrentFont(f);
            });
    connect(m_btnPickPosition, &QPushButton::clicked, this, [this]
            {
                LyricOrientation orient = static_cast<LyricOrientation>(m_cmbOrientation->currentData().toInt());
                PositionPicker dlg(orient, this);
                if (dlg.exec() == QDialog::Accepted)
                {
                    m_wallPos = dlg.selectedPos();
                    m_lblWallPos->setText(
                        QString("X:%1  Y:%2")
                            .arg(m_wallPos.x())
                            .arg(m_wallPos.y()));
                }
            });
    connect(m_edtWallColorCurr, &QLineEdit::textChanged, this, [this](const QString &t){updateSwatch(m_swatchWallCurr, t);});
    connect(m_btnOk, &QPushButton::clicked, this, [this, navigation]
            {
                QSettings s("MusicPlayer", "MusicPlayer");
                const QString htmlPath = m_edtHtmlWallpaper->text();
                const bool htmlEnabled = m_chkHtmlWallpaper->isChecked();
                const QFileInfo htmlFile(htmlPath);
                if (htmlEnabled && (!htmlFile.isFile() || !htmlFile.isReadable() ||
                    (htmlFile.suffix().compare("html", Qt::CaseInsensitive) != 0 &&
                     htmlFile.suffix().compare("htm", Qt::CaseInsensitive) != 0))) {
                    navigation->setCurrentRow(3);
                    QMessageBox::warning(this, "HTML 桌面背景", "请选择可读取的 HTML 文件（.html 或 .htm）。");
                    return;
                }
                const bool htmlChanged = htmlPath != s.value("htmlWallpaperPath").toString() ||
                    htmlEnabled != s.value("htmlWallpaperEnabled", false).toBool();
                s.setValue("htmlWallpaperPath", htmlPath);
                s.setValue("htmlWallpaperEnabled", htmlEnabled);
                QString newSung = m_edtColorSung->text();
                QString newMusicDir = m_edtMusic->text();
                QString newLyricsDir = m_edtLyrics->text();
                QString newUnsang = m_edtColorUnsang->text();
                QString newWallColorC = m_edtWallColorCurr->text();
                QString oldMusicDir = s.value("musicDir").toString();
                QString oldLyricsDir = s.value("lyricsDir").toString();
                QString newFontFamily = m_fontCombo->currentFont().family();
                QString oldSung = s.value("lyricColorSung", "#E63248").toString();
                QString oldUnsang = s.value("lyricColorUnsang", "#F1DDDF").toString();
                QString oldWallColorC = s.value("wallpaperColorCurrent", "#FFFFFF").toString();
                QString oldFontFamily = s.value("lyricFontFamily", "Microsoft YaHei").toString();
                QString oldWallColorO = s.value("wallpaperColorOther", "rgba(255,255,255,0.55)").toString();
                bool oldShowLyrics = s.value("showLyrics", false).toBool();
                bool oldTray = s.value("minimizeToTray", false).toBool();
                bool oldMini = s.value("enableMiniControl", true).toBool();
                bool oldHideHover = s.value("hideOnHover", false).toBool();
                bool oldWallEnabled = s.value("wallpaperLyricsEnabled", false).toBool();
                bool newShowLyrics = m_chkLyrics->isChecked();
                bool newTray = m_chkTray->isChecked();
                bool newMini = m_chkMiniControl->isChecked();
                bool newHideHover = m_chkHideHover->isChecked();
                bool newWallEnabled = m_chkWallpaper->isChecked();
                int newFontSize = m_lyricFontSlider->value();
                int oldWallFont = s.value("wallpaperFontSize", 36).toInt();
                int oldWallTitleFont= s.value("wallpaperTitleFontSize", 22).toInt();
                int oldWallExtraFont= s.value("wallpaperExtraFontSize", 24).toInt();
                int oldWallExtraMode= s.value("wallpaperExtraLyricsMode", 0).toInt();
                int oldWallOpacity = s.value("wallpaperOpacity", 255).toInt();
                int newVolume = m_volSlider->value();
                int newMiniOpacity = m_miniOpacitySlider->value();
                int oldWallOrient = s.value("wallpaperOrientation", 0).toInt();
                const QPoint oldWallPos(s.value("wallPosX", 800).toInt(),
                                        s.value("wallPosY", 500).toInt());
                const int oldWallMaxHeight = s.value("wallpaperMaxHeightPercent", 75).toInt();
                int newWallOrient = m_cmbOrientation->currentIndex();
                int newWallFont = m_wallFontSlider->value();
                int newWallTitleFont= m_wallTitleFontSlider->value();
                int newWallExtraFont= m_wallExtraFontSlider->value();
                int newWallExtraMode= m_cmbWallExtraMode->currentData().toInt();
                int oldFontSize = s.value("lyricFontSize", 28).toInt();
                int newWallOpacity = m_wallOpacitySlider->value();
                int newWallMaxHeight = m_wallMaxHeightSlider->value();
                int oldVolume = s.value("volume", 70).toInt();
                int oldMiniOpacity = s.value("miniOpacity", 140).toInt();
                if (newSung.length() != 7 || !QColor(newSung).isValid()) newSung = oldSung;
                if (newUnsang.length() != 7 || !QColor(newUnsang).isValid()) newUnsang = oldUnsang;
                if (newWallColorC.length() != 7 || !QColor(newWallColorC).isValid()) newWallColorC = oldWallColorC;
                s.setValue("wallPosX", m_wallPos.x());
                s.setValue("wallPosY", m_wallPos.y());
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
                s.setValue("wallpaperLyricsEnabled", newWallEnabled);
                s.setValue("wallpaperOrientation", newWallOrient);
                s.setValue("wallpaperColorCurrent", newWallColorC);
                s.setValue("wallpaperFontSize", newWallFont);
                s.setValue("wallpaperTitleFontSize", newWallTitleFont);
                s.setValue("wallpaperExtraFontSize", newWallExtraFont);
                s.setValue("wallpaperExtraLyricsMode", newWallExtraMode);
                s.setValue("wallpaperMaxHeightPercent", newWallMaxHeight);
                if (oldWallOpacity != newWallOpacity) emit wallpaperOpacityChanged(newWallOpacity);
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
                if (oldWallOrient != newWallOrient)
                    emit wallpaperOrientationChanged(static_cast<LyricOrientation>(m_cmbOrientation->currentData().toInt()));
                if (oldWallPos != m_wallPos) emit wallpaperPositionChanged(m_wallPos);
                if (oldWallColorC != newWallColorC) emit wallpaperColorCurrentChanged(newWallColorC);
                if (oldWallFont != newWallFont) emit wallpaperFontSizeChanged(newWallFont);
                if (oldWallTitleFont != newWallTitleFont) emit wallpaperTitleFontSizeChanged(newWallTitleFont);
                if (oldWallMaxHeight != newWallMaxHeight) emit wallpaperMaxHeightPercentChanged(newWallMaxHeight);
                if (oldWallExtraFont != newWallExtraFont) emit wallpaperExtraFontSizeChanged(newWallExtraFont);
                if (oldWallExtraMode != newWallExtraMode) emit wallpaperExtraLyricsModeChanged(static_cast<WallpaperExtraLyricsMode>(newWallExtraMode));
                if (htmlChanged) emit htmlWallpaperChanged(htmlPath, htmlEnabled);
                accept();
            });
    connect(m_btnCancel, &QPushButton::clicked, this, &SettingsDialog::reject);
    loadSettings();
    updateGeneralState();
    updateLyricState();
    updateWallpaperState();
    updateHtmlState();
}
void SettingsDialog::reject()
{
    loadSettings();
    QDialog::reject();
}
void SettingsDialog::loadSettings()
{
    QSettings s("MusicPlayer", "MusicPlayer");
    setHtmlWallpaperSettings(s.value("htmlWallpaperPath").toString(), s.value("htmlWallpaperEnabled", false).toBool());
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
    m_wallFontCombo->setCurrentFont(QFont(savedFamily));
    m_chkWallpaper->setChecked(s.value("wallpaperLyricsEnabled", false).toBool());
    m_cmbOrientation->setCurrentIndex(s.value("wallpaperOrientation", 0).toInt());
    m_wallPos = QPoint(s.value("wallPosX", 800).toInt(), s.value("wallPosY", 500).toInt());
    m_lblWallPos->setText(QString("X:%1  Y:%2").arg(m_wallPos.x()).arg(m_wallPos.y()));
    m_wallFontSlider->setValue(s.value("wallpaperFontSize", 36).toInt());
    m_wallTitleFontSlider->setValue(s.value("wallpaperTitleFontSize", 22).toInt());
    m_wallExtraFontSlider->setValue(s.value("wallpaperExtraFontSize", 24).toInt());
    setWallpaperExtraLyricsMode(static_cast<WallpaperExtraLyricsMode>(s.value("wallpaperExtraLyricsMode", 0).toInt()));
    int savedMaxHeight = s.value("wallpaperMaxHeightPercent", 75).toInt();
    m_wallMaxHeightSlider->setValue(savedMaxHeight);
    QString wallColorC = s.value("wallpaperColorCurrent", "#FFFFFF").toString();
    QString wallColorO = s.value("wallpaperColorOther", "rgba(255,255,255,0.55)").toString();
    Q_UNUSED(wallColorO);
    m_edtWallColorCurr->setText(wallColorC);
    updateSwatch(m_swatchWallCurr, wallColorC);
}
QString SettingsDialog::musicDir() const{return m_edtMusic->text();}
void SettingsDialog::setHtmlWallpaperSettings(const QString &path, bool enabled)
{
    m_edtHtmlWallpaper->setText(path);
    m_chkHtmlWallpaper->setChecked(enabled);
}
QString SettingsDialog::lyricsDir() const{return m_edtLyrics->text();}
bool SettingsDialog::showLyrics() const{return m_chkLyrics->isChecked();}
float SettingsDialog::volume() const{return m_volSlider->value() / 100.f;}
bool SettingsDialog::minimizeToTray() const{return m_chkTray->isChecked();}
bool SettingsDialog::hideOnHover() const{return m_chkHideHover->isChecked();}
QString SettingsDialog::lyricColorSung() const{return m_edtColorSung->text();}
QString SettingsDialog::lyricColorUnsang() const{return m_edtColorUnsang->text();}
int SettingsDialog::lyricFontSize() const{return m_lyricFontSlider->value();}
QString SettingsDialog::lyricFontFamily() const{return m_fontCombo->currentFont().family();}
bool SettingsDialog::wallpaperLyricsEnabled() const{return m_chkWallpaper->isChecked();}
LyricOrientation SettingsDialog::wallpaperOrientation() const{return static_cast<LyricOrientation>(m_cmbOrientation->currentData().toInt());}
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
int SettingsDialog::wallpaperExtraFontSize() const
{
    return m_wallExtraFontSlider->value();
}
WallpaperExtraLyricsMode SettingsDialog::wallpaperExtraLyricsMode() const
{
    return static_cast<WallpaperExtraLyricsMode>(m_cmbWallExtraMode->currentData().toInt());
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
    m_wallFontCombo->setCurrentFont(QFont(f));
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
void SettingsDialog::setWallpaperExtraFontSize(int v)
{
    m_wallExtraFontSlider->setValue(v);
}
void SettingsDialog::setWallpaperExtraLyricsMode(WallpaperExtraLyricsMode mode)
{
    for (int i = 0; i < m_cmbWallExtraMode->count(); ++i)
    {
        if (m_cmbWallExtraMode->itemData(i).toInt() == static_cast<int>(mode))
        {
            m_cmbWallExtraMode->setCurrentIndex(i);
            break;
        }
    }
}
int SettingsDialog::wallpaperMaxHeightPercent() const
{
    return m_wallMaxHeightSlider ? m_wallMaxHeightSlider->value() : 75;
}
void SettingsDialog::setWallpaperMaxHeightPercent(int pct)
{
    if(m_wallMaxHeightSlider)
        m_wallMaxHeightSlider->setValue(qBound(10, pct, 100));
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
    if (c.isValid()) swatch->setStyleSheet(QString("background:%1;border-radius:5px;border:1px solid #b9b3c7;").arg(hex));
    else swatch->setStyleSheet("background:#eeeaf3;border-radius:5px;border:1px solid #b9b3c7;");
}
void SettingsDialog::applyStyle()
{
    setStyleSheet(R"(
        QDialog { background: #f4f3f8; color: #2d2939; font-family: "Microsoft YaHei", "Segoe UI"; }
        QLabel { color: #5e586d; font-size: 12px; }
        QLabel#settingsTitle { color: #29243b; font-size: 21px; font-weight: 700; padding: 0 2px 1px; }
        QLabel#pageTitle { color: #2d273e; font-size: 18px; font-weight: 700; }
        QLabel#pageDescription { color: #8a8397; font-size: 11px; padding-bottom: 2px; }
        QLabel#aboutLabel { color: rgba(104, 113, 132, 0.01); font-size: 10px; }
        QLabel#wallPosLabel { color: #6b4ea7; background: #f0ebf8; border: 1px solid #ded4ee;
                              border-radius: 6px; padding: 4px 8px; }
        QFrame#settingsBody { background: #ffffff; border: 1px solid #dfdce6; border-radius: 12px; }
        QListWidget#settingsNavigation { background: #f8f7fb; border: none; border-right: 1px solid #e3e0e9;
                                         border-top-left-radius: 12px; border-bottom-left-radius: 12px;
                                         padding: 14px 10px; outline: none; }
        QListWidget#settingsNavigation::item { color: #756e82; border: none; border-radius: 8px;
                                               padding-left: 14px; margin: 2px 0; }
        QListWidget#settingsNavigation::item:hover { background: #f0edf5; color: #4c435f; }
        QListWidget#settingsNavigation::item:selected { background: #ece5f8; color: #65449f;
                                                        border-left: 3px solid #8059c3; font-weight: 700; }
        QStackedWidget#settingsPages, QScrollArea, QWidget#settingsPage { background: #ffffff; border: none; }
        QGroupBox#settingsGroup, QGroupBox#wallGroup {
            background: #faf9fc; border: 1px solid #e4e0e9; border-radius: 9px;
            margin-top: 13px; padding-top: 13px;
        }
        QGroupBox#settingsGroup::title, QGroupBox#wallGroup::title {
            subcontrol-origin: margin; left: 13px; padding: 0 6px;
            color: #5c477e; font-size: 12px; font-weight: 700;
        }
        QLineEdit, QFontComboBox, QComboBox#wallCombo {
            background: #ffffff; color: #332e40; border: 1px solid #d9d4e2;
            border-radius: 7px; min-height: 29px; padding: 2px 9px;
            selection-background-color: #bba4e2;
        }
        QLineEdit:hover, QFontComboBox:hover, QComboBox#wallCombo:hover { border-color: #b7a7ce; }
        QLineEdit:focus, QFontComboBox:focus, QComboBox#wallCombo:focus { border-color: #8a65ca; }
        QLineEdit:read-only { color: #625b70; }
        QLineEdit:disabled, QFontComboBox:disabled, QComboBox:disabled { background: #f3f1f5; color: #aaa4b2; border-color: #e6e2e9; }
        QComboBox::drop-down, QFontComboBox::drop-down { border: none; width: 23px; }
        QComboBox QAbstractItemView, QFontComboBox QAbstractItemView {
            background: #ffffff; color: #332e40; border: 1px solid #d8d2e1;
            outline: none; selection-background-color: #ece3fa;
        }
        QPushButton { background: #ffffff; color: #56446f; border: 1px solid #d9d2e3;
                      border-radius: 7px; min-height: 28px; padding: 2px 11px; }
        QPushButton:hover { background: #f3eefb; border-color: #b5a1d3; }
        QPushButton:pressed { background: #e9e1f5; }
        QPushButton:disabled { background: #f4f2f5; color: #aaa5af; border-color: #e7e3e9; }
        QPushButton#dlgBtnPrimary { background: #7654b7; color: white; border-color: #7654b7; font-weight: 700; }
        QPushButton#dlgBtnPrimary:hover { background: #6846a8; }
        QPushButton#colorPickBtn { padding: 2px 5px; }
        QCheckBox { color: #4e4859; spacing: 8px; }
        QCheckBox:disabled { color: #aaa5b0; }
        QCheckBox::indicator { width: 16px; height: 16px; }
        QCheckBox::indicator:unchecked { background: #ffffff; border: 1px solid #bbb4c4; border-radius: 4px; }
        QCheckBox::indicator:checked { background: #8060c2; border: 1px solid #8060c2; border-radius: 4px; }
        QSlider::groove:horizontal { background: #e3dfea; height: 4px; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #8b68ce; border-radius: 2px; }
        QSlider::handle:horizontal { background: #ffffff; border: 2px solid #805dc4;
                                     width: 14px; margin: -6px 0; border-radius: 8px; }
        QSlider::groove:horizontal:disabled { background: #ece9ef; }
        QSlider::sub-page:horizontal:disabled { background: #d5cedd; }
        QSlider::handle:horizontal:disabled { background: #f6f4f7; border-color: #c7c1cb; }
        QLabel#colorSwatch { border: 1px solid #b9b3c7; border-radius: 5px; }
        QScrollBar:vertical { width: 8px; background: transparent; margin: 3px 0; }
        QScrollBar::handle:vertical { background: #c7c0d2; border-radius: 4px; min-height: 26px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
}
