#ifndef STATICRESOURCEROUTER_H
#define STATICRESOURCEROUTER_H

#include <QtCore/qglobal.h>
#include "filerouter.h"

class StaticResourceRouter : public FileRouter
{
    Q_OBJECT
public:
    explicit StaticResourceRouter(QObject *parent = nullptr);

    // Router interface
public:
    QString pathPattern() const override;
};

#endif // STATICRESOURCEROUTER_H
