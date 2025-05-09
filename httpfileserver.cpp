#include "httpfileserver.h"
#include <QHttpServer>
#include <QTcpServer>
#include <QDir>
#include <numeric>
#include <QCoreApplication>
#include <QMimeDatabase>
#ifdef ENABLE_GUI
#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QMenu>
#else
#include <QProcess>
#endif

class HttpFileServerPrivate
{
public:
    explicit HttpFileServerPrivate() = default;

    static void handleRequest(const QUrl path, const QHttpServerRequest &request, QHttpServerResponder &responder)
    {
        qDebug() << "path:" << path;
        QFileInfo fileInfo(ROOT_DIR + "/" + path.toString());
        if (!fileInfo.exists())
            return responder.write("{\"message\": \"Directory or file not found\"}", "application/json",
                                   QHttpServerResponse::StatusCode::NotFound);
        if(fileInfo.isFile())
        {

            if (QFile *file = new QFile(fileInfo.absoluteFilePath()); file->open(QIODevice::ReadOnly)) {
                QMimeDatabase db;
                QByteArray mime = db.mimeTypeForFile(fileInfo).name().toUtf8();
                return responder.write(file, mime);
            }
            else
            {
                delete file;
                return responder.write("{\"message\": \"Directory or file not found\"}", "application/json",
                                       QHttpServerResponse::StatusCode::InternalServerError);
            }
        }
        else if(fileInfo.isDir())
        {
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
                responder.write(html.toUtf8(), "text/html");
            }
            else
            {
                qWarning() << "无法访问目录" << path.toString();

                responder.write("{\"message\": \"Directory access denied\"}", "application/json",
                                QHttpServerResponse::StatusCode::Forbidden);
            }
        }
    }

#ifdef ENABLE_GUI
    bool showTrayIcon()
    {
        if (QSystemTrayIcon::isSystemTrayAvailable() && !trayIcon) {
            trayIcon.reset(new QSystemTrayIcon(QIcon(APP_ICON), qApp));
            trayIcon->setToolTip("Http File Server");
            trayIcon->setContextMenu(new QMenu);
            auto *exitAction = new QAction("Exit", trayIcon.data());
            QAction::connect(exitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
            trayIcon->contextMenu()->addAction(exitAction);
            trayIcon->show();
        }
        if(isListening)
        {
            trayIcon->showMessage("提示", "Http File Server 正在运行...", QIcon(APP_ICON), 1000);
        }
        else
        {
            trayIcon->showMessage("提示", "Http File Server 停止运行", QIcon(APP_ICON), 1000);
            QThread::msleep(500);
        }
        return trayIcon != nullptr;
    }
#endif

    HttpFileServerPrivate(const HttpFileServerPrivate &) = delete;
    HttpFileServerPrivate& operator=(const HttpFileServerPrivate &) = delete;

    ~HttpFileServerPrivate() = default;

    QScopedPointer<QHttpServer> server;
#ifdef ENABLE_GUI
    QScopedPointer<QSystemTrayIcon> trayIcon;
#endif
    static QString ROOT_DIR;
    QHostAddress hostAddress = QHostAddress::Any;
    quint16 port = 80;
    bool isListening = false;
};
QString HttpFileServerPrivate::ROOT_DIR = "";
HttpFileServer::HttpFileServer(QObject *parent)
    : QObject{parent},
    d_ptr{new HttpFileServerPrivate}
{

    setRootDir(QCoreApplication::applicationDirPath());
}

HttpFileServer::~HttpFileServer()
{
    close();
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

bool HttpFileServer::listen(const QHostAddress &address, quint16 port)
{
    qInfo() << "监听地址:" << address.toString() << "端口:" << port;
    if (isListening())
    {
        qWarning() << "服务器已经在监听中";
        return isListening();
    }
    d_ptr->hostAddress = address;
    d_ptr->port = port;
    auto tcpServer = new QTcpServer(this);
    if (d_ptr->isListening = tcpServer->listen(d_ptr->hostAddress, d_ptr->port) ; !isListening()) {

        qCritical() << "监听失败：" << tcpServer->errorString();
        tcpServer->close();
        tcpServer->deleteLater();
        return isListening();
    }

    d_ptr->server.reset(new QHttpServer(this));
    d_ptr->server->route("/", &HttpFileServerPrivate::handleRequest);
    if (d_ptr->isListening = d_ptr->server->bind(tcpServer) ; !isListening()) {
        close();
        qCritical() << "QHttpServer 绑定失败";
        return isListening();
    }

    qInfo() << "目录浏览服务器已启动, 文件目录:" << HttpFileServerPrivate::ROOT_DIR;
    qInfo() << "浏览：http://127.0.0.1:80/";
#ifdef ENABLE_GUI
    d_ptr->showTrayIcon();
#endif
    return isListening();
}

bool HttpFileServer::isListening() const
{
    return d_ptr->isListening;
}

void HttpFileServer::close()
{
    if (d_ptr->server)
    {
        auto servers = d_ptr->server->servers();
        for(auto *server : std::as_const(servers))
        {
            server->close();
            server->deleteLater();
        }
        d_ptr->isListening = false;
    }
#ifdef ENABLE_GUI
    d_ptr->showTrayIcon();
#endif
    d_ptr->server.reset();
}

void HttpFileServer::openRootIndexInBrowser()
{
    if (isListening())
    {
        QHostAddress address = d_ptr->hostAddress == QHostAddress::Any ? QHostAddress::LocalHost : d_ptr->hostAddress;
#ifdef ENABLE_GUI
        QDesktopServices::openUrl(QString("http://%1:%2").arg(address.toString(), QString::number(d_ptr->port)));
#else
        QProcess::startDetached("cmd", {"/c", "start", QString("http://%1:%2").arg(address.toString(), QString::number(d_ptr->port))});
#endif
    }
}

QHostAddress HttpFileServer::hostAddress() const
{
    return d_ptr->hostAddress;
}

quint16 HttpFileServer::port() const
{
    return d_ptr->port;
}
