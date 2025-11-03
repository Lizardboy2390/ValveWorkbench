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
    // Liang-Barsky clipping in data space against [xStart,xStop] x [yStart,yStop]
    double dx = x2 - x1;
    double dy = y2 - y1;
    double p[4] = { -dx, dx, -dy, dy };
    double q[4] = { x1 - xStart, xStop - x1, y1 - yStart, yStop - y1 };
    double u1 = 0.0;
    double u2 = 1.0;

    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0.0) {
            if (q[i] < 0.0) {
                // Line parallel and outside boundary
                return nullptr;
            }
        } else {
            double r = q[i] / p[i];
            if (p[i] < 0.0) {
                if (r > u2) return nullptr;
                if (r > u1) u1 = r;
            } else {
                if (r < u1) return nullptr;
                if (r < u2) u2 = r;
            }
        }
    }

    double cx1 = x1 + u1 * dx;
    double cy1 = y1 + u1 * dy;
    double cx2 = x1 + u2 * dx;
    double cy2 = y1 + u2 * dy;

    // Transform to scene coordinates
    double sx1 = (cx1 - xStart) * xScale;
    double sy1 = PLOT_HEIGHT - (cy1 - yStart) * yScale;
    double sx2 = (cx2 - xStart) * xScale;
    double sy2 = PLOT_HEIGHT - (cy2 - yStart) * yScale;

    return scene->addLine(sx1, sy1, sx2, sy2, pen);
}

QGraphicsTextItem *Plot::createLabel(double x, double y, double value, const QColor &color)
{
    QGraphicsTextItem *text;
    char labelText[16];
    sprintf(labelText, "%.1fv", value + 0.04);
    text = scene->addText(labelText);
    text->setPos((x - xStart) * xScale + 5, PLOT_HEIGHT - (y - yStart) * yScale - 10);
    if (color.isValid()) {
        text->setDefaultTextColor(color);
    }

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
