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
    parameter[PAR_A] = new Parameter("A:", 0.0);
    parameter[PAR_ALPHA] = new Parameter("Alpha:", 0.0);
    parameter[PAR_BETA] = new Parameter("Beta:", 0.0);
    parameter[PAR_GAMMA] = new Parameter("Gamma:", 1.0);

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

double Model::screenCurrent(double va, double vg1, double vg2)
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
        Solve(options, &problem, &summary);
    } else if (mode == SCREEN_MODE) {
        Solve(options, &screenProblem, &summary);
    }

    converged = summary.termination_type == ceres::CONVERGENCE;

    qInfo(summary.FullReport().c_str());
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

QGraphicsItemGroup *Model::plotModel(Plot *plot, Measurement *measurement)
{
    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    QPen anodePen;
    anodePen.setColor(QColor::fromRgb(255, 0, 0));
    QPen screenPen;
    screenPen.setColor(QColor::fromRgb(0, 0, 255));

    int deviceType = measurement->getDeviceType();
    int testType = measurement->getTestType();
    if (deviceType == TRIODE) {
        if (testType == ANODE_CHARACTERISTICS) {
            double vgStart = measurement->getGridStart();
            double vgStop = measurement->getGridStop();
            double vgStep = measurement->getGridStep();

            double vg2 = measurement->getScreenStart();

            double vg1 = vgStart;
            while ( vg1 <= vgStop) {
                double vaStart = measurement->getAnodeStart();
                double vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                double vaPrev = vaStart;
                double iaPrev = anodeCurrent(vaStart, -vg1, vg2);

                double va = vaStart + vaInc;
                while (va < vaStop) {
                    double ia = anodeCurrent(va, -vg1, vg2);
                    group->addToGroup(plot->createSegment(vaPrev, iaPrev, va, ia, anodePen));

                    vaPrev = va;
                    iaPrev = ia;

                    va += vaInc;
                }

                vg1 += vgStep;
            }
        }
    } else if (deviceType == PENTODE) {
        if (testType == ANODE_CHARACTERISTICS) {
            double vgStart = measurement->getGridStart();
            double vgStop = measurement->getGridStop();
            double vgStep = measurement->getGridStep();

            double vg2 = measurement->getScreenStart();

            double vg1 = vgStart;
            while ( vg1 <= vgStop) {
                double vaStart = measurement->getAnodeStart();
                double vaStop = measurement->getAnodeStop();
                double vaInc = (vaStop - vaStart) / 50;

                double vaPrev = vaStart;
                double iaPrev = anodeCurrent(vaStart, -vg1, vg2);

                double va = vaStart + vaInc;
                while (va < vaStop) {
                    double ia = anodeCurrent(va, -vg1, vg2);
                    group->addToGroup(plot->createSegment(vaPrev, iaPrev, va, ia, anodePen));

                    vaPrev = va;
                    iaPrev = ia;

                    va += vaInc;
                }

                vaPrev = vaStart;
                double ig2Prev = screenCurrent(vaStart, -vg1, vg2);

                va = vaStart + vaInc;
                while (va < vaStop) {
                    double ig2 = screenCurrent(va, -vg1, vg2);
                    group->addToGroup(plot->createSegment(vaPrev, ig2Prev, va, ig2, screenPen));

                    vaPrev = va;
                    ig2Prev = ig2;

                    va += vaInc;
                }

                vg1 += vgStep;
            }
        }
    }

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
