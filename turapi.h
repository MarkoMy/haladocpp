#ifndef TURAPI_H
#define TURAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QPointF>

class TuraAPI : public QObject
{
    Q_OBJECT
public:
    explicit TuraAPI(QString projectPath, QObject *parent = nullptr);

    // Ezeket hívja majd a MainWindow
    void geocodeAddress(const QString &address);
    void getRoute(QPointF start, QPointF end, int modeIndex);

signals:
    // Ezekkel jelez vissza a MainWindow-nak
    void addressFound(QPointF coord);
    void routeFound(QJsonObject routeData);
    void errorOccurred(QString message);

private:
    QNetworkAccessManager *manager;
    QString apiKey;
};

#endif // TURAPI_H
