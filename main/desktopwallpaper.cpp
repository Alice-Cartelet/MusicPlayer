#include "desktopwallpaper.h"
#include <QApplication>
#include <QScreen>
#include <QMap>
#include <QTimer>
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
    m_titleLabel = new RotatedLabel( m_desktopWidget);
    m_titleLabel ->setAlignment( Qt::AlignCenter);
    m_titleFx = new QGraphicsOpacityEffect( m_titleLabel);
    m_titleLabel ->setGraphicsEffect( m_titleFx);
    m_titleFx ->setOpacity(0.0);
    m_showStartupText = true;
    m_currentLine = "Alice-Cartelet";
    updateLayout();
    QTimer::singleShot( 1000, this, [this]
                       {
                           m_showStartupText = false;
                           m_currentLine.clear();
                           m_curLabel->clear();
                       });
}
DesktopWallpaperLyrics:: ~DesktopWallpaperLyrics()
{
    delete m_desktopWidget;
}
void DesktopWallpaperLyrics:: updateLyrics( const QString&, const QString& cur, const QString&)
{
    if(cur.isEmpty()) return;
    if(cur == m_currentLine) return;
    if(m_animating)
    {
        m_pendingLine = cur;
        return;
    }
    m_animating = true;
    m_pendingLine = cur;
    if(m_currentLine.isEmpty())
    {
        m_currentLine = m_pendingLine;
        updateLayout();
        m_fx->setOpacity(1);
        m_animating = false;
        return;
    }
    auto *fadeOut = new QPropertyAnimation( m_fx, "opacity", this);
    fadeOut->setDuration(500);
    fadeOut->setStartValue(1);
    fadeOut->setEndValue(0);
    connect( fadeOut, &QPropertyAnimation::finished, this, [this]
            {
                m_currentLine = m_pendingLine;
                updateLayout();
                auto *fadeIn = new QPropertyAnimation( m_fx, "opacity", this);
                fadeIn->setDuration(500);
                fadeIn->setStartValue(0);
                fadeIn->setEndValue(1);
                connect( fadeIn, &QPropertyAnimation::finished, this, [this]
                        {
                            m_animating = false;
                        }
                        );
                fadeIn ->start( QAbstractAnimation::DeleteWhenStopped);
            }
            );
    fadeOut ->start( QAbstractAnimation::DeleteWhenStopped);
}
void DesktopWallpaperLyrics:: updateLayout()
{
    m_curLabel->setMaxHeightRatio(m_maxHeightRatio);
    m_titleLabel->setMaxHeightRatio(m_maxHeightRatio);
    QString lyricText = m_currentLine;
    m_curLabel ->setVerticalMode( m_orientation == LyricOrientation::Vertical);
    m_curLabel ->setText( lyricText);
    applyTextStyle( m_curLabel, m_fontFamily, m_fontSize, m_colorCurrent, true);
    m_curLabel ->adjustSize();
    m_curLabel ->move( m_customPos);
    int titleFontSize = (m_titleFontSize > 0) ? m_titleFontSize : qMax(10, int(m_fontSize * 0.62));
    m_titleLabel ->setVerticalMode( m_orientation == LyricOrientation::Vertical);
    m_titleLabel ->setText( m_currentTitle);
    applyTextStyle( m_titleLabel, m_fontFamily, titleFontSize, m_colorCurrent, true);
    m_titleLabel ->adjustSize();
    const int spacing=4;
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
    m_curLabel->move(p);
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
        m_currentLine.clear();
        m_pendingLine.clear();
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
                m_currentLine.clear();
                m_animating = false;
                if(!m_pendingLine.isEmpty())
                {
                    m_currentLine = m_pendingLine;
                    m_pendingLine.clear();
                    updateLayout();
                    m_fx->setOpacity(1.0);
                }
            }
            );
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
    m_titleLabel->setMaxHeightRatio(m_maxHeightRatio);
    updateLayout();
}