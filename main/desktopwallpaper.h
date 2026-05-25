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
enum class LyricOrientation
{
    Horizontal,
    Vertical
};
enum class LyricPosition
{
    Custom
};
class RotatedLabel : public QLabel
{
    Q_OBJECT
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
    static int verticalMaxHeight()
    {
        const QScreen *screen = QApplication::primaryScreen();
        if(!screen) return 800;
        return screen->geometry().height() * 3 / 4;
    }
    QSize measureVerticalText(const QFont &f) const
    {
        QFontMetrics fm(f);
        int maxWidth = 0;
        int totalHeight = 0;
        QString latin;
        auto flushLatin = [&]()
        {
            if(latin.isEmpty()) return;
            int w = fm.boundingRect(latin).width();
            totalHeight += w + 6;
            maxWidth = qMax(maxWidth, fm.height() + 8);
            latin.clear();
        };
        const QString src = text();
        QTextBoundaryFinder finder(
            QTextBoundaryFinder::Grapheme,
            src);
        int start = 0;
        while(finder.toNextBoundary() != -1)
        {
            int end = finder.position();
            QString cluster = src.mid(start, end - start);
            start = end;
            if(cluster == "\n")
            {
                flushLatin();
                totalHeight += fm.height();
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
                totalHeight += qMax(2, fm.height() / 2);
                continue;
            }
            QChar first = cluster.at(0);
            maxWidth = qMax(maxWidth,fm.boundingRect(QString(verticalGlyph(first))).width() + 8);
            totalHeight += fm.height();
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
        const QColor penColor = palette().color(QPalette::WindowText);
        p.setPen(penColor);
        QFontMetrics fm(drawFont);
        const QString src = text();
        int y = 0;
        QString latin;
        auto drawLatinRun = [&]()
        {
            if(latin.isEmpty())return;
            int textW = fm.boundingRect(latin).width();
            int textH = fm.height();
            p.save();
            p.translate(width() / 2, y);
            p.rotate(90);
            p.drawText( QRect(0, -textH / 2, textW, textH), Qt::AlignLeft | Qt::AlignVCenter, latin );
            p.restore();
            y += textW + 6;
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
                y += fm.height();
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
                y += qMax(2, fm.height() / 2);
                continue;
            }
            QChar first = cluster.at(0);
            p.drawText( QRect(0, y, width(), fm.height()), Qt::AlignCenter, QString(verticalGlyph(first)) );
            y += fm.height();
        }
        drawLatinRun();
    }
private:
    bool m_vertical = false;
};
class DesktopWallpaperLyrics : public QObject
{
    Q_OBJECT
public:
    explicit DesktopWallpaperLyrics(QObject *parent=nullptr);
    ~DesktopWallpaperLyrics();
    bool isEnabled() const{return m_enabled;}
    void setEnabled(bool on);
    void updateLyrics(const QString&,const QString& cur,const QString&);
    void clearLyrics();
    void setOpacity(int value);
    void setBackgroundImage(const QString&);
    void setOrientation(LyricOrientation o);
    void setPosition(LyricPosition);
    void setCustomPosition(QPoint p);
    void setFontFamily(const QString&);
    void setFontSize(int);
    void setTitleFontSize(int);
    void setColorCurrent(const QString&);
    void setColorOther(const QString&);
    void setTextShadow(bool);
    void setTrackTitle(const QString &title);
private:
    void applyAsWallpaper();
    void updateLayout();
    void playFade();
private:
    QWidget *m_desktopWidget = nullptr;
    RotatedLabel *m_curLabel = nullptr;
    RotatedLabel *m_titleLabel = nullptr;
    QGraphicsOpacityEffect *m_titleFx = nullptr;
    QString m_currentTitle;
    QString m_currentLine;
    int m_opacity = 255;
    QString m_fontFamily;
    QString m_bgImagePath;
    bool m_animating = false;
    QString m_pendingLine;
    QString m_colorCurrent = "#FFFFFF";
    int m_fontSize = 36;
    int m_titleFontSize = -1;
    bool m_textShadow = true;
    bool m_enabled = false;
    bool m_attached = false;
    QPoint m_customPos = {800, 500};
    LyricOrientation m_orientation = LyricOrientation::Horizontal;
    bool m_showStartupText = false;
    QGraphicsOpacityEffect *m_fx = nullptr;
};