#include "MainWindow.h"
#include "Types.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DncScada::registerMetaTypes();

    DncScada::MainWindow window;
    window.resize(1280, 820);
    window.show();

    return app.exec();
}
