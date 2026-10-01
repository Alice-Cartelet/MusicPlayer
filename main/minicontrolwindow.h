#pragma once
#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include "miniconbutton.h"
class MiniControlWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MiniControlWindow( QWidget *parent = nullptr );
    void setProgress(qint64 pos,qint64 dur);
    void setPlaying(bool playing);
    void setOpacityValue(int value);
    void setCover( const QPixmap &pix );
    void setTitle( const QString &title );
signals:
    void playPauseClicked();
    void expandClicked();
    void nextClicked();
    void prevClicked();
protected:
    void paintEvent( QPaintEvent *event ) override;
    void mousePressEvent( QMouseEvent *e ) override;
    void mouseMoveEvent( QMouseEvent *e ) override;
    void resizeEvent(QResizeEvent *e) override;
    void mouseReleaseEvent( QMouseEvent *e ) override;
private:
    MiniconButton *m_btnPrev;
    QPushButton *m_btnExpand = nullptr;
    QWidget *m_progressBarBg = nullptr;
    QWidget *m_progressBarFill = nullptr;
    qint64 m_duration = 0;
    qint64 m_position = 0;
    MiniconButton *m_btnPlay;
    MiniconButton *m_btnNext;
    QWidget *m_titleContainer;
    QLabel *m_titleLabel1;
    QLabel *m_titleLabel2;
    bool m_playing = false;
    bool m_dragging = false;
    QPoint m_dragPos;
    int m_opacity = 85;
    QPixmap m_cover;
    QString m_title;
    QTimer *m_scrollTimer;
    int m_scrollOffset = 0;
    int m_textWidth = 0;
    bool m_needScroll = false;
}
;