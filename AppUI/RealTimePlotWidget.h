#ifndef DNC_SCADA_REAL_TIME_PLOT_WIDGET_H
#define DNC_SCADA_REAL_TIME_PLOT_WIDGET_H

#include "Types.h"

#include <QHash>
#include <QWidget>

namespace DncScada {

class RealTimePlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RealTimePlotWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;

public slots:
    void appendStatus(DncScada::DeviceStatus status);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Sample {
        double load = 0.0;
        double temperature = 0.0;
        double rpm = 0.0;
    };

    QVector<Sample> samples;
    int maxSamples = 300;
};

} // namespace DncScada

#endif // DNC_SCADA_REAL_TIME_PLOT_WIDGET_H
