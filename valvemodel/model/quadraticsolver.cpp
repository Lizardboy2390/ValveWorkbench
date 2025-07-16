#include "quadraticsolver.h"

QuadraticSolver::QuadraticSolver(double a_, double b_, double c_) : a(a_), b(b_), c(c_), fixedA(false), requirePositive(false), converged(false)
{
    // Initialize with provided values
}

void QuadraticSolver::addSample(double x, double y)
{
    // Skip invalid data points
    if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) {
        return;
    }
    
    // Store samples for direct calculation
    xs.append(x);
    ys.append(y);
}

void QuadraticSolver::solve()
{
    // Need at least 3 points for quadratic regression
    if (xs.size() < 3) {
        converged = false;
        return;
    }
    
    // Direct calculation of quadratic regression using least squares method
    // y = ax² + bx + c
    
    int n = xs.size();
    
    // If a is fixed, we're solving a simpler problem: y - a*x² = bx + c
    if (fixedA) {
        // Adjust y values by subtracting a*x²
        QVector<double> adjusted_ys;
        for (int i = 0; i < n; i++) {
            double x = xs.at(i);
            double y = ys.at(i);
            adjusted_ys.append(y - a * x * x);
        }
        
        // Now solve a linear regression on the adjusted values
        double sum_x = 0.0;
        double sum_y = 0.0;
        double sum_xy = 0.0;
        double sum_x2 = 0.0;
        
        for (int i = 0; i < n; i++) {
            double x = xs.at(i);
            double y = adjusted_ys.at(i);
            
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }
        
        double denominator = n * sum_x2 - sum_x * sum_x;
        
        // Check for division by zero
        if (std::abs(denominator) < 1e-10) {
            converged = false;
            return;
        }
        
        // Calculate b and c
        b = (n * sum_xy - sum_x * sum_y) / denominator;
        c = (sum_y - b * sum_x) / n;
    } else {
        // Full quadratic regression
        // We need to solve the normal equations:
        // [ sum(x⁴)    sum(x³)    sum(x²) ] [ a ]   [ sum(x²y) ]
        // [ sum(x³)    sum(x²)    sum(x)  ] [ b ] = [ sum(xy)  ]
        // [ sum(x²)    sum(x)     n       ] [ c ]   [ sum(y)   ]
        
        double sum_x = 0.0;
        double sum_y = 0.0;
        double sum_x2 = 0.0;
        double sum_x3 = 0.0;
        double sum_x4 = 0.0;
        double sum_xy = 0.0;
        double sum_x2y = 0.0;
        
        for (int i = 0; i < n; i++) {
            double x = xs.at(i);
            double y = ys.at(i);
            double x2 = x * x;
            
            sum_x += x;
            sum_y += y;
            sum_x2 += x2;
            sum_x3 += x2 * x;
            sum_x4 += x2 * x2;
            sum_xy += x * y;
            sum_x2y += x2 * y;
        }
        
        // Matrix solution using Cramer's rule for 3x3 system
        double det = sum_x4 * (sum_x2 * n - sum_x * sum_x) - 
                     sum_x3 * (sum_x3 * n - sum_x * sum_x2) + 
                     sum_x2 * (sum_x3 * sum_x - sum_x2 * sum_x2);
        
        // Check for singular matrix
        if (std::abs(det) < 1e-10) {
            converged = false;
            return;
        }
        
        double det_a = sum_x2y * (sum_x2 * n - sum_x * sum_x) - 
                       sum_xy * (sum_x3 * n - sum_x * sum_x2) + 
                       sum_y * (sum_x3 * sum_x - sum_x2 * sum_x2);
        
        double det_b = sum_x4 * (sum_xy * n - sum_y * sum_x) - 
                       sum_x2y * (sum_x3 * n - sum_x2 * sum_x) + 
                       sum_x2 * (sum_x3 * sum_y - sum_x2 * sum_xy);
        
        double det_c = sum_x4 * (sum_x2 * sum_y - sum_x * sum_xy) - 
                       sum_x3 * (sum_x3 * sum_y - sum_x * sum_x2y) + 
                       sum_x2y * (sum_x3 * sum_x - sum_x2 * sum_x2);
        
        a = det_a / det;
        b = det_b / det;
        c = det_c / det;
    }
    
    // Apply constraints if required
    if (requirePositive) {
        b = std::max(0.0, b);
        c = std::max(0.0, c);
    }
    
    converged = true;
    
    // Log results
    qInfo("Quadratic regression complete: y = %fx² + %fx + %f", a, b, c);
    
    // Optional: write data to CSV file for debugging
    QFile *logFile = new QFile("quadratic.csv");
    if (logFile->open(QIODevice::WriteOnly)) {
        logFile->write("x, y, fitted\n");
        for (int i = 0; i < xs.count(); i++) {
            double x = xs.at(i);
            double y = ys.at(i);
            double fitted = a * x * x + b * x + c;
            QString row = QString("%1, %2, %3").arg(x).arg(y).arg(fitted);
            logFile->write(row.toLatin1());
            logFile->write("\n");
        }
        logFile->close();
    }
    delete logFile;
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
