#include "httpclient.h"
#include <QDebug>
#include <QHash>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

struct HttpClientPrivate {
    HttpClientPrivate(const QString &url) : url(url), networkAccessManager(NULL), useInternalNetworkAccessManager(true) {}

    QString url; // 请求的 URL
    QUrlQuery params; // 请求的参数
    QHash<QString, QString> headers; // 请求的头
    QNetworkAccessManager *networkAccessManager;
    bool useInternalNetworkAccessManager; // 是否使用内部的 QNetworkAccessManager
};

HttpClient::HttpClient(const QString &url) : d(new HttpClientPrivate(url)) {
    //    qDebug() << "HttpClient";
}

HttpClient::~HttpClient() {
    //    qDebug() << "~HttpClient";
    delete d;
}

HttpClient &HttpClient::useManager(QNetworkAccessManager *manager) {
    d->networkAccessManager = manager;
    d->useInternalNetworkAccessManager = false;
    return *this;
}

// 增加参数
HttpClient &HttpClient::addParam(const QString &name, const QString &value) {
    d->params.addQueryItem(name, value);
    return *this;
}

// 增加访问头
HttpClient &HttpClient::addHeader(const QString &header, const QString &value) {
    d->headers[header] = value;
    return *this;
}

// 执行 GET 请求
void HttpClient::get(std::function<void (const QString &)> successHandler,
                     std::function<void (const QString &)> errorHandler,
                     const char *encoding) {
    execute(false, successHandler, errorHandler, encoding);
}

// 执行 POST 请求
void HttpClient::post(std::function<void (const QString &)> successHandler,
                      std::function<void (const QString &)> errorHandler,
                      const char *encoding) {
    execute(true, successHandler, errorHandler, encoding);
}

// 执行请求的辅助函数
void HttpClient::execute(bool posted,
                         std::function<void (const QString &)> successHandler,
                         std::function<void (const QString &)> errorHandler,
                         const char *encoding) {
    // 如果是 GET 请求，并且参数不为空，则编码请求的参数，放到 URL 后面
    if (!posted && !d->params.isEmpty()) {
        d->url += "?" + d->params.toString(QUrl::FullyEncoded);
    }

    QUrl urlx(d->url);
    QNetworkRequest request(urlx);

    // 把请求的头添加到 request 中
    QHashIterator<QString, QString> iter(d->headers);
    while (iter.hasNext()) {
        iter.next();
        request.setRawHeader(iter.key().toUtf8(), iter.value().toUtf8());
    }

    // 注意: 不能在 Lambda 表达式里使用 HttpClient 对象的成员数据，因其可能在网络访问未结束时就已经被析构掉了，
    // 所以如果要使用它的相关数据，定义一个局部变量来保存其数据，然后在 Lambda 表达式里访问这个局部变量

    // 如果不使用外部的 manager 则创建一个新的，在访问完成后会自动删除掉
    bool internal = d->useInternalNetworkAccessManager;
    QNetworkAccessManager *manager = internal ? new QNetworkAccessManager() : d->networkAccessManager;
    QNetworkReply *reply = posted ? manager->post(request, d->params.toString(QUrl::FullyEncoded).toUtf8()) : manager->get(request);

    // 请求结束时一次性读取所有响应数据
    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NoError && NULL != successHandler) {
            // 读取响应数据
            QTextStream in(reply);
            QString result;
            in.setCodec(encoding);

            while (!in.atEnd()) {
                result += in.readLine();
            }

            successHandler(result);
        }

        // 释放资源
        reply->deleteLater();
        if (internal) {
            manager->deleteLater();
        }
    });

    // 请求错误处理
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [=] {
        if (NULL != errorHandler) {
            errorHandler(reply->errorString());
        }
    });
}
