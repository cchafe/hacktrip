#include <QCoreApplication>
#include "jacktripclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    JackTripClient client;
    
    // Server IP address provided in the prompt
    const QString serverIp = "171.64.197.158";
    
    client.connectToServer(serverIp);

    // Start the Qt event loop
    return a.exec();
}

