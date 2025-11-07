#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowTitle("EasyKeyTrigger v0.0.2");

    w.show();
    return a.exec();
}
