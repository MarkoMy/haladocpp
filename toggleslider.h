#ifndef TOGGLESLIDER_H
#define TOGGLESLIDER_H

#include <QSlider>
#include <QMouseEvent>
#include <QCursor>

class ToggleSlider : public QSlider {
public:
    ToggleSlider(QWidget *parent = nullptr) : QSlider(Qt::Horizontal, parent) {
        setCursor(Qt::PointingHandCursor);
    }
protected:
    void mousePressEvent(QMouseEvent *ev) override {
        // Kattintásra váltunk
        setValue(value() == 0 ? 1 : 0);
        ev->accept();
    }
};

#endif // TOGGLESLIDER_H
