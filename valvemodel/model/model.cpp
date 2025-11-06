#include "model.h"
#include "estimate.h"
#include "../data/sweep.h"
#include "../data/sample.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

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
    parameter[PAR_OS] = new Parameter("Os:", 0.01);

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

        // Determine if this sweep's grid values (Vg1) are incorrectly positive.
        // If the first meaningful (finite, non-zero) Vg1 sample is > 0, we flip
        // the sign of Vg1 for all samples when adding to the solver. This does
        // not mutate the Measurement; it only corrects the data fed to the model.
        bool flipVg1 = false;
        {
            int probeCount = sweep->count();
            for (int pi = 0; pi < probeCount; ++pi) {
                Sample *probe = sweep->at(pi);
                double vgProbe = probe->getVg1();
                // If per-sample vg is unavailable or ~0, use the sweep's nominal grid for flip detection
                if (!std::isfinite(vgProbe) || std::fabs(vgProbe) <= 1e-9) {
                    vgProbe = sweep->getVg1Nominal();
                }
                if (std::isfinite(vgProbe) && std::fabs(vgProbe) > 1e-9) {
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

            // Use sample vg1 when available; otherwise fall back to the sweep's nominal grid
            double vg1raw = sample->getVg1();
            bool usedNominal = false;
            if (!std::isfinite(vg1raw) || std::fabs(vg1raw) < 1e-6) {
                vg1raw = sweep->getVg1Nominal();
                usedNominal = true;
            }

            // Force strictly non-positive for solver: treat any stored magnitude as negative bias
            const double vg1Corrected = -std::fabs(vg1raw);

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
    //parameter[PAR_OS]->setValue(estimate->getOs());
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
                            double vg = sample->getVg1();
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
                            double vgNominal = sweep->getVg1Nominal();
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
                            diff = std::fabs(candidateVg - firstVg);
                        }
                        qInfo("GRID STEP FALLBACK: comparing sweep %d Vg1Nominal=%.6f (diff vs sweep 0 = %.6f)",
                              sweepIndex, candidateVg, diff);
                        if (!std::isfinite(candidateVg)) {
                            continue;
                        }
                        const double stepCandidate = std::fabs(candidateVg - firstVg);
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
                vgStart = -std::fabs(nominal);
                vgStop = -std::fabs(nominal);
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

            // Set axes only if scene is empty to preserve existing measurement/Designer overlays
            double vaStart = measurement->getAnodeStart();
            double vaStop = measurement->getAnodeStop();
            if (!std::isfinite(vaStart)) vaStart = 0.0;
            if (!std::isfinite(vaStop) || vaStop <= vaStart) vaStop = std::max(vaStart + 10.0, 250.0);
            if (plot->getScene()->items().isEmpty()) {
                // Estimate max current at 0V grid (least negative) and highest anode voltage
                double iaEst_mA = anodeCurrent(vaStop, 0.0, vg2) * 1000.0;
                if (!std::isfinite(iaEst_mA) || iaEst_mA <= 0.0) iaEst_mA = 50.0;
                double yStop = std::max(50.0, iaEst_mA * 1.1);
                double xMajor = std::max(10.0, (vaStop - vaStart) / 10.0);
                double yMajor = std::max(5.0, yStop / 10.0);
                plot->setAxes(0.0, vaStop, xMajor, 0.0, yStop, yMajor);
            }

            double vg1 = vgStart;
            int curveCount = 0;
            while ( vg1 <= vgStop) {
                qInfo("TRIODE LOOP: vg1=%.3f, vgStart=%.3f, vgStop=%.3f, vgStep=%.3f, condition (%.3f <= %.3f) = %s",
                      vg1, vgStart, vgStop, vgStep, vg1, vgStop, (vg1 <= vgStop) ? "true" : "false");

                qInfo("Creating curve %d for vg1=%.3f", curveCount + 1, vg1);
                vaStart = measurement->getAnodeStart();
                vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                qInfo("Anode voltage range: start=%.1f, stop=%.1f, inc=%.3f", vaStart, vaStop, vaInc);

                double vaPrev = vaStart;
                double iaPrev = anodeCurrent(vaStart, vg1, vg2);

                double va = vaStart + vaInc;
                int segmentCount = 0;
                while (va < vaStop) {
                    qInfo("TRIODE: Calculating current for va=%.3f, vg1=%.3f", va, vg1);
                    double ia = anodeCurrent(va, vg1, vg2);
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
            double vgStart = measurement->getGridStart();
            double vgStop = measurement->getGridStop();
            double vgStep = measurement->getGridStep();

            qInfo("PENTODE STORED VALUES: vgStart=%.3f, vgStop=%.3f, vgStep=%.3f", vgStart, vgStop, vgStep);
            qInfo("PENTODE: Measurement has %d sweeps", measurement->count());

            // If range is invalid or we're plotting all sweeps, calculate from actual sweep data
            if ((vgStart == 0.0 && vgStop == 0.0) || sweep == nullptr) {
                if (measurement->count() > 0) {
                    // Find the actual min and max grid voltages from sweeps
                    double minVg = 0.0;
                    double maxVg = 0.0;
                    bool first = true;
                    bool validData = false;

                    for (int i = 0; i < measurement->count(); i++) {
                        double sweepVg = -measurement->at(i)->getVg1Nominal();
                        qInfo("PENTODE Sweep %d: Vg1Nominal=%.3f, negated=%.3f", i, measurement->at(i)->getVg1Nominal(), sweepVg);

                        // Skip invalid voltage values (outside reasonable range)
                        if (sweepVg >= -10.0 && sweepVg <= 10.0) {
                            if (first || sweepVg < minVg) minVg = sweepVg;
                            if (first || sweepVg > maxVg) maxVg = sweepVg;
                            first = false;
                            validData = true;
                        }
                    }

                    qInfo("PENTODE: After scanning: minVg=%.3f, maxVg=%.3f, validData=%s", minVg, maxVg, validData ? "true" : "false");

                    if (validData && (maxVg - minVg) <= 10.0) {
                        vgStart = minVg;
                        vgStop = maxVg;
                        qInfo("PENTODE: Calculated grid range from sweep data: start=%.3f, stop=%.3f", vgStart, vgStop);
                    } else {
                        // Fallback to reasonable defaults
                        vgStart = 0.0;
                        vgStop = -4.0;
                        qInfo("PENTODE: No valid sweep data found, using defaults: start=%.3f, stop=%.3f", vgStart, vgStop);
                    }
                }
            }

            // If step is 0 or invalid, calculate from actual sweep data
            if (vgStep == 0.0 || vgStep > (vgStop - vgStart)) {
                if (measurement->count() > 1) {
                    double firstVg = measurement->at(0)->getVg1Nominal();  // Already negated
                    double secondVg = measurement->at(1)->getVg1Nominal(); // Already negated

                    qInfo("PENTODE: First sweep Vg1Nominal=%.3f", measurement->at(0)->getVg1Nominal());
                    qInfo("PENTODE: Second sweep Vg1Nominal=%.3f", measurement->at(1)->getVg1Nominal());

                    // Calculate step - ensure it's positive for ascending loop
                    double calculatedStep = fabs(secondVg - firstVg);
                    qInfo("PENTODE: Calculated step: %.3f", calculatedStep);

                    if (calculatedStep > 0.0 && calculatedStep < 10.0 && calculatedStep != 0.0) {
                        vgStep = calculatedStep;
                        qInfo("PENTODE: Using calculated grid step: %.3f", vgStep);
                    } else {
                        vgStep = 0.5; // Default fallback for positive steps
                        qInfo("PENTODE: Invalid calculated step (%.3f), using default: %.3f", calculatedStep, vgStep);
                    }
                } else {
                    vgStep = 0.5; // Default fallback
                    qInfo("PENTODE: Using default grid step: %.3f", vgStep);
                }
            }

            if (sweep != nullptr) {
                vgStart = -sweep->getVg1Nominal();
                vgStop = -sweep->getVg1Nominal();
            }

            double vg2 = measurement->getScreenStart();

            qInfo("PENTODE: Final grid voltage range: start=%.3f, stop=%.3f, step=%.3f", vgStart, vgStop, vgStep);
            qInfo("PENTODE: Screen voltage: %.3f", vg2);

            double vg1 = vgStart;
            int curveCount = 0;
            while ( vg1 <= vgStop) {
                qInfo("PENTODE: Creating curve %d for vg1=%.3f", curveCount + 1, vg1);
                double vaStart = measurement->getAnodeStart();
                double vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                double vaPrev = vaStart;
                double iaPrev = anodeCurrent(vaStart, vg1, vg2);

                double va = vaStart + vaInc;
                int segmentCount = 0;
                while (va < vaStop) {
                    double ia = anodeCurrent(va, vg1, vg2);
                    QGraphicsItem *segment = plot->createSegment(vaPrev, iaPrev, va, ia, anodePen);

                    if (segment != nullptr) {
                        group->addToGroup(segment);
                        segmentCount++;
                    } else {
                        qWarning("PENTODE: Failed to create segment for va=%.3f, ia=%.3f", va, ia);
                    }

                    vaPrev = va;
                    iaPrev = ia;

                    va += vaInc;
                }

                if (showScreen) {
                    vaPrev = vaStart;
                    double ig2Prev = screenCurrent(vaStart, vg1, vg2); // assume same unit convention

                    va = vaStart + vaInc;
                    while (va < vaStop) {
                        double ig2 = screenCurrent(va, vg1, vg2);
                        QGraphicsItem *segment = plot->createSegment(vaPrev, ig2Prev, va, ig2, screenPen);

                        if (segment != nullptr) {
                            group->addToGroup(segment);
                        } else {
                            qWarning("PENTODE: Failed to create screen segment for va=%.3f, ig2=%.3f", va, ig2);
                        }

                        vaPrev = va;
                        ig2Prev = ig2;

                        va += vaInc;
                    }
                }

                qInfo("PENTODE: Curve %d completed: %d anode segments, %s screen segments",
                      curveCount + 1, segmentCount, showScreen ? "with" : "without");
                vg1 += vgStep;
                curveCount++;
            }

            qInfo("PENTODE: Total curves created: %d", curveCount);
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
