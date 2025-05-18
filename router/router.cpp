#include "router.h"
Router::Router(QObject *parent)
    : QObject{parent}
{
}

Router::RequestHandler Router::requestHandler()
{
    return RequestHandler();
}
