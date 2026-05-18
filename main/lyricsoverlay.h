#pragma once
#include <QWidget>
#include <QTimer>
#include <QFont>
#include "lrcxparser.h"
class LyricsOverlay : public QWidget {
    Q_OBJECT
public:
    explicit LyricsOverlay(
        QWidget *parent = nullptr);
    void loadLyrics(
        const QString &path);
    void updatePosition(
        qint64 posMs);
    void setLyricsVisible(
        bool v);
    bool lyricsVisible() const;
signals:
    void closeRequested();
protected:
    void paintEvent(
        QPaintEvent *) override;
    void showEvent(
        QShowEvent *e) override;
    void mousePressEvent(
        QMouseEvent *e) override;
    void mouseMoveEvent(
        QMouseEvent *e) override;
    void mouseReleaseEvent(
        QMouseEvent *e) override;
private:
    void buildFont();
    void animateTo(
        float endPx,
        int durationMs);
private:
    LrcxParser m_parser;
    int m_currentLineIdx = -1;
    QString m_lineText;
    QFont m_font;
    QVector<float> m_charPixelsF;
    float m_totalW = 0.f;
    int m_lineX0 = 0;
    int m_baseY = 0;
    float m_litPx = 0.f;
    float m_targetPx = 0.f;
    QTimer *m_animTimer = nullptr;
    float m_animStart = 0.f;
    float m_lineAlpha = 1.f;
    float m_targetAlpha = 1.f;
    float m_animEnd = 0.f;
    int m_animDur = 0;
    int m_animElapsed = 0;
    bool m_dragging = false;
    QPoint m_dragOffset;
}
;