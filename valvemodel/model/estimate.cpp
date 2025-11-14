#include "estimate.h"
#include "model.h"
#include "../data/sweep.h"
#include "linearsolver.h"
#include "quadraticsolver.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <limits>

Estimate::Estimate()
{

}

void Estimate::estimateTriode(Measurement *measurement) {
    estimateMu(measurement);
    estimateKg1X(measurement);
    estimateKp(measurement);
    estimateKvbKvb1(measurement);
}

namespace {
struct SweepStats {
    double vg1Nominal = 0.0;
    double vg2Nominal = 0.0;
    double vaLast = 0.0;
    double iaLast = 0.0;
    double ig2Last = 0.0;
};

std::vector<SweepStats> collectSweepStats(Measurement *measurement)
{
    std::vector<SweepStats> stats;
    if (!measurement) {
        return stats;
    }
    stats.reserve(measurement->count());
    for (int sw = 0; sw < measurement->count(); ++sw) {
        Sweep *s = measurement->at(sw);
        if (!s || s->count() == 0) {
            continue;
        }
        SweepStats info;
        info.vg1Nominal = s->getVg1Nominal();
        info.vg2Nominal = s->getVg2Nominal();
        Sample *tail = s->at(s->count() - 1);
        if (tail) {
            info.vaLast = tail->getVa();
            info.iaLast = tail->getIa();
            info.ig2Last = tail->getIg2();
        }
        stats.push_back(info);
    }
    return stats;
}

double median(const std::vector<double> &values)
{
    if (values.empty()) {
        return 0.0;
    }
    std::vector<double> copy = values;
    std::nth_element(copy.begin(), copy.begin() + copy.size() / 2, copy.end());
    return copy[copy.size() / 2];
}

double clampToRange(double value, double lower, double upper)
{
    if (upper < lower) {
        std::swap(lower, upper);
    }
    if (value < lower) {
        return lower;
    }
    if (value > upper) {
        return upper;
    }
    return value;
}

double fallbackIfInvalid(double value, double fallback)
{
    if (!std::isfinite(value)) {
        return fallback;
    }
    return value;
}
} // namespace

void Estimate::estimatePentode(Measurement *measurement, CohenHelieTriode *triodeModel, int modelType, bool secondaryEmission)
{
    const std::vector<SweepStats> stats = collectSweepStats(measurement);

    auto pickReferenceVg2 = [&]() -> double {
        std::vector<double> v;
        v.reserve(stats.size());
        for (const auto &s : stats) {
            if (std::isfinite(s.vg2Nominal) && s.vg2Nominal > 0.0) {
                v.push_back(s.vg2Nominal);
            }
        }
        return v.empty() ? 250.0 : median(v);
    };

    auto pickMostNegativeVg1 = [&]() -> double {
        double minVg1 = 0.0;
        bool found = false;
        for (const auto &s : stats) {
            if (!std::isfinite(s.vg1Nominal)) {
                continue;
            }
            double candidate = -std::fabs(s.vg1Nominal);
            if (!found || candidate < minVg1) {
                minVg1 = candidate;
                found = true;
            }
        }
        return found ? minVg1 : -20.0;
    };

    auto sampleHighVaCurrent = [&](double *iaOut, double *ig2Out) {
        double bestVa = -std::numeric_limits<double>::infinity();
        double iaCandidate = 0.0;
        double ig2Candidate = 0.0;
        for (const auto &s : stats) {
            if (s.vaLast > bestVa) {
                bestVa = s.vaLast;
                iaCandidate = s.iaLast;
                ig2Candidate = s.ig2Last;
            }
        }
        if (bestVa > -std::numeric_limits<double>::infinity()) {
            if (iaOut) *iaOut = iaCandidate;
            if (ig2Out) *ig2Out = ig2Candidate;
            return true;
        }
        return false;
    };

    double iaHigh = 0.0;
    double ig2High = 0.0;
    const bool haveHighVa = sampleHighVaCurrent(&iaHigh, &ig2High);

    if (triodeModel == nullptr) {
        mu = clampToRange(std::fabs(pickMostNegativeVg1()) * 0.6 + 6.0, 5.0, 18.0);
        x = clampToRange(1.3 + 0.02 * std::fabs(pickMostNegativeVg1()), 1.2, 1.6);
        kg1 = clampToRange((std::fabs(pickMostNegativeVg1()) + 3.0) * 0.08, 0.2, 1.5);
        kp = clampToRange(pickReferenceVg2() * 0.7, 40.0, 300.0);
        kvb = clampToRange(pickReferenceVg2(), 60.0, 400.0);
        kvb1 = clampToRange(pickReferenceVg2() / 20.0, 4.0, 25.0);
        vct = clampToRange(pickMostNegativeVg1() * -0.01, 0.0, 1.0);

        if (haveHighVa && ig2High > 1e-6) {
            const double vg2Nom = pickReferenceVg2();
            const double targetRatio = iaHigh / std::max(ig2High, 1e-6);
            kg2 = clampToRange(targetRatio * 0.5, 0.1, 15.0);
        } else {
            kg2 = clampToRange(kg1 * 5.0, 0.1, 15.0);
        }

        a = clampToRange(0.005 + 0.001 * std::fabs(pickMostNegativeVg1()), 0.0, 0.05);
        beta = clampToRange(0.08 + 0.002 * std::fabs(pickMostNegativeVg1()), 0.02, 0.25);
        gamma = clampToRange(1.2 - 0.01 * std::fabs(pickMostNegativeVg1()), 0.7, 1.5);
        psi = clampToRange(haveHighVa ? ig2High / std::max(iaHigh, 1e-6) : 3.0, 0.5, 6.0);
        omega = 200.0;
        lambda = 50.0;
        nu = 20.0;
        s = clampToRange((haveHighVa ? (iaHigh - ig2High) : 5.0) * 0.002, 0.0, 0.5);
        ap = clampToRange(0.015, 0.005, 0.05);
        qInfo("PENTODE SEED (MEAS): vg2Ref=%.3f, vg1Min=%.3f, iaHigh=%.3f, ig2High=%.3f", pickReferenceVg2(), pickMostNegativeVg1(), iaHigh, ig2High);
        qInfo("PENTODE SEED (MEAS): mu=%.3f x=%.3f kg1=%.3f kg2=%.3f kp=%.3f kvb=%.3f kvb1=%.3f vct=%.3f", mu, x, kg1, kg2, kp, kvb, kvb1, vct);
        qInfo("PENTODE SEED (MEAS): a=%.4f beta=%.4f gamma=%.3f psi=%.3f s=%.3f ap=%.3f", a, beta, gamma, psi, s, ap);
        return;
    }

    // Normal path seeded from an existing triode model
    mu = triodeModel->getParameter(PAR_MU);
    x = triodeModel->getParameter(PAR_X);
    kg1 = triodeModel->getParameter(PAR_KG1);
    kp = triodeModel->getParameter(PAR_KP);
    kvb = triodeModel->getParameter(PAR_KVB);
    kvb1 = triodeModel->getParameter(PAR_KVB1);
    vct = triodeModel->getParameter(PAR_VCT);

    estimateKg2(measurement, triodeModel);
    estimateA(measurement, triodeModel);
    estimateBetaGamma(measurement, triodeModel);
    //estimateAlphaBeta(measurement, triodeModel, modelType);
    // Derive kg2 by comparing fitted triode epk to screen current at high Va.
    {
        double kg2Numerator = 0.0;
        double kg2Denominator = 0.0;
        for (const auto &s : stats) {
            if (s.vaLast < 0.8 * measurement->getAnodeStop()) {
                continue;
            }
            double epk = triodeModel->cohenHelieEpk(s.vg2Nominal, s.vg1Nominal);
            if (std::isfinite(epk) && epk > 1e-9 && std::isfinite(s.ig2Last) && s.ig2Last > 1e-6) {
                kg2Numerator += epk;
                kg2Denominator += s.ig2Last;
            }
        }
        if (kg2Denominator > 1e-6) {
            kg2 = clampToRange(kg2Numerator / kg2Denominator, 0.1, 15.0);
        } else {
            kg2 = clampToRange(triodeModel->getParameter(PAR_KG1) * 5.0, 0.1, 15.0);
        }
    }

    if (secondaryEmission) {
        double meanRise = 0.0;
        int riseCount = 0;
        for (const auto &s : stats) {
            if (s.vaLast < measurement->getAnodeStop() * 0.3) {
                continue;
            }
            if (std::isfinite(s.ig2Last) && std::isfinite(s.iaLast)) {
                double total = s.iaLast + s.ig2Last;
                if (total > 1e-6) {
                    meanRise += s.ig2Last / total;
                    riseCount++;
                }
            }
        }
        const double ratio = (riseCount > 0) ? (meanRise / riseCount) : 0.02;
        s = clampToRange(ratio * 0.5, 0.0, 0.5);
        ap = clampToRange(0.01 + ratio * 0.02, 0.005, 0.05);
        omega = clampToRange(150.0 + ratio * 400.0, 50.0, 600.0);
        lambda = clampToRange(40.0 + ratio * 120.0, 10.0, 200.0);
        nu = clampToRange(15.0 + ratio * 40.0, 5.0, 80.0);
    }

    mu = clampToRange(mu, 3.0, 25.0);
    x = clampToRange(x, 1.1, 1.8);
    kg1 = clampToRange(kg1, 0.05, 5.0);
    kp = clampToRange(kp, 20.0, 400.0);
    kvb = clampToRange(kvb, 50.0, 600.0);
    kvb1 = clampToRange(kvb1, 1.0, 40.0);
    vct = clampToRange(vct, 0.0, 3.0);
    kg2 = clampToRange(kg2, 0.1, 20.0);
    a = clampToRange(a, 0.0, 0.05);
    beta = clampToRange(beta, 0.01, 0.3);
    gamma = clampToRange(gamma, 0.5, 2.0);
    psi = clampToRange(psi, 0.5, 8.0);
    omega = clampToRange(omega, 10.0, 800.0);
    lambda = clampToRange(lambda, 5.0, 250.0);
    nu = clampToRange(nu, 0.0, 120.0);
    s = clampToRange(s, 0.0, 1.0);
    ap = clampToRange(ap, 0.0, 0.2);
    qInfo("PENTODE SEED (TRI): vg2Ref=%.3f, vg1Min=%.3f, iaHigh=%.3f, ig2High=%.3f", pickReferenceVg2(), pickMostNegativeVg1(), iaHigh, ig2High);
    qInfo("PENTODE SEED (TRI): mu=%.3f x=%.3f kg1=%.3f kg2=%.3f kp=%.3f kvb=%.3f kvb1=%.3f vct=%.3f", mu, x, kg1, kg2, kp, kvb, kvb1, vct);
    qInfo("PENTODE SEED (TRI): a=%.4f beta=%.4f gamma=%.3f psi=%.3f s=%.3f ap=%.3f", a, beta, gamma, psi, s, ap);
}

double Estimate::anodeCurrent(double va, double vg1)
{
    return 0.0;
}

QGraphicsItemGroup *Estimate::plotModel(Plot *plot, Measurement *measurement)
{
    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    QPen estimatePen;
    estimatePen.setColor(QColor::fromRgb(0, 0, 255));

    int deviceType = measurement->getDeviceType();
    int testType = measurement->getTestType();
    if (deviceType == TRIODE) {
        if (testType == ANODE_CHARACTERISTICS) {
            double vgStart = measurement->getGridStart();
            double vgStop = measurement->getGridStop();
            double vgStep = measurement->getGridStep();

            double vg1 = vgStart;
            while ( vg1 <= vgStop) {
                double vaStart = measurement->getAnodeStart();
                double vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                double vaPrev = vaStart;
                double iaPrev = anodeCurrent(vaStart, vgStart);

                double va = vaStart + vaInc;
                while (va < vaStop) {
                    double ia = anodeCurrent(va, -vg1);
                    group->addToGroup(plot->createSegment(vaPrev, iaPrev, va, ia, estimatePen));

                    vaPrev = va;
                    iaPrev = ia;

                    va += vaInc;
                }

                vg1 += vgStep;
            }
        }
    }

    return group;
}

double Estimate::getPsi() const
{
    return psi;
}

void Estimate::setPsi(double newPsi)
{
    psi = newPsi;
}

double Estimate::getGamma() const
{
    return gamma;
}

void Estimate::setGamma(double newGamma)
{
    gamma = newGamma;
}

double Estimate::findVa(Sweep *sweep, double iax)
{
    int samples = sweep->count();
    double lowerVa = 0.0;
    double lowerIa = 0.0;
    double upperVa = 1000.0;
    double upperIa = 1000.0;

    for (int i = 0; i < samples; i++) {
        Sample *sample = sweep->at(i);
        double ia = sample->getIa();

        if (ia < iax && ia > lowerIa) {
            lowerIa = ia;
            lowerVa = sample->getVa();
        }

        if (ia >= iax && ia < upperIa) {
            upperIa = ia;
            upperVa = sample->getVa();
        }
    }

    if (upperIa >= 999.0) {
        return -1.0;
    }

    double slope = (upperIa - lowerIa) / (upperVa - lowerVa);

    return lowerVa + (iax - lowerIa) / slope; // Linear interpolation to iMu
}

double Estimate::findIa(Sweep *sweep, double vax)
{
    int samples = sweep->count();
    double lowerVa = 0.0;
    double lowerIa = 0.0;
    double upperVa = 1000.0;
    double upperIa = 1000.0;

    for (int i = 0; i < samples; i++) {
        Sample *sample = sweep->at(i);
        double va = sample->getVa();

        if (va < vax && va > lowerVa) {
            lowerVa = va;
            lowerIa = sample->getIa();
        }

        if (va >= vax && va < upperVa) {
            upperVa = va;
            upperIa = sample->getIa();
        }
    }

    if (upperVa >= 999.0) {
        return -1.0;
    }

    double slope = (upperIa - lowerIa) / (upperVa - lowerVa);

    return lowerIa + (vax - lowerVa) * slope; // Linear interpolation to iMu
}

void Estimate::estimateMu(Measurement *measurement)
{
    if (measurement->getDeviceType() == TRIODE) {
        if (measurement->getTestType() == ANODE_CHARACTERISTICS) {
            double iMu = measurement->getIaMax() * 0.05;
            if (iMu < 1) { // Set a minimum limit on iMu of 1mA
                iMu = 1;
            }
            Sweep *sweep = measurement->at(0);
            double va = findVa(sweep, iMu);
            double vg1 = sweep->getVg1Nominal();

            QList<double> muValues;

            int sweeps = measurement->count();
            for (int sw = 1; sw < sweeps; sw++) {
                sweep = measurement->at(sw);

                double vaNew = findVa(sweep, iMu);
                if (vaNew < 0) { // Sweep does not reach iMu so we can ignore this sweep (and further sweeps)
                    break;
                }

                double vg1New = sweep->getVg1Nominal();

                muValues.append((vaNew - va) / (vg1 - vg1New));

                va = vaNew;
                vg1 = vg1New;
            }

            double mux = 0.0;
            int values = muValues.count();
            for (int i = 0; i < values; i++) {
                mux += muValues.at(i);
            }

            if (values > 0) {
                mu = mux / values;
            }
        }
    }
}

void Estimate::estimateKg1X(Measurement *measurement)
{
    if (measurement->getDeviceType() == TRIODE) {
        LinearSolver *solver = new LinearSolver(1.4, log(0.5));

        double iThresh = measurement->getIaMax() * 0.40;

        int sweeps = measurement->count();
        for (int sw = 0; sw < sweeps; sw++) {
            Sweep *sweep = measurement->at(sw);

            int samples = sweep->count();
                for (int i = 0; i < samples; i++) {
                Sample *sample = sweep->at(i);

                if (sample->getIa() > iThresh) {
                    constexpr double milliampToAmp = 1000.0;
                    solver->addSample(
                        log(sample->getVa() / mu + sample->getVg1()),
                        log(sample->getIa() / milliampToAmp));
                }
            }
        }

        solver->solve();
        x = solver->getA();
        kg1 = exp(-solver->getB());
    }
}

void Estimate::estimateKp(Measurement *measurement)
{
    if (measurement->getDeviceType() == TRIODE) {
        if (measurement->getTestType() == ANODE_CHARACTERISTICS) {
            QList<double> kpValues;

            int sweeps = measurement->count();
            for (int sw = 0; sw < sweeps; sw++) {
                Sweep *sweep = measurement->at(s);
                double vg1 = sweep->getVg1Nominal();

                if (vg1 < -0.0001) {
                    double vt = -vg1 * mu;
                    double ia = findIa(sweep, vt);
                    if (ia > 0.0) {
                        kpValues.append(vt * log(2 ) / pow(ia * kg1, 1 / x));
                    }
                }
            }

            double kpx = 0.0;
            int values = kpValues.count();
            for (int i = 0; i < values; i++) {
                kpx += kpValues.at(i);
            }

            if (values > 0) {
                kp = kpx / values;
            }
        }
    }
}

void Estimate::estimateKvbKvb1(Measurement *measurement)
{
    QuadraticSolver *solver = new QuadraticSolver();
    solver->setFixedA(true);
    solver->setRequirePositive(true);

    bool hasSamples = false;

    double iThresh = measurement->getIaMax() * 0.20;

    int sweeps = measurement->count();
    for (int sw = 0; sw < sweeps; sw++) {
        Sweep *sweep = measurement->at(sw);

        if (sweep->getVg1Nominal() < -0.0001) {
            int samples = sweep->count();
            for (int i = 0; i < samples; i++) {
                Sample *sample = sweep->at(i);

                if (sample->getIa() > 0.0 && sample->getIa() < iThresh) {
                    double f = sample->getVg1() / (pow(sample->getIa() * kg1, 1 / x) / sample->getVa() - 1/ mu);

                    solver->addSample(sample->getVa(), f * f);
                    hasSamples = true;
                }
            }
        }
    }

    if (hasSamples) {
        solver->solve();
        if (solver->isConverged()) {
            double a = solver->getA();
            Q_UNUSED(a);
            kvb1 = solver->getB();
            kvb = solver->getC();
        }
    }

    delete solver;
}

/**
 * @brief Estimate::estimateKg2
 * @param measurement
 * @param triodeModel
 *
 * Estimating Kg2 is trivial as it simply involves a linear calculation with regard to Epk at when Va is high.
 * This is easily achieved by taking the average value of Kg2 for the sample at the end of each sweep. However,
 * it is important that the sweep terminates at high Va and early sweep terminations due to Pa being exceeded
 * must be excluded.
 */
void Estimate::estimateKg2(Measurement *measurement, CohenHelieTriode *triodeModel)
{
    int sweeps = measurement->count();

    double kg2Sum = 0.0;

    for (int sw = 0; sw < sweeps; sw++) { // Take the sweep with lowest -Vg1 that has a valid sample
        Sweep *sweep = measurement->at(sw);
        Sample *sample = sweep->at(sweep->count() - 1);
        if (sample->getVa() / measurement->getAnodeStop() > 0.9) { // Make sure we're over 90% Va(max)
            kg2 = triodeModel->cohenHelieEpk(sample->getVg2(), sample->getVg1()) / sample->getIg2();
            return;
        }
    }

    kg2 = 4.5 * kg1;

    return;
}

/**
 * @brief Estimate::estimateA
 * @param measurement
 * @param triodeModel
 *
 * A represents the residual slope of the *cathode* current with respect to Va and so can be estimated by examining
 * this gradient for Va between 90% and 100% of Va(max) for a given sweep.
 */
void Estimate::estimateA(Measurement *measurement, CohenHelieTriode *triodeModel)
{
    int sweeps = measurement->count();

    double aSum = 0.0;

    for (int sw = 0; sw < sweeps; sw++) {
        Sweep *sweep = measurement->at(sw);

        double epk = triodeModel->cohenHelieEpk(sweep->getVg2Nominal(), sweep->getVg1Nominal());

        int endSampleIndex = sweep->count() - 1;
        int startSampleIndex = endSampleIndex * 9 / 10;

        Sample *endSample = sweep->at(endSampleIndex);
        Sample *startSample = sweep->at(startSampleIndex);

        double endIk = endSample->getIa() + endSample->getIg2();
        double startIk = startSample->getIa() + startSample->getIg2();

        double slope = (endIk - startIk) / (endSample->getVa() - startSample->getVa());
        aSum += slope * kg1 / epk;
    }

    a = aSum / sweeps;
    a = 0.0;
}

/**
 * @brief Estimate::estimateAlphaBeta
 * @param measurement
 * @param triodeModel
 * @param modelType
 *
 * The estimation of Alpha and Beta depends on the nature of the chosen grid current scaling for low Va but, in either case,
 * necessarily starts with identifying useful samples from the data set, i.e. ones where Va is low. This is done by selecting
 * samples from the Measurement where Va is less than 5% of the maxmimum Va requested for the Measurement (anodeStop).
 *
 * Thereafter, the selected samples are applied to a linear fit specific to g(Va) to determine the parameter estimates.
 */
void Estimate::estimateAlphaBeta(Measurement *measurement, CohenHelieTriode *triodeModel, int modelType)
{
    double vaThreshold = measurement->getAnodeStop() * 0.05;

    double alphasAve = 0.0;
    double betaAve = 0.0;
    double alphas;

    int sweeps = measurement->count();
    for (int sw = 0; sw < sweeps; sw++) {
        LinearSolver solver(0.0, 0.0);

        Sweep *sweep = measurement->at(sw);

        int samples = sweep->count();
        for (int sa = 0; sa < samples; sa++) {
            Sample *sample = sweep->at(sa);
            if (sample->getVa() < vaThreshold) {
                double y = sample->getIg2() * kg2 / triodeModel->cohenHelieEpk(sample->getVg2(), sample->getVg1()) - 1.0;

                if (modelType == REEFMAN_DERK_PENTODE) {
                    solver.addSample(sample->getVa(), 1.0 / y);
                } else {
                    solver.addSample(pow(sample->getVa(), 1.5), std::log(y));
                }
            }
        }

        solver.solve();

        alphas = 1.0 / solver.getB();
        betaAve += alphas * solver.getA();
        if (modelType == REEFMAN_DERK_PENTODE) {
            alphasAve += 1.0 / solver.getB();
            betaAve += alphasAve * solver.getA();
        } else {
            alphasAve += std::exp(solver.getB());
            betaAve += std::pow(-1.0 * solver.getA(), 2.0 / 3.0);
        }
    }

    beta = betaAve / sweeps;
}

void Estimate::estimateBetaGamma(Measurement *measurement, CohenHelieTriode *triodeModel)
{
    // First find the earliest sweep with enough data points
    int n = 0;
    Sweep *sweep = measurement->at(n);
    while (sweep->count() < 10 && n < measurement->count() && sweep->at(sweep->count() - 1)->getVa() < measurement->getAnodeStop() * 0.9) {
        sweep = measurement->at(++n);
    }

    LinearSolver solver(0.0, 0.0);

    int samples = sweep->count();
    for (int sa = 1; sa < samples; sa++) {
        Sample *sample = sweep->at(sa);
        double iaExp = triodeModel->cohenHelieEpk(sample->getVg2(),sample->getVg1()) * (1.0 / triodeModel->getParameter(PAR_KG1) - 1.0 / kg2);
        double r = sample->getIa() / iaExp;
        double g = 1.0 - r;
        double x1 = 1.0 / g - 1.0;

        if (sample->getVa() > 0.0 && x1 > 0.0 && sample->getVa() < measurement->getAnodeStop() / 5.0) {
            solver.addSample(std::log(sample->getVa()), std::log(x1));
        }
    }

    solver.solve();

    beta = exp(solver.getB());
    gamma = solver.getA();

    beta = 0.1;
    gamma = 1.0;
    //if (gamma < 1.0) { // Don't start with gamma too low
    //    gamma = 1.0;
    //}
}

double Estimate::getMu() const
{
    return mu;
}

void Estimate::setMu(double newMu)
{
    mu = newMu;
}

double Estimate::getKg1() const
{
    return kg1;
}

void Estimate::setKg1(double newKg1)
{
    kg1 = newKg1;
}

double Estimate::getX() const
{
    return x;
}

void Estimate::setX(double newX)
{
    x = newX;
}

double Estimate::getKp() const
{
    return kp;
}

void Estimate::setKp(double newKp)
{
    kp = newKp;
}

double Estimate::getKvb() const
{
    return kvb;
}

void Estimate::setKvb(double newKvb)
{
    kvb = newKvb;
}

double Estimate::getKvb1() const
{
    return kvb1;
}

void Estimate::setKvb1(double newKvb1)
{
    kvb1 = newKvb1;
}

double Estimate::getVct() const
{
    return vct;
}

void Estimate::setVct(double newVct)
{
    vct = newVct;
}

double Estimate::getKg2() const
{
    return kg2;
}

void Estimate::setKg2(double newKg2)
{
    kg2 = newKg2;
}

double Estimate::getA() const
{
    return a;
}

void Estimate::setA(double newA)
{
    a = newA;
}

double Estimate::getAlpha() const
{
    return alpha;
}

void Estimate::setAlpha(double newAlpha)
{
    alpha = newAlpha;
}

double Estimate::getBeta() const
{
    return beta;
}

void Estimate::setBeta(double newBeta)
{
    beta = newBeta;
}

double Estimate::getOmega() const
{
    return omega;
}

void Estimate::setOmega(double newOmega)
{
    omega = newOmega;
}

double Estimate::getLambda() const
{
    return lambda;
}

void Estimate::setLambda(double newLambda)
{
    lambda = newLambda;
}

double Estimate::getNu() const
{
    return nu;
}

void Estimate::setNu(double newNu)
{
    nu = newNu;
}

double Estimate::getS() const
{
    return s;
}

void Estimate::setS(double newS)
{
    s = newS;
}

double Estimate::getAp() const
{
    return ap;
}

void Estimate::setAp(double newAp)
{
    ap = newAp;
}

QTreeWidgetItem *Estimate::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_ESTIMATE);

    item->setText(0, "Estimate");
    item->setIcon(0, QIcon(":/icons/estimate32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    parent->setExpanded(true);

    return item;
}

void Estimate::updateUI(QLabel *labels[], QLineEdit *values[])
{

}

void Estimate::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(mu));
    addProperty(properties, "Kg1", QString("%1").arg(kg1 / 1000.0));
    addProperty(properties, "X", QString("%1").arg(x));
    addProperty(properties, "Kp", QString("%1").arg(kp));
    addProperty(properties, "Kvb", QString("%1").arg(kvb));
    addProperty(properties, "Kvb1", QString("%1").arg(kvb1));
    addProperty(properties, "vct", QString("%1").arg(vct));
}
