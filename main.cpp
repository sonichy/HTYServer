#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qSetMessagePattern("[ %{file}: %{line} ] %{message}");
    QApplication a(argc, argv);
    a.setOrganizationName("HTY");
    a.setApplicationName("HTYServer");
    MainWindow w;
    w.show();
    return a.exec();
}