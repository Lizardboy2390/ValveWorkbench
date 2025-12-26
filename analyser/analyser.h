#ifndef ANALYSER_H
#define ANALYSER_H

#include <glog/logging.h>
#include <QRegularExpression>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTextStream>
#include <QTimer>
#include <preferencesdialog.h>

#include "../valvemodel/data/measurement.h"
#include "../valvemodel/constants.h"
#include "client.h"

#define VH       0   //Heater voltage  [example: 12.6V = adc391   6.3V = adc195]
#define IH       1   //Heater current
#define VG1      2   //Grid voltage 1  [example: 1V = dac60, 10V=dac605, 20V=dac1210, 30V=dac1815, 40V=dac2420, 50V=dac3020]
#define HV1      3   //Anode voltage 1 [example: 600V=3.97V=adc992   300V=1.98V=adc496    200V=0.76V=330   100V=0.381V=V=adc165]
#define IA_HI_1  4   //Anode current hi 1
#define IA_LO_1  5   //Anode current lo 1
#define VG2      6   //Grid voltage 2
#define HV2      7   //Anode voltage 2
#define IA_HI_2  8   //Anode current hi 2
#define IA_LO_2  9   //Anode current lo 2


enum eSamplingType {
    SMP_LINEAR,
    SMP_LOGARITHMIC
};

// Analyser: low-level measurement engine that drives the valve tester
// hardware over the serial S*/M* protocol and builds Measurement
// objects for the UI.
//
// Responsibilities:
//  - Translate high-level test configuration (deviceType, testType,
//    sweep ranges, and protection limits) into sequences of S* set
//    commands and Mode(2) M* measurement commands.
//  - Maintain the step/sweep state machines (stepValue/stepParameter,
//    sweepParameter, stepIndex/sweepIndex) so that each grid/screen/
//    anode family becomes its own Sweep in the resulting Measurement.
//  - Parse Mode(2) responses into Sample instances (createSample) and
//    enforce current/power limits for both primary and secondary
//    channels.
//  - Implement per-test and per-sweep hardware verification
//    (isVerifyingHardware) so that stored samples are only taken once
//    the anode/screen rails are at the configured start bias.
//  - Report progress and completion/abort events back to the Client
//    (ValveWorkbench) through the Client interface.
class Analyser
{
public:
    Analyser(Client *client, QSerialPort *port, QTimer *timeoutTimer);
    ~Analyser();

    void reset(void);

    void setDeviceType(int newDeviceType);

    void startTest();
    void stopTest();

    const QString &getHwVersion() const;

    const QString &getSwVersion() const;

    void setTestType(int newTestType);

    // Enable special multi-section / variant modes.
    void setIsDoubleTriode(bool isDouble);
    void setIsTriodeConnectedPentode(bool enable);

    void setSweepPoints(int newSweepPoints);

    void setSweepParameters(double aStart, double aStop, double aStep, double gStart, double gStop, double gStep, double sStart, double sStop, double sStep, double sgStart, double sgStop, double sgStep, double saStart, double saStop, double saStep);

    void setPMax(double newPMax);

    void setIaMax(double newIaMax);

    void setIg2Max(double newIg2Max);

    void setHeaterVoltage(double newHeaterVoltage);

    void setIsHeatersOn(bool newIsHeatersOn);

    double getMeasuredIaMax() const;

    double getMeasuredIg2Max() const;

    int getAveragingSamples() const;

    int getRetryLimitExceededCount() const;

    Measurement *getResult();

    bool getIsDataSetValid() const;

    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

    void handleCommandTimeout();

    void setPreferences(PreferencesDialog *newPreferences);

    void requestHvCalibrationSampleOnce();

    void setHvHold(int hvChannel, double volts, bool enabled);
    void requestHvDischarge();

    // Apply grid reference (command magnitude in volts, e.g. 5 or 60) to both grids (S2 and S6)
    void applyGridReferenceBoth(double commandVoltage, bool enabled);

private:
    double vRefMaster = 4.1;
    double vRefSlave = 4.1;

    int averagingSamples = 0;
    int retryLimitExceededCount = 0;

    double heaterVoltage;
    double aveHeaterVoltage = 6.3;
    double aveHeaterCurrent = 0.045;

    double anodeStart;
    double anodeStop;
    double anodeStep;

    double gridStart;
    double gridStop;
    double gridStep;

    double screenStart;
    double screenStop;
    double screenStep;

    double secondGridStart;
    double secondGridStop;
    double secondGridStep;

    double secondAnodeStart;
    double secondAnodeStop;
    double secondAnodeStep;

    double pMax = 0.0;
    double iaMax = 0.0;

    double measuredIaMax = 0.0;
    double measuredIg2Max = 0.0;

    QString hwVersion;
    QString swVersion;

    Client *client;

    QSerialPort *serialPort;
    bool awaitingResponse = false;
    QByteArray serialBuffer;

    QTimer *timeoutTimer;

    QList<QString> commandBuffer;
    QList<QString> setupCommands;
    QString stepCommandPrefix;
    QString sweepCommandPrefix;

    Measurement *result;

    int deviceType = TRIODE;
    int testType = ANODE_CHARACTERISTICS;

    PreferencesDialog *preferences;

    double convertMeasuredHvVoltage(int hvChannel, int adc);
    double convertMeasuredHvCurrent(int hvChannel, int adcHi, int adcLo);

    QList<double> stepValue;
    QList<int> stepParameter;
    QList<QList <int>> sweepParameter;
    int stepIndex = 0;
    int sweepIndex = 0;
    int stepType = 0;
    int sweepType = 0;
    int sweepPoints = 60;

    int sampleCount = 0;
    int samplesInCurrentSweep = 0;
    int expectedResponses = 0;

    bool isHeatersOn = false;
    bool isStopRequested = false;
    bool isTestRunning = false;
    bool isTestAborted = false;
    bool isEndSweep = false;
    bool isDataSetValid = false;
    bool isVersionRead = false;
    bool isMega = false;
    bool isDoubleTriode = false;
    bool isTriodeConnectedPentode = false;
    bool isVerifyingHardware = false;  // ← ADD THIS: Track verification state
    int verificationAttempts = 0;      // ← ADD THIS: Track verification attempts
    static const int MAX_VERIFICATION_ATTEMPTS = 3; // ← ADD THIS: Max verification retries

    bool hvCalibrationSampling = false;

    static QRegularExpression *sampleMatcher;
    static QRegularExpression *sampleMatcher2;
    static QRegularExpression *getMatcher;
    static QRegularExpression *infoMatcher;

    Sample *createSample(QString response);
    int convertTargetVoltage(int electrode, double voltage);
    double convertMeasuredVoltage(int electrode, int voltage);
    double convertMeasuredCurrent(int electrode, int current, int currentLo = 0, int currentHi = 0);
    void sendCommand(QString command);
    void nextCommand();
    void nextSample();
    void abortTest();
    QString buildSetCommand(QString command, int value);
    void checkResponse(QString response);
    void responseTimeout();
    void steppedSweep(double sweepStart, double sweepStop, double stepStart, double stepStop, double step);
    void singleSweep(double sweepStart, double sweepStop);
    double sampleFunction(double linearValue);
};

#endif // ANALYSER_H
