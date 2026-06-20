#ifndef DNC_SCADA_PLOT_WIDGET_H
#define DNC_SCADA_PLOT_WIDGET_H

#include "Types.h"
#include <QTimer>
#include <QWidget>

class QCustomPlot;
class QCPGraph;
class QCPItemTracer;

namespace DncScada {

class PlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlotWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;

public slots:
    void appendStatus(DeviceStatus status);
    void clear();

private slots:
    void refreshPlot();

private:
    struct Sample {
        double load = 0.0;
        double temperature = 0.0;
        double rpm = 0.0;
    };

    QCustomPlot *plot = nullptr;
    QCPGraph *loadGraph = nullptr;
    QCPGraph *tempGraph = nullptr;
    QCPGraph *rpmGraph = nullptr;
    QCPItemTracer *tracer = nullptr;
    QTimer *refreshTimer = nullptr;
    QVector<Sample> samples;
    int maxSamples = 300;
    double xIndex = 0.0;
    bool dirty = false;
};

} // namespace DncScada

#endif // DNC_SCADA_PLOT_WIDGET_H
