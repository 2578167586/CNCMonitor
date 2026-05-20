#include "MonitorWindow.h"

#include "Types.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DncScada::registerMetaTypes();

    DncScada::MonitorWindow window;
    window.resize(1180, 760);
    window.show();

    return app.exec();
}
