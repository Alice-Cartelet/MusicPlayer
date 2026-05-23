#include "lyricsoverlay.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QScreen>
#include <QApplication>
#include <QFontMetricsF>
#include <QSettings>
#include <QWindow>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#include <QTextBoundaryFinder>
#pragma comment(lib, "dwmapi.lib")
#endif
static QStringList splitUnicodeChars(const QString &s)
{
    QStringList out;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, s);
    int start = 0;
    while (true) {
        int end = finder.toNextBoundary();
        if (end == -1)
            break;
        out.append(s.mid(start, end - start));
        start = end;
    }
    return out;
}
static constexpr int kWinW    = 1000;
static constexpr int kPadX    = 80;
static constexpr int kMinWinW = 300;
static int calcWindowHeight(int fontSize)
{
    return qMax(72, fontSize * 3);
}
static float easeOut(float t) {
    float r = 1.f - t;
    return 1.f - r*r*r;
}
LyricsOverlay::LyricsOverlay(QWidget *parent): QWidget(parent,Qt::Window|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::Tool|Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    int h = calcWindowHeight(m_fontSize);
    setMinimumSize(kWinW, h);
    resize(kWinW, h);
    setMouseTracking(true);
    if (QScreen *scr = QApplication::primaryScreen())
    {
        QRect g = scr->availableGeometry();
        move(g.center().x() - kWinW / 2, g.bottom() - calcWindowHeight(m_fontSize) - 90);
    }
    buildFont();
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(16);
    connect(m_animTimer, &QTimer::timeout, this, [this]
    {
        m_animElapsed += 16;
        float t = qMin(1.f, (float)m_animElapsed / qMax(1, m_animDur));
        m_litPx = m_animStart + (m_animEnd - m_animStart) * easeOut(t);
        update();
        if (t >= 1.f)
            m_animTimer->stop();
    });
    m_hoverCheckTimer = new QTimer(this);
    m_hoverCheckTimer->setInterval(30);
    connect(m_hoverCheckTimer, &QTimer::timeout, this, [this]()
    {
        QPoint globalPos = QCursor::pos();
        QRect r = geometry();
        if (!r.contains(globalPos))
        {
            setAttribute(Qt::WA_TransparentForMouseEvents, false);
            setWindowOpacity(1.0);
            m_hoverCheckTimer->stop();
        }
    });
    m_topmostTimer = new QTimer(this);
    m_topmostTimer->setInterval(2000);
    connect(m_topmostTimer, &QTimer::timeout, this, [this]()
    {
        enforceTopmost();
    });
    m_topmostTimer->start();
}


void LyricsOverlay::enforceTopmost()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}
void LyricsOverlay::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    QSettings s("MusicPlayer", "MusicPlayer");
    if (s.contains("lyricsAnchorCenterX"))
    {
        m_anchorCenterX = s.value("lyricsAnchorCenterX").toInt();
        m_anchorY       = s.value("lyricsAnchorY").toInt();
        move(m_anchorCenterX - width() / 2, m_anchorY);
    }
    else if (QScreen *scr = QApplication::primaryScreen())
    {
        QRect g = scr->availableGeometry();
        m_anchorCenterX = g.center().x();
        m_anchorY = g.bottom() - calcWindowHeight(m_fontSize) - 90;
        move(m_anchorCenterX - width() / 2, m_anchorY);
    }
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    const DWORD DWMWCP_DONOTROUND = 1;
    DwmSetWindowAttribute(hwnd, 33, &DWMWCP_DONOTROUND, sizeof(DWMWCP_DONOTROUND));
    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
#endif
}
void LyricsOverlay::buildFont()
{
    m_font.setPointSize(m_fontSize);
    m_font.setBold(true);
    m_font.setFamilies({"Microsoft YaHei", "Arial", "Segoe UI"});
}
void LyricsOverlay::loadLyrics(const QString &path)
{
    m_parser.clear();
    m_currentLineIdx = -1;
    m_litPx = 0;
    m_targetPx = 0;
    m_lineText.clear();
    m_charPixelsF.clear();
    m_totalW = 0;
    m_animTimer->stop();
    if (!path.isEmpty())
        m_parser.load(path);
    update();
}
void LyricsOverlay::updatePosition(qint64 posMs)
{
    if (m_parser.isEmpty())
        return;
    int idx = m_parser.currentLine(posMs);
    if (idx < 0) {
        if (m_currentLineIdx != -1)
        {
            m_currentLineIdx = -1;
            m_lineText.clear();
            m_litPx = 0;
            m_animTimer->stop();
            update();
        }
        return;
    }
    const LrcLine &ln = m_parser.line(idx);
    if (idx != m_currentLineIdx)
    {
        m_currentLineIdx = idx;
        m_lineText = ln.text;
        m_animTimer->stop();
        m_litPx = 0;
        m_targetPx = 0;
        QFontMetricsF fmf(m_font);
        m_totalW = (float)fmf.horizontalAdvance(m_lineText);
        m_lineX0 = (width() - (int)m_totalW) / 2;
        m_baseY  = (height() + (int)(fmf.ascent() - fmf.descent())) / 2;
        m_charPixelsF.clear();
        float cx = 0.f;
        QStringList glyphs = splitUnicodeChars(m_lineText);
        for (const QString &g : glyphs)
        {
            m_charPixelsF.append(cx);
            cx += (float)fmf.horizontalAdvance(g);
        }
        m_charPixelsF.append(cx);
    }
    qint64 offset = posMs - ln.startMs;
    float targetPx = 0.f;
    if (ln.hasTiming && !ln.chars.isEmpty() && !m_charPixelsF.isEmpty())
    {
        const auto &chars = ln.chars;
        int ci = 0;
        for (int i = 0; i < chars.size(); ++i)
        {
            if (offset >= chars[i].offsetMs)
                ci = i;
            else
                break;
        }
        int charIdx = chars[ci].charIndex;
        qint64 t0 = chars[ci].offsetMs;
        qint64 t1 = (ci + 1 < chars.size()) ? chars[ci + 1].offsetMs : t0 + 300;
        int nextIdx;
        if (ci + 1 < chars.size())
            nextIdx = chars[ci + 1].charIndex;
        else
            nextIdx = splitUnicodeChars(m_lineText).size();
        int n = m_charPixelsF.size();
        float px0 = (charIdx < n) ? m_charPixelsF[charIdx] : m_totalW;
        float px1 = (nextIdx < n) ? m_charPixelsF[nextIdx] : m_totalW;

        float frac = (t1 > t0) ? (float)(offset - t0) / (float)(t1 - t0) : 1.f;
        frac = qBound(0.f, frac, 1.f);
        targetPx = px0 + (px1 - px0) * frac;
    }
    else
    {
        targetPx = m_totalW;
        if (idx != m_currentLineIdx)
        {
            m_lineAlpha = 0.f;
        }
        m_lineAlpha += 0.08f;
        if (m_lineAlpha > 1.f)
            m_lineAlpha = 1.f;
    }

    if (qAbs(targetPx - m_targetPx) > 0.5f)
    {
        int dur = (targetPx > m_litPx) ? 80 : 0;
        animateTo(targetPx, dur);
        m_targetPx = targetPx;
    }
}
void LyricsOverlay::animateTo(float endPx, int durationMs)
{
    m_animTimer->stop();
    m_animStart   = m_litPx;
    m_animEnd     = endPx;
    m_animDur     = durationMs;
    m_animElapsed = 0;
    if (durationMs <= 0)
    {
        m_litPx = endPx;
        update();
        return;
    }
    m_animTimer->start();
}
void LyricsOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.fillRect(rect(), Qt::transparent);

    QString displayText = m_lineText;
    if (displayText.isEmpty())
    {
        displayText = "暂无歌词";
    }
    p.setFont(m_font);
    QFontMetricsF fmf(m_font);
    float totalW = (float)fmf.horizontalAdvance(displayText);
    if (displayText == "暂无歌词")
    {
        m_litPx = totalW;
    }
    float lineX0 = (width() - totalW) / 2.f;
    float baseY  = (height() + fmf.ascent() - fmf.descent()) / 2.f;
    QPainterPath fullPath;
    {
        float x = lineX0;
        QStringList glyphs = splitUnicodeChars(displayText);
        for (const QString &g : glyphs)
        {
            fullPath.addText(x, baseY, m_font, g);
            x += (float)fmf.horizontalAdvance(g);
        }
    }
    QRectF textBounds = fullPath.boundingRect();
    if (!textBounds.isEmpty() && textBounds.width() > 0)
    {
        p.save();
        qreal padX = 60.0;
        qreal padY = 15.0;
        QRectF bgRect = textBounds.adjusted(-padX, -padY, padX, padY);
        p.translate(bgRect.center());
        qreal radius = bgRect.width() / 2.0;
        qreal scaleY = bgRect.height() / bgRect.width();
        p.scale(1.0, scaleY);
        QRadialGradient bgGradient(0, 0, radius);
        bgGradient.setColorAt(0.0, QColor(170, 170, 170, 100));
        bgGradient.setColorAt(0.3, QColor(170, 170, 170, 40));
        bgGradient.setColorAt(1.0, Qt::transparent);
        p.setPen(Qt::NoPen);
        p.setBrush(bgGradient);
        p.drawEllipse(QPointF(0, 0), radius, radius);
        p.restore();
    }
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 50));
    p.save();
    p.translate(0, 2);
    p.drawPath(fullPath);
    p.restore();
    p.setBrush(QColor(0, 0, 0, 25));
    p.save();
    p.translate(0, 1);
    p.drawPath(fullPath);
    p.restore();
    p.setBrush(m_colorUnsang);
    p.drawPath(fullPath);
    if (m_litPx > 0.f) {
        p.save();
        p.setClipRect(QRectF(lineX0, 0.f, m_litPx, (float)height()));
        QColor sung = m_colorSung;
        sung.setAlphaF(m_lineAlpha);
        p.setBrush(sung);
        p.drawPath(fullPath);
        p.restore();
    }
}
void LyricsOverlay::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
#else
        m_dragOffset = e->globalPos() - frameGeometry().topLeft();
#endif
    }
}
void LyricsOverlay::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        move(e->globalPosition().toPoint() - m_dragOffset);
#else
        move(e->globalPos() - m_dragOffset);
#endif
    }
}
void LyricsOverlay::resizeToText(const QString &text) {
    QFontMetricsF fmf(m_font);
    int newW = qMax(kMinWinW, (int)fmf.horizontalAdvance(text) + kPadX * 2);
    if (newW == width()) return;
    if (m_anchorCenterX < 0) {
        m_anchorCenterX = geometry().center().x();
        m_anchorY = geometry().top();
    }
    resize(newW, calcWindowHeight(m_fontSize));
    move(m_anchorCenterX - newW / 2, m_anchorY);
}
void LyricsOverlay::setHideOnHover(bool v) {
    m_hideOnHover = v;
}
void LyricsOverlay::setColors(const QString &sung, const QString &unsang) {
    QColor cs(sung), cu(unsang);
    if (cs.isValid())  m_colorSung   = cs;
    if (cu.isValid())  m_colorUnsang = cu;
    update();
}
void LyricsOverlay::setFontSize(int size) {
    m_fontSize = qBound(18, size, 60);
    buildFont();
    QFontMetricsF fmf(m_font);
    m_totalW = (float)fmf.horizontalAdvance(m_lineText);
    int newH = calcWindowHeight(m_fontSize);
    resize(width(), newH);
    update();
}
void LyricsOverlay::enterEvent(QEnterEvent *e) {
    QWidget::enterEvent(e);
    if (!m_hideOnHover)
        return;
    setWindowOpacity(0.0);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_hoverCheckTimer->start();
}
void LyricsOverlay::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);
}
void LyricsOverlay::mouseReleaseEvent(QMouseEvent *) {
    m_dragging = false;
    m_anchorCenterX = geometry().center().x();
    m_anchorY = geometry().top();
    QSettings s("MusicPlayer", "MusicPlayer");
    s.setValue("lyricsAnchorCenterX", m_anchorCenterX);
    s.setValue("lyricsAnchorY", m_anchorY);
}
void LyricsOverlay::setLyricsVisible(bool v) {
    setVisible(v);
}
bool LyricsOverlay::lyricsVisible() const {
    return isVisible();
}