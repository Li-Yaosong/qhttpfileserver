#ifndef FILEROUTER_H
#define FILEROUTER_H

#include "router.h"

class FileRouter : public Router
{
    Q_OBJECT
public:
    explicit FileRouter(QObject *parent = nullptr);
    ~FileRouter();

    // Router interface
public:
    QString pathPattern() const override;
    RequestHandler requestHandler() override;
};

#endif // FILEROUTER_H
