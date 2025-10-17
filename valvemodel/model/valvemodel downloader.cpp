#include "valvemodel downloader.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ValveModelDownloader::ValveModelDownloader(QObject *parent) : QObject(parent)
{
}

void ValveModelDownloader::searchValveModels(const QString &searchTerm)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QUrl url("https://api.example.com/valve-models?search=" + searchTerm); // Replace with actual API

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, &ValveModelDownloader::onNetworkReply);
}

QList<QString> ValveModelDownloader::getAvailableModels() const
{
    return availableModels;
}

void ValveModelDownloader::onNetworkReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        QString data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonArray array = doc.array();

        availableModels.clear();
        for (const QJsonValue &value : array) {
            QJsonObject obj = value.toObject();
            QString model = obj["name"].toString();
            availableModels.append(model);
        }

        emit searchCompleted(availableModels);
    } else {
        // Handle error
    }

    reply->deleteLater();
}
