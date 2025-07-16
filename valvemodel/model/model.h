#pragma once

#include <PreferencesDialog.h>
#include <QJsonObject>
#include <QString>
#include <QObject>

// Remove Ceres dependency
// #include "ceres/ceres.h"
// #include "glog/logging.h"

#include "../ui/parameter.h"
#include "../ui/uibridge.h"

#include "../data/measurement.h"

// Define a simple solver options structure to replace Ceres
struct SolverOptions {
    bool use_inner_iterations = false;
    bool use_nonmonotonic_steps = false;
    int trust_region_strategy_type = 0;
    int linear_solver_type = 0;
    int preconditioner_type = 0;
    int max_num_iterations = 100;
    double function_tolerance = 1e-6;
    double gradient_tolerance = 1e-10;
    double parameter_tolerance = 1e-8;
};

// Define a simple solver summary structure to replace Ceres
struct SolverSummary {
    int termination_type = 0;
    std::string FullReport() { return "Direct calculation used instead of Ceres solver"; }
};

// Constants to replace Ceres enums
const int CONVERGENCE = 1;

/**
 * @brief The eTriodeParameter enum
 *
 * Defines indexes into the array of model Parameters. This is useful because the models considered
 * represent progressive refinements, i.e. the more complex models reuse parameters from the simpler
 * models and complex models only need to define the additional parameters that they add to the model
 */
enum eTriodeParameter {
    PAR_MU,
    PAR_KG1,
    PAR_X,
    PAR_KP,
    PAR_KVB,
    PAR_KVB1,
    PAR_VCT,
    PAR_KG2,
    PAR_KG2A,
    PAR_A,
    PAR_ALPHA,
    PAR_BETA,
    PAR_GAMMA,
    PAR_OS,
    PAR_TAU,
    PAR_RHO,
    PAR_THETA,
    PAR_PSI,
    PAR_OMEGA,
    PAR_LAMBDA,
    PAR_NU,
    PAR_S,
    PAR_AP
};

enum eModelType {
    SIMPLE_TRIODE,
    KOREN_TRIODE,
    COHEN_HELIE_TRIODE,
    REEFMAN_DERK_PENTODE,
    REEFMAN_DERK_E_PENTODE,
    GARDINER_PENTODE
};

enum eModeType {
    NORMAL_MODE,
    SCREEN_MODE,
    ANODE_REMODEL_MODE
};

/**
 * @brief sgn
 * @param val The value for which to compute the Signum
 * @return The Signum function of val
 *
 * Returns +1 if val > 0, -1 if val < 0 and 0 if val == 0
 */
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

class Estimate;

/**
 * @brief The Model class
 *
 * Model is the abstract base class for all of the device models and defines the public API as
 * well as providing some core methods that are common. The model uses direct calculation
 * instead of the Ceres Solver library to compute valve characteristics.
 */
class Model : public UIBridge
{
    Q_OBJECT
public:
    Model();

    /**
     * @brief addSample adds a sample to the sample set that will be use to fit the model
     * @param va The anode voltage
     * @param ia The anode current in mA
     * @param vg1 The grid voltage
     * @param vg2 For pentodes only, the screen grid voltage
     */
    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0) = 0;
    /**
     * @brief fromJson reads the model parameters from a Json object
     * @param source The Json object to read
     */
    virtual void fromJson(QJsonObject source) = 0;
    /**
     * @brief toJson writes the current model parameters to a Json object
     * @param destination The Json object to write to
     */
    virtual void toJson(QJsonObject &destination) = 0;
    /**
     * @brief anodeCurrent calculates the modelled anode current
     * @param va The anode voltage
     * @param vg1 The grid voltage
     * @param vg2 For pentodes only, the screen grid voltage
     * @return The anode current in mA
     */
    virtual double triodeAnodeCurrent(double va, double vg1) = 0;
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0, bool secondaryEmission = true) = 0;
    virtual double anodeVoltage(double ia, double vg1, double vg2 = 0.0, bool secondaryEmission = true);
    virtual double screenCurrent(double va, double vg1, double vg2, bool secondaryEmission = true);
    virtual QString getName() = 0;
    virtual int getType() = 0;

    void addMeasurement(Measurement *measurement);
    void addMeasurements(QList<Measurement *> *measurements);
    void setEstimate(Estimate *estimate);

    void solve();

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

    QGraphicsItemGroup *plotModel(Plot *plot, Measurement *measurement, Sweep *sweep = nullptr);

    double getParameter(int parameterIndex);
    int getParameterCount() const;
    bool isConverged() const;

    int getMode() const;
    void setMode(int newMode);

    bool withSecondaryEmission() const;
    void setSecondaryEmission(bool newSecondaryEmission);

    bool getShowScreen() const;
    void setShowScreen(bool newShowScreen);

    void setPreferences(PreferencesDialog *newPreferences);

public slots:
    void solveThreaded();

signals:
    void modelReady();

protected:
    /**
     * @brief parameter The array of model Parameters linked to the UI
     */
    Parameter *parameter[24];
    /**
     * @brief options The options to be used for solving the model approximation
     */
    SolverOptions options;
    /**
     * @brief options The options to be used for solving the model approximation (screen current)
     */
    SolverOptions screenOptions;

    Estimate *estimate;

    bool converged = false;
    int mode = NORMAL_MODE;

    bool secondaryEmission;
    bool showScreen = true;

    PreferencesDialog *preferences;

    void setLowerBound(Parameter* parameter, double lowerBound);
    void setUpperBound(Parameter* parameter, double upperBound);
    void setLimits(Parameter* parameter, double lowerBound, double upperBound);
    virtual void setOptions() = 0;
};

