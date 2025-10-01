//
// Created by Kalász Bálint on 2025. 09. 20..
//

#include "MainWindow.hpp"
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);

    // Betöltünk egy térképet
    QPixmap map("C:/map.png");
    auto *mapItem = new QGraphicsPixmapItem(map);
    scene->addItem(mapItem);

    // === Bal felső sarok UI ===
    controlPanel = new QWidget(this);
    controlPanel->setStyleSheet(R"(
    QLineEdit {
        color: black;
        background-color: white;
        border: 1px solid gray;
        border-radius: 4px;
        padding: 4px;
    }

    QPushButton {
        color: white;
        background-color: #0078D7;  /* szép kék */
        border: none;
        border-radius: 6px;
        padding: 6px;
    }

    QPushButton:hover {
        background-color: #005a9e;
    }
)");

    fromEdit = new QLineEdit();
    fromEdit->setPlaceholderText("Honnan");

    toEdit = new QLineEdit();
    toEdit->setPlaceholderText("Hova");

    planButton = new QPushButton("Tervezés");

    auto *vbox = new QVBoxLayout();
    vbox->addWidget(fromEdit);
    vbox->addWidget(toEdit);
    vbox->addWidget(planButton);
    vbox->setContentsMargins(8, 8, 8, 8);
    controlPanel->setLayout(vbox);
    controlPanel->setFixedWidth(220);

    // === Layout a fő ablakban ===
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QHBoxLayout();
    central->setLayout(mainLayout);

    // A view megy hátra, a panel előre
    QVBoxLayout *overlayLayout = new QVBoxLayout();
    overlayLayout->addWidget(controlPanel, 0, Qt::AlignTop | Qt::AlignLeft);
    overlayLayout->addStretch();

    QHBoxLayout *containerLayout = new QHBoxLayout();
    containerLayout->addWidget(view);
    containerLayout->addLayout(overlayLayout);

    mainLayout->addLayout(containerLayout);

    // Debug a gombra
    connect(planButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Útvonal tervezés:"
                 << fromEdit->text()
                 << "->"
                 << toEdit->text();
    });

    // Ablak beállítások
    resize(800, 600);
    setWindowTitle("Map Test - Qt");
}