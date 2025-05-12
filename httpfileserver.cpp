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
#include <QFileDialog>
#else
#include <QProcess>
#endif

class HttpFileServerPrivate : public QObject
{
public:
    explicit HttpFileServerPrivate(HttpFileServer *parent)
        :q_ptr{parent}
    {

    }
    static void respondFile(QHttpServerResponder &responder, const QString &filePath)
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
    static QString readTemplateFile(const QString &filePath)
    {
        if (QFile file(filePath); !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "无法打开模板文件";
            return QString();
        }
        else
        {
            QString html = file.readAll();
            file.close();
            return html;
        }
    }
    static QString itemTemplate(const QString &icon,const QString &displayName,
                                        const QString &href,
                                        const QString &fileSize = "",
                                        const QString &lastModified = "")
    {
        QString html = readTemplateFile(":/html/file-list-item-template.html");
        html.replace("${item-icon}$", icon);
        html.replace("${display-name}$", displayName);
        html.replace("${href}$", href);
        html.replace("${file-size}$", fileSize);
        html.replace("${last-modified}$", lastModified);
        return html;
    }
    static void handleRequest(const QUrl path, const QHttpServerRequest &request, QHttpServerResponder &responder)
    {
        QString pathStr = path.toString();
        if(pathStr.startsWith("*static"))
        {
            return respondFile(responder, pathStr.replace("*static", ":/html/static"));
        }

        QDir dir(ROOT_DIR);
        QFileInfo fileInfo(dir, pathStr);
        if (!fileInfo.exists())
            return responder.write("{\"message\": \"Directory or file not found\"}", "application/json",
                                   QHttpServerResponse::StatusCode::NotFound);
        if(fileInfo.isFile())
        {
            return respondFile(responder, fileInfo.absoluteFilePath());
        }
        else if(fileInfo.isDir())
        {
            QString html = readTemplateFile(":/html/index-template.html");

            if(dir.cd(pathStr))
            {
                if (!pathStr.isEmpty()) {
                    auto subPathList = pathStr.split("/", Qt::SkipEmptyParts);
                    pathStr = subPathList.join("/") + "/";
                    subPathList.removeLast();
                    QString subPath = subPathList.join("/");
                    QString parentDirHtml = itemTemplate("icon-arrow-up", "Parent Directory", subPath);
                    html.replace("${Parent Directory}$", parentDirHtml);
                }
                else
                {
                    html.replace("${Parent Directory}$", "");
                }

                const auto infoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
                html.replace("${file-list}$", std::accumulate(infoList.begin(), infoList.end(), QString(),
                        [&pathStr](const QString &list, const QFileInfo &info) {
                            QString fileName = info.isDir() ? info.fileName() + "/" : info.fileName();
                            return list + itemTemplate(info.isDir() ? "icon-folder-close" : "icon-file",
                                                        fileName,
                                                        pathStr + fileName,
                                                        info.isDir() ? "" : QString::number(info.size()),
                                                        info.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
                        }));
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
    void activateTrayIcon(QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::Trigger) {
            QString directory = QFileDialog::getExistingDirectory(nullptr, "选择文件目录", ROOT_DIR);
            if (!directory.isEmpty()) {
                q_ptr->setRootDir(directory);
                qDebug() << "选择的目录:" << directory;
            }
        }
    }
#ifdef ENABLE_GUI
    bool showTrayIcon()
    {
        if (QSystemTrayIcon::isSystemTrayAvailable() && !trayIcon) {
            trayIcon.reset(new QSystemTrayIcon(QIcon(APP_ICON), qApp));
            QSystemTrayIcon::connect(trayIcon.data(), &QSystemTrayIcon::activated, this ,&HttpFileServerPrivate::activateTrayIcon);
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
private:
    HttpFileServer *q_ptr = nullptr;
};
QString HttpFileServerPrivate::ROOT_DIR = "";
HttpFileServer::HttpFileServer(QObject *parent)
    : QObject{parent},
    d_ptr{new HttpFileServerPrivate(this)}
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
