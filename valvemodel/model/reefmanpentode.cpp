#include "reefmanpentode.h"

#include <cmath>

struct DerkPentodeResidual {
    DerkPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T g = 1.0 / (1.0 + beta[0] * va_);
        T ia = epk * ((1.0 / kg1[0] - 1.0 / kg2[0]) * (1.0 - g) + a[0] * va_ / kg1[0]);
        residual[0] = ia_ - ia;
        return !(isnan(ia) || isinf(ia));
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

struct DerkEPentodeResidual {
    DerkEPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, T* residual) const {
        // DerkE pentode (DEPIa-style) residual: mirror ReefmanPentode::anodeCurrent
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (T(1.0) / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);

        // Alpha_s is stored in PAR_BETA, knee parameter in PAR_ALPHA, consistent with Reefman mapping
        const T kg1v = kg1[0];
        const T kg2v = kg2[0];
        const T As   = a[0];
        const T alpha_s = beta[0];      // screen weight
        const T beta_knee = alpha[0];   // knee shaping parameter

        if (!(kg1v > T(0.0)) || !(kg2v > T(0.0))) {
            residual[0] = T(0.0);
            return true;
        }

        // Alpha = 1 - Kg1/Kg2 * (1 + Alpha_s)
        const T Alpha = T(1.0) - kg1v / kg2v * (T(1.0) + alpha_s);

        // Base DEPIa expression (no secondary emission term yet)
        const T base_linear = T(1.0) / kg1v - T(1.0) / kg2v + As * va_ / kg1v;
        const T decay = exp(-pow(beta_knee * va_, T(1.5)));
        const T subterm = (Alpha / kg1v + alpha_s / kg2v) * decay;
        // uTmax returns Ia in mA by multiplying by 1000; apply the same scaling here
        const T ia = T(1000.0) * epk * (base_linear - subterm);
        residual[0] = ia_ - ia;
        return !(isnan(ia) || isinf(ia));
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

double ReefmanPentode::anodeCurrent(double va, double vg1, double vg2)
{
    const double kg1   = parameter[PAR_KG1]->getValue();
    const double kg2   = parameter[PAR_KG2]->getValue();
    const double A     = parameter[PAR_A]->getValue();
    const double alpha_s = parameter[PAR_BETA]->getValue();   // α_s (screen weight)
    const double betaKnee = parameter[PAR_ALPHA]->getValue(); // β (knee shaping)

    // Use the same DEPIa-style base current expression as the residuals
    const double kp    = parameter[PAR_KP]->getValue();
    const double kvb   = parameter[PAR_KVB]->getValue();
    const double kvb1  = parameter[PAR_KVB1]->getValue();
    const double vct   = parameter[PAR_VCT]->getValue();
    const double x     = parameter[PAR_X]->getValue();
    const double mu    = parameter[PAR_MU]->getValue();

    // Guard against invalid base parameters
    if (!std::isfinite(kp) || !std::isfinite(kvb) || !std::isfinite(kvb1) ||
        !std::isfinite(vct) || !std::isfinite(x) || !std::isfinite(mu)) {
        return 0.0;
    }

    // f(Vg2) term and epk base as in Derk/DerkE residuals
    double f = std::sqrt(kvb + kvb1 * vg2 + vg2 * vg2);
    double inner = 1.0 / mu + (vg1 + vct) / f;
    double logArg = 1.0 + std::exp(kp * inner);
    if (logArg <= 0.0) {
        return 0.0;
    }
    double epk = std::pow(vg2 * std::log(logArg) / kp, x);

    if (!std::isfinite(epk) || epk <= 0.0) {
        return 0.0;
    }

    if (kg1 <= 0.0 || kg2 <= 0.0 || !std::isfinite(betaKnee)) {
        return 0.0;
    }

    // Alpha = 1 − (kg1/kg2) * (1 + α_s)
    const double Alpha = 1.0 - (kg1 / kg2) * (1.0 + alpha_s);

    // Base term shared by Derk and DerkE: 1/kg1 − 1/kg2 + A * Va / kg1
    double base = (1.0 / kg1 - 1.0 / kg2) + (A * va / kg1);

    if (modelType == DERK) {
        // Derk model (Eq. 25 in Theory.pdf): subtract common / (1 + φ Va)
        const double betaS = alpha_s;
        const double common = Alpha / kg1 + betaS / kg2;
        double denom = 1.0 + betaKnee * va;
        if (denom < 1e-9) {
            denom = 1e-9;
        }
        base -= common / denom;
    } else {
        // DerkE model (DEPIa-style): subtract exp(-(β Va)^{3/2}) * (Alpha/kg1 + α_s/kg2)
        const double betaS = alpha_s;
        const double common = Alpha / kg1 + betaS / kg2;
        double t = betaKnee * va;
        if (t < 0.0) {
            t = 0.0;
        }
        double decay = std::exp(-std::pow(t, 1.5));
        base -= decay * common;
    }

    // uTmax DEPIa returns Ia in mA via a 1000x factor; measurements here are in mA,
    // so apply the same scaling so magnitudes match the data.
    double ia = 1000.0 * epk * base;
    if (!std::isfinite(ia) || ia <= 0.0) {
        return 0.0;
    }
    return ia;
}

ReefmanPentode::ReefmanPentode(int newType) : modelType(newType)
{
    secondaryEmission = false;
}

void ReefmanPentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    if (modelType == DERK) {
        anodeProblem.AddResidualBlock(
            new AutoDiffCostFunction<DerkPentodeResidual, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(
                new DerkPentodeResidual(va, vg1, ia, vg2, ig2)),
            NULL,
            parameter[PAR_KG1]->getPointer(),
            parameter[PAR_KP]->getPointer(),
            parameter[PAR_KVB]->getPointer(),
            parameter[PAR_KVB1]->getPointer(),
            parameter[PAR_VCT]->getPointer(),
            parameter[PAR_X]->getPointer(),
            parameter[PAR_MU]->getPointer(),
            parameter[PAR_KG2]->getPointer(),
            parameter[PAR_A]->getPointer(),
            parameter[PAR_ALPHA]->getPointer(),
            parameter[PAR_BETA]->getPointer());
    } else {
        anodeProblem.AddResidualBlock(
            new AutoDiffCostFunction<DerkEPentodeResidual, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(
                new DerkEPentodeResidual(va, vg1, ia, vg2, ig2)),
            NULL,
            parameter[PAR_KG1]->getPointer(),
            parameter[PAR_KP]->getPointer(),
            parameter[PAR_KVB]->getPointer(),
            parameter[PAR_KVB1]->getPointer(),
            parameter[PAR_VCT]->getPointer(),
            parameter[PAR_X]->getPointer(),
            parameter[PAR_MU]->getPointer(),
            parameter[PAR_KG2]->getPointer(),
            parameter[PAR_A]->getPointer(),
            parameter[PAR_ALPHA]->getPointer(),
            parameter[PAR_BETA]->getPointer());
    }

}

void ReefmanPentode::fromJson(QJsonObject source)
{

}

void ReefmanPentode::toJson(QJsonObject &destination)
{

}

void ReefmanPentode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_VCT]); i++;

    updateParameter(labels[i], values[i], parameter[PAR_KG2]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_A]); i++;
    //updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]); i++;
}

QString ReefmanPentode::getName()
{
    return "Cohen Helie Pentode";
}

int ReefmanPentode::getType()
{
    return REEFMAN_DERK_PENTODE;
}

void ReefmanPentode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue()));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));
    addProperty(properties, "Kvb1", QString("%1").arg(parameter[PAR_KVB1]->getValue()));
    addProperty(properties, "vct", QString("%1").arg(parameter[PAR_VCT]->getValue()));

    addProperty(properties, "Kg2", QString("%1").arg(parameter[PAR_KG2]->getValue()));
    addProperty(properties, "A", QString("%1").arg(parameter[PAR_A]->getValue()));
    //addProperty(properties, "alpha", QString("%1").arg(parameter[PAR_ALPHA]->getValue()));
    addProperty(properties, "beta", QString("%1").arg(parameter[PAR_BETA]->getValue()));
}

bool ReefmanPentode::withSecondaryEmission() const
{
    return secondaryEmission;
}

void ReefmanPentode::setSecondaryEmission(bool newSecondaryEmission)
{
    secondaryEmission = newSecondaryEmission;
}

int ReefmanPentode::getModelType() const
{
    return modelType;
}

void ReefmanPentode::setModelType(int newModelType)
{
    modelType = newModelType;
}

void ReefmanPentode::setOptions()
{
    // Keep triode mu fixed (comes from triode fit), but allow the pentode
    // shaping and screen-related parameters to vary so the solver can find
    // realistic Ia magnitudes.
    anodeProblem.SetParameterBlockConstant(parameter[PAR_MU]->getPointer());

    // All other key pentode parameters (Kg1, Kp, Kvb, Kvb1, Vct, Kg2, A,
    // Alpha, Beta) are left free, with bounds instead of being frozen.

    // Basic non-negativity for shaping terms.
    anodeProblem.SetParameterLowerBound(parameter[PAR_A]->getPointer(), 0, 0.0);
    anodeProblem.SetParameterLowerBound(parameter[PAR_ALPHA]->getPointer(), 0, 0.0);
    anodeProblem.SetParameterLowerBound(parameter[PAR_BETA]->getPointer(), 0, 0.0);

    // Add simple upper bounds to keep the optimiser in a sane region.
    anodeProblem.SetParameterUpperBound(parameter[PAR_A]->getPointer(), 0, 0.5);
    anodeProblem.SetParameterUpperBound(parameter[PAR_ALPHA]->getPointer(), 0, 2.0);
    anodeProblem.SetParameterUpperBound(parameter[PAR_BETA]->getPointer(), 0, 2.0);

    options.max_num_iterations = 200;
    options.max_num_consecutive_invalid_steps = 20;
    options.use_inner_iterations = true;
    options.linear_solver_type = ceres::DENSE_QR;
    options.preconditioner_type = ceres::SUBSET;
}
