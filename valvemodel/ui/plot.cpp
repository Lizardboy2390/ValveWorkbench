#include "plot.h"

Plot::Plot()
{
    scene = new QGraphicsScene();
}

void Plot::setAxes(double _xStart, double _xStop, double xMajorDivision, double _yStart, double _yStop, double yMajorDivision, int xLabelEvery, int yLabelEvery)
{
    xStart = _xStart;
    xStop = _xStop;
    yStart = _yStart;
    yStop = _yStop;

    xScale = PLOT_WIDTH / (xStop - xStart);
    yScale = PLOT_HEIGHT / (yStop - yStart);

    // Debug output for axis setup
    // qDebug("Plot::setAxes: xStart=%f, xStop=%f, yStart=%f, yStop=%f", xStart, xStop, yStart, yStop);
    // qDebug("Plot::setAxes: xScale=%f, yScale=%f", xScale, yScale);

    double rounding = 0.5;

    scene->clear();

    if (xScale < 0) {
        if (xMajorDivision > 0) {
            xMajorDivision = -xMajorDivision;
        }
    } else {
        if (xMajorDivision < 0) {
            xMajorDivision = -xMajorDivision;
        }
    }
    double x = xStart;
    int i = 0;
    while (x <= xStop) {
        scene->addLine((x - xStart) * xScale, 0, (x - xStart) * xScale, PLOT_HEIGHT);
        rounding = (x > 0) ? 0.5 : -0.5;
        if (xLabelEvery == 0 || (i % xLabelEvery) == 0) {
            QGraphicsTextItem *text;
            char labelText[16];
            if (xMajorDivision < 1.0) {
                sprintf(labelText, "%.1f", x);
            } else {
                sprintf(labelText, "%d", (int) (x + rounding));
            }
            text = scene->addText(labelText);
            double offset = 6.0 * strlen(labelText);
            text->setPos((x - xStart) * xScale - offset, PLOT_HEIGHT + 10);
        }

        x += xMajorDivision;
        i++;
    }

    if (yScale < 0) {
        if (yMajorDivision > 0) {
            yMajorDivision = -yMajorDivision;
        }
    } else {
        if (yMajorDivision < 0) {
            yMajorDivision = -yMajorDivision;
        }
    }
    double y = yStart;
    i = 0;
    while (y <= yStop) {
        scene->addLine(0, PLOT_HEIGHT - (y - yStart) * yScale, PLOT_WIDTH, PLOT_HEIGHT - (y - yStart) * yScale);
        rounding = (y > 0) ? 0.5 : -0.5;
        if (yLabelEvery == 0 || (i % yLabelEvery) == 0) {
            QGraphicsTextItem *text;
            char labelText[16];
            if (yMajorDivision < 1.0) {
                sprintf(labelText, "%.1f", y);
            } else {
                sprintf(labelText, "%d", (int) (y + rounding));
            }
            text = scene->addText(labelText);
            double offset = 12.0 * strlen(labelText);
            text->setPos(-10 - offset, PLOT_HEIGHT - (y - yStart) * yScale - 10);
        }

        y += yMajorDivision;
        i++;
    }
}

QGraphicsScene *Plot::getScene()
{
    return scene;
}

QGraphicsLineItem *Plot::createSegment(double x1, double y1, double x2, double y2, QPen pen)
{
    double x1_ = (x1 - xStart) * xScale;
    double y1_ = PLOT_HEIGHT - (y1 - yStart) * yScale;
    double x2_ = (x2 - xStart) * xScale;
    double y2_ = PLOT_HEIGHT - (y2 - yStart) * yScale;

    // Debug output for coordinate transformation issues
    // qDebug("Plot::createSegment: x1=%f->%f, y1=%f->%f, x2=%f->%f, y2=%f->%f",
    //        x1, x1_, y1, y1_, x2, x2_, y2, y2_);

    if (x1_ < 0.0 || x1_ > PLOT_WIDTH || x2_ < 0.0 || x2_ > PLOT_WIDTH || y1_ < 0.0 || y1_ > PLOT_HEIGHT || y2_ < 0.0 || y2_ > PLOT_HEIGHT) {
        qWarning("Segment out of bounds - skipping: x1=%f,y1=%f to x2=%f,y2=%f", x1, y1, x2, y2);
        return nullptr; // Return null instead of invisible segment
    }

    return scene->addLine(x1_, y1_, x2_, y2_, pen);
}

QGraphicsTextItem *Plot::createLabel(double x, double y, double value)
{
    QGraphicsTextItem *text;
    char labelText[16];
    sprintf(labelText, "%.1fv", value + 0.04);
    text = scene->addText(labelText);
    text->setPos((x - xStart) * xScale + 5, PLOT_HEIGHT - (y - yStart) * yScale - 10);

    return text;
}

void Plot::clear()
{
    scene->clear();
}

void Plot::add(QGraphicsItem *item)
{
    if (item != nullptr) {
        scene->addItem(item);
    }
}

void Plot::remove(QGraphicsItem *item)
{
    if (item != nullptr) {
        scene->removeItem(item);
    }
}
