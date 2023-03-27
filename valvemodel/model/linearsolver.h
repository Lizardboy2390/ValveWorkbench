#ifndef LINEARSOLVER_H
#define LINEARSOLVER_H

#include <QJsonObject>
#include <QString>
#include <QFile>

#include "ceres/ceres.h"
#include "glog/logging.h"

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solve;
using ceres::Solver;

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
    /**
     * @brief problem The Ceres Problem used for model fitting
     */
    Problem problem;
    /**
     * @brief options The options to be used by Ceres for solving the model approximation
     */
    Solver::Options options;
};

#endif // LINEARSOLVER_H
