#include <iostream>
#include <vector>
#include <ceres/ceres.h>

// Example cost function for curve fitting
struct CURVE_FITTING_COST {
    CURVE_FITTING_COST(double x, double y) : _x(x), _y(y) {}

    template <typename T>
    bool operator()(const T* const abc, T* residual) const {
        residual[0] = T(_y) - ceres::exp(abc[0] * T(_x) * T(_x) + abc[1] * T(_x) + abc[2]);
        return true;
    }

    const double _x, _y;
};

int main() {
    double a = 1.0, b = 2.0, c = 1.0; // True parameters
    int N = 100; // Data points
    double w_sigma = 1.0; // Noise sigma
    double abc[3] = {0, 0, 0}; // Initial estimates

    std::vector<double> x_data, y_data;
    for (int i = 0; i < N; i++) {
        double x = i / 100.0;
        x_data.push_back(x);
        y_data.push_back(exp(a * x * x + b * x + c) + /* noise */ 0); // Simplified
    }

    ceres::Problem problem;
    for (int i = 0; i < N; i++) {
        problem.AddResidualBlock(
            new ceres::AutoDiffCostFunction<CURVE_FITTING_COST, 1, 3>(
                new CURVE_FITTING_COST(x_data[i], y_data[i])),
            nullptr,
            abc
        );
    }

    ceres::Solver::Options options;
    options.linear_solver_type = ceres::DENSE_QR;
    options.minimizer_progress_to_stdout = true;
    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);

    std::cout << summary.BriefReport() << std::endl;
    std::cout << "Estimated a,b,c = " << abc[0] << " " << abc[1] << " " << abc[2] << std::endl;
    return 0;
}
