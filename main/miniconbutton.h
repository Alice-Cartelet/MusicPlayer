#pragma once
#include <QPushButton>
class MiniconButton : public QPushButton
{
    Q_OBJECT
public:
    enum IconType
    {
        Prev,
        Play,
        Pause,
        Next
    };
    explicit MiniconButton(
        IconType type,
        QWidget *parent = nullptr
        );

    void setIconType(IconType type);
protected:
    void paintEvent(
        QPaintEvent *event
        ) override;
private:
    IconType m_type;
};