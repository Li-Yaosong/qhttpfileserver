#include "httpfileserver.h"
#include <QHttpServer>
#include <QTcpServer>
#include <QDir>
#include <numeric>
#include <qcoreapplication.h>


class HttpFileServerPrivate
{
public:
    explicit HttpFileServerPrivate()
        :server(new QHttpServer)
    {
        // 目录浏览
        server->route("/", &HttpFileServerPrivate::handleRequest);
    }

    static QHttpServerResponse handleRequest(const QUrl path)
    {
        qDebug() << "path:" << path;

        QFileInfo fileInfo(ROOT_DIR + "/" + path.toString());
        if (!fileInfo.exists())
            return QHttpServerResponse("Directory or file not found\n", QHttpServerResponse::StatusCode::NotFound);
        if(fileInfo.isFile())
            return QHttpServerResponse::fromFile(fileInfo.absoluteFilePath());

        auto html = QString("<html><body><h3>Index of /%1</h3><ul>").arg(path.toString());

        QDir dir(ROOT_DIR);
        if(dir.cd(path.toString()))
        {
            if (!path.isEmpty()) {
                auto subPathList = path.toString().split("/", Qt::SkipEmptyParts);
                subPathList.removeLast();
                html += QString("<li><a href=\"/%1\">../</a></li>").arg(subPathList.join("/"));
            }

            const auto infoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
            html += std::accumulate(infoList.begin(), infoList.end(), QString(),
                    [&path](const QString &temp, const QFileInfo &info) {
                        QString fileName = info.isDir() ? info.fileName() + "/" : info.fileName();
                        QString filePathLink = fileName;
                        if(!path.isEmpty())
                        {
                            path.toString().endsWith("/") ? filePathLink.prepend( path.toString())
                                                          : filePathLink.prepend( path.toString() + "/");
                        }
                        return temp + QString("<li><a href=\"/%1\">%2</a></li>").arg(filePathLink, fileName);
                    });

            html += "</ul></body></html>";
            return QHttpServerResponse("text/html", html.toUtf8());
        }
        return QHttpServerResponse("Directory not found\n", QHttpServerResponse::StatusCode::NotFound);
    }

    HttpFileServerPrivate(const HttpFileServerPrivate &) = delete;
    HttpFileServerPrivate& operator=(const HttpFileServerPrivate &) = delete;

    ~HttpFileServerPrivate() = default;

    QScopedPointer<QHttpServer> server;
    static QString ROOT_DIR;

};
QString HttpFileServerPrivate::ROOT_DIR = "";
HttpFileServer::HttpFileServer(QObject *parent)
    : QObject{parent},
    d_ptr{new HttpFileServerPrivate}
{
    auto tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, 80)) {
        qCritical() << "监听失败：" << tcpServer->errorString();
        return;
    }

    if (!d_ptr->server->bind(tcpServer)) {
        qCritical() << "QHttpServer 绑定失败";
        return;
    }
    qInfo() << "目录浏览服务器已启动：";
    qInfo() << "浏览：http://127.0.0.1:80/";
    setRootDir(QCoreApplication::applicationDirPath());
}

HttpFileServer::~HttpFileServer()
{
    if (d_ptr->server)
    {
        auto servers = d_ptr->server->servers();
        for(auto *server : std::as_const(servers))
        {
            server->close();
            server->deleteLater();
        }
    }
    d_ptr->server.reset();
}

QString HttpFileServer::rootDir() const
{
    return HttpFileServerPrivate::ROOT_DIR;
}

void HttpFileServer::setRootDir(const QString &newRootDir)
{
    if (newRootDir == HttpFileServerPrivate::ROOT_DIR)
        return;
    HttpFileServerPrivate::ROOT_DIR = newRootDir;
    emit rootDirChanged(HttpFileServerPrivate::ROOT_DIR);
}
