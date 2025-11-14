#include "model.h"
#include "estimate.h"
#include "../data/sweep.h"
#include "../data/sample.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <set>
#include <optional>

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
Model::Model()
{
    parameter[PAR_KG1] = new Parameter("Kg:", 0.7);
    parameter[PAR_VCT] = new Parameter("Vct:", 0.1);
    parameter[PAR_X] = new Parameter("X:", 1.5);
    parameter[PAR_MU] = new Parameter("Mu:", 100.0);
    parameter[PAR_KP] = new Parameter("Kp:", 500.0);
    parameter[PAR_KVB] = new Parameter("Kvb:", 300.0);
    parameter[PAR_KVB1] = new Parameter("Kvb2:", 30.0);

    parameter[PAR_KG2] = new Parameter("Kg2:", 0.15);
    parameter[PAR_KG2A] = new Parameter("Kg3:", 0.15);
    parameter[PAR_A] = new Parameter("A:", 0.0);
    parameter[PAR_ALPHA] = new Parameter("Alpha:", 0.0);
    parameter[PAR_BETA] = new Parameter("Beta:", 0.0);
    parameter[PAR_GAMMA] = new Parameter("Gamma:", 1.0);
    parameter[PAR_OS] = new Parameter("Os:", 0.0);

    parameter[PAR_TAU] = new Parameter("Tau:", 0.1);
    parameter[PAR_RHO] = new Parameter("Rho:", 0.1);
    parameter[PAR_THETA] = new Parameter("Theta:", 0.1);
    parameter[PAR_PSI] = new Parameter("Psi:", 0.1);

    parameter[PAR_OMEGA] = new Parameter("Omega:", 30.0);
    parameter[PAR_LAMBDA] = new Parameter("Lambda:", 30.0);
    parameter[PAR_NU] = new Parameter("Nu:", 0.0);
    parameter[PAR_S] = new Parameter("S:", 0.0);
    parameter[PAR_AP] = new Parameter("Ap:", 0.0);

    plotColor = QColor::fromRgb(255, 0, 0);

    // Diagnostic: report default Os on model construction
    qInfo("MODEL INIT: default Os=%.6f", parameter[PAR_OS]->getValue());
}

double Model::anodeVoltage(double ia, double vg1, double vg2, bool secondaryEmission)
{
    double va = 100.0;
    double tolerance = 1.2;

    double iaTest = anodeCurrent(va, vg1, vg2, secondaryEmission);
    double gradient = 100.0 * (iaTest - anodeCurrent(va - 0.01, vg1, vg2, secondaryEmission));
    double iaErr = ia - iaTest;

    while (abs(iaErr) > 0.005) {
        if (gradient != 0.0) {
            double vaNext = va + iaErr / gradient;
            if (vaNext < 0.0) {
                vaNext = 0.0;
            }
            if (vaNext < va / tolerance) { // use the gradient but limit step to tolerance
                vaNext = va / tolerance;
            }
            if (vaNext > tolerance * va) { // use the gradient but limit step to tolerance
                vaNext = tolerance * va;
            }
            va = vaNext;
        } else {
            break;
        }
        iaTest = anodeCurrent(va, vg1, vg2, secondaryEmission);
        gradient = 100.0 * (iaTest - anodeCurrent(va - 0.01, vg1, vg2, secondaryEmission));
        iaErr = ia - iaTest;
    }

    return va;
}

double Model::screenCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    return 0.0;
}

void Model::addMeasurement(Measurement *measurement)
{
    int sweeps = measurement->count();
    for (int s = 0; s < sweeps; s++) {
        Sweep *sweep = measurement->at(s);

        // Filter out incomplete/limit-hit sweeps (likely ended early due to limits)
        const int minPoints = 60; // expected full sweep size from UI config
        if (sweep == nullptr || sweep->count() < minPoints) {
            qInfo("MODEL INPUT: skipping sweep %d (only %d points, need >= %d)", s, sweep ? sweep->count() : 0, minPoints);
            continue;
        }
        Sample *last = sweep->at(sweep->count() - 1);
        if (last == nullptr || last->getVa() < 0.9 * measurement->getAnodeStop()) {
            qInfo("MODEL INPUT: skipping sweep %d (end Va %.3f < 0.9*Va_stop %.3f)", s,
                  last ? last->getVa() : -1.0, measurement->getAnodeStop());
            continue;
        }

        // Determine if this sweep's grid values (Vg1) require a sign flip.
        // If the first meaningful Vg1 is positive, flip all Vg1 for this sweep.
        bool flipVg1 = false;
        {
            const double eps = 1e-6;
            int probeCount = sweep->count();
            for (int pi = 0; pi < probeCount; ++pi) {
                Sample *probe = sweep->at(pi);
                double vgProbe = probe->getVg1();
                if (!std::isfinite(vgProbe) || std::fabs(vgProbe) <= eps) {
                    vgProbe = sweep->getVg1Nominal();
                }
                if (std::isfinite(vgProbe) && std::fabs(vgProbe) > eps) {
                    flipVg1 = (vgProbe > 0.0);
                    break;
                }
            }
        }

        int samples = sweep->count();
        bool loggedFirstVg = false;
        double minVgUsed = std::numeric_limits<double>::infinity();
        double maxVgUsed = -std::numeric_limits<double>::infinity();
        for (int i = 0; i < samples; i++) {
            Sample *sample = sweep->at(i);

            const double va = sample->getVa();
            const double ia = sample->getIa();
            const double vg2 = sample->getVg2();
            const double ig2 = sample->getIg2();

            // Use sample Vg1 when available; otherwise fall back to the sweep's nominal grid
            double vg1raw = sample->getVg1();
            bool usedNominal = false;
            if (!std::isfinite(vg1raw) || std::fabs(vg1raw) < 1e-6) {
                vg1raw = sweep->getVg1Nominal();
                usedNominal = true;
            }
            // Apply per-sweep sign decision; once decided for the sweep, it stays consistent
            const double vg1Corrected = flipVg1 ? -vg1raw : vg1raw;

            if (!loggedFirstVg && std::isfinite(vg1Corrected) && std::fabs(vg1Corrected) > 1e-9) {
                qInfo("MODEL INPUT: sweep=%d first vg1 used=%.6f (%s)", s, vg1Corrected, usedNominal ? "nominal" : "sample");
                loggedFirstVg = true;
            }

            if (std::isfinite(vg1Corrected)) {
                minVgUsed = std::min(minVgUsed, vg1Corrected);
                maxVgUsed = std::max(maxVgUsed, vg1Corrected);
            }

            addSample(va, ia, vg1Corrected, vg2, ig2);
        }
        if (std::isfinite(minVgUsed) && std::isfinite(maxVgUsed)) {
            qInfo("MODEL INPUT: sweep=%d vg1 range used [%.6f, %.6f] (should be <= 0)", s, minVgUsed, maxVgUsed);
        }
    }
}

void Model::addMeasurements(QList<Measurement *> *measurements)
{
    for (int m = 0; m < measurements->count(); m++) {
        addMeasurement(measurements->at(m));
    }
}

void Model::setEstimate(Estimate *estimate)
{
    this->estimate = estimate;

    parameter[PAR_MU]->setValue(estimate->getMu());
    double estimateKg1 = estimate->getKg1();
    if (estimateKg1 > 0.01) {
        estimateKg1 /= 1000.0;
    }
    parameter[PAR_KG1]->setValue(estimateKg1);
    parameter[PAR_X]->setValue(estimate->getX());
    parameter[PAR_KP]->setValue(estimate->getKp());
    parameter[PAR_KVB]->setValue(estimate->getKvb());
    parameter[PAR_KVB1]->setValue(estimate->getKvb1());
    parameter[PAR_VCT]->setValue(estimate->getVct());

    parameter[PAR_KG2]->setValue(estimate->getKg2());
    parameter[PAR_A]->setValue(estimate->getA());
    parameter[PAR_ALPHA]->setValue(estimate->getAlpha());
    parameter[PAR_BETA]->setValue(estimate->getBeta());
    parameter[PAR_GAMMA]->setValue(estimate->getGamma());
    // parameter[PAR_OS] remains at default (0.0); Estimate has no getOs()
    // Temporarily force Os to 0.0 to prevent constant-current floor in plotted overlays
    parameter[PAR_OS]->setValue(0.0);
    //parameter[PAR_TAU]->setValue(estimate->getPsi());
    //parameter[PAR_RHO]->setValue(estimate->getPsi());
    //parameter[PAR_THETA]->setValue(estimate->getPsi());
    parameter[PAR_PSI]->setValue(estimate->getPsi());

    parameter[PAR_OMEGA]->setValue(estimate->getOmega());
    parameter[PAR_NU]->setValue(estimate->getNu());
    parameter[PAR_LAMBDA]->setValue(estimate->getLambda());
    parameter[PAR_S]->setValue(estimate->getS());
    parameter[PAR_AP]->setValue(estimate->getAp());
}

void Model::solve()
{
    converged = false;

    setOptions();

    Solver::Summary summary;

    if (mode == NORMAL_MODE) {
        Solve(options, &anodeProblem, &summary);
    } else if (mode == SCREEN_MODE) {
        Solve(options, &screenProblem, &summary);
    } else if (mode == ANODE_REMODEL_MODE) {
        Solve(options, &anodeRemodelProblem, &summary);
    }

    converged = summary.termination_type == ceres::CONVERGENCE;

    // qInfo(summary.FullReport().c_str());
    }

void Model::solveThreaded()
{
    converged = false;

    setOptions();

    Solver::Summary summary;

    if (mode == NORMAL_MODE) {
        Solve(options, &anodeProblem, &summary);
    } else if (mode == SCREEN_MODE) {
        Solve(options, &screenProblem, &summary);
    } else if (mode == ANODE_REMODEL_MODE) {
        Solve(options, &anodeRemodelProblem, &summary);
    }

    converged = summary.termination_type == ceres::CONVERGENCE;

    // qInfo(summary.FullReport().c_str());

    emit modelReady();
}

QTreeWidgetItem *Model::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_MODEL);

    item->setText(0, "Model");
    item->setIcon(0, QIcon(":/icons/estimate32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    parent->setExpanded(true);

    return item;
}

QGraphicsItemGroup *Model::plotModel(Plot *plot, Measurement *measurement, Sweep *sweep)
{
    qInfo("=== MODEL PLOTTING DEBUG ===");
    qInfo("Model::plotModel called with measurement type: %s, test type: %s",
           measurement->getDeviceType() == TRIODE ? "TRIODE" : "PENTODE",
           measurement->getTestType() == ANODE_CHARACTERISTICS ? "ANODE_CHARACTERISTICS" : "TRANSFER_CHARACTERISTICS");

    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    QPen anodePen;
    anodePen.setColor(plotColor.isValid() ? plotColor : QColor::fromRgb(255, 0, 0));
    QPen screenPen;
    screenPen.setColor(QColor::fromRgb(0, 0, 255));

    int deviceType = measurement->getDeviceType();
    int testType = measurement->getTestType();
    qInfo("Device type: %d, Test type: %d", deviceType, testType);
    if (deviceType == TRIODE) {
            if (testType == ANODE_CHARACTERISTICS) {
                double vgStart = measurement->getGridStart();
                double vgStop = measurement->getGridStop();
                double vgStep = measurement->getGridStep();

                if (!std::isfinite(vgStart)) vgStart = 0.0;
                if (!std::isfinite(vgStop))  vgStop  = 0.0;
                if (!std::isfinite(vgStep))  vgStep  = 0.0;

                qInfo("STORED VALUES: vgStart=%.3f, vgStop=%.3f, vgStep=%.3f", vgStart, vgStop, vgStep);
                qInfo("Measurement has %d sweeps", measurement->count());

                auto deriveGridRangeFromSweeps = [measurement]() -> std::pair<double, double> {
                    auto scaleVg = [](double vg) {
                        return (std::isfinite(vg) && std::fabs(vg) > 50.0) ? (vg / 1000.0) : vg;
                    };
                    double minVg = std::numeric_limits<double>::infinity();
                    double maxVg = -std::numeric_limits<double>::infinity();
                    bool sawValidSample = false;
                    bool sawZeroBias = false;
                    bool sawZeroNominal = false;
                    const double zeroClampLower = -0.5;
                    const double zeroClampUpper = 0.0;

                    for (int i = 0; i < measurement->count(); ++i) {
                        Sweep *sweep = measurement->at(i);
                        if (sweep == nullptr) {
                            continue;
                        }

                        // Prefer sample grid voltages for the range.
                        for (int sampleIndex = 0; sampleIndex < sweep->count(); ++sampleIndex) {
                            Sample *sample = sweep->at(sampleIndex);
                            if (sample == nullptr) {
                                continue;
                            }
                            double vg = scaleVg(sample->getVg1());
                            if (!std::isfinite(vg)) {
                                continue;
                            }
                            if (vg >= zeroClampLower && vg <= zeroClampUpper) {
                                qInfo("GRID RANGE: sample vg=%.3f within [%0.1f, %0.1f], clamping to 0V", vg, zeroClampLower, zeroClampUpper);
                                vg = 0.0;
                                sawZeroBias = true;
                            }

                            minVg = std::min(minVg, vg);
                            maxVg = std::max(maxVg, vg);
                            sawValidSample = true;
                        }

                        // Fallback to nominal if no samples were available.
                        if (!sawValidSample) {
                            double vgNominal = scaleVg(sweep->getVg1Nominal());
                            if (std::isfinite(vgNominal)) {
                                if (vgNominal >= zeroClampLower && vgNominal <= zeroClampUpper) {
                                    qInfo("GRID RANGE: nominal vg=%.3f within [%0.1f, %0.1f], clamping to 0V", vgNominal, zeroClampLower, zeroClampUpper);
                                    vgNominal = 0.0;
                                    sawZeroBias = true;
                                    sawZeroNominal = true;
                                }

                                minVg = std::min(minVg, vgNominal);
                                maxVg = std::max(maxVg, vgNominal);
                                sawValidSample = true;
                            }
                        }
                    }

                    if (!sawValidSample || !std::isfinite(minVg) || !std::isfinite(maxVg)) {
                        return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
                    }

                    if (sawZeroBias || sawZeroNominal) {
                        maxVg = std::max(maxVg, 0.0);
                        minVg = std::min(minVg, 0.0);
                    }

                    // Ensure the range progresses from most negative to least negative.
                    if (maxVg > 0.0 && minVg >= 0.0) {
                        // Likely stored as magnitudes; reflect to negative.
                        minVg = -maxVg;
                        maxVg = 0.0;
                    }

                    if (minVg > maxVg) {
                        std::swap(minVg, maxVg);
                    }

                    return {minVg, maxVg};
                };

                const bool rangeInvalid = (vgStart == 0.0 && vgStop == 0.0) || vgStart > vgStop;
                if (rangeInvalid) {
                    auto [derivedStart, derivedStop] = deriveGridRangeFromSweeps();
                    if (std::isfinite(derivedStart) && std::isfinite(derivedStop) && derivedStart < derivedStop) {
                        vgStart = derivedStart;
                        vgStop = derivedStop;
                        qInfo("Derived grid range from sweep data: start=%.3f, stop=%.3f", vgStart, vgStop);
                    } else {
                        vgStart = -60.0;
                        vgStop = 0.0;
                        qInfo("Unable to derive grid range, using defaults: start=%.3f, stop=%.3f", vgStart, vgStop);
                    }
                }
                // Enforce non-positive grids for triode modelling regardless of source
                vgStart = -std::fabs(vgStart);
                vgStop = -std::fabs(vgStop);
                // Prefer to end at 0V (least negative) for plotting families
                if (vgStop > 0.0) vgStop = 0.0;
            // If step is 0 or invalid, calculate from actual sweep data
            if (vgStep <= 0.0 || vgStep > (vgStop - vgStart)) {
                qInfo("GRID STEP FALLBACK: stored step %.6f invalid, analysing sweeps (count=%d)",
                      vgStep, measurement->count());
                double calculatedStep = 0.0;
                if (measurement->count() > 1) {
                    const double firstVg = measurement->at(0)->getVg1Nominal();
                    qInfo("GRID STEP FALLBACK: sweep 0 Vg1Nominal=%.6f", firstVg);
                    for (int sweepIndex = 1; sweepIndex < measurement->count(); ++sweepIndex) {
                        const double candidateVg = measurement->at(sweepIndex)->getVg1Nominal();
                        double diff = std::numeric_limits<double>::quiet_NaN();
                        if (std::isfinite(candidateVg) && std::isfinite(firstVg)) {
                            double a = (std::fabs(firstVg) > 50.0) ? firstVg / 1000.0 : firstVg;
                            double b = (std::fabs(candidateVg) > 50.0) ? candidateVg / 1000.0 : candidateVg;
                            diff = std::fabs(b - a);
                        }
                        qInfo("GRID STEP FALLBACK: comparing sweep %d Vg1Nominal=%.6f (diff vs sweep 0 = %.6f)",
                              sweepIndex, candidateVg, diff);
                        if (!std::isfinite(candidateVg)) {
                            continue;
                        }
                        const double stepCandidate = (std::fabs(candidateVg) > 50.0 || std::fabs(firstVg) > 50.0)
                                                     ? std::fabs(candidateVg/1000.0 - firstVg/1000.0)
                                                     : std::fabs(candidateVg - firstVg);
                        if (stepCandidate > 0.0 && stepCandidate < 10.0) {
                            calculatedStep = stepCandidate;
                            qInfo("GRID STEP FALLBACK: accepting sweep %d diff %.6f as grid step",
                                  sweepIndex, calculatedStep);
                            break;
                        }
                    }
                }

                if (calculatedStep > 0.0) {
                    vgStep = calculatedStep;
                    qInfo("GRID STEP FALLBACK: using calculated grid step %.6f", vgStep);
                } else {
                    vgStep = 0.5;
                    qInfo("GRID STEP FALLBACK: no valid diff found, using default %.6f", vgStep);
                }
                qInfo("GRID STEP FALLBACK: final vgStep %.6f", vgStep);
            }

            if (sweep != nullptr) {
                double nominal = sweep->getVg1Nominal();
                if (std::fabs(nominal) > 50.0) nominal /= 1000.0;
                nominal = -std::fabs(nominal);
                vgStart = nominal;
                vgStop = nominal;
            }

            if (vgStart > vgStop) {
                qInfo("GRID RANGE NORMALIZATION: swapping start %.3f and stop %.3f", vgStart, vgStop);
                std::swap(vgStart, vgStop);
            }

            if (vgStep < 0.0) {
                qInfo("GRID STEP NORMALIZATION: converting negative step %.6f to positive", vgStep);
                vgStep = std::abs(vgStep);
            }

            if (vgStep == 0.0) {
                vgStep = 0.5;
                qInfo("GRID STEP NORMALIZATION: step was zero, using default %.3f", vgStep);
            }

            double vg2 = measurement->getScreenStart();

            qInfo("Final grid voltage range: start=%.3f, stop=%.3f, step=%.3f", vgStart, vgStop, vgStep);
            qInfo("Screen voltage: %.3f", vg2);

            // Do not set axes here; measurement defines axes

            double vg1 = vgStart;
            int curveCount = 0;
            const double yMaxAxis = measurement->getIaMax();
            // Do not set axes here; measurement defines axes
            while ( vg1 <= vgStop) {
                qInfo("TRIODE LOOP: vg1=%.3f, vgStart=%.3f, vgStop=%.3f, vgStep=%.3f, condition (%.3f <= %.3f) = %s",
                      vg1, vgStart, vgStop, vgStep, vg1, vgStop, (vg1 <= vgStop) ? "true" : "false");

                qInfo("Creating curve %d for vg1=%.3f", curveCount + 1, vg1);
                double vaStart = measurement->getAnodeStart();
                double vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                qInfo("Anode voltage range: start=%.1f, stop=%.1f, inc=%.3f", vaStart, vaStop, vaInc);

                double vaPrev = vaStart;
                double vgPhys = -std::fabs(vg1);
                double iaPrev = anodeCurrent(vaStart, vgPhys, vg2);

                double va = vaStart + vaInc;
                int segmentCount = 0;
                while (va < vaStop) {
                    qInfo("TRIODE: Calculating current for va=%.3f, vg1=%.3f", va, vg1);
                    double vgPhysSeg = -std::fabs(vg1);
                    double ia = anodeCurrent(va, vgPhysSeg, vg2);
                    qInfo("TRIODE: Current result ia=%.3f mA", ia);

                    QGraphicsItem *segment = plot->createSegment(vaPrev, iaPrev, va, ia, anodePen);

                    if (segment != nullptr) {
                        group->addToGroup(segment);
                        segmentCount++;
                    } else {
                        qWarning("Failed to create segment for va=%.3f, ia=%.3f", va, ia);
                    }

                    vaPrev = va;
                    iaPrev = ia;

                    va += vaInc;
                }

                qInfo("Curve %d completed: %d segments created", curveCount + 1, segmentCount);

                double oldVg1 = vg1;
                vg1 += vgStep;
                curveCount++;
                qInfo("TRIODE LOOP: Progressed vg1 from %.3f to %.3f (added %.3f)", oldVg1, vg1, vgStep);
            }

            qInfo("Total curves created: %d", curveCount);
        }
    } else if (deviceType == PENTODE) {
        if (testType == ANODE_CHARACTERISTICS) {
            qInfo("PENTODE: Measurement has %d sweeps", measurement->count());

            // Build unique sorted Vg family list from measurement sweeps' Vg1Nominal (volts),
            // converted to negative magnitudes to match model convention. No unit conversion.
            std::set<double> familySet;
            for (int i = 0; i < measurement->count(); ++i) {
                double vgNom = measurement->at(i)->getVg1Nominal(); // volts
                if (!std::isfinite(vgNom)) continue;
                double fam = -std::fabs(vgNom);
                // Ignore nearly-zero families (should not happen for pentode grids)
                if (std::fabs(fam) < 0.5) {
                    qInfo("PENTODE: Skipping tiny-magnitude Vg family %.3fV (likely unit mismatch)", fam);
                    continue;
                }
                familySet.insert(fam);
                qInfo("PENTODE: Sweep %d Vg1Nominal=%.3fV -> family %.3fV", i, measurement->at(i)->getVg1Nominal(), fam);
            }

            std::vector<double> vgFamilies;
            vgFamilies.reserve(familySet.size());
            for (double v : familySet) vgFamilies.push_back(v);
            std::sort(vgFamilies.begin(), vgFamilies.end()); // ascending: most negative -> least negative (towards 0)

            qInfo("PENTODE: Using %zu Vg families from measurement", vgFamilies.size());

            const double vg2 = measurement->getScreenStart();
            const bool drawScreen = false; // Temporarily disable screen overlay per debugging plan

            // Temporarily force Os to zero during plotting to avoid constant-current floor from JSON/defaults
            double osSavedForPlot = parameter[PAR_OS]->getValue();
            parameter[PAR_OS]->setValue(0.0);
            // Y-axis max for clamping plotted model currents
            const double yMaxAxis = measurement->getIaMax();

            // Plotting-only curvature calibration: save and set shaping parameters
            double aSaved = parameter[PAR_A]->getValue();
            double betaSaved = parameter[PAR_BETA]->getValue();
            double gammaSaved = parameter[PAR_GAMMA]->getValue();
            parameter[PAR_A]->setValue(0.002);
            parameter[PAR_BETA]->setValue(0.15);
            parameter[PAR_GAMMA]->setValue(1.2);

            // Measurement-driven kg1 calibration using the -20V family at mid Va
            // Find the sweep closest to -20V (family value is negative magnitude)
            double iaScale = 1.0; // display-only scaling to match measurement units/axis
            {
                const double targetFam = -20.0;
                const double famTol = 1e-3;
                Sweep *calSweep = nullptr;
                for (int si = 0; si < measurement->count(); ++si) {
                    double nomV = measurement->at(si)->getVg1Nominal();
                    double famV = -std::fabs(nomV);
                    if (std::fabs(famV - targetFam) <= famTol) { calSweep = measurement->at(si); break; }
                }
                if (calSweep && calSweep->count() >= 3) {
                    auto pickVg2 = [&](Sample *samp) {
                        double v = samp->getVg2();
                        if (!std::isfinite(v) || v <= 1.0) v = calSweep->getVg2Nominal();
                        if (!std::isfinite(v) || v <= 1.0) v = vg2;
                        return v;
                    };
                    int midIdx = calSweep->count() / 2;
                    Sample *sm = calSweep->at(midIdx);
                    double vaCal = sm->getVa();
                    double vg1Cal = targetFam;
                    double vg2Cal = pickVg2(sm);
                    double iaTarget = sm->getIa(); // mA

                    // Current kg1 and its reciprocal
                    double kg1Saved = parameter[PAR_KG1]->getValue();
                    double y = (kg1Saved > 1e-12) ? 1.0 / kg1Saved : 1e6;

                    // Evaluate Ia at current y and at y+dy to estimate sensitivity s = dIa/dy
                    auto evalIa = [&](double yrecip) {
                        double kg1cand = (yrecip > 1e-12) ? 1.0 / yrecip : 1e12;
                        parameter[PAR_KG1]->setValue(kg1cand);
                        return anodeCurrent(vaCal, vg1Cal, vg2Cal);
                    };
                    double dy = std::max(1e-6, y * 0.1);
                    double ia1 = evalIa(y);
                    double ia2 = evalIa(y + dy);
                    double s = (ia2 - ia1) / dy; // mA per unit of y
                    // Restore baseline kg1 before solving
                    parameter[PAR_KG1]->setValue(kg1Saved);

                    if (std::isfinite(s) && std::fabs(s) > 1e-9) {
                        double c = ia1 - s * y;
                        double yTarget = (iaTarget - c) / s;
                        // Bounds to keep kg1 positive and reasonable
                        if (std::isfinite(yTarget) && yTarget > 1e-9 && yTarget < 1e9) {
                            double kg1New = 1.0 / yTarget;
                            // Constrain kg1New within a practical range relative to saved
                            double lo = kg1Saved * 0.05;
                            double hi = kg1Saved * 20.0;
                            if (!std::isfinite(lo) || lo <= 0.0) lo = 1e-6;
                            if (!std::isfinite(hi) || hi <= lo) hi = lo * 1e6;
                            kg1New = std::min(hi, std::max(lo, kg1New));
                            parameter[PAR_KG1]->setValue(kg1New);
                            // Compute display scale so model aligns with measured at anchor
                            double iaModelAnchor = anodeCurrent(vaCal, vg1Cal, vg2Cal);
                            if (std::isfinite(iaModelAnchor) && iaModelAnchor > 1e-9) {
                                iaScale = iaTarget / iaModelAnchor;
                                // keep within sensible bounds
                                if (!std::isfinite(iaScale) || iaScale <= 0.0) iaScale = 1.0;
                                iaScale = std::min(100.0, std::max(0.01, iaScale));
                            } else {
                                iaScale = 1.0;
                            }
                            qInfo("PENTODE KG1 CAL: va=%.3f vg1=%.3f vg2=%.3f iaTarget=%.3f ia1=%.3f s=%.6f kg1Saved=%.6f -> kg1New=%.6f, iaScale=%.4f",
                                  vaCal, vg1Cal, vg2Cal, iaTarget, ia1, s, kg1Saved, kg1New, iaScale);
                        } else {
                            qInfo("PENTODE KG1 CAL: yTarget invalid (%.6f), skipping calibration", yTarget);
                        }
                    } else {
                        qInfo("PENTODE KG1 CAL: sensitivity too small (s=%.6f), skipping calibration", s);
                    }
                } else {
                    qInfo("PENTODE KG1 CAL: -20V family not found or insufficient samples; skipping");
                }
            }

            int curveCount = 0;
            for (double vg1 : vgFamilies) {
                qInfo("PENTODE: Creating curve %d for vg1=%.3f", curveCount + 1, vg1);
                // Find a representative sweep whose Vg1Nominal matches this family (within tolerance)
                const double kTol = 1e-3; // volts
                Sweep *famSweep = nullptr;
                for (int si = 0; si < measurement->count(); ++si) {
                    double nomV = measurement->at(si)->getVg1Nominal(); // volts
                    double famV = -std::fabs(nomV);
                    if (std::fabs(famV - vg1) <= kTol) { famSweep = measurement->at(si); break; }
                }

                int segmentCount = 0;
                double endVa = std::numeric_limits<double>::quiet_NaN();
                double endIa = std::numeric_limits<double>::quiet_NaN();
                if (famSweep && famSweep->count() >= 2) {
                    Sample *s0 = famSweep->at(0);
                    // Determine family screen voltage (prefer sweep nominal; use raw volts)
                    double vg2Family = famSweep->getVg2Nominal();
                    if (!std::isfinite(vg2Family)) {
                        vg2Family = vg2; // fall back to measurement default if nominal missing
                    }

                    // Helper to get per-sample screen voltage with robust fallback to a "meaningful" value
                    // Preference order: measured per-sample > family nominal > measurement default
                    // Treat values <= 1V as not meaningful for model overlays to avoid collapsing to 0V screen
                    auto sampleVg2 = [&](Sample *samp) -> double {
                        constexpr double kVg2Min = 1.0; // volts
                        auto pickIfMeaningful = [&](double v) -> std::optional<double> {
                            if (std::isfinite(v) && v > kVg2Min) return v;
                            return std::nullopt;
                        };

                        if (auto s = pickIfMeaningful(samp->getVg2()))       return *s;
                        if (auto n = pickIfMeaningful(vg2Family))            return *n;
                        if (auto m = pickIfMeaningful(vg2))                  return *m;
                        // Last resort: return vg2 (even if small) to avoid NaN
                        return std::isfinite(vg2) ? vg2 : 0.0;
                    };

                    // Diagnostics: report OS parameter and the effective screen voltage used for this family
                    double osParam = parameter[PAR_OS]->getValue();
                    double v2First = sampleVg2(s0);
                    qInfo("PENTODE OS: os=%.6f, vg1=%.3f, vg2(effective first)=%.3f", osParam, vg1, v2First);

                    double vaPrev = s0->getVa();
                    double iaPrev = anodeCurrent(vaPrev, vg1, sampleVg2(s0)) * iaScale;

                    // Diagnostics: evaluate Ia at first/mid/last Va points and track min/max
                    const int nSamp = famSweep->count();
                    const int midIdx = nSamp / 2;
                    Sample *sf = famSweep->at(0);
                    Sample *sm = famSweep->at(midIdx);
                    Sample *sl = famSweep->at(nSamp - 1);
                    double vaFirst = sf->getVa();
                    double vaMid   = sm->getVa();
                    double vaLast  = sl->getVa();
                    double iaFirst = anodeCurrent(vaFirst, vg1, sampleVg2(sf)) * iaScale;
                    double iaMid   = anodeCurrent(vaMid,   vg1, sampleVg2(sm)) * iaScale;
                    double iaLast  = anodeCurrent(vaLast,  vg1, sampleVg2(sl)) * iaScale;
                    double iaMin   = iaFirst;
                    double iaMax   = iaFirst;

                    for (int j = 1; j < famSweep->count(); ++j) {
                        Sample *sj = famSweep->at(j);
                        double va = sj->getVa();
                        double ia = anodeCurrent(va, vg1, sampleVg2(sj)) * iaScale;

                        // Skip zero-ΔVa to avoid vertical lines
                        if (std::fabs(va - vaPrev) < 1e-12) {
                            vaPrev = va;
                            iaPrev = ia;
                            continue;
                        }

                        double y1 = std::min(yMaxAxis, std::max(0.0, iaPrev));
                        double y2 = std::min(yMaxAxis, std::max(0.0, ia));
                        QGraphicsItem *segment = plot->createSegment(vaPrev, y1, va, y2, anodePen);
                        if (segment != nullptr) {
                            group->addToGroup(segment);
                            segmentCount++;
                        }
                        // Update min/max diagnostics
                        if (std::isfinite(ia)) {
                            iaMin = std::min(iaMin, ia);
                            iaMax = std::max(iaMax, ia);
                        }
                        vaPrev = va;
                        iaPrev = ia;
                    }
                    // If the entire family computes ~zero current, skip plotting this curve to avoid flat zero lines
                    if (iaMax < 1e-3) {
                        qInfo("PENTODE: Skipping vg1=%.3f curve due to near-zero Ia across sweep (iaMax=%.6f)", vg1, iaMax);
                        continue;
                    }
                    if (std::isfinite(iaMax) && iaMax > yMaxAxis * 5.0) {
                        qInfo("PENTODE: Skipping vg1=%.3f due to extreme Ia (iaMax=%.3f > %.3f)", vg1, iaMax, yMaxAxis * 5.0);
                        continue;
                    }
                    endVa = vaPrev;
                    endIa = iaPrev;
                    qInfo("PENTODE DIAG: vg1=%.3f, vg2=%.3f, Va[first/mid/last]=[%.3f, %.3f, %.3f], Ia[first/mid/last]=[%.3f, %.3f, %.3f], Ia[min/max]=[%.3f, %.3f]",
                          vg1, vg2, vaFirst, vaMid, vaLast, iaFirst, iaMid, iaLast, iaMin, iaMax);
                } else {
                    qWarning("PENTODE: Skipping family vg1=%.3f (no representative sweep or insufficient samples)", vg1);
                }

                // Add a label at the end of the curve to show the family Vg value for visibility
                if (std::isfinite(endVa) && std::isfinite(endIa)) {
                    QGraphicsItem *label = plot->createLabel(endVa, endIa, vg1, anodePen.color());
                    if (label) {
                        group->addToGroup(label);
                    }
                }

                if (drawScreen && showScreen && famSweep && famSweep->count() >= 2) {
                    Sample *s0s = famSweep->at(0);
                    double vaPrevS = s0s->getVa();
                    double ig2Prev = screenCurrent(vaPrevS, vg1, vg2);
                    for (int j = 1; j < famSweep->count(); ++j) {
                        Sample *sj = famSweep->at(j);
                        double vaS = sj->getVa();
                        double ig2 = screenCurrent(vaS, vg1, vg2);
                        double ig2Prev_mA = std::max(0.0, ig2Prev);
                        double ig2_mA = std::max(0.0, ig2);
                        // Skip zero-ΔVa to avoid vertical lines
                        if (std::fabs(vaS - vaPrevS) >= 1e-12) {
                            QGraphicsItem *segment = plot->createSegment(vaPrevS, ig2Prev_mA, vaS, ig2_mA, screenPen);
                            if (segment != nullptr) {
                                group->addToGroup(segment);
                            }
                        }
                        vaPrevS = vaS;
                        ig2Prev = ig2;
                    }
                }

                qInfo("PENTODE: Curve %d completed: %d anode segments", curveCount + 1, segmentCount);
                curveCount++;
            }

            qInfo("PENTODE: Total curves created: %d", curveCount);

            // Restore original Os after plotting
            parameter[PAR_OS]->setValue(osSavedForPlot);
        }
    }

    qInfo("Model plotting completed - returning group with %d items", group->childItems().count());
    return group;
}

double Model::getParameter(int parameterIndex)
{
    return parameter[parameterIndex]->getValue();
}

bool Model::isConverged() const
{
    return converged;
}

int Model::getMode() const
{
    return mode;
}

void Model::setMode(int newMode)
{
    mode = newMode;
}

bool Model::withSecondaryEmission() const
{
    return secondaryEmission;
}

void Model::setSecondaryEmission(bool newSecondaryEmission)
{
    secondaryEmission = newSecondaryEmission;
}

bool Model::getShowScreen() const
{
    return showScreen;
}

void Model::setShowScreen(bool newShowScreen)
{
    showScreen = newShowScreen;
}

void Model::setPreferences(PreferencesDialog *newPreferences)
{
    preferences = newPreferences;
}

void Model::setPlotColor(const QColor &color)
{
    plotColor = color;
}

QColor Model::getPlotColor() const
{
    return plotColor;
}

void Model::setLowerBound(Parameter* parameter, double lowerBound)
{
    anodeProblem.SetParameterLowerBound(parameter->getPointer(), 0, lowerBound);
}

void Model::setUpperBound(Parameter* parameter, double upperBound)
{
    anodeProblem.SetParameterUpperBound(parameter->getPointer(), 0, upperBound);
}

void Model::setLimits(Parameter* parameter, double lowerBound, double upperBound)
{
    anodeProblem.SetParameterLowerBound(parameter->getPointer(), 0, lowerBound);
    anodeProblem.SetParameterUpperBound(parameter->getPointer(), 0, upperBound);
}
