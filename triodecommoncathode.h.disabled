#ifndef TRIODECOMMONCATHODE_H
#define TRIODECOMMONCATHODE_H

#include "valvemodel/circuit/circuit.h"
#include "valvemodel/constants.h"
#include <QVector>
#include <QPointF>

class TriodeCommonCathode : public Circuit
{
    Q_OBJECT

public:
    TriodeCommonCathode();
    virtual ~TriodeCommonCathode();

    int getDeviceType(int index) override;
    void plot(Plot *plot) override;
    void update(int index) override;
    void updateUI(QLabel *labels[], QLineEdit *values[]);  // New declaration

    void calculateOperatingPoint();
    double calculateGain();

private:
    void calculateAnodeLoadLine(double vb, double ra, double rl);
    void calculateCathodeLoadLine(double rk);
    QPointF findOperatingPoint();
    QPointF lineIntersection(QPointF p1, QPointF p2, QPointF p3, QPointF p4);
    bool isOnSegment(QPointF p, QPointF a, QPointF b);

    QVector<QPointF> anodeLoadLine;
    QVector<QPointF> cathodeLoadLine;
    QPointF operatingPoint;
};

#endif // TRIODECOMMONCATHODE_H
