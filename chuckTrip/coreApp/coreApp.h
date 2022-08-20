#ifndef COREAPP_H
#define COREAPP_H

#include "coreApp_global.h"
#include "qcoreapplication.h"
#include <iostream>
#include <thread>

class COREAPP_EXPORT CoreApp :public QObject
{
    Q_OBJECT
public:
    CoreApp();
private:
    static void task1(std::string msg);
    void start();
    inline static int argc = 1;
    inline static char name[] = "CoreApp";
    inline static QCoreApplication * app = NULL;
};

 void CoreApp::task1(std::string msg)
{
    std::cout << "task1 says: " << msg;
}

CoreApp::CoreApp()
{
    std::cout << " instantiating CoreApp = "<< name << std::endl;
//    std::thread t1(task1, "Hello");
//    t1.join();
//    start();
}
void CoreApp::start()
{
    std::cout << " starting CoreApp = "<< name << std::endl;
    char * argv[2];
    argv[0] = name;
    argv[1] = NULL;
    app = new QCoreApplication(argc, argv);
    app->exec();
}
#endif // COREAPP_H
