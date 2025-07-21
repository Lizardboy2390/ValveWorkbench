#include "linearsolver.h"
#include <cmath>

<<<<<<< Updated upstream
LinearSolver::LinearSolver(double a_, double b_) : a(a_), b(b_), converged(false)
=======
struct LinearResidual {
    LinearResidual(double x, double y) : y_(y), x_(x) {}

    template <typename T>
    bool operator()(const T* const a, const T* const b, T* residual) const {
        residual[0] = y_ - (a[0] * x_ + b[0]);
        return !(std::isnan(y_) || std::isnan(x_) || std::isinf(y_) || std::isinf(x_));
    }

private:
    const double y_;
    const double x_;
};

LinearSolver::LinearSolver(double a_, double b_) : a(a_), b(b_)
>>>>>>> Stashed changes
{
    // Initialize with provided values
}

void LinearSolver::addSample(double x, double y)
{
    // Skip invalid data points
    if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) {
        return;
    }
    
    // Store samples for direct calculation
    xs.append(x);
    ys.append(y);
}

void LinearSolver::solve()
{
    // Need at least 2 points for linear regression
    if (xs.size() < 2) {
        converged = false;
        return;
    }
    
    // Direct calculation of linear regression using least squares method
    // y = ax + b
    // Formulas:
    // a = (n*sum(xy) - sum(x)*sum(y)) / (n*sum(x^2) - sum(x)^2)
    // b = (sum(y) - a*sum(x)) / n
    
    int n = xs.size();
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xy = 0.0;
    double sum_x2 = 0.0;
    
    for (int i = 0; i < n; i++) {
        double x = xs.at(i);
        double y = ys.at(i);
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }
    
    double denominator = n * sum_x2 - sum_x * sum_x;
    
    // Check for division by zero (or very small value)
    if (std::abs(denominator) < 1e-10) {
        // Can't solve - vertical line case or numerical issue
        converged = false;
        return;
    }
    
    // Calculate slope (a) and intercept (b)
    a = (n * sum_xy - sum_x * sum_y) / denominator;
    b = (sum_y - a * sum_x) / n;
    
    converged = true;
    
    // Log results
    qInfo("Linear regression complete: y = %fx + %f", a, b);
    
    // Optional: write data to CSV file for debugging
    QFile *logFile = new QFile("linear.csv");
    if (logFile->open(QIODevice::WriteOnly)) {
        logFile->write("x, y\n");
        for (int i = 0; i < xs.count(); i++) {
            QString row = QString("%1, %2").arg(xs.at(i)).arg(ys.at(i));
            logFile->write(row.toLatin1());
            logFile->write("\n");
        }
        logFile->close();
    }
    delete logFile;
}

double LinearSolver::getA() const
{
    return a;
}

double LinearSolver::getB() const
{
    return b;
}

bool LinearSolver::isConverged() const
{
    return converged;
}
