#include "mainwindow.h"
#include "../libduplex/libduplex.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
//    Libduplex dup;
//    std::cout << "dup finish" << std::endl;
    return a.exec();
}
