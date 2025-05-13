#ifndef UTIL_H
#define UTIL_H

#include <qglobal.h>
#include <QHttpServerResponder>
#include <QFileInfo>

class Util
{
public:
    Util();
    static void handleRequest(const QUrl path, const QHttpServerRequest &request, QHttpServerResponder &responder);
    static bool setRootDir(const QString &dir);
    static QString rootDir();
private:
    struct HtmlItemAccumulator {
    public:
        explicit HtmlItemAccumulator(const QString& path);
        QString operator()(const QString& list, const QFileInfo& info) const;
    private:
        QString pathStr = "";
    };
};

#endif // UTIL_H
