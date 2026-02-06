#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Unzipper Pro");
    app.setApplicationVersion("2.0.0");
    app.setApplicationDisplayName("Unzipper Pro");

    MainWindow window;
    window.show();

    return app.exec();
}
