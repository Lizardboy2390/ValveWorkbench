#include "linearsolver.h"

struct LinearResidual {
    LinearResidual(double x, double y) : y_(y), x_(x) {}

    template <typename T>
    bool operator()(const T* const a, const T* const b, T* residual) const {
        residual[0] = y_ - (a[0] * x_ + b[0]);
        return true;
    }

private:
    const double y_;
    const double x_;
};

LinearSolver::LinearSolver(double a_, double b_) : a(a_), b(b_)
{

}

void LinearSolver::addSample(double x, double y)
{
    xs.append(x);
    ys.append(y);

    problem.AddResidualBlock(
        new AutoDiffCostFunction<LinearResidual, 1, 1, 1>(
            new LinearResidual(x, y)),
        NULL, &a, &b);
}

void LinearSolver::solve()
{
    Solver::Summary summary;
    Solve(options, &problem, &summary);

    qInfo(summary.BriefReport().c_str());

    QFile *logFile = new QFile("linear.csv");
    logFile->open(QIODevice::WriteOnly);

    for (int i = 0; i < xs.count(); i++) {
        QString row = QString("%1, %2").arg(xs.at(i)).arg(ys.at(i));
        logFile->write(row.toLatin1());
        logFile->write("\n");
    }

    logFile->close();
}

double LinearSolver::getA() const
{
    return a;
}

double LinearSolver::getB() const
{
    return b;
}
