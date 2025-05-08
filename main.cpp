#include <QCoreApplication>
#include "httpfileserver.h"
const QString ROOT_DIR = "A:/mirror";

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    HttpFileServer server;
    server.setRootDir(ROOT_DIR);
    return app.exec();
}
