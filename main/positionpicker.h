#pragma once
#include <QDialog>
#include <QPoint>
class QLabel;
class PositionPicker : public QDialog
{
    Q_OBJECT
public: explicit PositionPicker(QWidget *p=nullptr);
    QPoint selectedPos() const;
protected: void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private: void loadWallpaper();
    QLabel *m_background=nullptr;
    QLabel *m_preview=nullptr;
    QLabel *m_tip=nullptr;
    QPoint m_pos;
    QPoint m_offset;
    bool m_drag=false;
}
;