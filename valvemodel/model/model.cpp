#include "model.h"
#include "estimate.h"
#include "../data/sweep.h"

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

        int samples = sweep->count();
            for (int i = 0; i < samples; i++) {
            Sample *sample = sweep->at(i);

            addSample(sample->getVa(), sample->getIa(), sample->getVg1(), sample->getVg2(), sample->getIg2());
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
    parameter[PAR_KG1]->setValue(estimate->getKg1());
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
    anodePen.setColor(QColor::fromRgb(255, 0, 0));
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

            qInfo("STORED VALUES: vgStart=%.3f, vgStop=%.3f, vgStep=%.3f", vgStart, vgStop, vgStep);
            qInfo("Measurement has %d sweeps", measurement->count());

            // If range is invalid, calculate from actual sweep data
            if (vgStart == 0.0 && vgStop == 0.0) {
                if (measurement->count() > 0) {
                    // Find the actual min and max grid voltages from sweeps
                    double minVg = 0.0;
                    double maxVg = 0.0;
                    bool first = true;
                    bool validData = false;

                    for (int i = 0; i < measurement->count(); i++) {
                        double sweepVg = -measurement->at(i)->getVg1Nominal();
                        qInfo("Sweep %d: Vg1Nominal=%.3f, negated=%.3f", i, measurement->at(i)->getVg1Nominal(), sweepVg);

                        // Skip invalid voltage values (outside reasonable range)
                        if (sweepVg >= -10.0 && sweepVg <= 10.0) {
                            if (first || sweepVg < minVg) minVg = sweepVg;
                            if (first || sweepVg > maxVg) maxVg = sweepVg;
                            first = false;
                            validData = true;
                        }
                    }

                    qInfo("After scanning: minVg=%.3f, maxVg=%.3f, validData=%s", minVg, maxVg, validData ? "true" : "false");

                    if (validData && (maxVg - minVg) <= 10.0) {
                        vgStart = minVg;
                        vgStop = maxVg;
                        qInfo("Calculated grid range from sweep data: start=%.3f, stop=%.3f", vgStart, vgStop);
                    } else {
                        // Fallback to reasonable defaults
                        vgStart = 0.0;
                        vgStop = -4.0;
                        qInfo("No valid sweep data found, using defaults: start=%.3f, stop=%.3f", vgStart, vgStop);
                    }
                }
            }

            // If step is 0 or invalid, calculate from actual sweep data
            if (vgStep == 0.0 || vgStep > (vgStop - vgStart)) {
                if (measurement->count() > 1) {
                    double firstVg = -measurement->at(0)->getVg1Nominal();
                    double secondVg = -measurement->at(1)->getVg1Nominal();

                    qInfo("First sweep Vg1Nominal=%.3f, negated=%.3f", measurement->at(0)->getVg1Nominal(), firstVg);
                    qInfo("Second sweep Vg1Nominal=%.3f, negated=%.3f", measurement->at(1)->getVg1Nominal(), secondVg);

                    // Validate the calculated step is reasonable
                    double calculatedStep = secondVg - firstVg;
                    qInfo("Calculated step: %.3f", calculatedStep);

                    if (calculatedStep > -10.0 && calculatedStep < 10.0 && calculatedStep != 0.0) {
                        vgStep = calculatedStep;
                        qInfo("Using calculated grid step: %.3f", vgStep);
                    } else {
                        vgStep = -0.5; // Default fallback for negative steps
                        qInfo("Invalid calculated step (%.3f), using default: %.3f", calculatedStep, vgStep);
                    }
                } else {
                    vgStep = -0.5; // Default fallback
                    qInfo("Using default grid step: %.3f", vgStep);
                }
            }

            if (sweep != nullptr) {
                vgStart = -sweep->getVg1Nominal();
                vgStop = -sweep->getVg1Nominal();
            }

            double vg2 = measurement->getScreenStart();

            qInfo("Final grid voltage range: start=%.3f, stop=%.3f, step=%.3f", vgStart, vgStop, vgStep);
            qInfo("Screen voltage: %.3f", vg2);

            double vg1 = vgStart;
            int curveCount = 0;
            while ( vg1 <= vgStop) {
                qInfo("Creating curve %d for vg1=%.3f", curveCount + 1, vg1);
                double vaStart = measurement->getAnodeStart();
                double vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                qInfo("Anode voltage range: start=%.1f, stop=%.1f, inc=%.3f", vaStart, vaStop, vaInc);

                double vaPrev = vaStart;
                double iaPrev = anodeCurrent(vaStart, -vg1, vg2);

                double va = vaStart + vaInc;
                int segmentCount = 0;
                while (va < vaStop) {
                    double ia = anodeCurrent(va, -vg1, vg2);
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
                vg1 += vgStep;
                curveCount++;
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

            // If range is invalid, calculate from actual sweep data
            if (vgStart == 0.0 && vgStop == 0.0) {
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
                    double firstVg = -measurement->at(0)->getVg1Nominal();
                    double secondVg = -measurement->at(1)->getVg1Nominal();

                    qInfo("PENTODE: First sweep Vg1Nominal=%.3f, negated=%.3f", measurement->at(0)->getVg1Nominal(), firstVg);
                    qInfo("PENTODE: Second sweep Vg1Nominal=%.3f, negated=%.3f", measurement->at(1)->getVg1Nominal(), secondVg);

                    // Validate the calculated step is reasonable
                    double calculatedStep = secondVg - firstVg;
                    qInfo("PENTODE: Calculated step: %.3f", calculatedStep);

                    if (calculatedStep > -10.0 && calculatedStep < 10.0 && calculatedStep != 0.0) {
                        vgStep = calculatedStep;
                        qInfo("PENTODE: Using calculated grid step: %.3f", vgStep);
                    } else {
                        vgStep = -0.5; // Default fallback for negative steps
                        qInfo("PENTODE: Invalid calculated step (%.3f), using default: %.3f", calculatedStep, vgStep);
                    }
                } else {
                    vgStep = -0.5; // Default fallback
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
                double iaPrev = anodeCurrent(vaStart, -vg1, vg2);

                double va = vaStart + vaInc;
                int segmentCount = 0;
                while (va < vaStop) {
                    double ia = anodeCurrent(va, -vg1, vg2);
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
                    double ig2Prev = screenCurrent(vaStart, -vg1, vg2);

                    va = vaStart + vaInc;
                    while (va < vaStop) {
                        double ig2 = screenCurrent(va, -vg1, vg2);
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
