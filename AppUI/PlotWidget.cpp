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
        double x = plot->xAxis->pixelToCoord(plot->mapFromGlobal(QCursor::pos()).x());
        int idx = qRound(x);
        if (idx >= 0 && idx < samples.size()) {
            const Sample &s = samples.at(idx);
            tracer->setVisible(true);
            tracer->setGraph(loadGraph);
            tracer->setGraphKey(idx);
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

    loadGraph->data()->clear();
    tempGraph->data()->clear();
    rpmGraph->data()->clear();

    for (int i = 0; i < samples.size(); ++i) {
        const Sample &s = samples.at(i);
        loadGraph->addData(i, qBound(0.0, s.load, 100.0));
        tempGraph->addData(i, qBound(0.0, s.temperature, 100.0));
        rpmGraph->addData(i, qBound(0.0, s.rpm, 100.0));
    }

    if (samples.size() > 1) {
        plot->xAxis->setRange(qMax(0.0, double(samples.size() - maxSamples)), double(samples.size() + 10));
    }

    plot->replot();
}

void PlotWidget::clear() {
    samples.clear();
    loadGraph->data()->clear();
    tempGraph->data()->clear();
    rpmGraph->data()->clear();
    plot->replot();
}

} // namespace DncScada
