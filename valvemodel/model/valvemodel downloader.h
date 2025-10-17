#ifndef VALVEMODELDOWNLOADER_H
#define VALVEMODELDOWNLOADER_H

#include <QObject>
#include <QString>
#include <QList>

class ValveModelDownloader : public QObject
{
    Q_OBJECT

public:
    explicit ValveModelDownloader(QObject *parent = nullptr);

    void searchValveModels(const QString &searchTerm);
    QList<QString> getAvailableModels() const;

signals:
    void searchCompleted(const QList<QString> &models);
    void downloadCompleted(const QString &modelData);

private slots:
    void onNetworkReply();

private:
    QList<QString> availableModels;
};

#endif // VALVEMODELDOWNLOADER_H
