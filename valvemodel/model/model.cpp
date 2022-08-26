#include "model.h"

/**
 * @brief Model::anodeVoltage
 * @param ia The desired anode current
 * @param vg1 The grid voltage
 * @param vg2 For pentodes, the screen grid voltage
 * @return The anode voltage that will result in the desired anode current
 *
 * Uses a gradient based search to find the anode voltage that will result in the specified
 * anode current given the specified grid voltages. This is provided to enable the accurate
 * determination of a cathode load line.
 */
double Model::anodeVoltage(double ia, double vg1, double vg2)
{
    double va = 100.0;
    double tolerance = 1.2;

    double iaTest = anodeCurrent(va, vg1, vg2);
    double gradient = iaTest - anodeCurrent(va - 1.0, vg1, vg2);
    double iaErr = ia - iaTest;

    while ((abs(iaErr) / ia) > 0.01) {
        if (gradient != 0.0) {
            double vaNext = va + iaErr / gradient;
            if (vaNext < va / tolerance) { // use the gradient but limit step to 20%
                vaNext = va / tolerance;
            }
            if (vaNext > tolerance * va) { // use the gradient but limit step to 20%
                vaNext = tolerance * va;
            }
            va = vaNext;
        } else {
            va = 2 * va;
        }
        iaTest = anodeCurrent(va, vg1, vg2);
        gradient = iaTest - anodeCurrent(va - 1.0, vg1, vg2);
        iaErr = ia - iaTest;
    }

    return va;
}

void Model::solve()
{
    setOptions();

    Solver::Summary summary;
    Solve(options, &problem, &summary);

    qInfo(summary.BriefReport().c_str());
}

void Model::setLowerBound(Parameter* parameter, double lowerBound)
{
    problem.SetParameterLowerBound(parameter->getPointer(), 0, lowerBound);
}

void Model::setUpperBound(Parameter* parameter, double upperBound)
{
    problem.SetParameterUpperBound(parameter->getPointer(), 0, upperBound);
}

void Model::setLimits(Parameter* parameter, double lowerBound, double upperBound)
{
    problem.SetParameterLowerBound(parameter->getPointer(), 0, lowerBound);
    problem.SetParameterUpperBound(parameter->getPointer(), 0, upperBound);
}

