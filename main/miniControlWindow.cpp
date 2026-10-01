#include "minicontrolwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QSettings>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
static QPixmap blurPixmap( const QPixmap &src, const QSize &size)
{
    if(src.isNull()) return QPixmap();
    QPixmap scaled = src.scaled( size * 1.8, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation );
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(scaled);
    auto *blur = new QGraphicsBlurEffect;
    blur->setBlurRadius(42);
    item.setGraphicsEffect(blur);
    scene.addItem(&item);
    QPixmap result(size);
    result.fill(Qt::transparent);
    QPainter ptr(&result);
    scene.render( &ptr, QRectF(), QRectF( 0, 0, size.width(), size.height() ) );
    return result;
}
MiniControlWindow::MiniControlWindow( QWidget *parent) : QWidget(parent)
{
    setWindowFlags( Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint );
    setAttribute( Qt::WA_TranslucentBackground );
    resize(220,72);
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins( 10, 8, 10, 8 );
    root->setSpacing(6);
    QWidget *titleWrap = new QWidget;
    titleWrap->setFixedHeight(18);
    QHBoxLayout *titleRoot = new QHBoxLayout(titleWrap);
    titleRoot->setContentsMargins(0, 0, 0, 0);
    m_titleContainer = new QWidget;
    m_titleContainer->setFixedSize(190, 18);
    m_titleLabel1 = new QLabel(m_titleContainer);
    m_titleLabel2 = new QLabel(m_titleContainer);
    QString titleCss = R"( QLabel
{
    color:rgba(25,25,30,230);
    font-size:11px;
    font-weight:700;
    background:transparent;
}
)";
    m_titleLabel1->setStyleSheet(titleCss);
    m_titleLabel2->setStyleSheet(titleCss);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(255,255,255,140));
    shadow->setOffset(0,1);
    m_titleLabel1->setGraphicsEffect(shadow);
    titleRoot->addWidget(m_titleContainer);
    titleRoot->addStretch();
    m_btnExpand = new QPushButton("↖", m_titleContainer);
    m_btnExpand->setGeometry(0, -1, 20, 20);
    m_btnExpand->setCursor(Qt::PointingHandCursor);
    m_btnExpand->raise();
    m_btnExpand->setStyleSheet(R"( QPushButton
{
    background:transparent;
    border:none;
    color:rgba(20,20,25,170);
    font-size:13px;
    font-weight:bold;
}
QPushButton:hover
{
    color:white;
}
)");
    connect( m_btnExpand, &QPushButton::clicked, this, &MiniControlWindow::expandClicked );
    root->addWidget(titleWrap);
    m_btnPrev = new MiniconButton( MiniconButton::Prev );
    m_btnPlay = new MiniconButton( MiniconButton::Play );
    m_btnNext = new MiniconButton( MiniconButton::Next );
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(6);
    btnLayout->addStretch();
    btnLayout->addWidget( m_btnPrev );
    btnLayout->addWidget( m_btnPlay );
    btnLayout->addWidget( m_btnNext );
    btnLayout->addStretch();
    root->addLayout(btnLayout);
    m_progressBarBg = new QWidget(this);
    m_progressBarBg->setGeometry( 12, height() - 6, width() - 24, 2 );
    m_progressBarBg->setStyleSheet(R"( background:rgba(255,255,255,35);border-radius:1px;)");
    m_progressBarFill = new QWidget(m_progressBarBg);
    m_progressBarFill->setGeometry(0,0,0,2);
    m_progressBarFill->setStyleSheet(R"( background:rgba(255,255,255,180);border-radius:1px;)");
    connect( m_btnPrev, &QPushButton::clicked, this, &MiniControlWindow::prevClicked );
    connect( m_btnPlay, &QPushButton::clicked, this, &MiniControlWindow::playPauseClicked );
    connect( m_btnNext, &QPushButton::clicked, this, &MiniControlWindow::nextClicked );
    m_scrollTimer = new QTimer(this);
    connect( m_scrollTimer, &QTimer::timeout, this, [this]
            {
                if(!m_needScroll) return;
                m_scrollOffset--;
                int spacing = 40;
                int totalWidth = m_textWidth + spacing;
                if(m_scrollOffset <= -totalWidth)
                {
                    m_scrollOffset = 0;
                }
                m_titleLabel1->move( m_scrollOffset, 0 );
                m_titleLabel2->move( m_scrollOffset + totalWidth, 0 );
            }
            );
    m_scrollTimer->start(16);
    QSettings s;
    move( s.value( "miniControlPos", QPoint(100,100) ).toPoint() );
    setWindowOpacity( m_opacity / 100.0 );
}
void MiniControlWindow::setProgress(qint64 pos,qint64 dur)
{
    m_position = pos;
    m_duration = dur;
    if(!m_progressBarBg || dur <= 0) return;
    int w = m_progressBarBg->width();
    double ratio = double(pos) / double(dur);
    int fillW = qMax(2,int(w * ratio));
    m_progressBarFill->setGeometry( 0, 0, fillW, m_progressBarBg->height() );
}
void MiniControlWindow::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if(m_progressBarBg)
    {
        m_progressBarBg->setGeometry( 12, height() - 6, width() - 24, 2 );
    }
}
void MiniControlWindow::setPlaying( bool playing)
{
    m_playing = playing;
    m_btnPlay->setIconType( playing ? MiniconButton::Pause : MiniconButton::Play );
}
void MiniControlWindow::setOpacityValue( int value)
{
    m_opacity = value;
    setWindowOpacity( value / 255.0 );
}
void MiniControlWindow::setCover( const QPixmap &pix)
{
    m_cover = pix;
    update();
}
void MiniControlWindow::setTitle( const QString &title)
{
    m_title = title;
    m_titleLabel1->setText(title);
    m_titleLabel2->setText(title);
    QFontMetrics fm( m_titleLabel1->font() );
    m_textWidth = fm.horizontalAdvance(title);
    int labelWidth = qMax( m_textWidth + 20, 190 );
    m_titleLabel1->resize( labelWidth, 18 );
    m_titleLabel2->resize( labelWidth, 18 );
    m_needScroll = m_textWidth > 170;
    m_scrollOffset = 0;
    if(!m_needScroll)
    {
        int centerX = (190 - m_textWidth) / 2;
        m_titleLabel1->move( centerX, 0 );
        m_titleLabel2->hide();
    }
    else
    {
        m_titleLabel2->show();
        m_titleLabel1->move( 0, 0 );
        m_titleLabel2->move( labelWidth + 40, 0 );
    }
}
void MiniControlWindow::paintEvent( QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint( QPainter::Antialiasing );
    p.setRenderHint( QPainter::SmoothPixmapTransform );
    if(!m_cover.isNull())
    {
        QPixmap blurred = blurPixmap( m_cover, size() );
        p.drawPixmap( rect(), blurred );
    }
    else
    {
        p.fillRect( rect(), QColor(220,225,235) );
    }
    p.fillRect( rect(), QColor( 255, 255, 255, 105 ) );
    QLinearGradient topGlow( 0, 0, 0, height() );
    topGlow.setColorAt( 0, QColor(255,210,220,65) );
    topGlow.setColorAt( 0.25, QColor(255,255,255,20) );
    topGlow.setColorAt( 1, QColor(255,255,255,0) );
    p.fillRect( rect(), topGlow );
    QRadialGradient centerGlow( width()/2, height()/2, width()*0.8 );
    centerGlow.setColorAt( 0, QColor(255,255,255,35) );
    centerGlow.setColorAt( 1, QColor(255,255,255,0) );
    p.fillRect( rect(), centerGlow );
    p.fillRect( 0, 0, width(), 1, QColor(255,255,255,120) );
    p.setPen( QColor(255,255,255,90) );
    p.drawRect( rect().adjusted(0,0,-1,-1) );
}
void MiniControlWindow::mousePressEvent( QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        m_dragging = true;
        m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}
void MiniControlWindow::mouseMoveEvent( QMouseEvent *e)
{
    if(m_dragging)
    {
        move( e->globalPosition().toPoint() - m_dragPos );
    }
}
void MiniControlWindow::mouseReleaseEvent( QMouseEvent *)
{
    m_dragging = false;
    QSettings s;
    s.setValue( "miniControlPos", pos() );
}