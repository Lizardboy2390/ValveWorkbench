#ifndef QUADRATICSOLVER_H
#define QUADRATICSOLVER_H

#include <QJsonObject>
#include <QString>

#include "ceres/ceres.h"
#include "glog/logging.h"

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solve;
using ceres::Solver;

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

protected:
    double a;
    double b;
    double c;
    /**
     * @brief problem The Ceres Problem used for model fitting
     */
    Problem problem;
    /**
     * @brief options The options to be used by Ceres for solving the model approximation
     */
    Solver::Options options;
};

#endif // QUADRATICSOLVER_H
