#include "positionpicker.h"

#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QSettings>

#include <windows.h>

PositionPicker::PositionPicker(QWidget *p)
    :QDialog(p)
{
    setWindowTitle("选择歌词位置");

    resize(1280,720);

    setStyleSheet(R"(

QDialog{
background:#101015;
}

QPushButton{

background:#6a5cff;
border:none;

border-radius:10px;

color:white;

padding:8px 20px;
font-size:14px;
}

QPushButton:hover{

background:#8276ff;
}

)");

    m_background=new QLabel(this);

    m_background->setGeometry(rect());

    loadWallpaper();

    // 半透明遮罩

    QLabel *overlay=
        new QLabel(this);

    overlay->setGeometry(rect());

    overlay->setStyleSheet(
        "background:rgba(0,0,0,80);"
        );

    // 歌词预览

    m_preview=
        new QLabel(
            "🚩",
            this);

    m_preview->setStyleSheet(R"(

background:rgba(30,30,40,180);

color:white;

font-size:32px;

font-weight:bold;

padding:18px;

border-radius:15px;

)");

    m_preview->adjustSize();

    m_preview->move(
        width()/2-
            m_preview->width()/2,

        height()/2);

    m_pos=m_preview->pos();

    // 坐标显示

    m_tip=
        new QLabel(this);

    m_tip->setStyleSheet(R"(

color:white;

font-size:13px;

background:rgba(0,0,0,150);

padding:6px;

border-radius:6px;

)");

    m_tip->move(20,20);

    m_tip->setText(
        QString(
            "X:%1 Y:%2")
            .arg(m_pos.x())
            .arg(m_pos.y()));

    QPushButton *ok=
        new QPushButton(
            "确定",
            this);

    ok->move(
        width()-130,
        height()-60);

    connect(
        ok,
        &QPushButton::clicked,
        this,
        &QDialog::accept);
}

void PositionPicker::loadWallpaper()
{
    wchar_t path[MAX_PATH];

    SystemParametersInfoW(
        SPI_GETDESKWALLPAPER,
        MAX_PATH,
        path,
        0);

    QString wallpaper=
        QString::fromWCharArray(
            path);

    QPixmap img(wallpaper);

    m_background->setPixmap(
        img.scaled(
            size(),
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation));

}

void PositionPicker::resizeEvent(QResizeEvent*)
{
    if(m_background)
    {
        m_background->setGeometry(rect());

        loadWallpaper();
    }
}

QPoint PositionPicker::selectedPos() const
{
    return m_pos;
}

void PositionPicker::mousePressEvent(
    QMouseEvent *e)
{
    if(
        m_preview->geometry()
            .contains(
                e->pos()))
    {
        m_drag=true;

        m_offset=
            e->pos()
            -
            m_preview->pos();
    }
}

void PositionPicker::mouseMoveEvent(
    QMouseEvent *e)
{
    if(!m_drag)
        return;

    QPoint p=
        e->pos()
        -
        m_offset;

    m_preview->move(p);

    m_pos=p;

    m_tip->setText(
        QString(
            "X:%1 Y:%2")
            .arg(p.x())
            .arg(p.y()));
}

void PositionPicker::mouseReleaseEvent(
    QMouseEvent*)
{
    m_drag=false;
}