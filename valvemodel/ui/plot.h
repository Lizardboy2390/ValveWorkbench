#pragma once

#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QColor>

#define PLOT_WIDTH 430
#define PLOT_HEIGHT 370

class Plot
{
public:
    Plot();

    void setAxes(double xStart, double xStop, double xMajorDivision, double yStart, double yStop, double yMajorDivision, int xLabelEvery = 0, int yLabelEvery = 0);
    QGraphicsScene *getScene();
    QGraphicsLineItem *createSegment(double x1, double y1, double x2, double y2, QPen pen);
    QGraphicsTextItem *createLabel(double x, double y, double value, const QColor &color = QColor());
    void clear();
    void add(QGraphicsItem *item);
    void remove(QGraphicsItem *item);
    
    // Public getters for coordinate transformation
    double getXScale() const { return xScale; }
    double getYScale() const { return yScale; }
    double getXStart() const { return xStart; }
    double getYStart() const { return yStart; }

private:
    QGraphicsScene *scene;
    double xScale;
    double yScale;
    double xStart;
    double yStart;
    double xStop;
    double yStop;
};
