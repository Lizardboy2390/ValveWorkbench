#include "quadraticsolver.h"

struct QuadraticResidual {
    QuadraticResidual(double y, double x) : y_(y), x_(x) {}

    template <typename T>
    bool operator()(const T* const a, const T* const b, const T* const c, T* residual) const {
        residual[0] = y_ - a[0] * x_ * x_ + b[0] * x_ + c[0];
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
    problem.AddResidualBlock(
        new AutoDiffCostFunction<QuadraticResidual, 1, 1, 1, 1>(
            new QuadraticResidual(x, y)),
        NULL, &a, &b, &c);
}

void QuadraticSolver::solve()
{
    Solver::Summary summary;
    Solve(options, &problem, &summary);

    qInfo(summary.BriefReport().c_str());
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
