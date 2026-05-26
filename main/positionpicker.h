#pragma once
#include <QDialog>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QPixmap>
#include "desktopwallpaper.h"   // LyricOrientation

class QLabel;

class PositionPicker : public QDialog
{
    Q_OBJECT
public:
    explicit PositionPicker(LyricOrientation orientation = LyricOrientation::Horizontal,
                            QWidget *p = nullptr);
    QPoint selectedPos() const;

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;

private:
    void   loadWallpaper();
    QPoint toReal(QPoint preview) const;
    QPoint toPreview(QPoint real) const;
    QSize  previewLyricSize() const;
    void   updateCoordLabel();

    LyricOrientation m_orientation;
    QPixmap  m_wallpaper;
    QLabel  *m_coordLabel = nullptr;
    QLabel  *m_hint       = nullptr;
    QRect    m_canvas;
    QSize    m_desktopSize;
    QPoint   m_anchor;
    QPoint   m_realPos;
    bool     m_drag       = false;
    QPoint   m_dragOffset;
};