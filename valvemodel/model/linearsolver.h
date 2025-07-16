#ifndef LINEARSOLVER_H
#define LINEARSOLVER_H

#include <QJsonObject>
#include <QString>
#include <QFile>

// Ceres and glog dependencies removed
// Direct mathematical calculations are now used instead
#include <vector>
#include <cmath>

// Solves the data set for y = ax + b

class LinearSolver
{
public:
    LinearSolver(double a = 1.0, double b = 1.0);

    void addSample(double x, double y);
    void solve();
    double getA() const;
    double getB() const;

    bool isConverged() const;

protected:
    QList<double> xs;
    QList<double> ys;

    double a;
    double b;

    bool converged = false;
};

#endif // LINEARSOLVER_H
