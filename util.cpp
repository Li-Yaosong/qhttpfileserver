#include "util.h"
#include <QFile>
#include <QDebug>
#include <QHttpServerResponse>
#include <QMimeDatabase>
#include <QDir>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

class UtilPrivate
{
public:
    static QString ROOT_DIR;
};
QString UtilPrivate::ROOT_DIR = "";
Util::Util() {}

QString Util::readTemplateFile(const QString &filePath)
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

void Util::respondFile(QHttpServerResponder &responder, const QString &filePath)
{
    if (auto *file = new QFile(filePath); file->open(QIODevice::ReadOnly)) {
        QMimeDatabase db;
        QByteArray mime = db.mimeTypeForFile(filePath).name().toUtf8();
        responder.write(file, mime);
    }
    else
    {
        qWarning() << "无法打开文件" << filePath;
        responder.write(errorJson("Directory or file not found"), QHttpServerResponse::StatusCode::NotFound);
    }
}

QString Util::itemTemplate(const QString &icon, const QString &displayName, const QString &href, const QString &fileSize, const QString &lastModified)
{
    QString html = readTemplateFile(":/html/file-list-item-template.html");
    html.replace("${item-icon}$", icon);
    html.replace("${display-name}$", displayName);
    html.replace("${href}$", href);
    html.replace("${file-size}$", fileSize);
    html.replace("${last-modified}$", lastModified);
    return html;
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

QJsonDocument Util::errorJson(QString message)
{
    QJsonObject json;
    json.insert("message", message);
    return QJsonDocument(json);
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

    return list + Util::itemTemplate(info.isDir() ? "icon-folder-close" : "icon-file",
                                            fileName,
                                            pathStr + fileName,
                                            fileSizeStr,
                                            info.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
}
