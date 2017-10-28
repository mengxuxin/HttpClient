#include <QCoreApplication>
#include "httpclient.h"

#include <functional>
#include <QDebug>
#include <QNetworkAccessManager>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    {
        // 在代码块里执行网络访问，是为了测试 HttpClient 对象在被析构后，网络访问的回调函数仍然能正常执行
        // [[1]] GET 请求无参数
        HttpClient("http://localhost:8080/device").get([](const QString &response) {
            qDebug() << response << '\n';
        });

        // [[2]] GET 请求有参数，有自定义 header
        HttpClient("http://localhost:8080/signIn")
                .addParam("id", "1")
                .addParam("name", QStringLiteral("zhugeliang"))
                .addHeader("token", "123AS#D")
                .get([](const QString &response) {
            qDebug() << response << '\n';
        });

        // [[3]] POST 请求有参数，有自定义 header
        HttpClient("http://localhost:8080/signIn")
                .addParam("id", "2")
                .addParam("name", "卧龙")
                .addHeader("token", "DER#2J7")
                .addHeader("content-type", "application/x-www-form-urlencoded")
                .post([](const QString &response) {
            qDebug() << response << '\n';
        });

        // [[4]] 每创建一个 QNetworkAccessManager 对象都会创建一个线程，当频繁的访问网络时，为了节省线程资源，调用 useManager()
        // 使用共享的 QNetworkAccessManager，它不会被 HttpClient 删除。
        // 如果下面的代码不传入 QNetworkAccessManager，从任务管理器里可以看到创建了几千个线程。
        QNetworkAccessManager *manager = new QNetworkAccessManager();
        for (int i = 0; i < 10; ++i) {
            HttpClient("http://localhost:8080/device").useManager(manager).get([=](const QString &response) {
                qDebug() << response << ", " << i  << '\n';
            });
        }
    }

    return app.exec();
}
