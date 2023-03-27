#include "quadraticsolver.h"

struct QuadraticResidual {
    QuadraticResidual(double x, double y) : y_(y), x_(x) {}

    template <typename T>
    bool operator()(const T* const a, const T* const b, const T* const c, T* residual) const {
        residual[0] = y_ - (a[0] * x_ * x_ + b[0] * x_ + c[0]);
        return true;
    }

private:
    const double y_;
    const double x_;
};

QuadraticSolver::QuadraticSolver(double a_, double b_, double c_) : a(a_), b(b_), c(c_)
{

}

void QuadraticSolver::addSample(double x, double y)
{
    xs.append(x);
    ys.append(y);

    problem.AddResidualBlock(
        new AutoDiffCostFunction<QuadraticResidual, 1, 1, 1, 1>(
            new QuadraticResidual(x, y)),
        NULL, &a, &b, &c);
}

void QuadraticSolver::solve()
{
    if (requirePositive) {
        problem.SetParameterLowerBound(&b, 0, 0.0);
        problem.SetParameterLowerBound(&c, 0, 0.0);
    }

    if (fixedA) {
        problem.SetParameterBlockConstant(&a);
    }

    Solver::Summary summary;
    Solve(options, &problem, &summary);

    converged = summary.termination_type == ceres::CONVERGENCE;

    qInfo(summary.BriefReport().c_str());

    QFile *logFile = new QFile("quadratic.csv");
    logFile->open(QIODevice::WriteOnly);

    for (int i = 0; i < xs.count(); i++) {
        QString row = QString("%1, %2").arg(xs.at(i)).arg(ys.at(i));
        logFile->write(row.toLatin1());
        logFile->write("\n");
    }

    logFile->close();
}

double QuadraticSolver::getA() const
{
    return a;
}

double QuadraticSolver::getB() const
{
    return b;
}

double QuadraticSolver::getC() const
{
    return c;
}

bool QuadraticSolver::getFixedA() const
{
    return fixedA;
}

void QuadraticSolver::setFixedA(bool newFixedA)
{
    fixedA = newFixedA;
}

bool QuadraticSolver::getRequirePositive() const
{
    return requirePositive;
}

void QuadraticSolver::setRequirePositive(bool newRequirePositive)
{
    requirePositive = newRequirePositive;
}

bool QuadraticSolver::isConverged() const
{
    return converged;
}
