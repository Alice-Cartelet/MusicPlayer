#pragma once
#include <QObject>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPaintEvent>
#include <QPoint>
#include <QTextBoundaryFinder>
#include <QSize>
#include <QImage>
#include <QPixmap>
#include <QTransform>
#include <QFontMetrics>
#include <QMap>
#include <QPalette>
#include <QColor>
#include <QApplication>
#include <QScreen>
#include <QVariantAnimation>
enum class LyricOrientation
{
    Horizontal,
    Vertical
};
enum class LyricPosition
{
    Custom
};
enum class WallpaperExtraLyricsMode
{
    None = 0,
    Translation = 1,
    NextLine = 2
};
class RotatedLabel : public QLabel
{
public:
    explicit RotatedLabel(QWidget *parent=nullptr)
        : QLabel(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
    }
    void setVerticalMode(bool on)
    {
        if(m_vertical == on) return;
        m_vertical = on;
        updateGeometry();
        update();
    }
    bool isVerticalMode() const
    {
        return m_vertical;
    }
    // 供 applyTextStyle 调用，确保竖排 paintEvent 也能取到正确颜色
    void setTextColor(const QString &hex)
    {
        m_textColor = QColor(hex);
        update();
    }
    void setMaxHeightRatio(float r)
    {
        m_maxHeightRatio = qBound(0.1f, r, 1.0f);
        updateGeometry();
        update();
    }
protected:
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
    static QChar verticalGlyph(QChar c)
    {
        static const QMap<QChar,QChar> verticalMap =
            {
                {QChar(u'（'),QChar(u'︵')},
                {QChar(u'）'),QChar(u'︶')},
                {QChar(u'('),QChar(u'︵')},
                {QChar(u')'),QChar(u'︶')},
                {QChar(u'【'),QChar(u'︻')},
                {QChar(u'】'),QChar(u'︼')},
                {QChar(u'['),QChar(u'﹇')},
                {QChar(u']'),QChar(u'﹈')},
                {QChar(u'{'),QChar(u'︷')},
                {QChar(u'}'),QChar(u'︸')},
                {QChar(u'《'),QChar(u'︽')},
                {QChar(u'》'),QChar(u'︾')},
                {QChar(u'〈'),QChar(u'︿')},
                {QChar(u'〉'),QChar(u'﹀')},
                {QChar(u'<'),QChar(u'︿')},
                {QChar(u'>'),QChar(u'﹀')},
                {QChar(u'「'),QChar(u'﹁')},
                {QChar(u'」'),QChar(u'﹂')},
                {QChar(u'『'),QChar(u'﹃')},
                {QChar(u'』'),QChar(u'﹄')},
                {QChar(u'〔'),QChar(u'︹')},
                {QChar(u'〕'),QChar(u'︺')},
                {QChar(u'—'),QChar(u'︱')},
                {QChar(u'-'),QChar(u'︲')},
                {QChar(u'_'),QChar(u'︳')},
                {QChar(u'：'),QChar(u'︓')},
                {QChar(u':'),QChar(u'︓')},
                {QChar(u'；'),QChar(u'︔')},
                {QChar(u';'),QChar(u'︔')},
                {QChar(u'！'),QChar(u'︕')},
                {QChar(u'!'),QChar(u'︕')},
                {QChar(u'？'),QChar(u'︖')},
                {QChar(u'?'),QChar(u'︖')},
                {QChar(u'…'),QChar(u'︙')},
                {QChar(u'·'),QChar(u'・')},
                {QChar(u'、'),QChar(u'︑')},
                {QChar(u'。'),QChar(u'︒')},
                {QChar(u'，'),QChar(u'︐')},
                {QChar(u'．'),QChar(u'︒')}
            };
        return verticalMap.value(c, c);
    }
    int verticalMaxHeight() const
    {
        const QScreen *screen = QApplication::primaryScreen();
        if(!screen) return 800;
        return int(screen->geometry().height() * m_maxHeightRatio);
    }
    QSize measureVerticalText(const QFont &f) const
    {
        QFontMetrics fm(f);
        const int lineH = fm.lineSpacing();
        int maxWidth = 0;
        int totalHeight = 0;
        QString latin;
        auto flushLatin = [&]()
        {
            if(latin.isEmpty()) return;
            // Latin run is rotated 90 deg: advance width becomes vertical height
            int w = fm.horizontalAdvance(latin);
            totalHeight += w + lineH / 4;
            maxWidth = qMax(maxWidth, lineH);
            latin.clear();
        };
        const QString src = text();
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, src);
        int start = 0;
        while(finder.toNextBoundary() != -1)
        {
            int end = finder.position();
            QString cluster = src.mid(start, end - start);
            start = end;
            if(cluster == "\n")
            {
                flushLatin();
                totalHeight += lineH;
                continue;
            }
            if(isLatinWordChar(cluster))
            {
                latin += cluster;
                continue;
            }
            flushLatin();
            if(cluster.trimmed().isEmpty())
            {
                totalHeight += lineH / 2;
                continue;
            }
            QChar first = cluster.at(0);
            maxWidth = qMax(maxWidth,
                            fm.horizontalAdvance(QString(verticalGlyph(first))) + 8);
            totalHeight += lineH;
        }
        flushLatin();
        return QSize(maxWidth + 8, totalHeight + 8);
    }
    QFont fittedVerticalFont() const
    {
        QFont base = font();
        int px = base.pixelSize();
        if(px <= 0)
        {
            px = base.pointSize();
            if(px <= 0)
                px = 36;
        }
        const int minPx = 10;
        const int limitH = verticalMaxHeight();
        for(int s = px; s >= minPx; --s)
        {
            QFont test = base;
            test.setPixelSize(s);
            if(measureVerticalText(test).height() <= limitH) return test;
        }
        QFont test = base;
        test.setPixelSize(minPx);
        return test;
    }
    QSize sizeHint() const override
    {
        if(!m_vertical) return QLabel::sizeHint();
        QFont f = fittedVerticalFont();
        return measureVerticalText(f);
    }
    QSize minimumSizeHint() const override
    {
        return sizeHint();
    }
    void paintEvent(QPaintEvent *e) override
    {
        if(!m_vertical)
        {
            QLabel::paintEvent(e);
            return;
        }
        Q_UNUSED(e);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        p.setRenderHint(QPainter::VerticalSubpixelPositioning, true);
#endif
        QFont drawFont = fittedVerticalFont();
        p.setFont(drawFont);
        // 优先用显式设置的颜色；fallback 到 palette（横排模式由 stylesheet 驱动，
        // 竖排走自定义 paintEvent，必须用 m_textColor 才能生效）
        const QColor penColor = m_textColor.isValid()
                                    ? m_textColor
                                    : palette().color(QPalette::WindowText);
        p.setPen(penColor);
        QFontMetrics fm(drawFont);
        const QString src = text();
        int y = 0;
        QString latin;
        const int lineH = fm.lineSpacing();
        auto drawLatinRun = [&]()
        {
            if(latin.isEmpty()) return;
            int textW = fm.horizontalAdvance(latin);
            int textH = fm.height();
            // Rotate 90 deg so Latin reads bottom-to-top, centred in column
            p.save();
            // translate to: horizontally centred, vertically at current y
            p.translate(width() / 2, y + textW / 2);
            p.rotate(90);
            // after rotation: x axis points down, y axis points left
            // draw text so it starts at the rotated origin
            p.drawText(QRect(-textW / 2, -textH / 2, textW, textH),
                       Qt::AlignHCenter | Qt::AlignVCenter, latin);
            p.restore();
            y += textW + lineH / 4;
            latin.clear();
        };
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme,src);
        int start = 0;
        while(finder.toNextBoundary() != -1)
        {
            int end = finder.position();
            QString cluster = src.mid(start, end - start);
            start = end;
            if(cluster == "\n")
            {
                drawLatinRun();
                y += lineH;
                continue;
            }
            if(isLatinWordChar(cluster))
            {
                latin += cluster;
                continue;
            }
            drawLatinRun();
            if(cluster.trimmed().isEmpty())
            {
                y += lineH / 2;
                continue;
            }
            QChar first = cluster.at(0);
            p.drawText(QRect(0, y, width(), lineH), Qt::AlignCenter, QString(verticalGlyph(first)));
            y += lineH;
        }
        drawLatinRun();
    }
private:
    bool m_vertical = false;
    QColor m_textColor;  // 显式颜色，用于竖排 paintEvent
    float m_maxHeightRatio = 0.75f;
};
class DesktopWallpaperLyrics : public QObject
{
    Q_OBJECT
public:
    explicit DesktopWallpaperLyrics(QObject *parent=nullptr);
    ~DesktopWallpaperLyrics();
    bool isEnabled() const{return m_enabled;}
    void setEnabled(bool on);
    void updateLyrics(const QString&,const QString& cur,const QString&, const QString &translation = QString());
    void clearLyrics();
    void setOpacity(int value);
    void setBackgroundImage(const QString&);
    void setOrientation(LyricOrientation o);
    void setPosition(LyricPosition);
    void setCustomPosition(QPoint p);
    void setFontFamily(const QString&);
    void setFontSize(int);
    void setTitleFontSize(int);
    void setExtraFontSize(int);
    void setExtraLyricsMode(WallpaperExtraLyricsMode mode);
    void setColorCurrent(const QString&);
    void setColorOther(const QString&);
    void setTextShadow(bool);
    void setTrackTitle(const QString &title);
    void setMaxHeightRatio(float ratio);
private:
    void applyAsWallpaper();
    void updateLayout();
    void animateNextLinePromotion(const QString &cur, const QString &nextExtra);
    void playFade();
private:
    QWidget *m_desktopWidget = nullptr;
    RotatedLabel *m_curLabel = nullptr;
    RotatedLabel *m_extraLabel = nullptr;
    RotatedLabel *m_titleLabel = nullptr;
    QGraphicsOpacityEffect *m_titleFx = nullptr;
    QString m_currentTitle;
    QString m_currentLine;
    QString m_currentExtraLine;
    int m_opacity = 255;
    QString m_fontFamily;
    QString m_bgImagePath;
    bool m_animating = false;
    QString m_pendingLine;
    QString m_pendingExtraLine;
    QString m_colorCurrent = "#FFFFFF";
    int m_fontSize = 36;
    int m_titleFontSize = -1;
    int m_extraFontSize = 24;
    WallpaperExtraLyricsMode m_extraMode = WallpaperExtraLyricsMode::None;
    bool m_textShadow = true;
    bool m_enabled = false;
    bool m_attached = false;
    QPoint m_customPos = {800, 500};
    LyricOrientation m_orientation = LyricOrientation::Horizontal;
    bool m_showStartupText = false;
    QGraphicsOpacityEffect *m_fx = nullptr;
    QGraphicsOpacityEffect *m_extraFx = nullptr;
    float m_maxHeightRatio = 0.75f;
};
