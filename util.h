#ifndef UTIL_H
#define UTIL_H

#include <qglobal.h>
#include <QHttpServerResponder>
#include <QFileInfo>

class Util
{
public:
    Util();
    static QString readTemplateFile(const QString &filePath);
    static void respondFile(QHttpServerResponder &responder, const QString &filePath);

    static QString itemTemplate(const QString &icon,const QString &displayName,
                                const QString &href,
                                const QString &fileSize = "",
                                const QString &lastModified = "");
    static bool setRootDir(const QString &dir);
    static QString rootDir();

    struct HtmlItemAccumulator {
    public:
        explicit HtmlItemAccumulator(const QString& path);
        QString operator()(const QString& list, const QFileInfo& info) const;
    private:
        QString pathStr = "";
    };
};

#endif // UTIL_H
