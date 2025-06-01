#include "staticresourcerouter.h"
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QHttpServerResponse>

StaticResourceRouter::StaticResourceRouter(QObject *parent)
    : FileRouter{parent}
{}

QString StaticResourceRouter::pathPattern() const
{
    return "/.static/";
}
