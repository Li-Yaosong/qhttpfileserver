#ifndef HTTPFILESERVER_H
#define HTTPFILESERVER_H

#include <QObject>
#include <QScopedPointer>
#include <QHostAddress>
class Router;
class HttpFileServerPrivate;
class HttpFileServer final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootDir READ rootDir WRITE setRootDir NOTIFY rootDirChanged)
    Q_PROPERTY(QHostAddress hostAddress READ hostAddress CONSTANT)
    Q_PROPERTY(quint16 port READ port CONSTANT)
    Q_DISABLE_COPY(HttpFileServer)
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
    /**
     * @brief listen 监听指定的主机地址和端口号
     * @param address 主机地址
     * @param port 端口号
     * @return bool 是否成功监听
     */
    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 80);
    /**
     * @brief isListening 是否正在监听
     * @return bool 是否正在监听
     */
    bool isListening() const;
    /**
     * @brief close 关闭服务器
     */
    void close();
    /**
     * @brief openRootIndexInBrowser 在浏览器打开根目录
     */
    void openRootIndexInBrowser();

    /**
     * @brief hostAddress 服务器监听的主机地址
     * @return QHostAddress 主机地址
     */
    QHostAddress hostAddress() const;
    /**
     * @brief port 服务器监听的端口号
     * @return quint16 端口号
     */
    quint16 port() const;
    /**
     * @brief addRouter 添加路由器
     * @param router 路由器智能指针
     */
    void addRouter(const QSharedPointer<Router>& router);

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
