#include "positionpicker.h"

#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QSettings>
#include <QApplication>
#include <QFont>

#include <windows.h>

// ── helpers ───────────────────────────────────────────────────────────────

static QSize primaryDesktopSize()
{
    QScreen *s = QGuiApplication::primaryScreen();
    return s ? s->size() : QSize(1920, 1080);
}

// ── constants ─────────────────────────────────────────────────────────────

static constexpr int   kToolbarH  = 52;   // bottom toolbar height
static constexpr int   kMargin    = 0;    // canvas margin from dialog edges
static constexpr int   kPinR      = 9;    // anchor dot radius
static constexpr int   kDlgW      = 780;
static constexpr int   kDlgH      = 500;

// ── ctor ──────────────────────────────────────────────────────────────────

PositionPicker::PositionPicker(LyricOrientation orientation, QWidget *p)
    : QDialog(p)
    , m_orientation(orientation)
{
    setWindowTitle("选择歌词位置");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(kDlgW, kDlgH);

    setStyleSheet(R"(
QDialog {
    background: #0d0f1a;
}
QLabel#coordLabel {
    color: #D8DCFF;
    font-size: 13px;
    font-family: "Microsoft YaHei";
    background: transparent;
}
QLabel#hintLabel {
    color: #6B70A8;
    font-size: 11px;
    font-family: "Microsoft YaHei";
    background: transparent;
}
QPushButton#okBtn {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
        stop:0 #7568FF, stop:1 #9B5CFF);
    border: none;
    border-radius: 10px;
    color: white;
    font-size: 13px;
    font-family: "Microsoft YaHei";
    font-weight: bold;
    padding: 0px;
    min-width: 80px;
    min-height: 34px;
}
QPushButton#okBtn:hover {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
        stop:0 #8A7FFF, stop:1 #AD73FF);
}
QPushButton#cancelBtn {
    background: rgba(35,40,68,200);
    border: 1px solid rgba(90,100,180,0.35);
    border-radius: 10px;
    color: #B0B5E8;
    font-size: 13px;
    font-family: "Microsoft YaHei";
    padding: 0px;
    min-width: 68px;
    min-height: 34px;
}
QPushButton#cancelBtn:hover {
    background: rgba(50,57,95,220);
    border-color: rgba(120,130,220,0.6);
}
)");

    m_desktopSize = primaryDesktopSize();

    // ── canvas rect (fills top portion, toolbar at bottom) ────────────────
    const int canvasW = kDlgW - kMargin * 2;
    const int canvasH = kDlgH - kToolbarH - kMargin;

    // keep aspect ratio of real desktop
    int drawH = canvasW * m_desktopSize.height() / m_desktopSize.width();
    if (drawH > canvasH) drawH = canvasH;
    int drawW = drawH * m_desktopSize.width() / m_desktopSize.height();

    const int cx = (kDlgW - drawW) / 2;
    const int cy = (canvasH - drawH) / 2;
    m_canvas = QRect(cx, cy, drawW, drawH);

    // ── load wallpaper NOW (canvas is defined) ─────────────────────────────
    loadWallpaper();

    // ── restore saved position ─────────────────────────────────────────────
    QSettings s("MusicPlayer", "MusicPlayer");
    m_realPos = QPoint(
        s.value("wallPosX", m_desktopSize.width()  / 2).toInt(),
        s.value("wallPosY", m_desktopSize.height() / 2).toInt()
        );
    m_anchor = toPreview(m_realPos);

    // ── toolbar widgets ────────────────────────────────────────────────────
    const int tbY   = kDlgH - kToolbarH;
    const int tbMid = tbY + kToolbarH / 2;

    m_hint = new QLabel("点击或拖动蓝点，设置歌词左上角位置", this);
    m_hint->setObjectName("hintLabel");
    m_hint->adjustSize();
    m_hint->move(16, tbMid - m_hint->height() / 2);

    m_coordLabel = new QLabel(this);
    m_coordLabel->setObjectName("coordLabel");
    updateCoordLabel();   // sets text + adjustSize + centres vertically

    QPushButton *ok     = new QPushButton("确定", this);
    QPushButton *cancel = new QPushButton("取消", this);
    ok->setObjectName("okBtn");
    cancel->setObjectName("cancelBtn");

    ok->setFixedSize(80, 34);
    cancel->setFixedSize(68, 34);

    ok->move(kDlgW - ok->width() - 14, tbMid - ok->height() / 2);
    cancel->move(ok->x() - cancel->width() - 8, tbMid - cancel->height() / 2);

    // coord label between hint and cancel
    m_coordLabel->move(cancel->x() - m_coordLabel->width() - 16,
                       tbMid - m_coordLabel->height() / 2);

    connect(ok,     &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

// ── wallpaper ─────────────────────────────────────────────────────────────

void PositionPicker::loadWallpaper()
{
    wchar_t path[MAX_PATH] = {};
    SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0);
    QPixmap img(QString::fromWCharArray(path));
    if (!img.isNull())
        m_wallpaper = img.scaled(m_canvas.size(),
                                 Qt::KeepAspectRatioByExpanding,
                                 Qt::SmoothTransformation);
}

// ── coordinate conversion ─────────────────────────────────────────────────

QPoint PositionPicker::toReal(QPoint preview) const
{
    const double sx = double(m_desktopSize.width())  / m_canvas.width();
    const double sy = double(m_desktopSize.height()) / m_canvas.height();
    return QPoint(
        qRound((preview.x() - m_canvas.x()) * sx),
        qRound((preview.y() - m_canvas.y()) * sy)
        );
}

QPoint PositionPicker::toPreview(QPoint real) const
{
    const double sx = double(m_canvas.width())  / m_desktopSize.width();
    const double sy = double(m_canvas.height()) / m_desktopSize.height();
    return QPoint(
        qRound(real.x() * sx) + m_canvas.x(),
        qRound(real.y() * sy) + m_canvas.y()
        );
}

// ── lyric box size in preview coords ──────────────────────────────────────

QSize PositionPicker::previewLyricSize() const
{
    // Use realistic estimates of the rendered lyric size on the real desktop,
    // then scale down to preview coords.
    const double sx = double(m_canvas.width())  / m_desktopSize.width();
    const double sy = double(m_canvas.height()) / m_desktopSize.height();

    if (m_orientation == LyricOrientation::Horizontal)
        return QSize(qRound(600 * sx), qRound(80 * sy));
    else
        return QSize(qRound(72  * sx), qRound(340 * sy));
}

// ── coord label ───────────────────────────────────────────────────────────

void PositionPicker::updateCoordLabel()
{
    m_coordLabel->setText(
        QString("X %1  ·  Y %2")
            .arg(m_realPos.x(), 4)
            .arg(m_realPos.y(), 4)
        );
    m_coordLabel->adjustSize();
}

// ── paint ─────────────────────────────────────────────────────────────────

void PositionPicker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // ── full dialog background ──
    p.fillRect(rect(), QColor(13, 15, 26));

    // ── toolbar separator line ──
    const int tbY = kDlgH - kToolbarH;
    p.setPen(QPen(QColor(40, 45, 80), 1));
    p.drawLine(0, tbY, kDlgW, tbY);

    // ── canvas: wallpaper ──
    if (!m_wallpaper.isNull())
    {
        QPainterPath clip;
        clip.addRect(m_canvas);      // sharp corners — full bleed
        p.save();
        p.setClipPath(clip);
        p.drawPixmap(m_canvas.topLeft(), m_wallpaper);
        p.restore();
    }
    else
    {
        // fallback gradient
        QLinearGradient grad(m_canvas.topLeft(), m_canvas.bottomRight());
        grad.setColorAt(0, QColor(22, 26, 52));
        grad.setColorAt(1, QColor(14, 17, 36));
        p.fillRect(m_canvas, grad);
    }

    // ── dim overlay ──
    p.fillRect(m_canvas, QColor(0, 0, 0, 60));

    // ── canvas border ──
    p.setPen(QPen(QColor(60, 70, 140, 180), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(m_canvas);

    // ── lyric preview box (anchor = top-left, same as label->move()) ──
    {
        QSize  ls  = previewLyricSize();
        QRect  box(m_anchor, ls);
        box = box.intersected(m_canvas);

        // box fill
        p.setBrush(QColor(80, 70, 200, 35));
        p.setPen(Qt::NoPen);
        p.drawRect(box);

        // dashed border
        QPen dashedPen(QColor(180, 170, 255, 160), 1, Qt::DashLine);
        dashedPen.setDashPattern({4, 3});
        p.setPen(dashedPen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(box);

        // label
        p.setPen(QColor(255, 255, 255, 170));
        QFont lf("Microsoft YaHei", 8);
        p.setFont(lf);
        if (m_orientation == LyricOrientation::Horizontal)
        {
            p.drawText(box, Qt::AlignCenter, "歌词显示区域");
        }
        else
        {
            p.save();
            p.translate(box.center());
            p.rotate(-90);
            p.drawText(QRect(-box.height()/2, -box.width()/2,
                             box.height(), box.width()),
                       Qt::AlignCenter, "歌词显示区域");
            p.restore();
        }
    }

    // ── anchor pin ──
    {
        QPointF c(m_anchor);

        // shadow/glow ring
        {
            QRadialGradient glow(c, kPinR * 3.2);
            glow.setColorAt(0.0, QColor(130, 120, 255, 90));
            glow.setColorAt(0.5, QColor(100,  90, 220, 40));
            glow.setColorAt(1.0, QColor(  0,   0,   0,  0));
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(c, kPinR * 3.2, kPinR * 3.2);
        }

        // white ring
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawEllipse(c, kPinR + 2.5, kPinR + 2.5);

        // coloured fill
        QRadialGradient fill(c - QPointF(2, 2), kPinR * 1.4);
        fill.setColorAt(0, QColor(160, 150, 255));
        fill.setColorAt(1, QColor( 90,  80, 220));
        p.setBrush(fill);
        p.drawEllipse(c, kPinR, kPinR);

        // crosshair
        p.setPen(QPen(Qt::white, 1.5));
        int r = kPinR - 2;
        p.drawLine(QPointF(c.x() - r, c.y()), QPointF(c.x() + r, c.y()));
        p.drawLine(QPointF(c.x(), c.y() - r), QPointF(c.x(), c.y() + r));
    }
}

// ── mouse ─────────────────────────────────────────────────────────────────

void PositionPicker::mousePressEvent(QMouseEvent *e)
{
    if (!m_canvas.contains(e->pos())) return;

    if ((e->pos() - m_anchor).manhattanLength() <= kPinR * 3)
    {
        m_drag       = true;
        m_dragOffset = e->pos() - m_anchor;
    }
    else
    {
        m_anchor  = e->pos();
        m_realPos = toReal(m_anchor);
        updateCoordLabel();
        update();
    }
}

void PositionPicker::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_drag) return;

    QPoint np = e->pos() - m_dragOffset;
    np.setX(qBound(m_canvas.left(),  np.x(), m_canvas.right()));
    np.setY(qBound(m_canvas.top(),   np.y(), m_canvas.bottom()));

    m_anchor  = np;
    m_realPos = toReal(np);
    updateCoordLabel();
    update();
}

void PositionPicker::mouseReleaseEvent(QMouseEvent*)
{
    m_drag = false;
}

// ── getter ────────────────────────────────────────────────────────────────

QPoint PositionPicker::selectedPos() const
{
    return m_realPos;
}