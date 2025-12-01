#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QQmlContext>
#include <QQuickItem>
#include <QMessageBox>
#include <QGeoCoordinate>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // API Inicializálása (a makrót a CMake adja)
    api = new TuraAPI(PROJEKT_MAPPA, this);

    // API jeleinek bekötése
    connect(api, &TuraAPI::routeFound, this, &MainWindow::handleRouteFound);
    connect(api, &TuraAPI::errorOccurred, this, &MainWindow::handleError);
    // A geocoding választ trükkösebben kezeljük lent

    // UI ÉPÍTÉS
    this->setStyleSheet("background-color: #2b2b2b; font-family: 'Segoe UI', sans-serif;");
    QWidget *centralWidget = ui->centralwidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); mainLayout->setSpacing(0);

    // Bal oldali sáv
    QWidget *sidebar = new QWidget(this);
    sidebar->setFixedWidth(340);
    sidebar->setStyleSheet("background-color: #1e1e1e; border-right: 1px solid #333;");
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(25, 30, 25, 30); sidebarLayout->setSpacing(20);

    QLabel *appTitle = new QLabel("Túra Tervező", this);
    appTitle->setStyleSheet("font-size: 26px; font-weight: bold; color: #ffffff; border: none; margin-bottom: 10px;");
    appTitle->setAlignment(Qt::AlignCenter);
    sidebarLayout->addWidget(appTitle);

    QString inputStyle = "QLineEdit { background-color: #333; color: white; border: 1px solid #444; border-radius: 8px; padding: 12px; font-size: 14px; } QLineEdit:focus { border: 1px solid #4CAF50; }";
    startInput = new QLineEdit(this); startInput->setPlaceholderText("Honnan?"); startInput->setStyleSheet(inputStyle);
    endInput = new QLineEdit(this); endInput->setPlaceholderText("Hova?"); endInput->setStyleSheet(inputStyle);
    sidebarLayout->addWidget(startInput); sidebarLayout->addWidget(endInput);

    // Slider
    QHBoxLayout *switchLayout = new QHBoxLayout();
    QLabel *lblShort = new QLabel("Rövid", this); lblShort->setStyleSheet("color: white; font-weight: bold; border:none;");
    QLabel *lblEasy = new QLabel("Könnyű", this); lblEasy->setStyleSheet("color: white; font-weight: bold; border:none;");

    modeSlider = new ToggleSlider(this);
    modeSlider->setRange(0, 1); modeSlider->setFixedWidth(60); modeSlider->setFixedHeight(30);
    modeSlider->setStyleSheet("QSlider::groove:horizontal { background: #333; height: 24px; border-radius: 12px; border: 1px solid #444; } QSlider::handle:horizontal { background: #4CAF50; width: 22px; height: 22px; border-radius: 11px; margin: -1px 2px; }");

    switchLayout->addStretch(); switchLayout->addWidget(lblShort); switchLayout->addWidget(modeSlider); switchLayout->addWidget(lblEasy); switchLayout->addStretch();
    sidebarLayout->addLayout(switchLayout);

    planButton = new QPushButton("Túra Tervezése", this);
    planButton->setCursor(Qt::PointingHandCursor); planButton->setFixedHeight(50);
    planButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-size: 16px; font-weight: bold; border-radius: 8px; border: none; } QPushButton:hover { background-color: #45a049; }");
    connect(planButton, &QPushButton::clicked, this, &MainWindow::onPlanRouteClicked);
    sidebarLayout->addWidget(planButton);

    sidebarLayout->addStretch();
    infoLabel = new QLabel("Add meg az úticélokat!", this);
    infoLabel->setAlignment(Qt::AlignCenter); infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background-color: #252525; color: #ccc; border-radius: 8px; padding: 15px; font-size: 13px; border: 1px solid #333;");
    sidebarLayout->addWidget(infoLabel);

    mapView = new QQuickWidget(this);
    mapView->setSource(QUrl("qrc:/map.qml"));
    mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mapView->setStyleSheet("border: none;");

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(mapView, 1);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::onPlanRouteClicked()
{
    QString s = startInput->text();
    QString e = endInput->text();
    if(s.isEmpty() || e.isEmpty()) return;

    planButton->setEnabled(false); planButton->setText("Keresés...");
    startFound = false;

    // Kicsit trükkös: Mivel a TuraAPI aszinkron, itt helyben kötjük be a választ
    // 1. Megkeressük az Indulást
    disconnect(api, &TuraAPI::addressFound, nullptr, nullptr); // Előző kapcsolat törlése
    connect(api, &TuraAPI::addressFound, this, [=](QPointF coord){
        if (!startFound) {
            // Ez volt az indulás
            tempStartCoord = coord;
            startFound = true;
            // 2. Most megkeressük az Érkezést
            api->geocodeAddress(e);
        } else {
            // Ez már az érkezés, megvan mindkettő -> Tervezés!
            api->getRoute(tempStartCoord, coord, modeSlider->value());
            planButton->setEnabled(true); planButton->setText("Túra Tervezése");
        }
    });

    api->geocodeAddress(s); // Indulás keresés indítása
}

void MainWindow::handleError(QString msg) {
    QMessageBox::warning(this, "Hiba", msg);
    planButton->setEnabled(true); planButton->setText("Túra Tervezése");
}

// Ez dolgozza fel a beérkező adatokat (Számolás, Rajzolás)
void MainWindow::handleRouteFound(QJsonObject routeData)
{
    QJsonArray features = routeData["features"].toArray();
    if(features.isEmpty()) return;

    QJsonObject feature = features[0].toObject();
    QJsonArray coords = feature["geometry"].toObject()["coordinates"].toArray();

    // --- Takarítás ---
    QMetaObject::invokeMethod(mapView->rootObject(), "clearRoute");

    QList<QPointF> routePoints;
    double calculatedAscent = 0;
    double distDown = 0, distFlat = 0, distMed = 0, distSteep = 0, totalDistCalc = 0;

    // --- Feldolgozás (Számítás) ---
    for(int i = 0; i < coords.size() - 1; ++i) {
        QJsonArray p1 = coords[i].toArray();
        double lon1 = p1[0].toDouble(); double lat1 = p1[1].toDouble(); double alt1 = (p1.size() > 2) ? p1[2].toDouble() : 0;

        QJsonArray p2 = coords[i+1].toArray();
        double lon2 = p2[0].toDouble(); double lat2 = p2[1].toDouble(); double alt2 = (p2.size() > 2) ? p2[2].toDouble() : 0;

        QGeoCoordinate geo1(lat1, lon1); QGeoCoordinate geo2(lat2, lon2);
        double distance = geo1.distanceTo(geo2);
        double elevationDiff = alt2 - alt1;
        QString colorStr = "#0000FF";

        if (distance > 0) {
            double gradient = (elevationDiff / distance) * 100.0;
            totalDistCalc += distance;
            if (gradient < -2.0) { colorStr = "#2196F3"; distDown += distance; }
            else if (gradient < 4.0) { colorStr = "#4CAF50"; distFlat += distance; }
            else if (gradient < 12.0) { colorStr = "#FFC107"; distMed += distance; }
            else { colorStr = "#F44336"; distSteep += distance; }
            if (elevationDiff > 0) calculatedAscent += elevationDiff;
        }
        // Rajzolás
        QMetaObject::invokeMethod(mapView->rootObject(), "addColoredSegment",
                                  Q_ARG(QVariant, lat1), Q_ARG(QVariant, lon1),
                                  Q_ARG(QVariant, lat2), Q_ARG(QVariant, lon2),
                                  Q_ARG(QVariant, colorStr));
        routePoints.append(QPointF(lat1, lon1));
    }
    if (!coords.isEmpty()) {
        QJsonArray lastP = coords.last().toArray();
        routePoints.append(QPointF(lastP[1].toDouble(), lastP[0].toDouble()));
    }

    // --- Statisztika Frissítése ---
    if (totalDistCalc > 0) {
        QMetaObject::invokeMethod(mapView->rootObject(), "updateStats",
                                  Q_ARG(QVariant, (distDown/totalDistCalc)*100),
                                  Q_ARG(QVariant, (distFlat/totalDistCalc)*100),
                                  Q_ARG(QVariant, (distMed/totalDistCalc)*100),
                                  Q_ARG(QVariant, (distSteep/totalDistCalc)*100));
    }

    QJsonObject summary = feature["properties"].toObject()["summary"].toObject();
    double ascent = summary["ascent"].toDouble();
    if (ascent < 1.0) ascent = calculatedAscent;

    infoLabel->setText(QString("Táv: %1 km | Idő: %2 óra %3 perc\nÖssz. Emelkedés: %4 m")
                           .arg(QString::number(summary["distance"].toDouble()/1000.0, 'f', 1))
                           .arg((int)(summary["duration"].toDouble()/3600))
                           .arg(((int)summary["duration"].toDouble()%3600)/60)
                           .arg((int)ascent));

    // --- Zoom ---
    if (!routePoints.isEmpty()) {
        double minLat=90, maxLat=-90, minLon=180, maxLon=-180;
        for (const QPointF &p : routePoints) {
            if (p.x()<minLat) minLat=p.x(); if (p.x()>maxLat) maxLat=p.x();
            if (p.y()<minLon) minLon=p.y(); if (p.y()>maxLon) maxLon=p.y();
        }
        QMetaObject::invokeMethod(mapView->rootObject(), "zoomToRect",
                                  Q_ARG(QVariant, minLat), Q_ARG(QVariant, minLon),
                                  Q_ARG(QVariant, maxLat), Q_ARG(QVariant, maxLon));

        QPointF s = routePoints.first(); QPointF e = routePoints.last();
        QMetaObject::invokeMethod(mapView->rootObject(), "setMarkers",
                                  Q_ARG(QVariant, s.x()), Q_ARG(QVariant, s.y()),
                                  Q_ARG(QVariant, e.x()), Q_ARG(QVariant, e.y()));
    }
}
