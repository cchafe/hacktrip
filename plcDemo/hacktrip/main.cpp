#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <iostream>

#ifdef __cplusplus
extern "C"
#endif
    const char* __asan_default_options() { return "detect_leaks=0"; }

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    std::cout << "hi\n";
    return a.exec();
}
