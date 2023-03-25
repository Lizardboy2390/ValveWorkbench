#include "estimate.h"
#include "model.h"
#include "../data/sweep.h"
#include "linearsolver.h"
#include "quadraticsolver.h"

Estimate::Estimate()
{

}

void Estimate::estimateTriode(Measurement *measurement) {
    estimateMu(measurement);
    estimateKg1X(measurement);
    estimateKp(measurement);
    // There does not appear to be a meangingful way of estimating Vct, Kvb or Kvb1 and so fixed values will be used
    //estimateKvbKvb1(measurement);
}


void Estimate::estimatePentode(Measurement *measurement, CohenHelieTriode *triodeModel, int modelType, bool secondaryEmission)
{
    mu = triodeModel->getParameter(PAR_MU);
    x = triodeModel->getParameter(PAR_X);
    kg1 = triodeModel->getParameter(PAR_KG1);
    kp = triodeModel->getParameter(PAR_KP);
    kvb = triodeModel->getParameter(PAR_KVB);
    kvb1 = triodeModel->getParameter(PAR_KVB1);
    vct = triodeModel->getParameter(PAR_VCT);

    estimateKg2(measurement, triodeModel);
    estimateA(measurement, triodeModel);
    //estimateAlphaBeta(measurement, triodeModel, modelType);
    if (secondaryEmission) {
        // Estimate S, ap, omega, nu and lambda
    }
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

double Estimate::getBlend() const
{
    return blend;
}

void Estimate::setBlend(double newBlend)
{
    blend = newBlend;
}

double Estimate::getOnset() const
{
    return onset;
}

void Estimate::setOnset(double newOnset)
{
    onset = newOnset;
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
                    solver->addSample(log(sample->getVa() / mu + sample->getVg1()), log(sample->getIa()));
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
                }
            }
        }
    }

    solver->solve();

    double a = solver->getA();
    kvb1 = solver->getB();
    kvb = solver->getC();
}

/**
 * @brief Estimate::estimateKg2
 * @param measurement
 * @param triodeModel
 *
 * Estimating Kg2 is trivial as it simply involves a linear calculation with regard to Epk at when Va is high.
 * This is easily achieved by taking the average value of Kg2 for the sample at the end of each sweep.
 */
void Estimate::estimateKg2(Measurement *measurement, CohenHelieTriode *triodeModel)
{
    int sweeps = measurement->count();

    double kg2Sum = 0.0;

    for (int sw = 0; sw < sweeps; sw++) {
        Sweep *sweep = measurement->at(sw);
        Sample *sample = sweep->at(sweep->count() - 1);
        kg2Sum += triodeModel->cohenHelieEpk(sample->getVg2(), sample->getVg1()) / sample->getIg2();
    }

    kg2 = kg2Sum / sweeps;
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

    int sweeps = measurement->count();
    for (int sw = 0; sw < sweeps; sw++) {
        LinearSolver solver(0.0, 0.0);

        Sweep *sweep = measurement->at(sw);

        int samples = sweep->count();
        for (int sa = 0; sa < samples; sa++) {
            Sample *sample = sweep->at(sa);
            if (sample->getVa() < vaThreshold) {
                double y = sample->getIg2() * kg2 / triodeModel->cohenHelieEpk(sample->getVg2(), sample->getVg1()) - 1.0;

                if (modelType == COHEN_HELIE_PENTODE) {
                    solver.addSample(sample->getVa(), 1.0 / y);
                } else {
                    solver.addSample(pow(sample->getVa(), 1.5), std::log(y));
                }
            }
        }

        solver.solve();

        if (modelType == COHEN_HELIE_PENTODE) {
            alphasAve += 1.0 / solver.getB();
            betaAve += alphasAve * solver.getA();
        } else {
            alphasAve += std::exp(solver.getB());
            betaAve += std::pow(-1.0 * solver.getA(), 2.0 / 3.0);
        }
    }

    alphas = alphasAve / sweeps;
    beta = betaAve / sweeps;

    alpha = 1.0 - (1 + alphas) * triodeModel->getParameter(PAR_KG1) / kg2;
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
    addProperty(properties, "Kg1", QString("%1").arg(kg1));
    addProperty(properties, "X", QString("%1").arg(x));
    addProperty(properties, "Kp", QString("%1").arg(kp));
    addProperty(properties, "Kvb", QString("%1").arg(kvb));
    addProperty(properties, "Kvb1", QString("%1").arg(kvb1));
    addProperty(properties, "vct", QString("%1").arg(vct));
}
