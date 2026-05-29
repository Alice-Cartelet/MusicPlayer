#pragma once
#include <QWidget>
#include <QTimer>
#include <QCursor>
#include <QFont>
#include "lrcxparser.h"
class LyricsOverlay : public QWidget
{
    Q_OBJECT
public: explicit LyricsOverlay(QWidget *parent = nullptr);
    void loadLyrics(const QString &path);
    void updatePosition(qint64 posMs);
    void getLyricLines(qint64 posMs, QString &prev, QString &cur, QString &next) const;
    void getLyricLines(qint64 posMs, QString &prev, QString &cur, QString &next, QString &translation) const;
    void setLyricsVisible(bool v);
    bool lyricsVisible() const;
    void setHideOnHover(bool v);
    void setFontSize(int size);
    void setFontFamily(const QString &family);
    void setColors(const QString &sung,const QString &unsang);
signals: void closeRequested();
protected: void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;
private: void buildFont();
    void animateTo(float endPx,int durationMs);
    void resizeToText(const QString &text);
    void enforceTopmost();
private: LrcxParser m_parser;
    int m_currentLineIdx=-1;
    QString m_lineText;
    QFont m_font;
    int m_fontSize=28;
    QString m_fontFamily="Microsoft YaHei";
    QVector<float> m_charPixelsF;
    float m_totalW=0.f;
    int m_lineX0=0;
    int m_baseY=0;
    float m_litPx=0.f;
    float m_targetPx=0.f;
    QTimer *m_animTimer=nullptr;
    float m_animStart=0.f;
    float m_lineAlpha=1.f;
    float m_targetAlpha=1.f;
    float m_animEnd=0.f;
    int m_animDur=0;
    int m_animElapsed=0;
    bool m_dragging=false;
    QPoint m_dragOffset;
    int m_anchorCenterX=-1;
    int m_anchorY=-1;
    bool m_hideOnHover=false;
    QColor m_colorSung=QColor("#E63248");
    QColor m_colorUnsang=QColor("#F1DDDF");
    QTimer *m_hoverCheckTimer=nullptr;
    QTimer *m_topmostTimer=nullptr;
}
;
