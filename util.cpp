#include "util.h"
#include <QFile>
#include <QDebug>
#include <QHttpServerResponse>
#include <QMimeDatabase>
#include <QDir>
#include <QUrl>

class UtilPrivate
{
public:
    static QString readTemplateFile(const QString &filePath);
    static void respondFile(QHttpServerResponder &responder, const QString &filePath);

    static QString itemTemplate(const QString &icon,const QString &displayName,
                                const QString &href,
                                const QString &fileSize = "",
                                const QString &lastModified = "");
    static QString ROOT_DIR;
};
QString UtilPrivate::ROOT_DIR = "";
Util::Util() {}

QString UtilPrivate::readTemplateFile(const QString &filePath)
{
    if (auto *file = new QFile(filePath); !file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开模板文件";
        return QString();
    }
    else
    {
        QString html = file->readAll();
        file->close();
        return html;
    }
}

void UtilPrivate::respondFile(QHttpServerResponder &responder, const QString &filePath)
{
    if (QFile file(filePath); file.open(QIODevice::ReadOnly)) {
        QMimeDatabase db;
        QByteArray mime = db.mimeTypeForFile(filePath).name().toUtf8();
        responder.write(&file, mime);
    }
    else
    {
        qWarning() << "无法打开文件" << filePath;
        responder.write("{\"message\": \"Directory or file not found\"}", "application/json",
                        QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QString UtilPrivate::itemTemplate(const QString &icon, const QString &displayName, const QString &href, const QString &fileSize, const QString &lastModified)
{
    QString html = readTemplateFile(":/html/file-list-item-template.html");
    html.replace("${item-icon}$", icon);
    html.replace("${display-name}$", displayName);
    html.replace("${href}$", href);
    html.replace("${file-size}$", fileSize);
    html.replace("${last-modified}$", lastModified);
    return html;
}

void Util::handleRequest(const QUrl path, const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    QString pathStr = path.toString();
    if(pathStr.startsWith("*static"))
    {
        return UtilPrivate::respondFile(responder, pathStr.replace("*static", ":/html/static"));
    }
    QDir dir(UtilPrivate::ROOT_DIR);
    QFileInfo fileInfo(dir, pathStr);
    if (!fileInfo.exists())
        return responder.write("{\"message\": \"Directory or file not found\"}", "application/json",
                               QHttpServerResponse::StatusCode::NotFound);
    if(fileInfo.isFile())
    {
        return UtilPrivate::respondFile(responder, fileInfo.absoluteFilePath());
    }
    else if(fileInfo.isDir())
    {
        QString html = UtilPrivate::readTemplateFile(":/html/index-template.html");

        if(dir.cd(pathStr))
        {
            if (!pathStr.isEmpty()) {
                auto subPathList = pathStr.split("/", Qt::SkipEmptyParts);
                pathStr = subPathList.join("/") + "/";
                subPathList.removeLast();
                QString subPath = subPathList.join("/");
                QString parentDirHtml = UtilPrivate::itemTemplate("icon-arrow-up", "Parent Directory", subPath);
                html.replace("${Parent Directory}$", parentDirHtml);
            }
            else
            {
                html.replace("${Parent Directory}$", "");
            }

            const auto infoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
            html.replace("${file-list}$",
                         std::accumulate(infoList.begin(), infoList.end(), QString(), HtmlItemAccumulator(pathStr)));
            responder.write(html.toUtf8(), "text/html");
        }
        else
        {
            qWarning() << "无法访问目录" << pathStr;
            responder.write("{\"message\": \"Directory access denied\"}", "application/json",
                            QHttpServerResponse::StatusCode::Forbidden);
        }
    }
}

bool Util::setRootDir(const QString &dir)
{
    if(dir != UtilPrivate::ROOT_DIR)
    {
        UtilPrivate::ROOT_DIR = dir;
        return true;
    }
    return false;
}

QString Util::rootDir()
{
    return UtilPrivate::ROOT_DIR;
}

Util::HtmlItemAccumulator::HtmlItemAccumulator(const QString &path)
    : pathStr(path)
{

}

QString Util::HtmlItemAccumulator::operator()(const QString &list, const QFileInfo &info) const {
    QString fileName = info.fileName();
    QString fileSizeStr;

    if(info.isDir()) {
        fileName += "/";
    } else {
        quint64 fileSize = info.size();
        if (fileSize >= 1024 * 1024 * 1024)
            fileSizeStr = QString::number(fileSize / (1024 * 1024 * 1024)) + " GB";
        else if (fileSize >= 1024 * 1024)
            fileSizeStr = QString::number(fileSize / (1024 * 1024)) + " MB";
        else if (fileSize >= 1024)
            fileSizeStr = QString::number(fileSize / 1024) + " KB";
        else
            fileSizeStr = QString::number(fileSize) + " B";
    }

    return list + UtilPrivate::itemTemplate(info.isDir() ? "icon-folder-close" : "icon-file",
                                            fileName,
                                            pathStr + fileName,
                                            fileSizeStr,
                                            info.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
}
