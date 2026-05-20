#ifndef DNC_SCADA_MAIN_WINDOW_H
#define DNC_SCADA_MAIN_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QPropertyAnimation>

namespace DncScada {

class MonitorPage;
class SimulatorPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setSimulatorRunning(bool running);

private slots:
    void setStatusMessage(const QString &message);

private:
    void buildUi();
    void switchPage(int index);

    QStackedWidget *stack = nullptr;
    MonitorPage *monitorPage = nullptr;
    SimulatorPage *simulatorPage = nullptr;

    QWidget *sidebar = nullptr;
    QPushButton *monitorNavBtn = nullptr;
    QPushButton *simulatorNavBtn = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *simStatusLabel = nullptr;
    QLabel *statusLabel = nullptr;

    int sidebarCollapsedWidth = 56;
    int sidebarExpandedWidth = 200;
    bool sidebarExpanded = false;
};

} // namespace DncScada

#endif // DNC_SCADA_MAIN_WINDOW_H
