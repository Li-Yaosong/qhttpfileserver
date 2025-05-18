#ifndef ROUTER_H
#define ROUTER_H

#include <QObject>

class QHttpServerRequest;
class QHttpServerResponder;

class Router : public QObject
{
    Q_OBJECT
public:
    using RequestHandler = void (*)(const QUrl, const QHttpServerRequest&, QHttpServerResponder&);
    explicit Router(QObject *parent = nullptr);
    ~Router() = default;
    virtual QString pathPattern() const = 0;
    virtual RequestHandler requestHandler();
};
using RouterMap = QMap<QString, QSharedPointer<Router>>;
#endif // ROUTER_H
