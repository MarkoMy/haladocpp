//
// Created by Kalász Bálint on 2025. 09. 20..
//

#ifndef TERKEP_MAINWINDOW_HPP
#define TERKEP_MAINWINDOW_HPP

#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class MainWindow: public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    QGraphicsView* view;
    QGraphicsScene* scene;

    QWidget* controlPanel;      // bal felső sarokba kerül
    QLineEdit* fromEdit;
    QLineEdit* toEdit;
    QPushButton* planButton;
};


#endif //TERKEP_MAINWINDOW_HPP