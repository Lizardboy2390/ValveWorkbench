#include "analyser.h"
#include "client.h"
#include <QSerialPort>
#include <QTimer>

#include "../valvemodel/data/sample.h"

#include <QDebug>
#include <cmath>

QRegularExpression *Analyser::sampleMatcher = new QRegularExpression(R"(^OK: Mode\(2\) (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+))");
QRegularExpression *Analyser::sampleMatcher2 = new QRegularExpression(R"(^OK: Mode\(2\) (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+))");
QRegularExpression *Analyser::getMatcher = new QRegularExpression(R"(^OK: Get\((\d+)\) = (\d+))");
QRegularExpression *Analyser::infoMatcher = new QRegularExpression(R"(^OK: Info\((\d+)\) = (.*)\r)");

Analyser::Analyser(Client *client_, QSerialPort *port, QTimer *timeout) : client(client_), serialPort(port), timeoutTimer(timeout)
{
}

Analyser::~Analyser()
{
}

void Analyser::setDeviceType(int newDeviceType)
{
    deviceType = newDeviceType;
}

Sample *Analyser::createSample(QString response)
{
    QRegularExpressionMatch match12 = sampleMatcher2->match(response);
    QRegularExpressionMatch match10;
    bool use12 = match12.hasMatch();
    if (!use12) {
        match10 = sampleMatcher->match(response);
        if (!match10.hasMatch()) {
            qWarning("Failed to parse Mode(2) response: %s", response.toStdString().c_str());
            return nullptr;
        }
    }

    if (use12) {
        qInfo("Raw capture: vh=%d ih=%d vg1=%d va1=%d ia1=%d ia1_lo=%d vg3=%d va2=%d ia2=%d ia2_lo=%d ia_hi=%d ig2_hi=%d",
              match12.captured(1).toInt(), match12.captured(2).toInt(), match12.captured(3).toInt(), match12.captured(4).toInt(),
              match12.captured(5).toInt(), match12.captured(6).toInt(), match12.captured(7).toInt(), match12.captured(8).toInt(),
              match12.captured(9).toInt(), match12.captured(10).toInt(), match12.captured(11).toInt(), match12.captured(12).toInt());
    } else {
        qInfo("Raw capture(10): vh=%d ih=%d vg1=%d va1=%d ia1=%d ia1_lo=%d va2=%d ia2=%d ia_hi=%d ig2_hi=%d",
              match10.captured(1).toInt(), match10.captured(2).toInt(), match10.captured(3).toInt(), match10.captured(4).toInt(),
              match10.captured(5).toInt(), match10.captured(6).toInt(), match10.captured(7).toInt(), match10.captured(8).toInt(),
              match10.captured(9).toInt(), match10.captured(10).toInt());
    }

    double vg1 = convertMeasuredVoltage(GRID, (use12 ? match12.captured(3).toInt() : match10.captured(3).toInt()));
    double va = convertMeasuredVoltage(ANODE, (use12 ? match12.captured(4).toInt() : match10.captured(4).toInt()));
    int ia_hi = (use12 ? match12.captured(11).toInt() : match10.captured(9).toInt());
    double ia = convertMeasuredCurrent(ANODE, (use12 ? match12.captured(5).toInt() : match10.captured(5).toInt()),
                                       (use12 ? match12.captured(6).toInt() : match10.captured(6).toInt()), ia_hi) * 1000;
    double vg2 = 0.0;
    double ig2 = 0.0;
    double vg3 = 0.0;
    double va2 = 0.0;
    double ia2 = 0.0;

    if (isDoubleTriode && use12) {
        vg3 = convertMeasuredVoltage(GRID, match12.captured(7).toInt());
        va2 = convertMeasuredVoltage(ANODE, match12.captured(8).toInt());
        int ia2_hi = match12.captured(12).toInt();
        ia2 = convertMeasuredCurrent(ANODE, match12.captured(9).toInt(), match12.captured(10).toInt(), ia2_hi) * 1000;
        qInfo("Sample (double triode): vg1(set)=%.3f vg3(set)=%.3f va=%.3f va2=%.3f ia=%.3f ia2(pre)=%.4f", vg1, vg3, va, va2, ia, ia2);
        qInfo("Raw ADC primary: HI=%d LO=%d (%.3fmA)", (use12 ? match12.captured(5).toInt() : match10.captured(5).toInt()), (use12 ? match12.captured(6).toInt() : match10.captured(6).toInt()), ia);
        qInfo("Raw ADC secondary: HI=%d LO=%d HI2=%d -> raw=%.4fmA", match12.captured(9).toInt(), match12.captured(10).toInt(), match12.captured(12).toInt(), ia2);
    } else if (!isDoubleTriode) {
        if (use12) {
            vg2 = convertMeasuredVoltage(SCREEN, match12.captured(8).toInt());
            ig2 = convertMeasuredCurrent(SCREEN, match12.captured(9).toInt(), match12.captured(10).toInt(), match12.captured(12).toInt()) * 1000;
        }
        qInfo("Sample (single channel): vg1(set)=%.3f vg2=%.3f va=%.3f ia=%.3f ig2=%.3f", vg1, vg2, va, ia, ig2);
        qInfo("Raw ADC primary: HI=%d LO=%d (%.3fmA)", (use12 ? match12.captured(5).toInt() : match10.captured(5).toInt()), (use12 ? match12.captured(6).toInt() : match10.captured(6).toInt()), ia);
    }
    double vh = convertMeasuredVoltage(HEATER, (use12 ? match12.captured(1).toInt() : match10.captured(1).toInt()));
    double ih = convertMeasuredCurrent(HEATER, (use12 ? match12.captured(2).toInt() : match10.captured(2).toInt()));

    // Debug raw ADC values
    int rawCurrent = (use12 ? match12.captured(5).toInt() : match10.captured(5).toInt());
    int rawCurrentLo = (use12 ? match12.captured(6).toInt() : match10.captured(6).toInt());
    int rawCurrentHi = (use12 ? match12.captured(11).toInt() : match10.captured(9).toInt());

    // qInfo("Raw ADC values - Current: %d, CurrentLo: %d, CurrentHi: %d", rawCurrent, rawCurrentLo, rawCurrentHi);

    // This line adjusts the anode current measurement to account for the current that always flows through the voltage sense network
    // The resistance of the voltage sense network (3 x 470k + 2 * 4k7) is 1.4194M and the current needs to be adjusted for mA
    ia = ia - va / 1419.4;
    // For very low anode currents, this could lead to very small negative values, hence...
    if (ia < 0.0) {
        ia = 0.0;
    }

    // Apply the same correction for the second triode section
   
    ia2 = ia2 - va2 / 1419.4;
    if (ia2 < 0.0) {
        ia2 = 0.0;
    }

    if (ia > measuredIaMax) {
        measuredIaMax = ia;
    }

    if (ig2 > measuredIg2Max) {
        measuredIg2Max = ig2;
    }

    Sample *sample = new Sample(vg1, va, ia, vg2, ig2, vh, ih, vg3, va2, ia2);

     qInfo("Converted values - Va: %.3fV, Ia: %.3fmA, Vg1: %.3fV, Vg2: %.3fV", va, ia, vg1, vg2);

    return sample;
}

int Analyser::convertTargetVoltage(int electrode, double voltage)
{
    int value = 0;

    switch (electrode) {
    case HEATER:
        value = (voltage * 1023 * 470 / 3770 / vRefSlave);
        break;
    case ANODE:
    case SCREEN:
        value = (voltage * 1023 * 9400 / 1419400 / vRefMaster);
        break;
    case GRID:
        value = (voltage * 4095 / 16.5 / vRefMaster);
        break;
    default:
        break;
    }

    return value;
}

double Analyser::convertMeasuredVoltage(int electrode, int voltage)
{
    double value = 0;

    switch (electrode) {
    case HEATER:
        value = (((double) voltage) / 1023 / 470 * 3770 * vRefSlave);
        break;
    case ANODE:
    case SCREEN:
        value = (((double) voltage) / 1023 / 9400 * 1419400 * vRefMaster);
        break;
    case GRID:
        value = -(((double) voltage) / 4095 * 16.5 * vRefMaster);
        break;
    default:
        break;
    }

    return value;
}

double Analyser::convertMeasuredCurrent(int electrode, int current, int currentLo, int currentHi)
{
    double value = 0;

    switch (electrode) {
    case HEATER:
        value = (((double) current) / 1023 / 0.22 * vRefSlave);
        value += 0.045; // apparent 45mA offset (i.e. 0.011v across R sense)
        break;
    case ANODE:
    case SCREEN: {
        const double highRangeDivisor = 2.0 * 33.0;
        const double lowRangeDivisor = 2.0 * 3.333333;

        bool highRangeSaturated = current >= 1000;
        if (highRangeSaturated && currentLo > 0) {
            value = ((double) currentLo) * vRefMaster / 1023 / lowRangeDivisor;
            // qInfo("Using low range: %f mA", value * 1000);
        } else {
            value = ((double) current) * vRefMaster / 1023 / highRangeDivisor;
            // qInfo("Using high range: %f mA", value * 1000);
        }
        // Clamp to hardware maximum measurable current: 50 mA = 0.05 A
        if (value < 0.0) value = 0.0;
        if (value > 0.05) value = 0.05;
        break;
    }
    case GRID:
        break;
    default:
        break;
    }

    return value;
}

void Analyser::reset()
{
    measuredIaMax = 0.0;
    measuredIg2Max = 0.0;
    isVersionRead = false;
    expectedResponses = 0;
    isVerifyingHardware = false;  // ← ADD THIS
    verificationAttempts = 0;     // ← ADD THIS
}

const QString &Analyser::getHwVersion() const
{
    return hwVersion;
}

const QString &Analyser::getSwVersion() const
{
    return swVersion;
}

void Analyser::setTestType(int newTestType)
{
    testType = newTestType;
}

void Analyser::setIsDoubleTriode(bool isDouble)
{
    isDoubleTriode = isDouble;
}

void Analyser::setSweepPoints(int newSweepPoints)
{
    sweepPoints = newSweepPoints;
}

void Analyser::setSweepParameters(double aStart, double aStop, double aStep, double gStart, double gStop, double gStep, double sStart, double sStop, double sStep, double sgStart, double sgStop, double sgStep, double saStart, double saStop, double saStep)
{
    anodeStart = aStart;
    anodeStop = aStop;
    anodeStep = aStep;

    gridStart = gStart;
    gridStop = gStop;
    gridStep = gStep;

    screenStart = sStart;
    screenStop = sStop;
    screenStep = sStep;

    secondGridStart = sgStart;
    secondGridStop = sgStop;
    secondGridStep = sgStep;

    secondAnodeStart = saStart;
    secondAnodeStop = saStop;
    secondAnodeStep = saStep;
}

void Analyser::setPMax(double newPMax)
{
    pMax = newPMax;
}

void Analyser::setIaMax(double newIaMax)
{
    iaMax = newIaMax;
}

void Analyser::setIg2Max(double newIg2Max)
{
    measuredIg2Max = newIg2Max;
}

void Analyser::setHeaterVoltage(double newHeaterVoltage)
{
    heaterVoltage = newHeaterVoltage;
}

void Analyser::setIsHeatersOn(bool newIsHeatersOn)
{
    isHeatersOn = newIsHeatersOn;

    if (isHeatersOn) {
        sendCommand(buildSetCommand("S0 ", convertTargetVoltage(HEATER, heaterVoltage)));
    } else {
        sendCommand("S0 0");
    }
}

double Analyser::getMeasuredIaMax() const
{
    return measuredIaMax;
}

double Analyser::getMeasuredIg2Max() const
{
    return measuredIg2Max;
}

Measurement *Analyser::getResult()
{
    return result;
}

bool Analyser::getIsDataSetValid() const
{
    return isDataSetValid;
}

void Analyser::startTest()
{
    // Clear down the previous result set
    result = new Measurement();
    result->setTestType(testType);
    result->setDeviceType(deviceType);
    result->setIaMax(iaMax);
    result->setPMax(pMax);

    measuredIaMax = 0.0;
    measuredIg2Max = 0.0;

    isStopRequested = false;
    isTestAborted = false;
    isTestRunning = true;
    isEndSweep = false;
    isDataSetValid = false;

    switch (testType) {
    case ANODE_CHARACTERISTICS:
        stepType = GRID;
        stepCommandPrefix = "S2 ";
        sweepType = ANODE;
        sweepCommandPrefix = "S3 ";

        result->setAnodeStart(anodeStart);
        result->setAnodeStop(anodeStop);
        result->setAnodeStep(anodeStep);

        result->nextSweep(gridStart, screenStart);

        if (isDoubleTriode) {
            steppedSweep(secondAnodeStart, secondAnodeStop, secondGridStart, secondGridStop, secondGridStep);
            sweepCommandPrefix = "S3 ";
            stepCommandPrefix = "S6 ";
            stepType = GRID;
            sweepType = ANODE;

            if (!stepValue.isEmpty()) {
                double minGrid = stepValue.first();
                double maxGrid = stepValue.first();
                for (int i = 1; i < stepValue.size(); ++i) {
                    double v = stepValue.at(i);
                    if (v < minGrid) minGrid = v;
                    if (v > maxGrid) maxGrid = v;
                }
                double derivedStep = 0.0;
                if (stepValue.size() >= 2) {
                    derivedStep = stepValue.at(1) - stepValue.at(0);
                }

                result->setGridStart(minGrid);
                result->setGridStop(maxGrid);
                result->setGridStep(derivedStep);
                result->setAnodeStart(secondAnodeStart);
                result->setAnodeStop(secondAnodeStop);
                result->setAnodeStep(secondAnodeStep);
            }
        } else {
            steppedSweep(anodeStart, anodeStop, gridStart, gridStop, gridStep);

            if (!stepValue.isEmpty()) {
                double minGrid = stepValue.first();
                double maxGrid = stepValue.first();
                for (int i = 1; i < stepValue.size(); ++i) {
                    double v = stepValue.at(i);
                    if (v < minGrid) minGrid = v;
                    if (v > maxGrid) maxGrid = v;
                }
                double derivedStep = 0.0;
                if (stepValue.size() >= 2) {
                    derivedStep = stepValue.at(1) - stepValue.at(0);
                }

                // Log the grid step list for Anode Characteristics
                QString stepList;
                for (int i = 0; i < stepValue.size(); ++i) {
                    if (i) stepList.append(", ");
                    stepList.append(QString::asprintf("%.3f", stepValue.at(i)));
                }
                qInfo("AnodeChar steps: count=%d gridSteps=[%s]", stepValue.size(), stepList.toStdString().c_str());

                result->setGridStart(minGrid);
                result->setGridStop(maxGrid);
                result->setGridStep(derivedStep);
            }
        }

        if (deviceType == PENTODE) { // Anode swept, Grid stepped, Screen fixed
            result->setScreenStart(screenStart);

            sendCommand(buildSetCommand("S7 ", convertTargetVoltage(SCREEN, screenStart)));
        } else if (isDoubleTriode) { // First and second anode swept with same values, Second grid stepped, Main grid 0
            result->setAnodeStart(anodeStart);

            sendCommand(buildSetCommand("S2 ", 0)); // Set main grid to 0V
            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));

            int initialSecondaryGrid = convertTargetVoltage(GRID, secondGridStart);
            qInfo("Command: S6 %d (initial secondary grid)", initialSecondaryGrid);
            sendCommand(buildSetCommand("S6 ", initialSecondaryGrid));
            sendCommand(buildSetCommand("S7 ", convertTargetVoltage(ANODE, secondAnodeStart)));
        }
        if (isDoubleTriode && stepCommandPrefix == "S6 ") {
            sendCommand("S2 0");
            sendCommand(buildSetCommand("S6 ", stepParameter.at(0)));
        } else {
            // Log initial S2 setting at start of first sweep (Anode Characteristics)
            if (stepCommandPrefix == "S2 ") {
                double gridV = stepValue.isEmpty() ? 0.0 : stepValue.at(0);
                qInfo("AnodeChar set S2 (initial): code=%d grid=%.3fV", stepParameter.at(0), gridV);
            }
            sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(0)));
        }

        nextSample();
        break;
    case TRANSFER_CHARACTERISTICS:
        sweepType = GRID;
        sweepCommandPrefix = "S2 ";

        result->setGridStart(gridStart);
        result->setGridStop(gridStop);

        if (deviceType == PENTODE) { // Anode fixed, Screen stepped, Grid swept
            stepType = SCREEN;
            stepCommandPrefix = "S7 ";

            result->setAnodeStart(anodeStart);
            result->setScreenStart(screenStart);
            result->setScreenStop(screenStop);
            result->setScreenStep(screenStep);

            result->nextSweep(screenStart, anodeStart);

            steppedSweep(gridStop, gridStart, screenStart, screenStop, screenStep); // Sweep is reversed to finish on low (absolute) value

            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
        } else if (isDoubleTriode) { // Double triode transfer: step both anodes, sweep both grids together
            // Pre-flight: ensure hardware is in a clean state before starting first sweep
            // Discharge, force both anodes to 0V, and take a dummy measurement
            sendCommand("M1");
            sendCommand("S3 0");
            sendCommand("S7 0");
            sendCommand("M2");

            // Step both anodes (A and B) per step; sweep both grids (A=S2, B=S6) per point
            stepType = ANODE;
            stepCommandPrefix = "S3 ";
            sweepType = GRID;
            sweepCommandPrefix = "S2 ";

            result->setAnodeStart(anodeStart);
            result->setAnodeStop(anodeStop);
            result->setAnodeStep(anodeStep);
            result->nextSweep(anodeStart);

            // Sweep grids from gridStop down to gridStart so we finish near 0V (UI uses magnitudes)
            steppedSweep(gridStop, gridStart, anodeStart, anodeStop, anodeStep);

            // Initialize: first assert both anodes, then set starting grid sweep value for both grids
            {
                int codeS3 = convertTargetVoltage(ANODE, anodeStart);
                int codeS7 = convertTargetVoltage(ANODE, anodeStart);
                int codeS2 = convertTargetVoltage(GRID, gridStop);
                int codeS6 = convertTargetVoltage(GRID, gridStop);
                qInfo("Init (DT Transfer): S3 %d (anode A start)", codeS3);
                sendCommand(buildSetCommand("S3 ", codeS3));
                qInfo("Init (DT Transfer): S7 %d (anode B start)", codeS7);
                sendCommand(buildSetCommand("S7 ", codeS7));
                qInfo("Init (DT Transfer): S2 %d (grid A start)", codeS2);
                sendCommand(buildSetCommand("S2 ", codeS2));
                qInfo("Init (DT Transfer): S6 %d (grid B start)", codeS6);
                sendCommand(buildSetCommand("S6 ", codeS6));
            }

            // Explicitly queue first step anode command like later steps
            if (!stepParameter.isEmpty()) {
                qInfo("Init (DT Transfer): S3 first step code=%d", stepParameter.at(0));
                sendCommand(buildSetCommand("S3 ", stepParameter.at(0)));
            }
        } else { // Anode stepped, Grid swept
            stepType = ANODE;
            stepCommandPrefix = "S3 ";

            result->nextSweep(anodeStart);

            steppedSweep(gridStop, gridStart, anodeStart, anodeStop, anodeStep); // Sweep is reversed to finish on low (absolute) value

            sendCommand("S7 0");
            // Ensure anode is set to starting voltage before beginning grid sweep
            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
        }
        // Initialize first step only for branches that have not already explicitly set it
        if (deviceType == PENTODE || (isDoubleTriode && stepCommandPrefix != "S3 ")) {
            sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(0)));
        }

        nextSample();
        break;
    case SCREEN_CHARACTERISTICS:
        // Anode fixed, Grid stepped, Screen swept
        stepType = GRID;
        stepCommandPrefix = "S2 ";
        sweepType = SCREEN;
        sweepCommandPrefix = "S7 ";

        result->setAnodeStart(anodeStart);
        result->setGridStart(gridStart);
        result->setGridStop(gridStop);
        result->setGridStep(gridStep);
        result->setScreenStart(screenStart);
        result->setScreenStop(screenStop);

        result->nextSweep(gridStart, anodeStart);

        steppedSweep(screenStart, screenStop, gridStart, gridStop, gridStep);

        sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));

        nextSample();
        break;
    default:
        break;
    }
}

void Analyser::stopTest()
{
    isStopRequested = true;
}

void Analyser::nextSample() {
    // qInfo("=== NEXT SAMPLE DEBUG ===");
    // qInfo("Analyser: nextSample called, stepIndex=%d, sweepIndex=%d, isEndSweep=%d", stepIndex, sweepIndex, isEndSweep);
    // qInfo("expectedResponses: %d, awaitingResponse: %d", expectedResponses, awaitingResponse);
    // qInfo("stepParameter.length(): %d, sweepParameter.size(): %d", stepParameter.length(), sweepParameter.size());
    if (stepIndex < stepParameter.length()) {
        // qInfo("sweepParameter[%d].length(): %d", stepIndex, sweepParameter.at(stepIndex).length());
    }
         // Run the next value in the sweep
    if (!isEndSweep && sweepIndex < sweepParameter.at(stepIndex).length()) {

        // qInfo("Sending sweep command: S%d %d", sweepType, sweepParameter.at(stepIndex).at(sweepIndex));
        if (sweepIndex == 0) {
            // qInfo("This is the first command for stepIndex=%d", stepIndex);
        }

        int sweepValue = sweepParameter.at(stepIndex).at(sweepIndex);

        // Hard clamp GRID command to UI range to prevent any overshoot (e.g., jump to -8V)
        if (testType == TRANSFER_CHARACTERISTICS && sweepCommandPrefix == "S2 ") {
            int codeLo = convertTargetVoltage(GRID, std::min(gridStart, gridStop));
            int codeHi = convertTargetVoltage(GRID, std::max(gridStart, gridStop));
            if (codeLo > codeHi) std::swap(codeLo, codeHi);
            if (sweepValue < codeLo) sweepValue = codeLo;
            if (sweepValue > codeHi) sweepValue = codeHi;
        }

        // For Transfer mode: discharge and reassert anode only at the first point of each sweep
        if (testType == TRANSFER_CHARACTERISTICS && sweepIndex == 0) {
            sendCommand("M1");
            // Reassert anode after discharge at start of sweep
            if (stepType == ANODE) {
                // Triode transfer: anode is stepped per stepIndex
                if (stepIndex < stepParameter.length()) {
                    sendCommand(buildSetCommand("S3 ", stepParameter.at(stepIndex)));
                }
            } else {
                // Pentode transfer: anode fixed at anodeStart
                sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
            }
        }

        if (isDoubleTriode && stepCommandPrefix == "S6 " && stepIndex < stepParameter.length()) {
            const int primaryGrid = stepParameter.at(stepIndex);
            sendCommand(buildSetCommand("S2 ", primaryGrid));
        }

        QString primaryCommand = buildSetCommand(sweepCommandPrefix, sweepValue);
        qInfo("Command: %s (primary sweep)", primaryCommand.toStdString().c_str());
        sendCommand(primaryCommand);
        if (isDoubleTriode) {
            if (sweepCommandPrefix == "S3 ") {
                // Sweeping anode: track secondary anode (S7)
                QString secondaryCommand = buildSetCommand("S7 ", sweepValue);
                qInfo("Command: %s (secondary anode tracking)", secondaryCommand.toStdString().c_str());
                sendCommand(secondaryCommand);
            } else if (sweepCommandPrefix == "S2 ") {
                // Sweeping grid: track secondary grid (S6)
                QString secondaryGridCommand = buildSetCommand("S6 ", sweepValue);
                qInfo("Command: %s (secondary grid tracking)", secondaryGridCommand.toStdString().c_str());
                sendCommand(secondaryGridCommand);
            }
        }
        if (testType == TRANSFER_CHARACTERISTICS) {
            // Refire each point
            sendCommand("M6");
        }
        sendCommand("M2");
    } else {
        // qInfo("=== END OF SWEEP DEBUG ===");
        // qInfo("End of sweep reached, moving to next step");
        // qInfo("Before increment: stepIndex=%d, sweepIndex=%d", stepIndex, sweepIndex);
        stepIndex++;
        sweepIndex = 0;
        isEndSweep = false;
        measuredIaMax = 0.0;      // ← ADD THIS
        measuredIg2Max = 0.0;     // ← ADD THIS
        // qInfo("After increment: stepIndex=%d, sweepIndex=%d, isEndSweep=%d", stepIndex, sweepIndex, isEndSweep);
        // qInfo("Reset isEndSweep to false for stepIndex=%d", stepIndex);

        if (stepIndex < stepParameter.length()) {// There is another sweep to measure
            // qInfo("=== NEW SWEEP DEBUG ===");
            // qInfo("Creating new sweep - stepIndex: %d, total steps: %d", stepIndex, stepParameter.length());
            double v1Nominal = stepValue.at(stepIndex);
            double v2Nominal = 0.0;
            if (deviceType == PENTODE) {
                if (testType == ANODE_CHARACTERISTICS) {
                    v2Nominal = screenStart;
                } else {
                    v2Nominal = anodeStart;
                }
            }

            result->nextSweep(v1Nominal, v2Nominal);
            // qInfo("Created new sweep for stepIndex: %d, v1Nominal: %f, v2Nominal: %f", stepIndex, v1Nominal, v2Nominal);

            // Verify hardware is at safe 0V state before starting sweep
            // qInfo("=== HARDWARE VERIFICATION ===");
            sendCommand("M1"); // Discharge capacitors

            // Set grid voltage for new step
            // qInfo("Setting grid voltage for new step: S2 %d", stepParameter.at(stepIndex));
            if (isDoubleTriode && stepCommandPrefix == "S6 ") {
                qInfo("Command: S6 %d (secondary grid step)", stepParameter.at(stepIndex));
            }
            if (isDoubleTriode && stepCommandPrefix == "S6 ") {
                sendCommand("S2 0");
                sendCommand(buildSetCommand("S6 ", stepParameter.at(stepIndex)));
            } else {
                // Log S2 setting at the start of each new sweep (Anode Characteristics)
                if (stepCommandPrefix == "S2 ") {
                    double gridV = (stepIndex < stepValue.size()) ? stepValue.at(stepIndex) : 0.0;
                    qInfo("AnodeChar set S2 (new sweep): stepIndex=%d code=%d grid=%.3fV", stepIndex, stepParameter.at(stepIndex), gridV);
                }
                sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(stepIndex)));
            }

            // Set anode voltage to 0 for verification
            // qInfo("Setting anode voltage to 0V for verification");
            sendCommand(buildSetCommand("S3 ", 0));
            if (isDoubleTriode) {
                sendCommand(buildSetCommand("S7 ", 0));
            }

            // Take verification measurement
            // qInfo("Taking verification measurement to confirm 0V state");
            sendCommand("M2");

            // Set verification state for next response
            isVerifyingHardware = true;
            verificationAttempts = 0;
        } else {
            // qInfo("Test completed, sending M1");
            sendCommand("M1");
            isDataSetValid = true;
            isTestRunning = false;

            // qInfo("Calling client->testFinished()");
            client->testFinished();
        }
    }

    int progress = ((stepIndex * sweepPoints) + sweepIndex) * 100 / (sweepPoints * stepParameter.length());
    // qInfo("Sending progress: %d", progress);
    client->testProgress(progress);
    // qInfo("Sweep progress: %d%%", progress); // Added debug log
}

void Analyser::abortTest()
{
    // qInfo("Analyser: abortTest called, isTestRunning=%d", isTestRunning);
    isTestRunning = false;
    isTestAborted = true;
    isDataSetValid = false;
    commandBuffer.clear();

    client->testAborted();
}

QString Analyser::buildSetCommand(QString command, int value)
{
    QString stringValue;
    stringValue.setNum(value);
    command.append(stringValue);

    return command;
}

void Analyser::sendCommand(QString command)
{
    // qInfo("=== SEND COMMAND DEBUG ===");
    // qInfo("Attempting to send command: %s", command.toStdString().c_str());
    // qInfo("awaitingResponse: %d, commandBuffer.size(): %d", awaitingResponse, commandBuffer.size());

    if (awaitingResponse) { // Need to wait for previous command to complete (or timeout) before sending next command
        // qInfo("Buffering command (waiting for previous response): %s", command.toStdString().c_str());
        commandBuffer.append(command);

        return;
    }

    // qInfo("Sending command immediately: %s", command.toStdString().c_str());

    if (command.startsWith("S") || command.startsWith("M")) {
        // qDebug("Sending command: %s", command.toStdString().c_str());
    }

    // Track ACKs for primary anode (S3), secondary anode/screen (S7), and measurement (M2)
    if (command.startsWith("S3") || command.startsWith("S7") || command.startsWith("M2")) {
        expectedResponses++;
        // qInfo("Incremented expectedResponses to: %d", expectedResponses);
    }

    QByteArray c = command.toLatin1();

    serialPort->write(c);
    serialPort->write("\r\n");

    timeoutTimer->start(30000);
    awaitingResponse = true;
    // qInfo("Command sent, awaitingResponse set to true");
}

void Analyser::nextCommand()
{
    // qInfo("=== NEXT COMMAND DEBUG ===");
    // qInfo("nextCommand called, commandBuffer.size(): %d", commandBuffer.size());

    if (!commandBuffer.isEmpty()) { // There is a command to send
        QString command = commandBuffer.takeFirst();
        // qInfo("Processing buffered command: %s", command.toStdString().c_str());
        // qInfo("commandBuffer.size() after takeFirst: %d", commandBuffer.size());
        sendCommand(command);
    } else {
        // qInfo("No buffered commands to process");
    }
}

void Analyser::checkResponse(QString response)
{
    timeoutTimer->stop();

    if (sweepIndex == 0) {
        isEndSweep = false;
        measuredIaMax = 0.0;     // ← ADD THIS  
        measuredIg2Max = 0.0;    // ← ADD THIS
        // qInfo("Reset isEndSweep to false for new sweep stepIndex=%d", stepIndex);
    }

    // qInfo("Received response: %s", response.toStdString().c_str());

    QString message = " Response received: ";
    message += response;
    // qInfo(message.toStdString().c_str());

    if (response == "\n") {
        return;
    }

    if (isStopRequested) {
        isStopRequested = false;
        isTestRunning = false;
        return;
    }

    if (response.startsWith("OK: Get")) {
        QRegularExpressionMatch match = getMatcher->match(response);
        if (match.lastCapturedIndex() == 2) {
            int variable = match.captured(1).toInt();
            int value = match.captured(2).toInt();

            if (variable == VH) {
                double measuredHeaterVoltage = convertMeasuredVoltage(HEATER, value);
                aveHeaterVoltage = aveHeaterVoltage * 0.75 + measuredHeaterVoltage;
            } else if (variable == IH) {
                double measuredHeaterCurrent = convertMeasuredCurrent(HEATER, value);
                aveHeaterCurrent = aveHeaterCurrent * 0.75 + measuredHeaterCurrent;
            }
        }
    } else if (response.startsWith("OK: Info")) {
        QRegularExpressionMatch match = infoMatcher->match(response);
        if (match.lastCapturedIndex() == 2) {
            int info = match.captured(1).toInt();
            QString value = match.captured(2);

            if (info == 0) {
                hwVersion = value;
                isMega = (hwVersion == "Rev 2 (Nano3)");
                if (isMega) {
                    vRefSlave = 4.1;
                }
            } else if (info == 1) {
                swVersion = value;
            }
        }
    } else if (response.startsWith("OK: Set")) {
        // qInfo("Processing OK: Set response");
        expectedResponses--;
        // qInfo("Decremented expectedResponses to: %d", expectedResponses);
    } else if (response.startsWith("OK: Mode(2)")) {
        if (isTestRunning) {
            Sample *sample = createSample(response);

            double va = sample->getVa();
            double ia = sample->getIa();
            double va2 = sample->getVa2();
            double ia2 = sample->getIa2();

            // Handle verification measurements
            if (isVerifyingHardware) {
                // qInfo("=== VERIFICATION MEASUREMENT ===");
                // qInfo("Verification measurement: va=%.1fV, ia=%.3fmA", va, ia);

                if (va < 1.0 && ia < 0.001 && va2 < 1.0 && ia2 < 0.001) { // Close enough to 0V/0mA
                    // qInfo("Verification PASSED - hardware at safe 0V state");
                    isVerifyingHardware = false;
                    verificationAttempts = 0;

                    // Now send the first actual sample
                    int firstSampleValue = sweepParameter.at(stepIndex).at(0);
                    // Ensure anodes are asserted for Transfer mode before first actual sample
                    if (testType == TRANSFER_CHARACTERISTICS) {
                        if (stepType == ANODE) {
                            if (stepIndex < stepParameter.length()) {
                                int anodeCode = stepParameter.at(stepIndex);
                                sendCommand(buildSetCommand("S3 ", anodeCode));
                                if (isDoubleTriode) {
                                    // Keep secondary anode matched to primary for this step
                                    sendCommand(buildSetCommand("S7 ", anodeCode));
                                }
                            }
                        } else {
                            int anodeCode = convertTargetVoltage(ANODE, anodeStart);
                            sendCommand(buildSetCommand("S3 ", anodeCode));
                            if (isDoubleTriode) {
                                sendCommand(buildSetCommand("S7 ", anodeCode));
                            }
                        }
                    }
                    // One-line fix for Anode Characteristics: re-assert grid (S2) after verification PASS
                    if (testType == ANODE_CHARACTERISTICS && stepCommandPrefix == "S2 " && stepIndex < stepParameter.length()) {
                        sendCommand(buildSetCommand("S2 ", stepParameter.at(stepIndex)));
                    }

                    if (isDoubleTriode && stepCommandPrefix == "S6 " && stepIndex < stepParameter.length()) {
                        const int primaryGrid = stepParameter.at(stepIndex);
                        sendCommand(buildSetCommand("S2 ", primaryGrid));
                    }

                    QString resumeCommand = buildSetCommand(sweepCommandPrefix, firstSampleValue);
                    // qInfo("Sending first actual sample after verification: %s", resumeCommand.toStdString().c_str());
                    sendCommand(resumeCommand);
                    if (isDoubleTriode) {
                        sendCommand(buildSetCommand("S7 ", firstSampleValue));
                    }
                    sendCommand("M2");
                } else {
                    verificationAttempts++;

                    if (verificationAttempts >= MAX_VERIFICATION_ATTEMPTS) {
                        qWarning("Verification failed after %d attempts - aborting sweep", MAX_VERIFICATION_ATTEMPTS);
                        isVerifyingHardware = false;
                        verificationAttempts = 0;
                        isEndSweep = true;
                        return;
                    } else {
                        // Retry: discharge, force both anodes and grids to 0V, then verify at 0/0/0/0
                        sendCommand("M1");
                        sendCommand("S3 0");
                        if (isDoubleTriode) sendCommand("S7 0");
                        sendCommand("S2 0");
                        if (isDoubleTriode) sendCommand("S6 0");
                        sendCommand("M2");
                    }
                }
            } else {
                // Normal sample processing
                // Hard limit: if either channel reaches 50 mA or more, end this sweep
                if (ia >= 50.0 || ia2 >= 50.0) {
                    isEndSweep = true;
                }

                double power1 = ia * va / 1000.0;
                double power2 = ia2 * va2 / 1000.0;
                if (ia > iaMax || ia2 > iaMax || power1 > pMax || power2 > pMax) {
                    isEndSweep = true;
                    // qInfo("SWEEP LIMIT: ia=%.3fmA va=%.1fV (limits: %.1fmA %.3fW)", ia, va, iaMax, pMax);
                }

                result->addSample(sample);

                if (!isEndSweep) {
                    sweepIndex++;
                }

                nextSample();
            }
        }

        if (expectedResponses <= 0 && isDataSetValid) {
            isTestRunning = false;
        }
    } else if (!response.startsWith("OK:")) {
        abortTest();
    }

    // At this point, the response has been fully processed, any additional test commands have been queued, so
    // we can now flag that we're no longer waiting for a response and process the next command in the buffer
    // qInfo("=== RESPONSE PROCESSED DEBUG ===");
    // qInfo("Response fully processed, setting awaitingResponse = false");
    // qInfo("About to call nextCommand, commandBuffer.size(): %d", commandBuffer.size());
    awaitingResponse = false;
    nextCommand();
}

void Analyser::handleReadyRead()
{
    QByteArray data = serialPort->readAll();
    // qInfo("=== HANDLE READY READ DEBUG ===");
    // qInfo("Received %d bytes from serial port", data.size());
    // qInfo("Raw data: %s", data.toStdString().c_str());

    serialBuffer.append(data);

    if (awaitingResponse) {
        // qInfo("Currently awaiting response, checking for line ending");
        if (serialBuffer.contains('\n') || serialBuffer.contains('\r')) {
            // We have a complete line and so can process it as a response
            // qInfo("Found complete line, processing response");
            // qInfo("Full buffer: %s", serialBuffer.toStdString().c_str());

            checkResponse(serialBuffer);

            serialBuffer.clear();
            // qInfo("Buffer cleared after processing response");
        } else {
            // qInfo("Incomplete line, waiting for more data");
        }
    } else {
        // qInfo("Not awaiting response - unexpected data received");
        // We should log the unexpected characters
    }
}

void Analyser::handleCommandTimeout()
{
    qWarning("Test timeout");
    if (expectedResponses > 0) {
        qWarning("Timeout with %d unprocessed responses - data may be incomplete", expectedResponses);
    }
    awaitingResponse = false;
    timeoutTimer->stop();

    abortTest();
}


void Analyser::setPreferences(PreferencesDialog *newPreferences)
{
    preferences = newPreferences;
}

void Analyser::applyGridReferenceBoth(double commandVoltage, bool enabled)
{
    // commandVoltage is the magnitude (e.g., 5 or 60). For grids, hardware expects a positive DAC code
    // to generate a negative grid potential, consistent with convertTargetVoltage(GRID, +magnitude).
    qInfo("applyGridReferenceBoth: cmd=%.3f enabled=%d portOpen=%d isTestRunning=%d awaitingResponse=%d",
          commandVoltage, enabled ? 1 : 0,
          (serialPort && serialPort->isOpen()) ? 1 : 0,
          isTestRunning ? 1 : 0,
          awaitingResponse ? 1 : 0);

    if (!serialPort || !serialPort->isOpen()) {
        qWarning("Grid reference requested but serial port is not open");
        return;
    }

    QString cmdPrimary;
    QString cmdSecondary;
    if (enabled) {
        // Desired actual grid potentials are negative of the magnitude
        const double desiredV = -commandVoltage;
        double cmdVg1 = desiredV;
        double cmdVg2 = desiredV;
        if (preferences) {
            // Preferences provide linear mapping based on measured low/high entries
            cmdVg1 = preferences->grid1CommandForDesired(desiredV);
            cmdVg2 = preferences->grid2CommandForDesired(desiredV);
        }
        // Hardware expects positive magnitude for DAC conversion
        int codeVg1 = convertTargetVoltage(GRID, std::fabs(cmdVg1));
        int codeVg2 = convertTargetVoltage(GRID, std::fabs(cmdVg2));
        cmdPrimary = buildSetCommand("S2 ", codeVg1);
        cmdSecondary = buildSetCommand("S6 ", codeVg2);
        qInfo("Grid ref (calibrated): desired=%.3f cmdVg1=%.3f cmdVg2=%.3f codes=(%d,%d)",
              desiredV, cmdVg1, cmdVg2, codeVg1, codeVg2);
    } else {
        cmdPrimary = "S2 0";
        cmdSecondary = "S6 0";
    }

    // If idle (no test running and not awaiting a response), write immediately without engaging
    // the test command pipeline. Otherwise, enqueue via sendCommand to preserve sequencing.
    if (!isTestRunning && !awaitingResponse) {
        serialPort->write(cmdPrimary.toLatin1());
        serialPort->write("\r\n");
        serialPort->write(cmdSecondary.toLatin1());
        serialPort->write("\r\n");
        // Do not start timeout or set awaitingResponse for these calibration nudges
        qInfo("Grid ref (immediate): %s, %s", cmdPrimary.toStdString().c_str(), cmdSecondary.toStdString().c_str());
    } else {
        sendCommand(cmdPrimary);
        sendCommand(cmdSecondary);
    }
}

void Analyser::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ReadError) {

    }
}

void Analyser::steppedSweep(double sweepStart, double sweepStop, double stepStart, double stepStop, double step)
{
    double increment = 1.0 / sweepPoints;

    // Handle both ascending and descending step ranges
    bool isDescending = stepStart > stepStop;
    double stepIncrement = isDescending ? -fabs(step) : fabs(step);
    double endCondition = isDescending ? stepStop - 0.01 : stepStop + 0.01;

    double stepVoltage = stepStart;

    stepValue.clear();
    stepParameter.clear();
    sweepParameter.clear();
    stepIndex = 0;
    sweepIndex = 0;

    // Ensure we generate at least one step
    int maxSteps = fabs((stepStop - stepStart) / step) + 1;
    int stepCount = 0;

    while (((isDescending && stepVoltage >= endCondition) || (!isDescending && stepVoltage <= endCondition)) && stepCount < maxSteps) {
        if (stepType == GRID) {
            stepValue.append(-stepVoltage);  // Store the actual negative voltage for grid
        } else {
            stepValue.append(stepVoltage);
        }

        stepParameter.append(convertTargetVoltage(stepType, stepVoltage));

        QList<int> thisSweep;
        double sweep = 0.0;
        while (sweep <= 1.01) {
            double sweepVoltage;
            if (preferences->getSamplingType() == SMP_LOGARITHMIC) {
                sweepVoltage = sweepStart + (sweepStop - sweepStart) * sampleFunction(sweep);
            } else {
                sweepVoltage = sweepStart + (sweepStop - sweepStart) * sweep;
            }
            thisSweep.append(convertTargetVoltage(sweepType, sweepVoltage));
            sweep += increment;
        }

        sweepParameter.append(thisSweep);

        stepVoltage += stepIncrement;
        stepCount++;
    }

    // qInfo("Generated sweep parameters: %d steps, each with %d points", stepParameter.length(), sweepParameter.at(0).length());
}

double Analyser::sampleFunction(double linearValue)
{
    // Converts a linear % value to a transformed % value to concentrate sweep sample points where there is most change
    if (deviceType == TRIODE && testType == ANODE_CHARACTERISTICS) { // A triode may start to conduct at any anode voltage so we can't predict where the "bend" will be...
        return linearValue; // ...so a linear sampling is best
    } else if (deviceType == PENTODE && testType == ANODE_CHARACTERISTICS) { // A Pentode has a knee as the anode voltage rises from 0v...
        // ...so we want more smaples early on - the code below uses the profile of a log pot to do the necessary bending

        // From: https://electronics.stackexchange.com/questions/304692/formula-for-logarithmic-audio-taper-pot
        // y = a.b^x - a
        // b = ((1 / ym) - 1)^2
        // a = 1 / (b - 1)

        //double b = pow((1.0 / ym) - 1.0, 2);

        double ym = 0.2;

        double b = ((1.0 / ym) - 1.0);
        b = b * b;

        double a = 1 / (b - 1.0);

        return a * pow(b, linearValue) - a;
    }

    return linearValue; // Backstop is to return linear sampling
}