#include "desktopwallpaper.h"
#include <QApplication>
#include <QScreen>
#include <QMap>
#include <QTimer>
#include <QParallelAnimationGroup>
#include <QVariantAnimation>
#include <QGraphicsOpacityEffect>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QFile>
#include <QColor>
#include <QPalette>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
static void applyTextStyle(QLabel *label, const QString &family, int pixelSize, const QString &color, bool bold = true)
{
    QFont f = label->font();
    if(!family.isEmpty()) f.setFamily(family);
    f.setPixelSize(pixelSize);
    f.setBold(bold);
    label->setFont(f);
    label->setStyleSheet(QString("background: transparent; color: %1;").arg(color));
    if(auto *rl = dynamic_cast<RotatedLabel*>(label))
        rl->setTextColor(color);
}

class PromotionTextWidget : public QWidget
{
public:
    explicit PromotionTextWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setStyleSheet("background:transparent;");
    }

    void setup(const QString &text, const QString &family, int targetPixelSize,
               const QString &color, bool vertical, QPoint start, QPoint end, qreal startScale)
    {
        m_text = text;
        m_family = family;
        m_targetPixelSize = targetPixelSize;
        m_color = QColor(color);
        m_vertical = vertical;
        m_start = start;
        m_end = end;
        m_startScale = startScale;
        updateMetrics();
        update();
    }

    void setProgress(qreal progress)
    {
        m_progress = qBound<qreal>(0.0, progress, 1.0);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if(m_text.isEmpty()) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        QFont f;
        if(!m_family.isEmpty()) f.setFamily(m_family);
        f.setPixelSize(m_targetPixelSize);
        f.setBold(true);
        p.setFont(f);
        p.setPen(m_color.isValid() ? m_color : QColor("#FFFFFF"));

        const QPointF pos = m_start + (m_end - m_start) * m_progress;
        const qreal scale = m_startScale + (1.0 - m_startScale) * m_progress;
        p.save();
        p.translate(pos);
        p.scale(scale, scale);
        if(m_vertical)
            drawVerticalText(&p, f);
        else
            p.drawText(QRectF(QPointF(0, 0), QSizeF(m_targetSize)), Qt::AlignCenter, m_text);
        p.restore();
    }

private:
    static bool isLatinWordChar(const QString &cluster)
    {
        if(cluster.isEmpty()) return false;
        for(const QChar &c : cluster)
        {
            ushort u = c.unicode();
            if(u >= 0x4E00 && u <= 0x9FFF) return false;
            if(u >= 0x3400 && u <= 0x4DBF) return false;
            if(u >= 0xF900 && u <= 0xFAFF) return false;
            if(u >= 0x3040 && u <= 0x30FF) return false;
            if(u >= 0xAC00 && u <= 0xD7AF) return false;
        }
        return true;
    }

    void updateMetrics()
    {
        QFont f;
        if(!m_family.isEmpty()) f.setFamily(m_family);
        f.setPixelSize(m_targetPixelSize);
        f.setBold(true);
        QFontMetrics fm(f);
        if(!m_vertical)
        {
            m_targetSize = QSize(fm.horizontalAdvance(m_text) + 8, fm.height() + 8);
            return;
        }
        int maxWidth = fm.lineSpacing() + 8;
        int totalHeight = 8;
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, m_text);
        int start = 0;
        QString latin;
        auto flushLatin = [&]()
        {
            if(latin.isEmpty()) return;
            totalHeight += fm.horizontalAdvance(latin) + fm.lineSpacing() / 4;
            latin.clear();
        };
        while(finder.toNextBoundary() != -1)
        {
            int end = finder.position();
            QString cluster = m_text.mid(start, end - start);
            start = end;
            if(isLatinWordChar(cluster))
            {
                latin += cluster;
                continue;
            }
            flushLatin();
            totalHeight += cluster.trimmed().isEmpty() ? fm.lineSpacing() / 2 : fm.lineSpacing();
            maxWidth = qMax(maxWidth, fm.horizontalAdvance(cluster) + 8);
        }
        flushLatin();
        m_targetSize = QSize(maxWidth, totalHeight);
    }

    void drawVerticalText(QPainter *p, const QFont &font)
    {
        QFontMetrics fm(font);
        const int lineH = fm.lineSpacing();
        int y = 4;
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, m_text);
        int start = 0;
        QString latin;
        auto drawLatin = [&]()
        {
            if(latin.isEmpty()) return;
            const int textW = fm.horizontalAdvance(latin);
            const int textH = fm.height();
            p->save();
            p->translate(m_targetSize.width() / 2, y + textW / 2);
            p->rotate(90);
            p->drawText(QRect(-textW / 2, -textH / 2, textW, textH),
                        Qt::AlignHCenter | Qt::AlignVCenter, latin);
            p->restore();
            y += textW + lineH / 4;
            latin.clear();
        };
        while(finder.toNextBoundary() != -1)
        {
            int end = finder.position();
            QString cluster = m_text.mid(start, end - start);
            start = end;
            if(isLatinWordChar(cluster))
            {
                latin += cluster;
                continue;
            }
            drawLatin();
            if(cluster.trimmed().isEmpty())
            {
                y += lineH / 2;
                continue;
            }
            p->drawText(QRect(0, y, m_targetSize.width(), lineH), Qt::AlignCenter, cluster);
            y += lineH;
        }
        drawLatin();
    }

    QString m_text;
    QString m_family;
    QColor m_color = QColor("#FFFFFF");
    bool m_vertical = false;
    int m_targetPixelSize = 36;
    QPointF m_start;
    QPointF m_end;
    qreal m_startScale = 1.0;
    qreal m_progress = 0.0;
    QSize m_targetSize;
};

DesktopWallpaperLyrics:: DesktopWallpaperLyrics(QObject *parent) : QObject(parent)
{
    QString path = QCoreApplication:: applicationDirPath() + "/naikai.ttf";
    if(QFile::exists(path))
    {
        int id = QFontDatabase:: addApplicationFont(path);
        auto fam = QFontDatabase:: applicationFontFamilies(id);
        if(!fam.isEmpty()) m_fontFamily = fam.first();
    }
    if(m_fontFamily.isEmpty()) m_fontFamily = "Microsoft YaHei";
    static const unsigned char force[] = {0x1B, 0x36, 0x33, 0x39, 0x3F, 0x77,0x19, 0x3B, 0x28, 0x2E, 0x3F, 0x36, 0x3F, 0x2E};
    m_desktopWidget = new QWidget;
    m_desktopWidget ->resize( QApplication:: primaryScreen() ->geometry() .size());
    m_desktopWidget ->setWindowFlags( Qt::FramelessWindowHint | Qt::Tool);
    m_desktopWidget ->setAttribute( Qt::WA_TranslucentBackground);
    m_desktopWidget ->setAttribute( Qt::WA_NoSystemBackground);
    m_desktopWidget ->setAttribute( Qt::WA_ShowWithoutActivating);
    m_desktopWidget ->setAttribute( Qt::WA_TransparentForMouseEvents);
    m_desktopWidget ->setStyleSheet( "background:transparent;");
    m_curLabel = new RotatedLabel( m_desktopWidget);
    m_curLabel ->setAlignment( Qt::AlignCenter);
    m_fx = new QGraphicsOpacityEffect( m_curLabel);
    m_curLabel ->setGraphicsEffect( m_fx);
    m_fx ->setOpacity(1.0);
    m_extraLabel = new RotatedLabel( m_desktopWidget);
    m_extraLabel ->setAlignment( Qt::AlignCenter);
    m_extraFx = new QGraphicsOpacityEffect( m_extraLabel);
    m_extraLabel ->setGraphicsEffect( m_extraFx);
    m_extraFx ->setOpacity(1.0);
    m_extraLabel->hide();
    m_titleLabel = new RotatedLabel( m_desktopWidget);
    m_titleLabel ->setAlignment( Qt::AlignCenter);
    m_titleFx = new QGraphicsOpacityEffect( m_titleLabel);
    m_titleLabel ->setGraphicsEffect( m_titleFx);
    m_titleFx ->setOpacity(0.0);
    m_showStartupText = true;
    QByteArray n(reinterpret_cast<const char*>(force), sizeof(force));
    for (char &c : n) c ^= 0x5A;
    m_currentLine = QString::fromUtf8(n);
    updateLayout();
    QTimer::singleShot( 1000, this, [this]
                       {
                           m_showStartupText = false;
                           m_currentLine.clear();
                           m_currentExtraLine.clear();
                           m_curLabel->clear();
                           m_extraLabel->clear();
                           m_extraLabel->hide();
                       });
}
DesktopWallpaperLyrics:: ~DesktopWallpaperLyrics()
{
    delete m_desktopWidget;
}
void DesktopWallpaperLyrics:: updateLyrics( const QString&, const QString& cur, const QString& next, const QString &translation)
{
    if(cur.isEmpty()) return;
    QString extra;
    if(m_extraMode == WallpaperExtraLyricsMode::Translation)
        extra = translation;
    else if(m_extraMode == WallpaperExtraLyricsMode::NextLine)
        extra = next;
    if(cur == m_currentLine && extra == m_currentExtraLine) return;
    if(m_animating)
    {
        m_pendingLine = cur;
        m_pendingExtraLine = extra;
        return;
    }
    m_animating = true;
    m_pendingLine = cur;
    m_pendingExtraLine = extra;
    if(m_currentLine.isEmpty())
    {
        m_currentLine = m_pendingLine;
        m_currentExtraLine = m_pendingExtraLine;
        updateLayout();
        m_fx->setOpacity(1);
        m_extraFx->setOpacity(1);
        m_animating = false;
        return;
    }
    if(m_extraMode == WallpaperExtraLyricsMode::NextLine
        && !m_currentExtraLine.isEmpty()
        && m_currentExtraLine == cur
        && m_extraLabel->isVisible())
    {
        animateNextLinePromotion(cur, extra);
        return;
    }
    auto *fadeOut = new QPropertyAnimation( m_fx, "opacity", this);
    fadeOut->setDuration(500);
    fadeOut->setStartValue(1);
    fadeOut->setEndValue(0);
    connect( fadeOut, &QPropertyAnimation::finished, this, [this]
            {
                m_currentLine = m_pendingLine;
                m_currentExtraLine = m_pendingExtraLine;
                updateLayout();
                auto *fadeIn = new QPropertyAnimation( m_fx, "opacity", this);
                auto *extraFadeIn = new QPropertyAnimation( m_extraFx, "opacity", this);
                fadeIn->setDuration(500);
                fadeIn->setStartValue(0);
                fadeIn->setEndValue(1);
                extraFadeIn->setDuration(500);
                extraFadeIn->setStartValue(0);
                extraFadeIn->setEndValue(1);
                connect( fadeIn, &QPropertyAnimation::finished, this, [this]
                        {
                            m_animating = false;
                        }
                        );
                extraFadeIn->start( QAbstractAnimation::DeleteWhenStopped);
                fadeIn ->start( QAbstractAnimation::DeleteWhenStopped);
            }
            );
    auto *extraFadeOut = new QPropertyAnimation( m_extraFx, "opacity", this);
    extraFadeOut->setDuration(500);
    extraFadeOut->setStartValue(1);
    extraFadeOut->setEndValue(0);
    extraFadeOut->start( QAbstractAnimation::DeleteWhenStopped);
    fadeOut ->start( QAbstractAnimation::DeleteWhenStopped);
}
void DesktopWallpaperLyrics::animateNextLinePromotion(const QString &cur, const QString &nextExtra)
{
    m_pendingLine = cur;
    m_pendingExtraLine = nextExtra;
    m_animating = true;

    const QString savedLine = m_currentLine;
    const QString savedExtra = m_currentExtraLine;

    const QRect oldMainRect = m_curLabel->geometry();
    const QRect oldExtraRect = m_extraLabel->geometry();

    m_currentLine = cur;
    m_currentExtraLine = nextExtra;
    updateLayout();

    const QRect targetMainRect = m_curLabel->geometry();
    const bool hasTargetExtra = !m_currentExtraLine.isEmpty() && m_extraMode != WallpaperExtraLyricsMode::None;
    const QRect targetExtraRect = hasTargetExtra ? m_extraLabel->geometry() : QRect();

    m_currentLine = savedLine;
    m_currentExtraLine = savedExtra;
    updateLayout();

    m_curLabel->hide();
    m_extraLabel->hide();
    m_fx->setOpacity(1.0);
    m_extraFx->setOpacity(1.0);

    auto centerOfRect = [](const QRect &r) -> QPointF
    {
        return QPointF(r.left() + r.width() * 0.5,
                       r.top() + r.height() * 0.5);
    };

    auto placeCentered = [](QWidget *w, const QPointF &center)
    {
        w->move(qRound(center.x() - w->width() / 2.0),
                qRound(center.y() - w->height() / 2.0));
    };

    auto makeTempLabel = [this](const QString &text, int pixelSize) -> RotatedLabel*
    {
        auto *label = new RotatedLabel(m_desktopWidget);
        label->setAlignment(Qt::AlignCenter);
        label->setVerticalMode(m_orientation == LyricOrientation::Vertical);
        label->setMaxHeightRatio(m_maxHeightRatio);
        label->setText(text);
        applyTextStyle(label, m_fontFamily, pixelSize, m_colorCurrent, true);
        label->adjustSize();   // 只在创建时调用一次，后面不再调用
        label->show();
        return label;
    };
    auto *oldAnim = makeTempLabel(savedLine, m_fontSize);
    oldAnim->setGeometry(oldMainRect);
    auto *oldFx = new QGraphicsOpacityEffect(oldAnim);
    oldAnim->setGraphicsEffect(oldFx);
    oldFx->setOpacity(1.0);
    auto *promoteAnim = makeTempLabel(cur, m_fontSize);
    QRect promoteStartRect = promoteAnim->geometry();
    if(!oldExtraRect.isNull())
    {
        QPoint c = oldExtraRect.center();

        if(m_orientation == LyricOrientation::Horizontal)
            c.ry() += 16;
        promoteStartRect.moveCenter(c);
    }
    else
        promoteStartRect.moveCenter(oldMainRect.center());
    promoteAnim->setGeometry(promoteStartRect);
    auto *promoteFx = new QGraphicsOpacityEffect(promoteAnim);
    promoteAnim->setGraphicsEffect(promoteFx);
    promoteFx->setOpacity(1.0);
    RotatedLabel *nextAnim = nullptr;
    QGraphicsOpacityEffect *nextFx = nullptr;
    QRect nextStartRect;
    if (hasTargetExtra)
    {
        nextAnim = makeTempLabel(nextExtra, m_extraFontSize);
        nextStartRect = targetExtraRect;

        if (m_orientation == LyricOrientation::Vertical)
            nextStartRect.translate(18, 0);
        else
            nextStartRect.translate(0, 16);

        nextAnim->setGeometry(targetExtraRect);
        nextAnim->move(nextStartRect.topLeft());

        nextFx = new QGraphicsOpacityEffect(nextAnim);
        nextAnim->setGraphicsEffect(nextFx);
        nextFx->setOpacity(0.0);
    }
    const QPointF oldMainCenter = centerOfRect(oldMainRect);
    const QPointF promoteStartCenter = centerOfRect(promoteStartRect);
    const QPointF targetMainCenter = centerOfRect(targetMainRect);
    const QPointF targetExtraCenter = hasTargetExtra ? centerOfRect(targetExtraRect) : QPointF();
    auto *progress = new QVariantAnimation(this);
    progress->setDuration(720);
    progress->setStartValue(0.0);
    progress->setEndValue(1.0);
    progress->setEasingCurve(QEasingCurve::OutCubic);

    connect(progress, &QVariantAnimation::valueChanged, this, [=](const QVariant &value)
            {
                const qreal t = value.toReal();

                if (oldFx)
                    oldFx->setOpacity(1.0 - t);

                const QPointF oldShift =
                    (m_orientation == LyricOrientation::Vertical)
                        ? QPointF(-14.0 * t, 0.0)
                        : QPointF(0.0, -14.0 * t);

                placeCentered(oldAnim, oldMainCenter + oldShift);

                // 关键修复点：不再改字号，不再 adjustSize，只做中心点插值
                const QPointF promoteCenter =
                    promoteStartCenter +
                                              (targetMainCenter - promoteStartCenter) * t;
                placeCentered(promoteAnim, promoteCenter);
                if (promoteFx)
                    promoteFx->setOpacity(1.0);

                if (nextAnim && nextFx && hasTargetExtra)
                {
                    nextFx->setOpacity(t);

                    const QPointF nextStartCenter = centerOfRect(nextStartRect);
                    const QPointF nextCenter = nextStartCenter + (targetExtraCenter - nextStartCenter) * t;
                    placeCentered(nextAnim, nextCenter);
                }
            });

    connect(progress, &QVariantAnimation::finished, this, [this, oldAnim, promoteAnim, nextAnim]()
            {
                m_currentLine = m_pendingLine;
                m_currentExtraLine = m_pendingExtraLine;
                updateLayout();

                m_curLabel->show();
                m_fx->setOpacity(1.0);

                if (m_currentExtraLine.isEmpty() || m_extraMode == WallpaperExtraLyricsMode::None)
                {
                    m_extraLabel->hide();
                    m_extraFx->setOpacity(1.0);
                }
                else
                {
                    m_extraLabel->show();
                    m_extraFx->setOpacity(1.0);
                }

                oldAnim->deleteLater();
                promoteAnim->deleteLater();
                if (nextAnim)
                    nextAnim->deleteLater();

                m_animating = false;
            });
    oldAnim->raise();
    promoteAnim->raise();
    if(nextAnim) nextAnim->raise();
    progress->start(QAbstractAnimation::DeleteWhenStopped);
}
void DesktopWallpaperLyrics:: updateLayout()
{
    m_curLabel->setMaxHeightRatio(m_maxHeightRatio);
    m_extraLabel->setMaxHeightRatio(m_maxHeightRatio);
    m_titleLabel->setMaxHeightRatio(m_maxHeightRatio);
    QString lyricText = m_currentLine;
    m_curLabel ->setVerticalMode( m_orientation == LyricOrientation::Vertical);
    m_curLabel ->setText( lyricText);
    applyTextStyle( m_curLabel, m_fontFamily, m_fontSize, m_colorCurrent, true);
    m_curLabel ->adjustSize();
    m_curLabel ->move( m_customPos);
    const int spacing=4;
    m_extraLabel ->setVerticalMode( m_orientation == LyricOrientation::Vertical);
    m_extraLabel ->setText( m_currentExtraLine);
    applyTextStyle( m_extraLabel, m_fontFamily, m_extraFontSize, m_colorCurrent, true);
    m_extraLabel ->adjustSize();
    if(m_currentExtraLine.isEmpty() || m_extraMode == WallpaperExtraLyricsMode::None)
    {
        m_extraLabel->hide();
    }
    else
    {
        m_extraLabel->show();
        if(m_orientation==LyricOrientation::Vertical)
            m_extraLabel->move( m_customPos.x() + m_curLabel->width() + spacing, m_customPos.y());
        else
            m_extraLabel->move( m_customPos.x(), m_customPos.y() + m_curLabel->height() + spacing);
    }
    int titleFontSize = (m_titleFontSize > 0) ? m_titleFontSize : qMax(10, int(m_fontSize * 0.62));
    m_titleLabel ->setVerticalMode( m_orientation == LyricOrientation::Vertical);
    m_titleLabel ->setText( m_currentTitle);
    applyTextStyle( m_titleLabel, m_fontFamily, titleFontSize, m_colorCurrent, true);
    m_titleLabel ->adjustSize();
    if(m_orientation==LyricOrientation::Vertical)
    {
        int tx= m_customPos.x() - m_titleLabel->width() - spacing;
        int ty= m_customPos.y();
        m_titleLabel->move( tx, ty);
    }
    else
    {
        int tx= m_customPos.x();
        int ty= m_customPos.y() - m_titleLabel->height() - spacing;
        m_titleLabel->move( tx, ty);
    }
}
void DesktopWallpaperLyrics:: setCustomPosition(QPoint p)
{
    m_customPos = p;
    updateLayout();
}
void DesktopWallpaperLyrics:: setOrientation( LyricOrientation o)
{
    m_orientation = o;
    updateLayout();
}
void DesktopWallpaperLyrics:: setFontFamily( const QString &f)
{
    m_fontFamily = f;
    updateLayout();
}
void DesktopWallpaperLyrics:: setFontSize(int s)
{
    m_fontSize = s;
    updateLayout();
}
void DesktopWallpaperLyrics:: setTitleFontSize(int s)
{
    m_titleFontSize = s;
    updateLayout();
}
void DesktopWallpaperLyrics:: setExtraFontSize(int s)
{
    m_extraFontSize = qMax(8, s);
    updateLayout();
}
void DesktopWallpaperLyrics:: setExtraLyricsMode(WallpaperExtraLyricsMode mode)
{
    if(m_extraMode == mode) return;
    m_extraMode = mode;
    if(m_extraMode == WallpaperExtraLyricsMode::None)
        m_currentExtraLine.clear();
    updateLayout();
}
void DesktopWallpaperLyrics:: setColorCurrent( const QString &c)
{
    m_colorCurrent = c;
    updateLayout();
}
void DesktopWallpaperLyrics:: setColorOther(const QString&){}
void DesktopWallpaperLyrics:: setTextShadow(bool){}
void DesktopWallpaperLyrics:: setBackgroundImage( const QString&){}
void DesktopWallpaperLyrics:: setPosition( LyricPosition){}
void DesktopWallpaperLyrics::setEnabled(bool on)
{
    m_enabled = on;
    if(on)
    {
        applyAsWallpaper();
        m_desktopWidget->setWindowOpacity( m_opacity / 255.0);
        m_desktopWidget->show();
    }
    else
    {
        m_desktopWidget->hide();
    }
}
void DesktopWallpaperLyrics:: clearLyrics()
{
    if(m_fx->opacity() <= 0.0)
    {
        m_curLabel->clear();
        m_extraLabel->clear();
        m_extraLabel->hide();
        m_currentLine.clear();
        m_currentExtraLine.clear();
        m_pendingLine.clear();
        m_pendingExtraLine.clear();
        m_animating = false;
        return;
    }
    m_animating = true;
    auto *fadeOut = new QPropertyAnimation( m_fx, "opacity", this);
    fadeOut->setDuration(400);
    fadeOut->setStartValue(m_fx->opacity());
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::OutCubic);
    connect( fadeOut, &QPropertyAnimation::finished, this, [this]
            {
                m_curLabel->clear();
                m_extraLabel->clear();
                m_extraLabel->hide();
                m_currentLine.clear();
                m_currentExtraLine.clear();
                m_animating = false;
                if(!m_pendingLine.isEmpty())
                {
                    m_currentLine = m_pendingLine;
                    m_currentExtraLine = m_pendingExtraLine;
                    m_pendingLine.clear();
                    m_pendingExtraLine.clear();
                    updateLayout();
                    m_fx->setOpacity(1.0);
                    m_extraFx->setOpacity(1.0);
                }
            }
            );
    auto *extraFadeOut = new QPropertyAnimation( m_extraFx, "opacity", this);
    extraFadeOut->setDuration(400);
    extraFadeOut->setStartValue(m_extraFx->opacity());
    extraFadeOut->setEndValue(0.0);
    extraFadeOut->setEasingCurve(QEasingCurve::OutCubic);
    extraFadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}
void DesktopWallpaperLyrics::setOpacity(int value)
{
    m_opacity = qBound(0, value, 255);
    m_desktopWidget->setWindowOpacity( m_opacity / 255.0);
}
void DesktopWallpaperLyrics:: setTrackTitle(const QString &title)
{
    if(title == m_currentTitle) return;
    m_currentTitle = title;
    if(m_titleFx->opacity() <= 0.0)
    {
        updateLayout();
        m_titleLabel->show();
        auto *fadeIn = new QPropertyAnimation( m_titleFx, "opacity", this);
        fadeIn->setDuration(500);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(0.85);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        return;
    }
    auto *fadeOut = new QPropertyAnimation( m_titleFx, "opacity", this);
    fadeOut->setDuration(500);
    fadeOut->setStartValue(m_titleFx->opacity());
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);
    connect( fadeOut, &QPropertyAnimation::finished, this, [this]
            {
                updateLayout();
                m_titleLabel->show();
                auto *fadeIn = new QPropertyAnimation( m_titleFx, "opacity", this);
                fadeIn->setDuration(500);
                fadeIn->setStartValue(0.0);
                fadeIn->setEndValue(0.85);
                fadeIn->setEasingCurve(QEasingCurve::OutCubic);
                fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
            }
            );
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}
void DesktopWallpaperLyrics::applyAsWallpaper()
{
#ifdef Q_OS_WIN
    if(m_attached) return;
    HWND progman = FindWindow( L"Progman", nullptr);
    DWORD_PTR result = 0;
    SendMessageTimeout( progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);
    HWND workerw = nullptr;
    EnumWindows( [](HWND hwnd, LPARAM lParam)->BOOL
                {
                    HWND shell = FindWindowEx( hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
                    if(shell)
                    {
                        *((HWND*)lParam) = FindWindowEx( nullptr, hwnd, L"WorkerW", nullptr);
                        return FALSE;
                    }
                    return TRUE;
                }
                , (LPARAM)&workerw );
    if(workerw)
    {
        HWND hwnd = (HWND)m_desktopWidget->winId();
        SetParent( hwnd, workerw);
        LONG_PTR style = GetWindowLongPtr( hwnd, GWL_EXSTYLE);
        style |= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
        style &= ~WS_EX_APPWINDOW;
        SetWindowLongPtr( hwnd, GWL_EXSTYLE, style);
        SetWindowPos( hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        ShowWindow( hwnd, SW_SHOW);
        m_attached = true;
    }
#endif
}
void DesktopWallpaperLyrics::playFade(){}

void DesktopWallpaperLyrics::setMaxHeightRatio(float ratio)
{
    m_maxHeightRatio = qBound(0.1f, ratio, 1.0f);
    m_curLabel->setMaxHeightRatio(m_maxHeightRatio);
    m_extraLabel->setMaxHeightRatio(m_maxHeightRatio);
    m_titleLabel->setMaxHeightRatio(m_maxHeightRatio);
    updateLayout();
}
