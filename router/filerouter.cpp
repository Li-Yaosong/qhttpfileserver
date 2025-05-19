#include "filerouter.h"
#include "util.h"
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#include <QUrl>
#include <QDir>
#include <QJsonDocument>

FileRouter::FileRouter(QObject *parent)
    : Router{parent}
{}

FileRouter::~FileRouter()
{
    qDebug() << "FileRouter析构";
}

QString FileRouter::pathPattern() const
{
    return QString("/");
}

Router::RequestHandler FileRouter::requestHandler()
{
    return [](const QUrl path, const QHttpServerRequest &request, QHttpServerResponder &responder) {
        QString pathStr = path.toString();
        if(pathStr.startsWith("*static"))
        {
            pathStr.replace("*static", ":/html/static");
        }
        QDir dir(Util::rootDir());
        QFileInfo fileInfo(dir, pathStr);
        if (fileInfo.exists())
        {
            if(fileInfo.isFile())
            {
                return Util::respondFile(responder, fileInfo.absoluteFilePath());
            }
            else if(fileInfo.isDir() && fileInfo.isNativePath())
            {
                QString html = Util::readTemplateFile(":/html/index-template.html");

                if(dir.cd(pathStr))
                {
                    if (!pathStr.isEmpty()) {
                        auto subPathList = pathStr.split("/", Qt::SkipEmptyParts);
                        pathStr = subPathList.join("/") + "/";
                        subPathList.removeLast();
                        QString subPath = subPathList.join("/");
                        QString parentDirHtml = Util::itemTemplate("icon-arrow-up", "Parent Directory", subPath);
                        html.replace("${Parent Directory}$", parentDirHtml);
                    }
                    else
                    {
                        html.replace("${Parent Directory}$", "");
                    }

                    const auto infoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
                    html.replace("${file-list}$",
                                 std::accumulate(infoList.begin(), infoList.end(), QString(), Util::HtmlItemAccumulator(pathStr)));
                    responder.write(html.toUtf8(), "text/html");
                }
                else
                {
                    qWarning() << "无法访问目录" << pathStr;
                    responder.write(Util::errorJson("Directory access denied"),
                                    QHttpServerResponse::StatusCode::Forbidden);
                }
            }
        }
        responder.write(Util::errorJson("Directory or file not found"), QHttpServerResponse::StatusCode::NotFound);
    };
}
