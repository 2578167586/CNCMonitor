#include "PlotWidget.h"
#include "qcustomplot.h"

#include <QLabel>
#include <QMouseEvent>
#include <QToolTip>
#include <QVBoxLayout>

namespace DncScada {

PlotWidget::PlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(220);

    plot = new QCustomPlot(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot);

    plot->setBackground(QColor("#0d1117"));
    plot->xAxis->setLabelColor(QColor("#8b949e"));
    plot->yAxis->setLabelColor(QColor("#8b949e"));
    plot->xAxis->setTickLabelColor(QColor("#8b949e"));
    plot->yAxis->setTickLabelColor(QColor("#8b949e"));
    plot->xAxis->setBasePen(QPen(QColor("#30363d")));
    plot->yAxis->setBasePen(QPen(QColor("#30363d")));
    plot->xAxis->setTickPen(QPen(QColor("#30363d")));
    plot->yAxis->setTickPen(QPen(QColor("#30363d")));
    plot->xAxis->grid()->setPen(QPen(QColor("#21262d"), 1, Qt::DotLine));
    plot->yAxis->grid()->setPen(QPen(QColor("#21262d"), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridVisible(false);
    plot->yAxis->grid()->setSubGridVisible(false);

    plot->xAxis->setRange(0, maxSamples);
    plot->yAxis->setRange(0, 100);
    plot->xAxis->setLabel(QStringLiteral("Samples"));
    plot->yAxis->setLabel(QStringLiteral("Value"));

    loadGraph = plot->addGraph();
    loadGraph->setPen(QPen(QColor("#3b82f6"), 2));
    loadGraph->setName(QStringLiteral("Load"));

    tempGraph = plot->addGraph();
    tempGraph->setPen(QPen(QColor("#22c55e"), 2));
    tempGraph->setName(QStringLiteral("Temp"));

    rpmGraph = plot->addGraph();
    rpmGraph->setPen(QPen(QColor("#ef4444"), 2));
    rpmGraph->setName(QStringLiteral("RPM/60"));

    plot->legend->setVisible(true);
    plot->legend->setBrush(QBrush(QColor(22, 27, 34, 200)));
    plot->legend->setTextColor(QColor("#e6edf3"));
    plot->legend->setBorderPen(QPen(QColor("#30363d")));
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    tracer = new QCPItemTracer(plot);
    tracer->setVisible(false);
    tracer->setStyle(QCPItemTracer::tsCircle);
    tracer->setPen(QPen(QColor("#e6edf3")));
    tracer->setBrush(QColor("#3b82f6"));
    tracer->setSize(7);

    connect(plot, &QCustomPlot::mouseMove, this, [this](QMouseEvent *) {
        const double x = plot->xAxis->pixelToCoord(plot->mapFromGlobal(QCursor::pos()).x());
        const int key = qRound(x);
        // Convert from graph key to sample array index
        const int idx = key - qMax(0, static_cast<int>(xIndex) - samples.size());
        if (idx >= 0 && idx < samples.size()) {
            const Sample &s = samples.at(idx);
            tracer->setVisible(true);
            tracer->setGraph(loadGraph);
            tracer->setGraphKey(key);
            tracer->updatePosition();
            QToolTip::showText(QCursor::pos(),
                QStringLiteral("Load: %1  Temp: %2  RPM/60: %3")
                    .arg(s.load, 0, 'f', 1)
                    .arg(s.temperature, 0, 'f', 1)
                    .arg(s.rpm, 0, 'f', 1),
                this);
        } else {
            tracer->setVisible(false);
            QToolTip::hideText();
        }
    });

    // Throttle replot to ~20 FPS (50 ms interval)
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(50);
    connect(refreshTimer, &QTimer::timeout, this, &PlotWidget::refreshPlot);
    refreshTimer->start();
}

QSize PlotWidget::minimumSizeHint() const {
    return QSize(480, 220);
}

void PlotWidget::appendStatus(DeviceStatus status) {
    Sample sample;
    sample.load = status.servoLoad;
    sample.temperature = status.temperature;
    sample.rpm = status.spindleRpm / 60.0;

    samples.append(sample);
    while (samples.size() > maxSamples) {
        samples.removeFirst();
    }

    // Incremental add — only the new point, no clearing
    const double key = xIndex;
    loadGraph->addData(key, qBound(0.0, sample.load, 100.0));
    tempGraph->addData(key, qBound(0.0, sample.temperature, 100.0));
    rpmGraph->addData(key, qBound(0.0, sample.rpm, 100.0));

    // Remove data points that fell out of the window
    const double removeBefore = key - maxSamples;
    loadGraph->data()->removeBefore(removeBefore);
    tempGraph->data()->removeBefore(removeBefore);
    rpmGraph->data()->removeBefore(removeBefore);

    ++xIndex;
    dirty = true;
}

void PlotWidget::refreshPlot() {
    if (!dirty)
        return;

    const double lastKey = xIndex - 1.0;
    if (lastKey > 0.0) {
        plot->xAxis->setRange(qMax(0.0, lastKey - maxSamples), lastKey + 10);
    }
    plot->replot();
    dirty = false;
}

void PlotWidget::clear() {
    samples.clear();
    xIndex = 0.0;
    loadGraph->data()->clear();
    tempGraph->data()->clear();
    rpmGraph->data()->clear();
    dirty = true;
    refreshPlot();
}

} // namespace DncScada
