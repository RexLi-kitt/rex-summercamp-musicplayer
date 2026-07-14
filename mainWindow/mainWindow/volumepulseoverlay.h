#ifndef VOLUMEPULSEOVERLAY_H
#define VOLUMEPULSEOVERLAY_H

#include <QWidget>
#include <QTimer>
#include <QColor>

class VolumePulseOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit VolumePulseOverlay(QWidget *parent = nullptr);

    void setVolume(int volume);  // 0~100
    void setBaseColor(const QColor &color); // 设置脉动基础色

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTimer *m_fadeTimer;
    qreal   m_pulseStrength;   // 0.0 ~ 1.0
    int     m_lastVolume;      // 用于去重
    QColor  m_baseColor;       // 脉动基础色
};

#endif // VOLUMEPULSEOVERLAY_H