#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QQuickWidget>
#include <QLabel>
#include "toggleslider.h" // <--- Saját widget
#include "turapi.h"       // <--- API kezelő

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPlanRouteClicked();
    void handleRouteFound(QJsonObject routeData);
    void handleError(QString msg);

private:
    void drawRoute(const QList<QPointF> &points); // Ez maradt, mert UI rajzolás

    Ui::MainWindow *ui;
    QLineEdit *startInput;
    QLineEdit *endInput;
    ToggleSlider *modeSlider;
    QPushButton *planButton;
    QLabel *infoLabel;
    QQuickWidget *mapView;

    TuraAPI *api; // <--- Hálózatkezelő objektum

    // Átmeneti tárolók a kétlépcsős kereséshez
    QPointF tempStartCoord;
    bool startFound = false;
};
#endif // MAINWINDOW_H
