#ifndef HTTPFILESERVER_H
#define HTTPFILESERVER_H

#include <QObject>
#include <QScopedPointer>
class HttpFileServerPrivate;
class HttpFileServer final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootDir READ rootDir WRITE setRootDir NOTIFY rootDirChanged)
public:
    explicit HttpFileServer(QObject *parent = nullptr);
    ~HttpFileServer();

    /**
     * @brief rootDir 文件服务器根目录
     * @return QString 根目录
     */
    QString rootDir() const;
    /**
     * @brief setRootDir 设置文件服务器根目录
     * @param newRootDir 新的根目录
     */
    void setRootDir(const QString &newRootDir);
Q_SIGNALS:
    /**
     * @brief rootDirChanged 根目录改变信号
     * @param newRootDir 新的根目录
     */
    void rootDirChanged(const QString &newRootDir);

private:

    QScopedPointer<HttpFileServerPrivate> d_ptr;
};

#endif // HTTPFILESERVER_H
