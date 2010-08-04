#include <QtGui/QApplication>
#include <QModelIndex>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.initData();

    w.show();
    return a.exec();
}
