#include "turapi.h"
#include <QUrlQuery>

TuraAPI::TuraAPI(QString projectPath, QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);

    // API Kulcs beolvasása itt történik
    QString keyPath = projectPath + "/apikey.txt";
    QFile file(keyPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        apiKey = in.readAll().trimmed();
        file.close();
    } else {
        emit errorOccurred("Nem találom az apikey.txt fájlt itt:\n" + keyPath);
    }
}

void TuraAPI::geocodeAddress(const QString &address)
{
    QString urlStr = QString("https://api.openrouteservice.org/geocode/search?api_key=%1&text=%2&layers=locality,venue,address&boundary.country=HU")
    .arg(apiKey).arg(address);

    QNetworkRequest request((QUrl(urlStr)));
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=](){
        if(reply->error() == QNetworkReply::NoError){
            QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
            QJsonArray features = json.object()["features"].toArray();
            if(!features.isEmpty()){
                QJsonArray coords = features[0].toObject()["geometry"].toObject()["coordinates"].toArray();
                emit addressFound(QPointF(coords[1].toDouble(), coords[0].toDouble())); // Lat, Lon
            } else {
                emit errorOccurred("Nem találom ezt a helyet: " + address);
            }
        } else {
            emit errorOccurred("Hálózati hiba (Geocoding)");
        }
        reply->deleteLater();
    });
}

void TuraAPI::getRoute(QPointF start, QPointF end, int modeIndex)
{
    QString profile = "foot-hiking";
    QString preference = "shortest";

    if (modeIndex == 1) {
        profile = "foot-walking";
        preference = "recommended";
    }

    QJsonObject jsonBody;
    QJsonArray coordsArray;
    coordsArray.append(QJsonArray({start.y(), start.x()})); // Lon, Lat
    coordsArray.append(QJsonArray({end.y(), end.x()}));

    jsonBody["coordinates"] = coordsArray;
    jsonBody["preference"] = preference;
    jsonBody["elevation"] = true;
    jsonBody["instructions"] = false;
    jsonBody["units"] = "m";

    QJsonDocument doc(jsonBody);
    QByteArray data = doc.toJson();

    QUrl url("https://api.openrouteservice.org/v2/directions/" + profile + "/geojson");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey.toUtf8());

    QNetworkReply *reply = manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [=](){
        if(reply->error() == QNetworkReply::NoError){
            QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
            // Itt csak továbbküldjük a nyers JSON-t a MainWindow-nak feldolgozásra
            emit routeFound(json.object());
        } else {
            emit errorOccurred("Hiba az útvonal letöltésekor: " + reply->errorString());
        }
        reply->deleteLater();
    });
}
