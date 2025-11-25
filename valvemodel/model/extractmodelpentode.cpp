#include "extractmodelpentode.h"

#include <cmath>

#include "model.h"

using ceres::AutoDiffCostFunction;

// Jet-friendly helpers (mirroring GardinerPentode) to avoid ceres::max/min,
// which are not provided for AutoDiff types.
template <typename T>
inline T tmax(const T &a, const T &b) {
    return (a > b) ? a : b;
}

template <typename T>
inline T tmin(const T &a, const T &b) {
    return (a < b) ? a : b;
}

// Shared helper for secondary emission term Psec(Va) as defined in the
// ExtractModel theory (Sec. 6.1/6.2):
//   Psec = S * Va * (1 + tanh(-ap * (Va - Vco)))
//   with Vco = Vg2 / lambda - nu * Vg1 - omega
template <typename T>
inline T psec_term(const T &va,
                   const T &vg1,
                   const T &vg2,
                   const T &S,
                   const T &ap,
                   const T &lambda,
                   const T &nu,
                   const T &omega)
{
    const T eps = T(1e-9);
    T vco = vg2 / (lambda + eps) - nu * vg1 - omega;
    return S * va * (T(1.0) + ceres::tanh(-ap * (va - vco)));
}

namespace {
struct ExtractDerkEPentodeResidual {
    ExtractDerkEPentodeResidual(double va, double vg1, double ia, double vg2)
        : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2) {}

    template <typename T>
    bool operator()(const T *const kg1,
                    const T *const kp,
                    const T *const kvb,
                    const T *const x,
                    const T *const mu,
                    const T *const kg2,
                    const T *const a,
                    const T *const alpha_s,
                    const T *const beta,
                    T *residual) const
    {
        const T eps = T(1e-12);

        T v2 = T(vg2_);
        T f = ceres::sqrt(tmax(kvb[0] + v2 * v2, eps));
        T y = kp[0] * (T(1.0) / tmax(mu[0], eps) + T(vg1_) / f);
        y = tmax(tmin(y, T(50.0)), T(-50.0));
        T base = v2 / tmax(kp[0], eps) * ceres::log(T(1.0) + ceres::exp(y));
        base = tmax(base, eps);
        T ip = ceres::exp(x[0] * ceres::log(base));

        T invKg1 = T(1.0) / tmax(kg1[0], eps);
        T invKg2 = T(1.0) / tmax(kg2[0], eps);
        T alpha = T(1.0) - kg1[0] / tmax(kg2[0], eps) * (T(1.0) + alpha_s[0]);

        T g = ceres::exp(-ceres::pow(beta[0] * T(va_), T(1.5)));
        T term = alpha * invKg1 + alpha_s[0] * invKg2;

        T iaModel = ip * (invKg1 - invKg2 + a[0] * T(va_) * invKg1 - g * term);

        residual[0] = T(ia_) - iaModel;
        return true;
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
};

// Residual including secondary emission via Psec(Va) (Eq. 42/46 in the
// ExtractModel theory). This is only used when preferences->useSecondaryEmission()
// is enabled.
struct ExtractDerkEPentodeSEResidual {
    ExtractDerkEPentodeSEResidual(double va, double vg1, double ia, double vg2)
        : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2) {}

    template <typename T>
    bool operator()(const T *const kg1,
                    const T *const kp,
                    const T *const kvb,
                    const T *const x,
                    const T *const mu,
                    const T *const kg2,
                    const T *const a,
                    const T *const alpha_s,
                    const T *const beta,
                    const T *const omega,
                    const T *const lambda,
                    const T *const nu,
                    const T *const S,
                    const T *const ap,
                    T *residual) const
    {
        const T eps = T(1e-12);

        T v2 = T(vg2_);
        T f = ceres::sqrt(tmax(kvb[0] + v2 * v2, eps));
        T y = kp[0] * (T(1.0) / tmax(mu[0], eps) + T(vg1_) / f);
        y = tmax(tmin(y, T(50.0)), T(-50.0));
        T base = v2 / tmax(kp[0], eps) * ceres::log(T(1.0) + ceres::exp(y));
        base = tmax(base, eps);
        T ip = ceres::exp(x[0] * ceres::log(base));

        T invKg1 = T(1.0) / tmax(kg1[0], eps);
        T invKg2 = T(1.0) / tmax(kg2[0], eps);
        T alpha = T(1.0) - kg1[0] / tmax(kg2[0], eps) * (T(1.0) + alpha_s[0]);

        T g = ceres::exp(-ceres::pow(beta[0] * T(va_), T(1.5)));
        T psec = psec_term(T(va_), T(vg1_), T(vg2_), S[0], ap[0], lambda[0], nu[0], omega[0]);
        T term = alpha * invKg1 + alpha_s[0] * invKg2;

        // Eq. (46): additional -Psec/kg2 term inside the Ip_Koren bracket.
        T iaModel = ip * (invKg1 - invKg2 + a[0] * T(va_) * invKg1
                          - psec * invKg2 - g * term);

        residual[0] = T(ia_) - iaModel;
        return true;
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
};
} // namespace

ExtractModelPentode::ExtractModelPentode()
{
}

double ExtractModelPentode::ipKoren(double vg2, double vg1,
                                    double kp, double kvb,
                                    double x, double mu)
{
    if (!std::isfinite(vg2) || !std::isfinite(vg1) ||
        !std::isfinite(kp) || !std::isfinite(kvb) ||
        !std::isfinite(x) || !std::isfinite(mu)) {
        return 0.0;
    }

    const double eps = 1e-12;
    double kpSafe = (kp > eps) ? kp : eps;
    double muSafe = (mu > eps) ? mu : eps;

    double f = std::sqrt(std::max(kvb + vg2 * vg2, eps));
    double y = kp * (1.0 / muSafe + vg1 / f);
    if (y > 50.0) y = 50.0;
    if (y < -50.0) y = -50.0;

    double base = (vg2 / kpSafe) * std::log(1.0 + std::exp(y));
    if (base <= eps) {
        return 0.0;
    }

    double ip = std::pow(base, x);
    if (!std::isfinite(ip) || ip < 0.0) {
        return 0.0;
    }

    return ip;
}

void ExtractModelPentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    Q_UNUSED(ig2);

    const bool useSE = (preferences && preferences->useSecondaryEmission());

    if (useSE) {
        anodeProblem.AddResidualBlock(
            new AutoDiffCostFunction<ExtractDerkEPentodeSEResidual, 1,
                                      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(
                new ExtractDerkEPentodeSEResidual(va, vg1, ia, vg2)),
            NULL,
            parameter[PAR_KG1]->getPointer(),
            parameter[PAR_KP]->getPointer(),
            parameter[PAR_KVB]->getPointer(),
            parameter[PAR_X]->getPointer(),
            parameter[PAR_MU]->getPointer(),
            parameter[PAR_KG2]->getPointer(),
            parameter[PAR_A]->getPointer(),
            parameter[PAR_ALPHA]->getPointer(),
            parameter[PAR_BETA]->getPointer(),
            parameter[PAR_OMEGA]->getPointer(),
            parameter[PAR_LAMBDA]->getPointer(),
            parameter[PAR_NU]->getPointer(),
            parameter[PAR_S]->getPointer(),
            parameter[PAR_AP]->getPointer());
    } else {
        anodeProblem.AddResidualBlock(
            new AutoDiffCostFunction<ExtractDerkEPentodeResidual, 1,
                                      1, 1, 1, 1, 1, 1, 1, 1, 1>(
                new ExtractDerkEPentodeResidual(va, vg1, ia, vg2)),
            NULL,
            parameter[PAR_KG1]->getPointer(),
            parameter[PAR_KP]->getPointer(),
            parameter[PAR_KVB]->getPointer(),
            parameter[PAR_X]->getPointer(),
            parameter[PAR_MU]->getPointer(),
            parameter[PAR_KG2]->getPointer(),
            parameter[PAR_A]->getPointer(),
            parameter[PAR_ALPHA]->getPointer(),
            parameter[PAR_BETA]->getPointer());
    }
}

double ExtractModelPentode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    const bool useSE = secondaryEmission && preferences && preferences->useSecondaryEmission();
    double kg1 = parameter[PAR_KG1]->getValue();
    double kg2 = parameter[PAR_KG2]->getValue();
    double a = parameter[PAR_A]->getValue();
    double alpha_s = parameter[PAR_ALPHA]->getValue();
    double beta = parameter[PAR_BETA]->getValue();

    if (kg1 <= 0.0 || kg2 <= 0.0) {
        return 0.0;
    }

    double kp = parameter[PAR_KP]->getValue();
    double kvb = parameter[PAR_KVB]->getValue();
    double x = parameter[PAR_X]->getValue();
    double mu = parameter[PAR_MU]->getValue();

    double ip = ipKoren(vg2, vg1, kp, kvb, x, mu);
    if (ip <= 0.0) {
        return 0.0;
    }

    double invKg1 = 1.0 / kg1;
    double invKg2 = 1.0 / kg2;
    double alpha = 1.0 - (kg1 / kg2) * (1.0 + alpha_s);

    double g = std::exp(-std::pow(beta * va, 1.5));
    double term = alpha * invKg1 + alpha_s * invKg2;

    double iaModel = ip * (invKg1 - invKg2 + a * va * invKg1 - g * term);

    if (useSE) {
        // Add secondary emission term Psec(Va) as in Eq. (42)/(46):
        // Ia(Va) = Ip_Koren(... - Psec/kg2 - g * (...)).
        const double S      = parameter[PAR_S]->getValue();
        const double ap     = parameter[PAR_AP]->getValue();
        const double lambda = parameter[PAR_LAMBDA]->getValue();
        const double nu     = parameter[PAR_NU]->getValue();
        const double omega  = parameter[PAR_OMEGA]->getValue();

        double psec = S * va * (1.0 + std::tanh(-ap * (va - (vg2 / (lambda + 1e-9) - nu * vg1 - omega))));
        iaModel += ip * (-psec * invKg2);
    }

    if (!std::isfinite(iaModel) || iaModel < 0.0) {
        return 0.0;
    }

    return iaModel;
}

double ExtractModelPentode::screenCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    double kg2 = parameter[PAR_KG2]->getValue();
    if (kg2 <= 0.0) {
        return 0.0;
    }

    double kp = parameter[PAR_KP]->getValue();
    double kvb = parameter[PAR_KVB]->getValue();
    double x = parameter[PAR_X]->getValue();
    double mu = parameter[PAR_MU]->getValue();
    double beta = parameter[PAR_BETA]->getValue();
    double alpha_s = parameter[PAR_ALPHA]->getValue();

    double ip = ipKoren(vg2, vg1, kp, kvb, x, mu);
    if (ip <= 0.0) {
        return 0.0;
    }

    double invKg2 = 1.0 / kg2;
    double g = std::exp(-std::pow(beta * va, 1.5));

    double psec = 0.0;
    if (secondaryEmission && preferences && preferences->useSecondaryEmission()) {
        const double S      = parameter[PAR_S]->getValue();
        const double ap     = parameter[PAR_AP]->getValue();
        const double lambda = parameter[PAR_LAMBDA]->getValue();
        const double nu     = parameter[PAR_NU]->getValue();
        const double omega  = parameter[PAR_OMEGA]->getValue();
        psec = S * va * (1.0 + std::tanh(-ap * (va - (vg2 / (lambda + 1e-9) - nu * vg1 - omega))));
    }

    // Beam tetrode + secondary emission (Eq. 45):
    // Ig2(Va) = Ip_Koren / kg2 * (1 + alpha_se * e^{-(beta Va)^{3/2}} + Psec).
    double ig2 = ip * invKg2 * (1.0 + alpha_s * g + psec);

    if (!std::isfinite(ig2) || ig2 < 0.0) {
        return 0.0;
    }

    return ig2;
}

void ExtractModelPentode::fromJson(QJsonObject source)
{
    KorenTriode::fromJson(source);

    if (source.contains("kg2") && source["kg2"].isDouble()) {
        parameter[PAR_KG2]->setValue(source["kg2"].toDouble());
    }

    if (source.contains("a") && source["a"].isDouble()) {
        parameter[PAR_A]->setValue(source["a"].toDouble());
    }

    if (source.contains("alpha") && source["alpha"].isDouble()) {
        parameter[PAR_ALPHA]->setValue(source["alpha"].toDouble());
    }

    if (source.contains("beta") && source["beta"].isDouble()) {
        parameter[PAR_BETA]->setValue(source["beta"].toDouble());
    }
}

void ExtractModelPentode::toJson(QJsonObject &destination)
{
    KorenTriode::toJson(destination);

    destination["kg2"] = parameter[PAR_KG2]->getValue();
    destination["a"] = parameter[PAR_A]->getValue();
    destination["alpha"] = parameter[PAR_ALPHA]->getValue();
    destination["beta"] = parameter[PAR_BETA]->getValue();

    destination["device"] = "pentode";
    destination["type"] = "extractDerkE";
}

void ExtractModelPentode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;

    updateParameter(labels[i], values[i], parameter[PAR_KG2]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_A]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]); i++;
}

QString ExtractModelPentode::getName()
{
    return "ExtractModel Pentode (DerkE exact)";
}

int ExtractModelPentode::getType()
{
    return EXTRACT_DERK_E_PENTODE;
}

void ExtractModelPentode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue()));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));

    addProperty(properties, "Kg2", QString("%1").arg(parameter[PAR_KG2]->getValue()));
    addProperty(properties, "A", QString("%1").arg(parameter[PAR_A]->getValue()));
    addProperty(properties, "alpha_s", QString("%1").arg(parameter[PAR_ALPHA]->getValue()));
    addProperty(properties, "beta", QString("%1").arg(parameter[PAR_BETA]->getValue()));
}

void ExtractModelPentode::setOptions()
{
    // Base pentode corridor: keep Kg1/Kg2/A/Beta in a sensible range
    setLowerBound(parameter[PAR_KG1], 0.02);
    setLowerBound(parameter[PAR_KG2], 0.05);
    setLowerBound(parameter[PAR_A],   0.0);
    setLimits     (parameter[PAR_BETA], 0.01, 0.5);

    // Secondary-emission geometry: constrain to realistic ranges so the
    // solver cannot wander into extreme Psec shapes that prevent
    // convergence.
    setLimits(parameter[PAR_OMEGA],  0.0, 600.0);  // Vco offset
    setLimits(parameter[PAR_LAMBDA], 5.0, 200.0);  // Vg2 / lambda scale
    setLimits(parameter[PAR_NU],     0.0, 80.0);   // nu * Vg1 weight
    setLimits(parameter[PAR_S],      0.0, 0.5);    // Psec amplitude
    setLimits(parameter[PAR_AP],     0.001, 0.05); // cross-over sharpness

    // Allow more iterations for the SE-enabled ExtractModel fit while
    // keeping other solver settings similar to the Gardiner pentode.
    options.max_num_iterations = 800;
    options.max_num_consecutive_invalid_steps = 20;
    options.linear_solver_type = ceres::DENSE_QR;
    // Looser tolerances: we care more about a stable, good fit than
    // squeezing out the last fractional percent of cost reduction.
    options.function_tolerance = 1e-5;
    options.gradient_tolerance = 1e-6;
}
