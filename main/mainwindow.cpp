#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QUrl>
#include <QCollator>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QKeyEvent>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QRandomGenerator>
#include <QSettings>
#include <QFile>
#include <QTimer>
#include <QWindow>
#include <QGraphicsDropShadowEffect>
#include <QMediaMetaData>
#include <QImage>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QProcess>
#include <QTextStream>
#include <QStringConverter>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QInputDialog>
#include <QMessageBox>
#include <QCursor>
class BlurredBackground : public QWidget
{
public: explicit BlurredBackground(QWidget *p = nullptr) : QWidget(p)
    {
    }
    void setSourcePixmap(const QPixmap &pix)
    {
        m_source = pix;
        m_blurred = QPixmap();
        update();
    }
    void clearSource()
    {
        m_source = QPixmap();
        m_blurred = QPixmap();
        update();
    }
protected: void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        if (m_source.isNull())
        {
            painter.fillRect(rect(), QColor(230, 228, 245));
            return;
        }
        if (m_blurred.isNull() || m_blurred.size() != size()) m_blurred = buildBlurred(size());
        painter.drawPixmap(0, 0, m_blurred);
        painter.fillRect(rect(), QColor(255, 255, 255, 60));
    }
    void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e);
        m_blurred = QPixmap();
        update();
    }
private: QPixmap buildBlurred(const QSize &targetSize) const
    {
        QPixmap scaled = m_source.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        if (scaled.size() != targetSize)
        {
            int ox = (scaled.width() - targetSize.width()) / 2;
            int oy = (scaled.height() - targetSize.height()) / 2;
            scaled = scaled.copy(ox, oy, targetSize.width(), targetSize.height());
        }
        QGraphicsScene scene;
        QGraphicsPixmapItem *item = scene.addPixmap(scaled);
        item->setPos(0, 0);
        auto *blur = new QGraphicsBlurEffect;
        blur->setBlurRadius(32);
        blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
        item->setGraphicsEffect(blur);
        QPixmap result(targetSize);
        result.fill(Qt::transparent);
        QPainter p(&result);
        scene.render(&p, QRectF(), QRectF(0, 0, targetSize.width(), targetSize.height()));
        return result;
    }
    QPixmap m_source;
    mutable QPixmap m_blurred;
}
;
class IconButton : public QPushButton
{
public:
    enum Icon
    {
        Play, Pause, Prev, Next
    }
    ;
    explicit IconButton(Icon icon, QWidget *p = nullptr) : QPushButton(p), m_icon(icon)
    {
        setText(QString());
    }
    void setIcon(Icon icon)
    {
        m_icon = icon; update();
    }
protected: void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect r = rect();
        int cx = r.width() / 2;
        int cy = r.height() / 2;
        bool isPlay = (m_icon == Play || m_icon == Pause);
        if (isPlay)
        {
            bool hov = underMouse();
            QLinearGradient g(0, 0, r.width(), r.height());
            g.setColorAt(0, hov ? QColor("#a07fff") : QColor("#8b6cff"));
            g.setColorAt(1, hov ? QColor("#8060ff") : QColor("#6a4eff"));
            p.setBrush(g);
            p.setPen(Qt::NoPen);
            p.drawEllipse(r.adjusted(1, 1, -1, -1));
        }
        p.setPen(Qt::NoPen);
        if (m_icon == Play)
        {
            int th = r.height() * 28 / 74;
            int tw = th * 90 / 100;
            p.setBrush(Qt::white);
            QPolygonF tri;
            tri << QPointF(cx - tw/2.0 + 1, cy - th/2.0) << QPointF(cx + tw/2.0 + 1, cy) << QPointF(cx - tw/2.0 + 1, cy + th/2.0);
            p.drawPolygon(tri);
        }
        else if (m_icon == Pause)
        {
            int S = qMin(r.width(), r.height());
            int bw = qMax(2, S * 7 / 74);
            int bh = qMax(4, S * 22 / 74);
            int half = qMax(2, S * 5 / 74);
            int lx = cx - half - bw;
            int rx = cx + half;
            int ty = cy - bh / 2;
            p.setBrush(Qt::white);
            p.drawRoundedRect(lx, ty, bw, bh, 2, 2);
            p.drawRoundedRect(rx, ty, bw, bh, 2, 2);
        }
        else if (m_icon == Prev)
        {
            bool hov = underMouse();
            p.setBrush(hov ? Qt::white : QColor("#b0b4d8"));
            int unit = r.height() * 11 / 46;
            int lw = qMax(2, r.height() * 3 / 46);
            int totalW = lw + 2 + unit;
            int x0 = cx - totalW / 2;
            p.drawRoundedRect(x0, cy - unit/2 - 1, lw, unit + 2, 1, 1);
            QPolygonF tri;
            int tx = x0 + lw + 2;
            tri << QPointF(tx + unit, cy - unit/2.0) << QPointF(tx, cy) << QPointF(tx + unit, cy + unit/2.0);
            p.drawPolygon(tri);
        }
        else if (m_icon == Next)
        {
            bool hov = underMouse();
            p.setBrush(hov ? Qt::white : QColor("#b0b4d8"));
            int unit = r.height() * 11 / 46;
            int lw = qMax(2, r.height() * 3 / 46);
            int totalW = unit + 2 + lw;
            int x0 = cx - totalW / 2;
            QPolygonF tri;
            tri << QPointF(x0, cy - unit/2.0) << QPointF(x0 + unit, cy) << QPointF(x0, cy + unit/2.0);
            p.drawPolygon(tri);
            int lx = x0 + unit + 2;
            p.drawRoundedRect(lx, cy - unit/2 - 1, lw, unit + 2, 1, 1);
        }
    }
    void enterEvent(QEnterEvent *e) override
    {
        QPushButton::enterEvent(e); update();
    }
    void leaveEvent(QEvent *e) override
    {
        QPushButton::leaveEvent(e); update();
    }
private: Icon m_icon;
}
;
class CoverLabel : public QLabel
{
public: explicit CoverLabel(QWidget *p = nullptr) : QLabel(p)
    {
    }
    void setCoverPixmap(const QPixmap &pix)
    {
        m_cover = pix;
        setText(QString());
        update();
    }
    void clearCover()
    {
        m_cover = QPixmap(); update();
    }
protected: void paintEvent(QPaintEvent *e) override
    {
        if (m_cover.isNull())
        {
            QLabel::paintEvent(e); return;
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawRoundedRect(rect().adjusted(4, 6, -4, -2), 16, 16);
        QPixmap scaled = m_cover.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int ox = (scaled.width() - width()) / 2;
        int oy = (scaled.height() - height()) / 2;
        QPainterPath clip;
        clip.addRoundedRect(rect(), 16, 16);
        p.setClipPath(clip);
        p.drawPixmap(-ox, -oy, scaled);
    }
private: QPixmap m_cover;
}
;
class GlassDialog : public QDialog
{
public: explicit GlassDialog( const QPixmap &bg, QWidget *parent = nullptr ) : QDialog(parent)
    {
        setWindowFlags( Qt::FramelessWindowHint | Qt::Dialog );
        setAttribute( Qt::WA_TranslucentBackground );
        resize(360, 180);
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(0,0,0,0);
        auto *blur = new BlurredBackground;
        blur->setSourcePixmap(bg);
        root->addWidget(blur);
        auto *content = new QWidget;
        content->setStyleSheet(R"( QWidget
        {
            background:rgba(255,255,255,90);
            border-radius:18px;
        }
        QLabel
        {
            color:white;
            font-size:14px;
            font-weight:bold;
            background:transparent;
        }
        QLineEdit
        {
            background:rgba(255,255,255,180);
            border:none;
            border-radius:10px;
            padding:8px 12px;
            color:#1a1a2e;
            font-size:13px;
        }
        QPushButton
        {
            background:rgba(255,255,255,80);
            border:none;
            border-radius:10px;
            padding:8px 16px;
            color:white;
            font-size:13px;
        }
        QPushButton:hover
        {
            background:rgba(255,255,255,120);
        }
        )");
        auto *lay = new QVBoxLayout(content);
        lay->setContentsMargins(24,24,24,24);
        blur->layout()->addWidget(content);
    }
}
;
class MarqueeLabel : public QWidget
{
public: explicit MarqueeLabel(QWidget *p = nullptr) : QWidget(p), m_offset(0), m_scrolling(false)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(16);
        connect(m_timer, &QTimer::timeout, this, [this]
                {
                    m_offset += 1;
                    int gap = 80;
                    if (m_offset >= m_textWidth + gap) m_offset = 0;
                    update();
                }
                );
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    void setText(const QString &text)
    {
        m_text = text; recalc();
    }
    void setFont(const QFont &f)
    {
        QWidget::setFont(f); recalc();
    }
    QSize sizeHint() const override
    {
        return
            {
                0, QFontMetrics(font()).height() + 16
            }
        ;
    }
protected: void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e); recalc();
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setFont(font());
        QFontMetrics fm(font());
        const int yOff = 1;
        if (!m_scrolling)
        {
            QRect r = rect().adjusted(0, yOff, 0, yOff);
            p.setPen(QColor("#1a1a2e"));
            p.drawText(r, Qt::AlignCenter, m_text);
            return;
        }
        int y = (height() + fm.ascent() - fm.descent()) / 2 + yOff;
        p.setPen(QColor("#1a1a2e"));
        int gap = 80;
        int x1 = -m_offset;
        p.drawText(x1, y, m_text);
        int x2 = x1 + m_textWidth + gap;
        p.drawText(x2, y, m_text);
    }
private: void recalc()
    {
        QFontMetrics fm(font());
        m_textWidth = fm.horizontalAdvance(m_text);
        bool need = m_textWidth > width() - 4;
        if (need && !m_scrolling)
        {
            m_offset = 0;
            m_timer->start();
            m_scrolling = true;
        }
        else if (!need && m_scrolling)
        {
            m_timer->stop();
            m_scrolling = false;
            m_offset = 0;
            update();
        }
        else if (!need)
        {
            update();
        }
    }
    QString m_text;
    QTimer *m_timer;
    int m_offset;
    int m_textWidth = 0;
    bool m_scrolling;
}
;
class TrackDelegate : public QStyledItemDelegate
{
public: explicit TrackDelegate(QObject *p = nullptr) : QStyledItemDelegate(p)
    {
    }
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        bool sel = opt.state & QStyle::State_Selected;
        bool hov = opt.state & QStyle::State_MouseOver;
        QRect r = opt.rect;
        if (!sel && hov)
        {
            p->setPen(Qt::NoPen);
            p->setBrush(QColor(0, 0, 0, 10));
            p->drawRoundedRect(r.adjusted(2, 1, -2, -1), 8, 8);
        }
        QRect editRect(r.right() - 30, r.top(), 30, r.height());
        QRect favRect(r.right() - 58, r.top(), 28, r.height());
        QStyleOptionViewItem o(opt);
        o.rect = r.adjusted(8, 0, -62, 0);
        o.state &= ~QStyle::State_MouseOver;
        o.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(p, o, idx);
        bool isFav = idx.data(Playlist::FavoriteRole).toBool();
        QFont heartFont = p->font();
        heartFont.setPointSize(13);
        p->setFont(heartFont);
        if (isFav)
        {
            p->setPen(QColor("#e74c6a"));
            p->drawText(favRect, Qt::AlignCenter, "♥");
        }
        else
        {
            QColor heartColor = hov ? QColor(180, 60, 100, 180) : QColor(0, 0, 0, 80);
            p->setPen(heartColor);
            p->drawText(favRect, Qt::AlignCenter, "♡");
        }
        QColor editColor = sel ? QColor(0, 0, 0, 230) : hov ? QColor(0, 0, 0, 220) : QColor(0, 0, 0, 170);
        QFont editFont = p->font();
        editFont.setPointSize(14);
        p->setFont(editFont);
        p->setPen(editColor);
        p->drawText(editRect, Qt::AlignCenter, "✎");
        p->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(0, 30);
    }
}
;
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAcceptDrops(true);
    setMinimumSize(560, 380);
    resize(560, 410);
    setWindowTitle("音乐播放器");
    setWindowIcon(QIcon("app.ico"));
    m_plManager = new PlaylistManager(this);
    setupPlayer();
    setupUI();
    connect(m_plManager, &PlaylistManager::favoriteChanged, this, [this](const QString &filePath, bool)
            {
                m_playlist->notifyFavoriteChanged(filePath);
            }
            );
    m_miniControl = new MiniControlWindow;
    connect(m_miniControl, &MiniControlWindow::playPauseClicked, this, &MainWindow::onPlayPause);
    connect(m_miniControl, &MiniControlWindow::prevClicked, this, &MainWindow::onPrevious);
    connect(m_miniControl, &MiniControlWindow::nextClicked, this, &MainWindow::onNext);
    applyStyleSheet();
    m_lyricsOverlay = new LyricsOverlay(nullptr);
    connect(m_lyricsOverlay, &LyricsOverlay::closeRequested, this, [this]
            {
                m_showLyrics = false;
                m_lyricsOverlay->hide();
            }
            );
    m_settingsDlg = new SettingsDialog(this);
    connect(m_settingsDlg, &SettingsDialog::musicDirChanged, this, &MainWindow::onMusicDirChanged);
    connect(m_settingsDlg, &SettingsDialog::lyricsDirChanged, this, &MainWindow::onLyricsDirChanged);
    connect(m_settingsDlg, &SettingsDialog::showLyricsChanged, this, [this](bool v)
            {
                m_showLyrics = v;
                m_lyricsOverlay->setLyricsVisible(v);
            }
            );
    connect(m_settingsDlg, &SettingsDialog::volumeChanged, this, [this](float v)
            {
                m_audio->setVolume(v);
            }
            );
    connect(m_settingsDlg, &SettingsDialog::minimizeToTrayChanged, this, [this](bool v)
            {
                m_minimizeToTray = v;
            }
            );
    connect(m_settingsDlg, &SettingsDialog::hideOnHoverChanged, this, [this](bool v)
            {
                m_lyricsOverlay->setHideOnHover(v);
            }
            );
    connect(m_settingsDlg, &SettingsDialog::miniControlChanged, this, [this](bool v)
            {
                m_enableMiniControl = v;
                if (v) m_miniControl->show(); else m_miniControl->hide();
            }
            );
    connect(m_settingsDlg, &SettingsDialog::miniOpacityChanged, this, [this](int v)
            {
                m_miniControl->setOpacityValue(v);
            }
            );
    connect(m_settingsDlg, &SettingsDialog::lyricColorsChanged, this, [this](const QString &sung, const QString &unsang)
            {
                m_lyricsOverlay->setColors(sung, unsang);
            }
            );
    connect(m_settingsDlg, &SettingsDialog::lyricFontSizeChanged, this, [this](int size)
            {
                m_lyricsOverlay->setFontSize(size);
            }
            );
    m_trayIcon = new QSystemTrayIcon(windowIcon(), this);
    m_trayMenu = new QMenu(this);
    m_trayMenu->setStyleSheet(R"( QMenu
    {
        background:rgba(255,255,255,245);
        border:1px solid rgba(0,0,0,0.12);
        padding:6px;
        border-radius:10px;
    }
    QMenu::item
    {
        background:transparent;
        color:#1a1a2e;
        padding:8px 24px;
    }
    QMenu::item:selected
    {
        background:rgba(139,108,255,0.12);
        border-radius:6px;
    }
    )");
    auto *actShow = m_trayMenu->addAction("显示主窗口");
    auto *actPlay = m_trayMenu->addAction("播放/暂停");
    auto *actNext = m_trayMenu->addAction("下一首");
    m_trayMenu->addSeparator();
    auto *actQuit = m_trayMenu->addAction("退出");
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setToolTip("音乐播放器");
    m_trayIcon->show();
    connect(actShow, &QAction::triggered, this, [this]
            {
                showNormal(); raise(); activateWindow();
            }
            );
    connect(actPlay, &QAction::triggered, this, &MainWindow::onPlayPause);
    connect(actNext, &QAction::triggered, this, &MainWindow::onNext);
    connect(actQuit, &QAction::triggered, this, [this]
            {
                m_minimizeToTray = false;
                if (m_trayIcon) m_trayIcon->hide();
                qApp->quit();
            }
            );
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    QSettings s("MusicPlayer", "MusicPlayer");
    m_musicDir = s.value("musicDir").toString();
    m_lyricsDir = s.value("lyricsDir").toString();
    m_showLyrics = s.value("showLyrics", false).toBool();
    m_minimizeToTray = s.value("minimizeToTray", false).toBool();
    m_enableMiniControl = s.value("enableMiniControl", true).toBool();
    if (m_enableMiniControl) m_miniControl->show();
    m_audio->setVolume(s.value("volume", 70).toInt() / 100.f);
    m_lyricsOverlay->setHideOnHover(s.value("hideOnHover", false).toBool());
    m_lyricsOverlay->setColors( s.value("lyricColorSung", "#E63248").toString(), s.value("lyricColorUnsang","#F1DDDF").toString() );
    m_lyricsOverlay->setFontSize(s.value("lyricFontSize", 28).toInt());
    if (!m_musicDir.isEmpty()) loadDir(m_musicDir);
    m_miniControl->setOpacityValue(s.value("miniOpacity", 85).toInt());
    if (m_enableMiniControl) m_miniControl->show();
    QTimer::singleShot(400, this, [this]
                       {
                           restorePlaybackState();
                       }
                       );
    if (s.value("alwaysOnTop", false).toBool())
    {
        m_alwaysOnTop = true;
        m_btnPin->setStyleSheet("background:rgba(139,108,255,200);color:white;border:none;");
        m_btnPin->setToolTip("取消置顶");
        QTimer::singleShot(0, this, [this]
                           {
                               if (windowHandle()) windowHandle()->setFlag(Qt::WindowStaysOnTopHint, true);
                           }
                           );
    }
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(10000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::savePlaybackState);
    m_autoSaveTimer->start();
}
MainWindow::~MainWindow()
{
}
void MainWindow::setupPlayer()
{
    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_playlist = new Playlist(this);
    m_playlist->setPlaylistManager(m_plManager);
    m_player->setAudioOutput(m_audio);
    m_audio->setVolume(0.7f);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &MainWindow::onMediaErrorOccurred);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this]
            {
                QVariant v = m_player->metaData().value(QMediaMetaData::ThumbnailImage);
                if (v.isValid())
                {
                    QImage img = v.value<QImage>();
                    if (!img.isNull())
                    {
                        QPixmap pix = QPixmap::fromImage(img);
                        m_miniControl->setCover(pix);
                        static_cast<CoverLabel*>(m_lblCover)->setCoverPixmap(pix);
                        return;
                    }
                }
                static_cast<CoverLabel*>(m_lblCover)->clearCover();
                m_lblCover->setText("♪");
            }
            );
}
void MainWindow::setupUI()
{
    QWidget *cen = new QWidget(this);
    setCentralWidget(cen);
    auto *bgCover = new BlurredBackground(cen);
    m_bgCover = bgCover;
    m_bgCover->setGeometry(rect());
    m_bgCover->lower();
    QVBoxLayout *vroot = new QVBoxLayout(cen);
    vroot->setContentsMargins(0, 0, 0, 0);
    vroot->setSpacing(0);
    m_titleBar = new QWidget;
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(38);
    QHBoxLayout *th = new QHBoxLayout(m_titleBar);
    th->setContentsMargins(12, 0, 8, 0);
    th->setSpacing(0);
    QLabel *winIcon = new QLabel("♫");
    winIcon->setObjectName("winIcon");
    winIcon->setFixedWidth(22);
    m_lblWinTitle = new QLabel("音乐播放器");
    m_lblWinTitle->setObjectName("winTitle");
    auto makeSysBtn = [](const QString &obj)
    {
        auto *b = new QPushButton;
        b->setObjectName(obj);
        b->setFixedSize(38, 38);
        b->setFlat(true);
        return b;
    }
    ;
    m_btnMin = makeSysBtn("wbMin");
    m_btnPin = makeSysBtn("wbPin");
    m_btnClose = makeSysBtn("wbClose");
    m_btnMin->setText("－");
    m_btnPin->setText("📌");
    m_btnClose->setText("✕");
    th->addWidget(winIcon);
    th->addSpacing(6);
    th->addWidget(m_lblWinTitle, 1);
    th->addWidget(m_btnMin);
    th->addWidget(m_btnPin);
    th->addWidget(m_btnClose);
    connect(m_btnMin, &QPushButton::clicked, this, [this]
            {
                if (m_minimizeToTray && m_trayIcon && m_trayIcon->isVisible()) hide();
                else showMinimized();
            }
            );
    connect(m_btnPin, &QPushButton::clicked, this, &MainWindow::onTogglePin);
    connect(m_btnClose, &QPushButton::clicked, this, &QMainWindow::close);
    vroot->addWidget(m_titleBar);
    QWidget *content = new QWidget;
    content->setObjectName("contentArea");
    QHBoxLayout *root = new QHBoxLayout(content);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    vroot->addWidget(content, 1);
    QWidget *left = new QWidget;
    left->setObjectName("leftPanel");
    left->setFixedWidth(250);
    QVBoxLayout *lv = new QVBoxLayout(left);
    lv->setContentsMargins(24, 24, 24, 24);
    auto *coverLbl = new CoverLabel;
    m_lblCover = coverLbl;
    m_lblCover->setObjectName("coverLabel");
    m_lblCover->setFixedSize(170, 170);
    m_lblCover->setAlignment(Qt::AlignCenter);
    m_lblCover->setText("♪");
    QHBoxLayout *cr = new QHBoxLayout;
    cr->addStretch();
    cr->addWidget(m_lblCover);
    cr->addStretch();
    lv->addLayout(cr);
    lv->addSpacing(90);
    QWidget *infoWrap = new QWidget;
    infoWrap->setObjectName("infoWrap");
    infoWrap->setFixedHeight(70);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWrap);
    infoLayout->setContentsMargins(1, 0, 1, 0);
    infoLayout->setSpacing(2);
    auto *marquee = new MarqueeLabel;
    m_lblTitle = marquee;
    m_lblTitle->setObjectName("musicTitle");
    {
        QFont f = font();
        f.setBold(true);
        marquee->setFont(f);
        marquee->updateGeometry();
    }
    marquee->setMinimumHeight(84);
    marquee->setText("暂无歌曲");
    m_lblArtist = new QLabel("拖拽音乐到窗口");
    m_lblArtist->setObjectName("artistLabel");
    m_lblArtist->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(m_lblTitle);
    infoLayout->addWidget(m_lblArtist);
    lv->addWidget(infoWrap);
    QHBoxLayout *sr = new QHBoxLayout;
    m_lblTimeElapsed = new QLabel("0:00");
    m_lblTimeTotal = new QLabel("0:00");
    m_lblTimeElapsed->setObjectName("timeLabel");
    m_lblTimeTotal->setObjectName("timeLabel");
    m_seekBar = new QSlider(Qt::Horizontal);
    m_seekBar->setFixedHeight(24);
    m_seekBar->setObjectName("seekBar");
    sr->addWidget(m_lblTimeElapsed);
    sr->addWidget(m_seekBar, 1);
    sr->addWidget(m_lblTimeTotal);
    lv->addLayout(sr);
    lv->addSpacing(-6);
    QWidget *ctrlWrap = new QWidget;
    ctrlWrap->setObjectName("ctrlWrap");
    ctrlWrap->setFixedHeight(90);
    QHBoxLayout *ctrl = new QHBoxLayout(ctrlWrap);
    ctrl->setContentsMargins(0, 0, 0, 0);
    ctrl->setSpacing(-5);
    m_btnPrev = new IconButton(IconButton::Prev);
    m_btnPlayPause = new IconButton(IconButton::Play);
    m_btnNext = new IconButton(IconButton::Next);
    m_btnPrev->setObjectName("prevBtn");
    m_btnPlayPause->setObjectName("playBtn");
    m_btnNext->setObjectName("nextBtn");
    m_btnPrev->setFixedSize(52, 52);
    m_btnNext->setFixedSize(52, 52);
    m_btnPlayPause->setFixedSize(74, 74);
    ctrl->addStretch();
    ctrl->addWidget(m_btnPrev, 0, Qt::AlignRight | Qt::AlignVCenter);
    ctrl->addSpacing(18);
    ctrl->addSpacing(-6);
    ctrl->addWidget(m_btnPlayPause, 0, Qt::AlignCenter);
    ctrl->addSpacing(8);
    ctrl->addSpacing(18);
    ctrl->addWidget(m_btnNext, 0, Qt::AlignLeft | Qt::AlignVCenter);
    ctrl->addStretch();
    ctrl->setAlignment(Qt::AlignCenter);
    lv->addWidget(ctrlWrap);
    lv->addStretch();
    QWidget *right = new QWidget;
    right->setObjectName("rightPanel");
    QVBoxLayout *rv = new QVBoxLayout(right);
    rv->setContentsMargins(0, 0, 0, 0);
    QWidget *hdr = new QWidget;
    hdr->setObjectName("plHeader");
    QHBoxLayout *hh = new QHBoxLayout(hdr);
    hh->setContentsMargins(16, 6, 16, 6);
    QLabel *plTit = new QLabel("播放列表");
    plTit->setObjectName("plTitle");
    m_btnPlaylistSwitch = new QPushButton("播放目录 ▾");
    m_btnPlaylistSwitch->setObjectName("playlistSwitchBtn");
    m_btnPlaylistSwitch->setToolTip("切换/管理歌单");
    m_lblStatus = new QLabel("0 首");
    m_lblStatus->setObjectName("statusLabel");
    m_btnShuffle = new QPushButton("顺序");
    m_btnShuffle->setObjectName("modeBtn");
    m_btnSettings = new QPushButton("⚙");
    m_btnSettings->setObjectName("settingsBtn");
    hh->addWidget(plTit);
    hh->addSpacing(6);
    hh->addWidget(m_btnPlaylistSwitch);
    hh->addSpacing(8);
    hh->addWidget(m_lblStatus);
    hh->addStretch();
    hh->addWidget(m_btnShuffle);
    hh->addSpacing(8);
    hh->addWidget(m_btnSettings);
    m_listView = new QListView;
    m_listView->setMouseTracking(true);
    m_listView->setObjectName("playlistView");
    m_listView->setItemDelegate(new TrackDelegate(this));
    m_listView->setModel(m_playlist);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    rv->addWidget(hdr);
    rv->addWidget(m_listView, 1);
    root->addWidget(left);
    root->addWidget(right, 1);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevious);
    connect(m_btnNext, &QPushButton::clicked, this, &MainWindow::onNext);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this]
            {
                QVariant v = m_player->metaData().value(QMediaMetaData::ThumbnailImage);
                if (v.isValid())
                {
                    QImage img = v.value<QImage>();
                    if (!img.isNull())
                    {
                        QPixmap pix = QPixmap::fromImage(img);
                        static_cast<CoverLabel*>(m_lblCover)->setCoverPixmap(pix);
                        static_cast<BlurredBackground*>(m_bgCover)->setSourcePixmap(pix);
                        return;
                    }
                }
                static_cast<CoverLabel*>(m_lblCover)->clearCover();
                m_lblCover->setText("♪");
                static_cast<BlurredBackground*>(m_bgCover)->clearSource();
            }
            );
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::onOpenSettings);
    connect(m_btnPlaylistSwitch,&QPushButton::clicked, this, &MainWindow::onPlaylistSwitchClicked);
    connect(m_listView, &QListView::doubleClicked, this, &MainWindow::onPlaylistDoubleClicked);
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex &idx)
            {
                QPoint pos = m_listView->viewport()->mapFromGlobal(QCursor::pos());
                QRect r = m_listView->visualRect(idx);
                QRect editRect(r.right() - 30, r.top(), 30, r.height());
                QRect favRect (r.right() - 58, r.top(), 28, r.height());
                if (editRect.contains(pos))
                {
                    editLyricsFile(idx.row());
                }
                else if (favRect.contains(pos))
                {
                    onToggleFavorite(idx.row());
                }
            }
            );
    connect(m_listView, &QListView::customContextMenuRequested, this, [this](const QPoint &pos)
            {
                QModelIndex idx = m_listView->indexAt(pos);
                if (!idx.isValid()) return;
                showAddToPlaylistMenu(idx.row(), m_listView->viewport()->mapToGlobal(pos));
            }
            );
    connect(m_seekBar, &QSlider::sliderPressed, this, [this]
            {
                m_seeking = true;
            }
            );
    connect(m_seekBar, &QSlider::sliderReleased, this, [this]
            {
                m_seeking = false;
                m_player->setPosition(m_seekBar->value());
            }
            );
    connect(m_btnShuffle, &QPushButton::clicked, this, [this]
            {
                QString color;
                if (m_playMode == Sequential)
                {
                    m_playMode = RepeatOne;
                    m_btnShuffle->setText("单曲");
                    color = "#f8c6b5";
                }
                else if (m_playMode == RepeatOne)
                {
                    m_playMode = Shuffle;
                    m_btnShuffle->setText("乱序");
                    color = "#29AFD4";
                }
                else
                {
                    m_playMode = Sequential;
                    m_btnShuffle->setText("顺序");
                    color = "#A172D0";
                }
                m_btnShuffle->setStyleSheet( QString("background:%1;border:1px solid rgba(0,0,0,0.12);" "border-radius:8px;padding:5px 12px;color:white;").arg(color) );
            }
            );
}
void MainWindow::applyStyleSheet()
{
    setStyleSheet(R"( QMainWindow,QWidget
    {
        background:transparent;
        color:#1a1a2e;
        font-family:"Microsoft YaHei","PingFang SC","Segoe UI";
        font-size:12px;
    }
    QWidget#titleBar
    {
        background:rgba(255,255,255,170);
        border-bottom:1px solid rgba(0,0,0,0.07);
    }
    QLabel#winIcon
    {
        font-size:16px;
        color:#8b6cff;
        background:transparent;
    }
    QLabel#winTitle
    {
        font-size:12px;
        color:#2a2a40;
        background:transparent;
    }
    QPushButton#wbMin, QPushButton#wbPin, QPushButton#wbMax, QPushButton#wbClose
    {
        background:transparent;
        border:none;
        color:#606080;
        font-size:13px;
    }
    QPushButton#wbMin:hover, QPushButton#wbPin:hover, QPushButton#wbMax:hover
    {
        background:rgba(0,0,0,0.07);
        color:#1a1a2e;
    }
    QPushButton#wbClose:hover
    {
        background:#e81123;
        color:white;
    }
    QWidget#leftPanel
    {
        background:rgba(255,255,255,160);
        border-right:1px solid rgba(255,255,255,0.6);
    }
    QLabel#coverLabel
    {
        background:rgba(220,220,235,180);
        border-radius:20px;
        border:1px solid rgba(255,255,255,0.8);
        font-size:48px;
        color:#9b8ec4;
    }
    QLabel#titleLabel
    {
        font-size:15px;
        font-weight:bold;
        color:#1a1a2e;
    }
    QWidget#musicTitle
    {
        font-size:18px;
        font-weight:bold;
        color:#1a1a2e;
    }
    QLabel#artistLabel
    {
        font-size:11px;
        color:#6b6e90;
    }
    QLabel#timeLabel
    {
        font-size:10px;
        color:#9093b0;
    }
    QSlider#seekBar::groove:horizontal
    {
        height:5px;
        background:rgba(0,0,0,0.12);
        border-radius:2px;
    }
    QSlider#seekBar::sub-page:horizontal
    {
        background:#8b6cff;
        border-radius:3px;
    }
    QSlider#seekBar::handle:horizontal
    {
        width:12px;
        background:white;
        border-radius:6px;
        border:1px solid rgba(0,0,0,0.15);
        margin:-4px 0;
    }
    QPushButton#playBtn
    {
        background:transparent;
        border:none;
    }
    QPushButton#prevBtn, QPushButton#nextBtn
    {
        background:transparent;
        border:none;
    }
    QWidget#rightPanel
    {
        background:rgba(255,255,255,110);
    }
    QWidget#plHeader
    {
        background:rgba(255,255,255,140);
        border-bottom:1px solid rgba(0,0,0,0.06);
    }
    QLabel#plTitle
    {
        font-size:12px;
        font-weight:bold;
        color:#2a2a3e;
    }
    QLabel#statusLabel
    {
        color:#8890b0;
    }
    QPushButton#modeBtn
    {
        background:rgba(255,255,255,180);
        border:1px solid rgba(0,0,0,0.12);
        border-radius:8px;
        padding:5px 12px;
        color:#5a5e80;
    }
    QPushButton#settingsBtn
    {
        background:transparent;
        border:none;
        color:#7a7e9e;
        font-size:16px;
    }
    QPushButton#settingsBtn:hover
    {
        color:#2a2a3e;
    }
    QPushButton#playlistSwitchBtn
    {
        background:rgba(139,108,255,0.10);
        border:1px solid rgba(139,108,255,0.30);
        border-radius:8px;
        padding:3px 10px;
        color:#7a5ccc;
        font-size:11px;
    }
    QPushButton#playlistSwitchBtn:hover
    {
        background:rgba(139,108,255,0.18);
        border-color:rgba(139,108,255,0.55);
        color:#5a3caa;
    }
    QListView#playlistView
    {
        background:transparent;
        border:none;
        outline:none;
        padding:6px;
    }
    QScrollBar:vertical
    {
        background:transparent;
        width:6px;
    }
    QScrollBar::handle:vertical
    {
        background:rgba(0,0,0,0.15);
        border-radius:3px;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical
    {
        height:0;
    }
    )");
}
void MainWindow::onPlaylistSwitchClicked()
{
    showPlaylistSwitchMenu();
}
void MainWindow::showPlaylistSwitchMenu()
{
    QMenu menu(this);
    menu.setStyleSheet(R"( QMenu
    {
        background:rgba(255,255,255,248);
        border:1px solid rgba(0,0,0,0.10);
        padding:6px 4px;
        border-radius:10px;
        min-width:160px;
    }
    QMenu::item
    {
        background:transparent;
        color:#1a1a2e;
        padding:7px 20px;
        border-radius:6px;
    }
    QMenu::item:selected
    {
        background:rgba(139,108,255,0.12);
        color:#5a3caa;
    }
    QMenu::separator
    {
        height:1px;
        background:rgba(0,0,0,0.07);
        margin:4px 10px;
    }
    )");
    QStringList names = m_plManager->playlistNames();
    if (m_currentPlaylistName != "全部音乐")
    {
        QAction *allAct = menu.addAction("全部音乐");
        connect(allAct, &QAction::triggered, this, [this]
                {
                    if (m_musicDir.isEmpty()) return;
                    QString currentFile;
                    if (m_currentIndex >= 0 && m_currentIndex < m_playlist->count())
                    {
                        currentFile = m_playlist->track(m_currentIndex).filePath;
                    }
                    m_currentPlaylistName = "全部音乐";
                    QStringList audioFiles;
                    QDirIterator it( m_musicDir, QDir::Files, QDirIterator::Subdirectories );
                    while (it.hasNext())
                    {
                        it.next();
                        QFileInfo fi = it.fileInfo();
                        if (QStringList
                            {
                                "mp3","flac","wav","ogg", "aac","m4a","wma","opus","ape"
                            }
                                .contains(fi.suffix().toLower()))
                        {
                            audioFiles << fi.absoluteFilePath();
                        }
                    }
                    QList<TrackItem> tracks;
                    int keepIndex = -1;
                    for (const QString &path : audioFiles)
                    {
                        QFileInfo fi(path);
                        TrackItem t;
                        t.filePath = fi.absoluteFilePath();
                        t.title = fi.completeBaseName();
                        if (t.filePath == currentFile) keepIndex = tracks.size();
                        tracks.append(t);
                    }
                    m_playlist->clear();
                    m_playlist->addTracks(tracks);
                    m_lblStatus->setText( QString("%1 首") .arg(m_playlist->count()) );
                    m_btnPlaylistSwitch->setText( "全部音乐 ▾" );
                    if (keepIndex >= 0)
                    {
                        m_currentIndex = keepIndex;
                        m_listView->setCurrentIndex( m_playlist->index(keepIndex) );
                    }
                    else if (m_playlist->count() > 0)
                    {
                        playTrack(0);
                    }
                    else
                    {
                        m_player->stop();
                        m_currentIndex = -1;
                        m_lblTitle->setText("暂无歌曲");
                        m_lblArtist->setText("歌单为空");
                        m_lblTimeElapsed->setText("0:00");
                        m_lblTimeTotal->setText("0:00");
                        m_seekBar->setValue(0);
                        m_listView->clearSelection();
                        static_cast<CoverLabel*>(m_lblCover) ->clearCover();
                        m_lblCover->setText("♪");
                    }
                }
                );
        menu.addSeparator();
    }
    for (const QString &name : names)
    {
        if (name == m_currentPlaylistName) continue;
        QAction *act = menu.addAction(name);
        connect(act, &QAction::triggered, this, [this, name]
                {
                    QStringList files = m_plManager->tracks(name);
                    QString currentFile;
                    if (m_currentIndex >= 0 && m_currentIndex < m_playlist->count())
                    {
                        currentFile = m_playlist->track(m_currentIndex).filePath;
                    }
                    m_currentPlaylistName = name;
                    QList<TrackItem> tracks;
                    int keepIndex = -1;
                    for (const QString &path : files)
                    {
                        QFileInfo fi(path);
                        if (!fi.exists()) continue;
                        TrackItem t;
                        t.filePath = fi.absoluteFilePath();
                        t.title = fi.completeBaseName();
                        if (t.filePath == currentFile) keepIndex = tracks.size();
                        tracks.append(t);
                    }
                    m_playlist->clear();
                    m_playlist->addTracks(tracks);
                    m_lblStatus->setText( QString("%1 首") .arg(m_playlist->count()) );
                    m_btnPlaylistSwitch->setText( name + " ▾" );
                    if (keepIndex >= 0)
                    {
                        m_currentIndex = keepIndex;
                        m_listView->setCurrentIndex( m_playlist->index(keepIndex) );
                    }
                    else if (m_playlist->count() > 0)
                    {
                        playTrack(0);
                    }
                    else
                    {
                        m_player->stop();
                        m_currentIndex = -1;
                        m_lblTitle->setText("暂无歌曲");
                        m_lblArtist->setText("歌单为空");
                        m_lblTimeElapsed->setText("0:00");
                        m_lblTimeTotal->setText("0:00");
                        m_seekBar->setValue(0);
                        m_listView->clearSelection();
                        static_cast<CoverLabel*>(m_lblCover) ->clearCover();
                        m_lblCover->setText("♪");
                    }
                }
                );
    }
    menu.addSeparator();
    QAction *actNew = menu.addAction("＋  新建歌单");
    connect(actNew, &QAction::triggered, this, [this]
            {
                bool ok = false;
                QDialog dlg(this);
                dlg.setWindowFlags( Qt::FramelessWindowHint | Qt::Dialog );
                dlg.setAttribute( Qt::WA_TranslucentBackground );
                dlg.resize(360, 180);
                QVBoxLayout *root = new QVBoxLayout(&dlg);
                root->setContentsMargins(0,0,0,0);
                auto *bg = new BlurredBackground;
                auto *mainBg = static_cast<BlurredBackground*>( m_bgCover );
                bg->setSourcePixmap( mainBg->grab() );
                root->addWidget(bg);
                QVBoxLayout *lay = new QVBoxLayout(bg);
                lay->setContentsMargins( 24,24,24,24 );
                QLabel *title = new QLabel("新建歌单");
                QLineEdit *edit = new QLineEdit;
                edit->setPlaceholderText( "输入歌单名称" );
                QHBoxLayout *btns = new QHBoxLayout;
                QPushButton *okBtn = new QPushButton("创建");
                QPushButton *cancelBtn = new QPushButton("取消");
                btns->addStretch();
                btns->addWidget(cancelBtn);
                btns->addWidget(okBtn);
                lay->addWidget(title);
                lay->addSpacing(12);
                lay->addWidget(edit);
                lay->addStretch();
                lay->addLayout(btns);
                bg->setStyleSheet(R"( QWidget
        {
            background:rgba(255,255,255,255);
            border-radius:18px;
        }
        QLabel
        {
            color:#1a1a2e;
            font-size:14px;
            font-weight:bold;
            background:transparent;
        }
        QLineEdit
        {
            background:rgba(255,255,255,230);
            border:1px solid rgba(0,0,0,0.10);
            border-radius:10px;
            padding:8px 12px;
            color:#1a1a2e;
            font-size:13px;
            selection-background-color:#8b6cff;
        }
        QPushButton
        {
            background:rgba(255,255,255,190);
            border:1px solid rgba(0,0,0,0.08);
            border-radius:10px;
            padding:8px 16px;
            color:#1a1a2e;
            font-size:13px;
        }
        QPushButton:hover
        {
            background:rgba(255,255,255,230);
        }
        )");
                connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
                connect(okBtn, &QPushButton::released, &dlg, [&]()
                        {
                            QString text = edit->text().trimmed();
                            if (text.isEmpty()) return;
                            dlg.setProperty( "playlistName", text );
                            dlg.done(QDialog::Accepted);
                        }
                        );
                if (dlg.exec() != QDialog::Accepted) return;
                QString name = dlg.property("playlistName") .toString() .trimmed();
                if (name.isEmpty()) return;
                if (!m_plManager->createPlaylist(name))
                {
                    QMessageBox::warning(this, "提示", QString("歌单「%1」已存在").arg(name));
                }
            }
            );
    QStringList all = m_plManager->playlistNames();
    all.removeAll(PlaylistManager::FAVORITES_NAME);
    if (!all.isEmpty())
    {
        QMenu *delMenu = menu.addMenu("✕  删除歌单");
        delMenu->setStyleSheet( menu.styleSheet() );
        for (const QString &name : all)
        {
            QAction *a = delMenu->addAction(name);
            connect(a, &QAction::triggered, this, [this, name]
                    {
                        QDialog dlg(this);
                        dlg.setWindowFlags( Qt::FramelessWindowHint | Qt::Dialog );
                        dlg.setAttribute( Qt::WA_TranslucentBackground );
                        dlg.resize(320, 170);
                        QVBoxLayout *root = new QVBoxLayout(&dlg);
                        root->setContentsMargins( 0,0,0,0 );
                        auto *bg = new BlurredBackground;
                        auto *mainBg = static_cast<BlurredBackground*>( m_bgCover );
                        bg->setSourcePixmap( mainBg->grab() );
                        root->addWidget(bg);
                        QVBoxLayout *lay = new QVBoxLayout(bg);
                        lay->setContentsMargins( 24,24,24,24 );
                        QLabel *title = new QLabel("");
                        QLabel *text = new QLabel( QString( "确定删除歌单\n“%1”？" ).arg(name) );
                        text->setWordWrap(true);
                        QHBoxLayout *btns = new QHBoxLayout;
                        QPushButton *okBtn = new QPushButton("删除");
                        QPushButton *cancelBtn = new QPushButton("取消");
                        btns->addStretch();
                        btns->addWidget(cancelBtn);
                        btns->addWidget(okBtn);
                        lay->addWidget(title);
                        lay->addSpacing(12);
                        lay->addWidget(text);
                        lay->addStretch();
                        lay->addLayout(btns);
                        bg->setStyleSheet(R"( QWidget
                {
                    background:rgba(255,255,255,255);
                    border-radius:18px;
                }
                QLabel
                {
                    color:#1a1a2e;
                    font-size:14px;
                    font-weight:bold;
                    background:transparent;
                }
                QPushButton
                {
                    background:rgba(255,255,255,190);
                    border:1px solid rgba(0,0,0,0.08);
                    border-radius:10px;
                    padding:8px 16px;
                    color:#1a1a2e;
                    font-size:13px;
                }
                QPushButton:hover
                {
                    background:rgba(255,255,255,230);
                }
                )");
                        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
                        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
                        if (dlg.exec() == QDialog::Accepted)
                        {
                            m_plManager ->removePlaylist(name);
                        }
                    }
                    );
        }
    }
    menu.exec( m_btnPlaylistSwitch->mapToGlobal( QPoint( 0, m_btnPlaylistSwitch->height() ) ) );
}
void MainWindow::onToggleFavorite(int trackRow)
{
    if (trackRow < 0 || trackRow >= m_playlist->count()) return;
    QString filePath = m_playlist->track(trackRow).filePath;
    m_plManager->toggleFavorite(filePath);
}
void MainWindow::showAddToPlaylistMenu(int trackRow, const QPoint &globalPos)
{
    if (trackRow < 0 || trackRow >= m_playlist->count()) return;
    QString filePath = m_playlist->track(trackRow).filePath;
    QMenu menu(this);
    menu.setStyleSheet(R"( QMenu
    {
        background:rgba(255,255,255,248);
        border:1px solid rgba(0,0,0,0.10);
        padding:6px 4px;
        border-radius:10px;
        min-width:150px;
    }
    QMenu::item
    {
        background:transparent;
        color:#1a1a2e;
        padding:7px 20px;
        border-radius:6px;
    }
    QMenu::item:selected
    {
        background:rgba(139,108,255,0.12);
        color:#5a3caa;
    }
    QMenu::separator
    {
        height:1px;
        background:rgba(0,0,0,0.07);
        margin:4px 10px;
    }
    )");
    QAction *headerAct = menu.addAction("加入歌单");
    headerAct->setEnabled(false);
    menu.addSeparator();
    QStringList names = m_plManager->playlistNames();
    for (const QString &name : names)
    {
        bool already = m_plManager->contains(name, filePath);
        QString label = already ? QString("✔ %1").arg(name) : name;
        QAction *act = menu.addAction(label);
        connect(act, &QAction::triggered, this, [this, name, filePath, already]
                {
                    if (already) m_plManager->removeFromPlaylist(name, filePath);
                    else m_plManager->addToPlaylist(name, filePath);
                }
                );
    }
    menu.addSeparator();
    QAction *actNew = menu.addAction("＋  新建并加入");
    connect(actNew, &QAction::triggered, this, [this, filePath]
            {
                bool ok = false;
                QString name = QInputDialog::getText( this, "新建歌单", "歌单名称：", QLineEdit::Normal, QString(), &ok );
                name = name.trimmed();
                if (!ok || name.isEmpty()) return;
                m_plManager->createPlaylist(name);
                m_plManager->addToPlaylist(name, filePath);
            }
            );
    menu.exec(globalPos);
}
void MainWindow::onAddToPlaylist(const QString &playlistName, int trackRow)
{
    if (trackRow < 0 || trackRow >= m_playlist->count()) return;
    QString filePath = m_playlist->track(trackRow).filePath;
    m_plManager->addToPlaylist(playlistName, filePath);
}
void MainWindow::onPlayPause()
{
    auto state = m_player->playbackState();
    if (state == QMediaPlayer::PlayingState)
    {
        m_player->pause();
        return;
    }
    if (m_currentIndex < 0)
    {
        if (m_playlist->count() > 0) playTrack(0);
        return;
    }
    m_player->play();
}
void MainWindow::onPrevious()
{
    int p = m_currentIndex - 1;
    if (p < 0) p = m_playlist->count() - 1;
    if (p >= 0) playTrack(p);
}
void MainWindow::onNext()
{
    int n = nextTrackIndex();
    if (n >= 0) playTrack(n);
}
void MainWindow::onPlaylistDoubleClicked(const QModelIndex &idx)
{
    playTrack(idx.row());
}
void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus s)
{
    if (s == QMediaPlayer::EndOfMedia)
    {
        int n = nextTrackIndex();
        if (n >= 0) playTrack(n); else m_player->stop();
    }
}
void MainWindow::onPlaybackStateChanged(QMediaPlayer::PlaybackState s)
{
    m_miniControl->setPlaying(s == QMediaPlayer::PlayingState);
    static_cast<IconButton*>(m_btnPlayPause)->setIcon( s == QMediaPlayer::PlayingState ? IconButton::Pause : IconButton::Play );
}
void MainWindow::onDurationChanged(qint64 d)
{
    m_seekBar->setRange(0, (int)d);
    m_lblTimeTotal->setText(formatTime(d));
    if (m_currentIndex >= 0) m_playlist->updateDuration(m_currentIndex, d);
}
void MainWindow::onPositionChanged(qint64 pos)
{
    if (!m_seeking)
    {
        m_seekBar->setValue((int)pos);
        m_lblTimeElapsed->setText(formatTime(pos));
    }
    if (m_showLyrics && m_lyricsOverlay->isVisible()) m_lyricsOverlay->updatePosition(pos);
}
void MainWindow::onMediaErrorOccurred(QMediaPlayer::Error, const QString &msg)
{
    m_lblStatus->setText("错误: " + msg);
}
void MainWindow::onOpenSettings()
{
    m_settingsDlg->setMusicDir(m_musicDir);
    m_settingsDlg->setLyricsDir(m_lyricsDir);
    m_settingsDlg->setShowLyrics(m_showLyrics);
    m_settingsDlg->setVolume(m_audio->volume());
    QSettings s("MusicPlayer", "MusicPlayer");
    m_settingsDlg->setMinimizeToTray(s.value("minimizeToTray", false).toBool());
    m_settingsDlg->setHideOnHover(s.value("hideOnHover", false).toBool());
    m_settingsDlg->setLyricColorSung(s.value("lyricColorSung", "#E63248").toString());
    m_settingsDlg->setLyricColorUnsang(s.value("lyricColorUnsang", "#F1DDDF").toString());
    m_settingsDlg->setLyricFontSize(s.value("lyricFontSize", 28).toInt());
    m_settingsDlg->exec();
}
void MainWindow::onMusicDirChanged(const QString &dir)
{
    m_musicDir = dir;
    if (dir.isEmpty()) return;
    m_player->stop();
    m_playlist->clear();
    m_currentIndex = -1;
    loadDir(dir);
}
void MainWindow::onLyricsDirChanged(const QString &dir)
{
    m_lyricsDir = dir;
    if (m_currentIndex >= 0) m_lyricsOverlay->loadLyrics(findLyricFile(m_playlist->track(m_currentIndex).filePath));
}
void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        if (isHidden())
        {
            showNormal(); raise(); activateWindow();
        }
        else hide();
    }
}
void MainWindow::onTogglePin()
{
    m_alwaysOnTop = !m_alwaysOnTop;
    if (windowHandle()) windowHandle()->setFlag(Qt::WindowStaysOnTopHint, m_alwaysOnTop);
    QSettings s("MusicPlayer", "MusicPlayer");
    s.setValue("alwaysOnTop", m_alwaysOnTop);
    if (m_alwaysOnTop)
    {
        m_btnPin->setStyleSheet( "background:rgba(139,108,255,200);color:white;border:none;");
        m_btnPin->setToolTip("取消置顶");
    }
    else
    {
        m_btnPin->setStyleSheet(QString());
        m_btnPin->setToolTip("置顶");
    }
}
void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent *e)
{
    QStringList p;
    for (const QUrl &u : e->mimeData()->urls()) if (u.isLocalFile()) p << u.toLocalFile();
    loadFiles(p);
}
void MainWindow::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_Space: onPlayPause(); break;
    case Qt::Key_Left: m_player->setPosition(m_player->position() - 5000); break;
    case Qt::Key_Right: m_player->setPosition(m_player->position() + 5000); break;
    case Qt::Key_N: onNext(); break;
    case Qt::Key_P: onPrevious(); break;
    default: QMainWindow::keyPressEvent(e);
    }
}
void MainWindow::closeEvent(QCloseEvent *e)
{
    savePlaybackState();
    QSettings s("MusicPlayer", "MusicPlayer");
    s.setValue("playMode", (int)m_playMode);
    m_player->stop();
    m_lyricsOverlay->close();
    m_miniControl->close();
    e->accept();
}
void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (m_bgCover) m_bgCover->setGeometry(rect());
}
void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_titleBar)
    {
        if (m_titleBar->geometry().contains(e->pos()))
        {
            m_dragging = true;
            m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
            e->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(e);
}
void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton))
    {
        move(e->globalPosition().toPoint() - m_dragOffset);
        e->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(e);
}
void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QMainWindow::mouseReleaseEvent(e);
}
void MainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    QMainWindow::mouseDoubleClickEvent(e);
}
void MainWindow::savePlaybackState()
{
    if (m_currentIndex < 0) return;
    QSettings s("MusicPlayer", "MusicPlayer");
    s.setValue("lastIndex", m_currentIndex);
    s.setValue("lastPosition", m_player->position());
    s.setValue("currentPlaylist", m_currentPlaylistName);
}
void MainWindow::restorePlaybackState()
{
    QSettings s("MusicPlayer", "MusicPlayer");
    m_playMode = (PlayMode)s.value("playMode", 0).toInt();
    QString color;
    switch (m_playMode)
    {
    case Sequential: m_btnShuffle->setText("顺序"); color = "#A172D0"; break;
    case RepeatOne: m_btnShuffle->setText("单曲"); color = "#f8c6b5"; break;
    case Shuffle: m_btnShuffle->setText("乱序"); color = "#29AFD4"; break;
    }
    m_btnShuffle->setStyleSheet( QString("background:%1;border:1px solid rgba(0,0,0,0.12);" "border-radius:8px;padding:5px 12px;color:white;").arg(color) );
    int index = s.value("lastIndex", -1).toInt();
    qint64 pos = s.value("lastPosition", 0).toLongLong();
    QString playlistName =s.value("currentPlaylist", "全部音乐").toString();
    if (playlistName == "全部音乐")
    {
        if (!m_musicDir.isEmpty())
        {
            m_playlist->clear();
            loadDir(m_musicDir);
            m_btnPlaylistSwitch->setText( "全部音乐 ▾" );
        }
    }
    else
    {
        QStringList files = m_plManager->tracks(playlistName);
        m_playlist->clear();
        QList<TrackItem> tracks;
        for (const QString &path : files)
        {
            QFileInfo fi(path);
            if (!fi.exists()) continue;
            TrackItem t;
            t.filePath = fi.absoluteFilePath();
            t.title = fi.completeBaseName();
            tracks.append(t);
        }
        m_playlist->addTracks(tracks);
        m_btnPlaylistSwitch->setText( playlistName + " ▾" );
        m_currentPlaylistName = playlistName;
    }
    if (index >= 0 && index < m_playlist->count())
    {
        m_currentIndex = index;
        TrackItem t = m_playlist->track(index);
        m_player->setSource(QUrl::fromLocalFile(t.filePath));
        m_lblTitle->setText(t.title);
        if (m_miniControl) m_miniControl->setTitle(t.title);
        m_lblArtist->setText(t.artist.isEmpty() ? "未知艺术家" : t.artist);
        setWindowTitle(t.title + " - 音乐播放器 by AliceCartelet");
        updateTitleBarTitle(t.title + " - 音乐播放器 by AliceCartelet");
        m_listView->setCurrentIndex(m_playlist->index(index));
        m_lyricsOverlay->loadLyrics(findLyricFile(t.filePath));
        connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this, pos](QMediaPlayer::MediaStatus status)
                {
                    if (status == QMediaPlayer::LoadedMedia) m_player->setPosition(pos);
                }
                , Qt::SingleShotConnection);
    }
}
static const QStringList &audioExts()
{
    static QStringList e =
        {
            "mp3","flac","wav","ogg","aac","m4a","wma","opus","ape"
        }
    ;
    return e;
}
void MainWindow::loadDir(const QString &dir)
{
    QStringList audioFiles;
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        QFileInfo fi = it.fileInfo();
        if (audioExts().contains(fi.suffix().toLower())) audioFiles << fi.absoluteFilePath();
    }
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    auto category = [](const QString &name) -> int
    {
        if (name.isEmpty()) return 2;
        QChar ch = name.at(0);
        ushort u = ch.unicode();
        if ((u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z')) return 0;
        if (u >= 0x4E00 && u <= 0x9FFF) return 1;
        return 2;
    }
    ;
    std::sort(audioFiles.begin(), audioFiles.end(), [&](const QString &a, const QString &b)
              {
                  QString nameA = QFileInfo(a).completeBaseName();
                  QString nameB = QFileInfo(b).completeBaseName();
                  int catA = category(nameA), catB = category(nameB);
                  if (catA != catB) return catA < catB;
                  return collator.compare(nameA, nameB) < 0;
              }
              );
    QList<TrackItem> tracks;
    for (const QString &path : audioFiles)
    {
        QFileInfo fi(path);
        TrackItem t;
        t.filePath = fi.absoluteFilePath();
        t.title = fi.completeBaseName();
        tracks.append(t);
    }
    if (tracks.isEmpty()) return;
    int first = m_playlist->count();
    m_playlist->addTracks(tracks);
    m_lblStatus->setText(QString("%1 首").arg(m_playlist->count()));
    if (m_currentIndex < 0) playTrack(first);
}
void MainWindow::loadFiles(const QStringList &paths)
{
    QList<TrackItem> tracks;
    for (const QString &p : paths)
    {
        QFileInfo fi(p);
        if (!fi.exists()) continue;
        if (fi.isDir())
        {
            loadDir(p); continue;
        }
        if (audioExts().contains(fi.suffix().toLower()))
        {
            TrackItem t;
            t.filePath = fi.absoluteFilePath();
            t.title = fi.completeBaseName();
            tracks.append(t);
        }
    }
    if (tracks.isEmpty()) return;
    int first = m_playlist->count();
    m_playlist->addTracks(tracks);
    m_lblStatus->setText(QString("%1 首").arg(m_playlist->count()));
    if (m_currentIndex < 0) playTrack(first);
}
QString MainWindow::findLyricFile(const QString &audioPath) const
{
    QFileInfo afi(audioPath);
    QString base = afi.fileName();
    QString stem = afi.completeBaseName();
    QStringList dirs;
    if (!m_lyricsDir.isEmpty()) dirs << m_lyricsDir;
    dirs << afi.absolutePath();
    for (const QString &d : dirs)
    {
        QDir dir(d);
        for (const QString &suf : QStringList
             {
                 " - .lrcx"," - .lrc",".lrcx",".lrc"
             }
             )
        {
            QString cand = suf.startsWith(' ') ? dir.filePath(base + suf) : dir.filePath(stem + suf);
            if (QFile::exists(cand)) return cand;
        }
    }
    return
        {
        }
    ;
}
void MainWindow::updateTitleBarTitle(const QString &title)
{
    if (m_lblWinTitle) m_lblWinTitle->setText(title);
}
void MainWindow::playTrack(int index)
{
    if (index < 0 || index >= m_playlist->count()) return;
    m_currentIndex = index;
    TrackItem t = m_playlist->track(index);
    m_player->setSource(QUrl::fromLocalFile(t.filePath));
    m_player->play();
    m_lblTitle->setText(t.title);
    if (m_miniControl) m_miniControl->setTitle(t.title);
    m_lblArtist->setText(t.artist.isEmpty() ? "未知艺术家" : t.artist);
    setWindowTitle(t.title + " - 音乐播放器 - AliceCartelet");
    updateTitleBarTitle(t.title + " - 音乐播放器 - AliceCartelet");
    m_listView->setCurrentIndex(m_playlist->index(index));
    m_lyricsOverlay->loadLyrics(findLyricFile(t.filePath));
    if (m_showLyrics) m_lyricsOverlay->show();
}
int MainWindow::nextTrackIndex() const
{
    int n = m_playlist->count();
    if (!n) return -1;
    if (m_playMode == RepeatOne) return m_currentIndex;
    if (m_playMode == Shuffle) return QRandomGenerator::global()->bounded(n);
    int nx = m_currentIndex + 1;
    return (nx < n) ? nx : -1;
}
QString MainWindow::formatTime(qint64 ms) const
{
    int s = ms / 1000;
    int m = s / 60;
    s %= 60;
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}
void MainWindow::editLyricsFile(int row)
{
    if (row < 0 || row >= m_playlist->count()) return;
    TrackItem t = m_playlist->track(row);
    QString lyricPath = findLyricFile(t.filePath);
    if (lyricPath.isEmpty())
    {
        QFileInfo fi(t.filePath);
        QDir dir(!m_lyricsDir.isEmpty() ? m_lyricsDir : fi.absolutePath());
        lyricPath = dir.filePath(fi.fileName() + " - .lrcx");
        QFile file(lyricPath);
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << "[00:00.00]\n";
            file.close();
        }
    }
    QProcess::startDetached("notepad.exe", QStringList() << lyricPath);
}