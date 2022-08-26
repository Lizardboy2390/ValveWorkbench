#include "gardinertriode.h"

struct GardinerTriodeResidual {
    GardinerTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const kvb, const T* const kvb1, const T* const vct, const T* const a, const T* const mu, T* residual) const {
        T ep = va_ * (1.0 / mu[0] + (vg_ + vct[0]) / sqrt(kvb[0] + kvb1[0] * va_ + va_ * va_));

        if (ep < 0.0) {
            ep = mu[0] - mu[0];
        }

        T ia = pow(ep, a[0]) / kg[0];
        residual[0] = ia_ - ia;
        return !(isnan(ia) || isinf(ia));
    }

private:
    const double va_;
    const double vg_;
    const double ia_;
};

GardinerTriode::GardinerTriode()
{
    parameter[PAR_KG1] = new Parameter("Kg:", 0.7);
    parameter[PAR_VCT] = new Parameter("Vct:", 0.2);
    parameter[PAR_X] = new Parameter("Alpha:", 1.5);
    parameter[PAR_MU] = new Parameter("Mu:", 100.0);
    parameter[PAR_KVB] = new Parameter("Kvb:", 300.0);
    parameter[PAR_KVB1] = new Parameter("Kvb1:", 30.0);
}

void GardinerTriode::addSample(double va, double ia, double vg1, double vg2)
{
    problem.AddResidualBlock(
        new AutoDiffCostFunction<GardinerTriodeResidual, 1, 1, 1, 1, 1, 1, 1>(
            new GardinerTriodeResidual(va, vg1, ia)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_KVB]->getPointer(),
        parameter[PAR_KVB1]->getPointer(),
        parameter[PAR_VCT]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer());
}

double GardinerTriode::anodeCurrent(double va, double vg1, double vg2)
{
    return gardinerCurrent(va, vg1,
        parameter[PAR_KG1]->getValue(),
        parameter[PAR_KVB]->getValue(),
        parameter[PAR_KVB1]->getValue(),
        parameter[PAR_VCT]->getValue(),
        parameter[PAR_X]->getValue(),
        parameter[PAR_MU]->getValue());
}

void GardinerTriode::fromJson(QJsonObject source)
{
    if (source.contains("kg") && source["kg"].isDouble()) {
        parameter[PAR_KG1]->setValue(source["kg"].toDouble());
    }

    if (source.contains("mu") && source["mu"].isDouble()) {
        parameter[PAR_MU]->setValue(source["mu"].toDouble());
    }

    if (source.contains("alpha") && source["alpha"].isDouble()) {
        parameter[PAR_X]->setValue(source["alpha"].toDouble());
    }

    if (source.contains("vct") && source["vct"].isDouble()) {
        parameter[PAR_VCT]->setValue(source["vct"].toDouble());
    }

    if (source.contains("kvb") && source["kvb"].isDouble()) {
        parameter[PAR_KVB]->setValue(source["kvb"].toDouble());
    }

    if (source.contains("kvb1") && source["kvb1"].isDouble()) {
        parameter[PAR_KVB1]->setValue(source["kvb1"].toDouble());
    }
}

void GardinerTriode::toJson(QJsonObject &destination, double vg1Max, double vg2Max)
{
    QJsonObject model;
    model["kg"] = parameter[PAR_KG1]->getValue();
    model["mu"] = parameter[PAR_MU]->getValue();
    model["alpha"] = parameter[PAR_X]->getValue();
    model["vct"] = parameter[PAR_VCT]->getValue();
    model["kvb"] = parameter[PAR_KVB]->getValue();
    model["kvb1"] = parameter[PAR_KVB1]->getValue();

    QJsonObject triode;
    triode["vg1Max"] = vg1Max;
    triode["gardiner"] = model;

    destination["triode"] = triode;
}

void GardinerTriode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_VCT]); i++;
}

QString GardinerTriode::getName()
{
    return QString("Gardiner Triode");
}

void GardinerTriode::setOptions()
{
    options.max_num_iterations = 100;
    options.minimizer_progress_to_stdout = true;

    setLowerBound(parameter[PAR_KG1], 0.0000001); // Kg > 0
    setLimits(parameter[PAR_X], 1.0, 2.0); // 1.0 <= alpha <= 2.0
    setLimits(parameter[PAR_MU], 1.0, 1000.0); // 1.0 <= mu <= 1000.0
    setLimits(parameter[PAR_VCT], 0.0, 2.0); // -2.0 <= Vct <= 2.0
    setLimits(parameter[PAR_KVB], 0.0, 10000.0); // 0 <= Kvb <= 10000.0
    setLimits(parameter[PAR_KVB1], 0.0, 1000.0); // 0.0 <= Kvb2 <= 1000.0
    options.linear_solver_type = ceres::CGNR;
    options.preconditioner_type = ceres::JACOBI;
}

double GardinerTriode::gardinerCurrent(double v, double vg, double kg1, double kvb, double kvb1, double vct, double x, double mu)
{
    double f = std::sqrt(kvb + v * kvb1 + v * v); // Models the island effect at low anode currents
    double ep = v * (1 / mu + (vg + vct) / f);

    if (ep < 0.0) { // Avoids the use of the mathematical device ln(1 + exp(x))
        ep = 0.0;
    }

    return pow(ep, x) / kg1; // Returns a real current (i.e. scaled by Kg)
}
