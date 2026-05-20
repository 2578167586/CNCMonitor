#include "RealTimePlotWidget.h"

#include <QPainter>

namespace DncScada {

RealTimePlotWidget::RealTimePlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(220);
    setAutoFillBackground(false);
}

QSize RealTimePlotWidget::minimumSizeHint() const
{
    return QSize(480, 220);
}

void RealTimePlotWidget::appendStatus(DeviceStatus status)
{
    Sample sample;
    sample.load = status.servoLoad;
    sample.temperature = status.temperature;
    sample.rpm = status.spindleRpm / 60.0;
    samples.append(sample);
    while (samples.size() > maxSamples) {
        samples.removeFirst();
    }
    update();
}

void RealTimePlotWidget::clear()
{
    samples.clear();
    update();
}

void RealTimePlotWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(250, 251, 252));

    const QRect plot = rect().adjusted(46, 18, -18, -34);
    painter.setPen(QColor(210, 216, 224));
    painter.drawRect(plot);
    for (int i = 1; i < 4; ++i) {
        const int y = plot.top() + plot.height() * i / 4;
        painter.drawLine(plot.left(), y, plot.right(), y);
    }

    const auto drawSeries = [&](int channel, const QColor &color, const QString &label) {
        if (samples.size() < 2) {
            return;
        }

        QPainterPath path;
        for (int i = 0; i < samples.size(); ++i) {
            double value = 0.0;
            if (channel == 0) {
                value = samples.at(i).load;
            } else if (channel == 1) {
                value = samples.at(i).temperature;
            } else {
                value = samples.at(i).rpm;
            }
            const double normalized = qBound(0.0, value / 100.0, 1.0);
            const double x = plot.left() + (plot.width() * i) / qMax(1, samples.size() - 1);
            const double y = plot.bottom() - normalized * plot.height();
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        painter.setPen(QPen(color, 2));
        painter.drawPath(path);

        painter.setPen(color);
        painter.drawText(plot.left() + 8 + channel * 120, rect().bottom() - 10, label);
    };

    drawSeries(0, QColor(39, 119, 214), QStringLiteral("Load"));
    drawSeries(1, QColor(16, 145, 116), QStringLiteral("Temp"));
    drawSeries(2, QColor(191, 85, 61), QStringLiteral("RPM/60"));
}

} // namespace DncScada
