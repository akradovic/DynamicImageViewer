// main.cpp
#include <QApplication>
#include "ui/mainwindow.h"

/**
 * @brief Application entry point.
 *
 * Initializes the Qt application and main window.
 *
 * @param argc Command line argument count.
 * @param argv Command line argument values.
 * @return Application exit code.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.showMaximized();

    return app.exec();
}
