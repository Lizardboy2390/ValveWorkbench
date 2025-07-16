#ifndef QUADRATICSOLVER_H
#define QUADRATICSOLVER_H

#include <QJsonObject>
#include <QString>
#include <QFile>

// Ceres and glog dependencies removed
// Direct mathematical calculations are now used instead
#include <vector>
#include <cmath>

// Solves the data set for y = ax^2 + bx + c

class QuadraticSolver
{
public:
    QuadraticSolver(double a = 1.0, double b = 1.0, double c = 1.0);

    void addSample(double x, double y);
    void solve();
    double getA() const;
    double getB() const;
    double getC() const;

    bool getFixedA() const;
    void setFixedA(bool newFixedA);

    bool getRequirePositive() const;
    void setRequirePositive(bool newRequirePositive);

    bool isConverged() const;

protected:
    QList<double> xs;
    QList<double> ys;

    bool fixedA = false;
    bool requirePositive = false;

    bool converged = false;

    double a;
    double b;
    double c;
};

#endif // QUADRATICSOLVER_H
