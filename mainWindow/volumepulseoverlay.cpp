#include "volumepulseoverlay.h"
#include <QPainter>
#include <QtMath>

VolumePulseOverlay::VolumePulseOverlay(QWidget *parent)
    : QWidget(parent), m_pulseStrength(0.0), m_lastVolume(0),
      m_baseColor(200, 60, 80) // 默认暗红
{
    // 透明背景，不接收鼠标事件
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(30);  // ~33 FPS
    connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
        // 脉冲衰减
        m_pulseStrength *= 0.9;
        if (m_pulseStrength < 0.01) {
            m_pulseStrength = 0.0;
            m_fadeTimer->stop();
        }
        update();  // 触发重绘
    });
}

void VolumePulseOverlay::setVolume(int volume)
{
    if (volume == m_lastVolume)
        return;
    m_lastVolume = volume;

    // 脉冲强度 = 音量/100，可以有非线性映射
    m_pulseStrength = qBound(0.0, volume / 100.0, 1.0);

    if (!m_fadeTimer->isActive())
        m_fadeTimer->start();

    update();
}

void VolumePulseOverlay::setBaseColor(const QColor &color)
{
    m_baseColor = color;
    // 如果当前有脉冲，立即重绘以显示新颜色
    if (m_pulseStrength > 0.001)
        update();
}

void VolumePulseOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_pulseStrength < 0.001)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 在进度条区域内绘制一条脉动带
    // 高度取控件高度的 60%
    qreal h = height();
    qreal barHeight = h * 0.6;
    qreal barY = (h - barHeight) / 2.0;

    QRectF barRect(0, barY, width(), barHeight);

    // 主色：使用 m_baseColor，透明度随脉冲强度递减
    int alpha = static_cast<int>(180 * m_pulseStrength);
    QColor baseColor(m_baseColor);
    baseColor.setAlpha(alpha);
    painter.setPen(Qt::NoPen);
    painter.setBrush(baseColor);
    painter.drawRect(barRect);

    // 中间高光（更亮）
    QColor lightColor = m_baseColor.lighter(150);
    lightColor.setAlpha(static_cast<int>(120 * m_pulseStrength));
    painter.setBrush(lightColor);
    painter.drawRect(barRect.adjusted(width()*0.1, 0, -width()*0.1, 0));
}