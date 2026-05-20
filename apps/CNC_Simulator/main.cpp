#include "SimulatorWindow.h"

#include "Types.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DncScada::registerMetaTypes();

    DncScada::SimulatorWindow window;
    window.resize(980, 620);
    window.show();

    return app.exec();
}
