#include "miniconbutton.h"
#include <QPainter>
#include <QPainterPath>
MiniconButton::MiniconButton( IconType type, QWidget *parent) : QPushButton(parent), m_type(type)
{
    setFixedSize(28,28);
    setCursor( Qt::PointingHandCursor );
    setStyleSheet(R"( QPushButton
    {
        background:rgba(255,255,255,145);
        border:1px solid rgba(255,255,255,120);
        border-radius:14px;
    }
    QPushButton:hover
    {
        background:rgba(255,255,255,175);
    }
    QPushButton:pressed
    {
        background:rgba(255,255,255,210);
    }
    )");
}
void MiniconButton::setIconType( IconType type)
{
    m_type = type;
    update();
}
void MiniconButton::paintEvent( QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    QPainter p(this);
    p.setRenderHint( QPainter::Antialiasing );
    p.setPen(Qt::NoPen);
    p.setBrush( QColor(25,25,30) );
    QRectF r = rect();
    if(m_type == Play)
    {
        QPainterPath path;
        path.moveTo( r.center().x()-4, r.center().y()-6 );
        path.lineTo( r.center().x()+6, r.center().y() );
        path.lineTo( r.center().x()-4, r.center().y()+6 );
        path.closeSubpath();
        p.drawPath(path);
    }
    else if(m_type == Pause)
    {
        p.drawRoundedRect( QRectF( r.center().x()-5, r.center().y()-6, 3, 12 ), 1, 1 );
        p.drawRoundedRect( QRectF( r.center().x()+2, r.center().y()-6, 3, 12 ), 1, 1 );
    }
    else if(m_type == Prev)
    {
        p.drawRoundedRect( QRectF( r.center().x()-7, r.center().y()-6, 2, 12 ), 1, 1 );
        QPainterPath tri;
        tri.moveTo( r.center().x()+5, r.center().y()-6 );
        tri.lineTo( r.center().x()-3, r.center().y() );
        tri.lineTo( r.center().x()+5, r.center().y()+6 );
        tri.closeSubpath();
        p.drawPath(tri);
    }
    else if(m_type == Next)
    {
        p.drawRoundedRect( QRectF( r.center().x()+5, r.center().y()-6, 2, 12 ), 1, 1 );
        QPainterPath tri;
        tri.moveTo( r.center().x()-5, r.center().y()-6 );
        tri.lineTo( r.center().x()+3, r.center().y() );
        tri.lineTo( r.center().x()-5, r.center().y()+6 );
        tri.closeSubpath();
        p.drawPath(tri);
    }
}