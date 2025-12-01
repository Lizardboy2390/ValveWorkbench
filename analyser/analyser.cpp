#include "analyser.h"
#include "client.h"
#include <QSerialPort>
#include <QTimer>

#include "../valvemodel/data/sample.h"
#include "../valvemodel/data/sweep.h"

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
    QRegularExpressionMatch match = sampleMatcher2->match(response);

    if (!match.hasMatch()) {
        qWarning("Failed to parse Mode(2) response: %s", response.toStdString().c_str());
        return nullptr;
    }

    qInfo("Raw capture: vh=%d ih=%d vg1=%d va1=%d ia1=%d ia1_lo=%d vg3=%d va2=%d ia2=%d ia2_lo=%d ia_hi=%d ig2_hi=%d",
          match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt(), match.captured(4).toInt(),
          match.captured(5).toInt(), match.captured(6).toInt(), match.captured(7).toInt(), match.captured(8).toInt(),
          match.captured(9).toInt(), match.captured(10).toInt(), match.captured(11).toInt(), match.captured(12).toInt());

    double vg1 = convertMeasuredVoltage(GRID, match.captured(3).toInt());       // Commanded grid bias (not sensed)
    double va = convertMeasuredVoltage(ANODE, match.captured(4).toInt());        // Primary anode voltage
    double ia = convertMeasuredCurrent(ANODE, match.captured(5).toInt(), match.captured(6).toInt(), match.captured(11).toInt()) * 1000;
    double vg2 = 0.0;                                                           // Screen grid (pentode) or unused
    double ig2 = 0.0;
    double vg3 = 0.0;                                                           // Commanded grid bias for secondary triode
    double va2 = 0.0;                                                           // Secondary anode voltage
    double ia2 = 0.0;

    if (isDoubleTriode) {
        vg3 = convertMeasuredVoltage(GRID, match.captured(7).toInt());          // Commanded secondary grid bias
        va2 = convertMeasuredVoltage(ANODE, match.captured(8).toInt());         // Secondary anode sense channel
        ia2 = convertMeasuredCurrent(ANODE, match.captured(9).toInt(), match.captured(10).toInt(), match.captured(12).toInt()) * 1000;

        qInfo("Sample (double triode): vg1(set)=%.3f vg3(set)=%.3f va=%.3f va2=%.3f ia=%.3f ia2(pre)=%.4f",
              vg1, vg3, va, va2, ia, ia2);
        qInfo("Raw ADC primary: HI=%d LO=%d (%.3fmA)",
              match.captured(5).toInt(), match.captured(6).toInt(), ia);
        qInfo("Raw ADC secondary: HI=%d LO=%d HI2=%d -> raw=%.4fmA",\
              match.captured(9).toInt(), match.captured(10).toInt(), match.captured(12).toInt(), ia2);
    } else {
        vg2 = convertMeasuredVoltage(SCREEN, match.captured(8).toInt());
        ig2 = convertMeasuredCurrent(SCREEN, match.captured(9).toInt(), match.captured(10).toInt(), match.captured(12).toInt()) * 1000;

        qInfo("Sample (single channel): vg1(set)=%.3f vg2=%.3f va=%.3f ia=%.3f ig2=%.3f",
              vg1, vg2, va, ia, ig2);
        qInfo("Raw ADC primary: HI=%d LO=%d (%.3fmA)",
              match.captured(5).toInt(), match.captured(6).toInt(), ia);
    }
    double vh = match.captured(1).toDouble();
    double ih = match.captured(2).toDouble();

    // Debug raw ADC values
    int rawCurrent = match.captured(5).toInt();
    int rawCurrentLo = match.captured(6).toInt();
    int rawCurrentHi = match.captured(11).toInt();

    // qInfo("Raw ADC values - Current: %d, CurrentLo: %d, CurrentHi: %d", rawCurrent, rawCurrentLo, rawCurrentHi);

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

void Analyser::setIsTriodeConnectedPentode(bool enable)
{
    // In triode-connected pentode mode we still report deviceType as PENTODE
    // so the measurement knows this is a pentode tube, but the analyser
    // drives the anode and screen together (S3 and S7 track) during anode
    // characteristics sweeps.
    isTriodeConnectedPentode = enable;
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

    // For triode-connected pentode mode we still drive pentode hardware
    // (S3 anode, S7 screen tracking), but we want the saved measurement to
    // behave like a triode anode-characteristics set for modelling and
    // seeding. Record it as a TRIODE and tag the triode-connected flag.
    if (isTriodeConnectedPentode && testType == ANODE_CHARACTERISTICS) {
        result->setDeviceType(TRIODE);
        result->setTriodeConnectedPentode(true);
    } else {
        result->setDeviceType(deviceType);
    }
    result->setIaMax(iaMax);
    result->setPMax(pMax);

    // Select firmware-side current averaging factor.
    // Default behaviour is automatic based on expected max anode current (iaMax, in mA),
    // but a Fixed mode in Preferences can override this with a user-specified sample count.
    int autoSamples = 3;
    if (iaMax <= 5.0) {
        autoSamples = 8;  // small-signal tubes
    } else if (iaMax <= 30.0) {
        autoSamples = 5;  // medium-current tubes
    } else {
        autoSamples = 3;  // high-current / power tubes
    }

    int avgSamples = autoSamples;
    if (preferences) {
        int mode = preferences->getAveragingMode();   // 0 = Auto, 1 = Fixed
        int fixedN = preferences->getAveragingFixedSamples();
        if (mode == 1) {
            if (fixedN < 1) fixedN = 1;
            if (fixedN > 8) fixedN = 8;
            avgSamples = fixedN;
        }
    }

    sendCommand(buildSetCommand("S0 ", avgSamples));

    measuredIaMax = 0.0;
    measuredIg2Max = 0.0;

    isStopRequested = false;
    isTestAborted = false;
    isTestRunning = true;
    isEndSweep = false;
    isDataSetValid = false;

    switch (testType) {
    case ANODE_CHARACTERISTICS: {
        stepType = GRID;
        stepCommandPrefix = "S2 ";
        sweepType = ANODE;
        sweepCommandPrefix = "S3 ";

        result->setAnodeStart(anodeStart);
        result->setAnodeStop(anodeStop);
        result->setAnodeStep(anodeStep);

        // For pentode anode characteristics, the physical grid bias is the
        // negative of the configured magnitude (gridStart). Use the actual
        // negative bias as Vg1 nominal so the first sweep reports the same
        // sign as the hardware (e.g. -20 V instead of +20 V).
        double vg1Nominal = gridStart;
        if (deviceType == PENTODE) {
            vg1Nominal = -gridStart;
        }

        result->nextSweep(vg1Nominal, screenStart);

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

        if (deviceType == PENTODE) { // Anode swept, Grid stepped, Screen fixed or tracking
            result->setScreenStart(screenStart);

            // Generate sweep/step parameters: anode sweep, grid step
            steppedSweep(anodeStart, anodeStop, gridStart, gridStop, gridStep);

            // Initial hardware verification at anodeStart and screenStart before the very first
            // sample to avoid Ig2 spike with Va~0. Mark that the upcoming Mode(2) is a
            // verification measurement so it is not stored as a data point.
            isVerifyingHardware = true;
            verificationAttempts = 0;

            // Sequence: S7 screenStart, M1 discharge, S3 anodeStart, M2 verify -> checkResponse
            // will handle PASS and reassert S7 and proceed with the first real sweep point.
            sendCommand(buildSetCommand("S7 ", convertTargetVoltage(SCREEN, screenStart)));
            sendCommand("M1");
            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
            sendCommand("M2");
        } else if (isDoubleTriode) { 
            result->setAnodeStart(anodeStart);

            int initialSecondaryGrid = convertTargetVoltage(GRID, secondGridStart);
            qInfo("Command: S6 %d (initial secondary grid)", initialSecondaryGrid);

            // Set BOTH grids to the same initial code, then bias both anodes
            sendCommand(buildSetCommand("S2 ", initialSecondaryGrid));
            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
            sendCommand(buildSetCommand("S6 ", initialSecondaryGrid));
            sendCommand(buildSetCommand("S7 ", convertTargetVoltage(ANODE, secondAnodeStart)));
        }
        if (isDoubleTriode && stepCommandPrefix == "S6 ") {
            int initialGridCode = stepParameter.at(0);
            // First grid family: drive S2 and S6 with the same step code
            sendCommand(buildSetCommand("S2 ", initialGridCode));
            sendCommand(buildSetCommand("S6 ", initialGridCode));
        } else {
            // Log initial S2 setting at start of first sweep (Anode Characteristics)
            if (stepCommandPrefix == "S2 ") {
                double gridV = stepValue.isEmpty() ? 0.0 : stepValue.at(0);
                qInfo("AnodeChar set S2 (initial): code=%d grid=%.3fV", stepParameter.at(0), gridV);
            }
            // For pentode anode characteristics, S2 will be reasserted after verification PASS in checkResponse()
            if (!(deviceType == PENTODE && testType == ANODE_CHARACTERISTICS && stepCommandPrefix == "S2 ")) {
                sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(0)));
            }
        }

        // If we initiated verification (pentode anode characteristics), do not advance to nextSample yet.
        // The verification Mode(2) will drive the PASS path that asserts S7 and sends the first real sample.
        if (!(deviceType == PENTODE && testType == ANODE_CHARACTERISTICS)) {
            nextSample();
        }
        break;
    }
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

            steppedSweep(gridStop, gridStart, screenStart, screenStop, screenStep);

            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
        } else if (isDoubleTriode) { // Both anodes step, both grids sweep together
            // Step both anodes together, sweep both grids together (same sweep values)
            stepType = ANODE;
            stepCommandPrefix = "S3 ";
            sweepType = GRID;
            sweepCommandPrefix = "S2 ";

            result->setAnodeStart(anodeStart);
            result->setAnodeStop(anodeStop);
            result->setAnodeStep(anodeStep);
            result->nextSweep(anodeStart);

            // Sweep grids from stop -> start (finish on lower absolute magnitude)
            steppedSweep(gridStop, gridStart, anodeStart, anodeStop, anodeStep);

            const int anodeStartCode = convertTargetVoltage(ANODE, anodeStart);
            const int gridStartCode  = convertTargetVoltage(GRID, gridStart);
            // Initialize both anodes to start and both grids to start
            sendCommand(buildSetCommand("S3 ", anodeStartCode)); // primary anode
            sendCommand(buildSetCommand("S7 ", anodeStartCode)); // secondary anode
            sendCommand(buildSetCommand("S2 ", gridStartCode));  // primary grid
            sendCommand(buildSetCommand("S6 ", gridStartCode));  // secondary grid
        } else { // Anode stepped, Grid swept
            stepType = ANODE;
            stepCommandPrefix = "S3 ";

            result->nextSweep(anodeStart);

            steppedSweep(gridStop, gridStart, anodeStart, anodeStop, anodeStep); // Sweep is reversed to finish on low (absolute) value

            sendCommand("S7 0");
            // Ensure anode is set to starting voltage before beginning grid sweep
            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
        }
        sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(0)));

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

        // Discharge capacitors at the first point of each sweep
        if (sweepIndex == 0) {
            if (testType == TRANSFER_CHARACTERISTICS) {
                // Transfer: discharge and reassert anode before first grid point
                sendCommand("M1");
                if (stepType == ANODE) {
                    if (stepIndex < stepParameter.length()) {
                        sendCommand(buildSetCommand("S3 ", stepParameter.at(stepIndex)));
                    }
                } else {
                    sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
                }
            } else if (testType == ANODE_CHARACTERISTICS) {
                // Anode Characteristics: discharge before first anode point
                // Bring screen to 0 V for verification in pentode mode, then re-assert screen voltage
                if (deviceType == PENTODE) {
                    sendCommand("S7 0");
                }
                sendCommand("M1");
                if (deviceType == PENTODE) {
                    // Re-apply intended screen voltage so the sweep runs at the test setting
                    sendCommand(buildSetCommand("S7 ", convertTargetVoltage(SCREEN, screenStart)));
                }
                // No need to reassert anode here; the next sweep command will set S3
            }
        }

        if (isDoubleTriode && sweepType == GRID) {
            // TRANSFER (double triode): sweep both grids together (S2 and S6)
            QString cmdS2 = buildSetCommand("S2 ", sweepValue);
            QString cmdS6 = buildSetCommand("S6 ", sweepValue);
            qInfo("Command: %s (primary grid sweep)", cmdS2.toStdString().c_str());
            sendCommand(cmdS2);
            qInfo("Command: %s (secondary grid sweep)", cmdS6.toStdString().c_str());
            sendCommand(cmdS6);
        } else {
            // Single triode or anode sweep cases (including triode-connected pentode)
            QString primaryCommand = buildSetCommand(sweepCommandPrefix, sweepValue);
            qInfo("Command: %s (primary sweep)", primaryCommand.toStdString().c_str());
            sendCommand(primaryCommand);

            // For double triode TRANSFER with ANODE sweep we mirror S3->S7.
            if (isDoubleTriode && sweepType == ANODE) {
                QString secondaryCommand = buildSetCommand("S7 ", sweepValue);
                qInfo("Command: %s (secondary anode tracking)", secondaryCommand.toStdString().c_str());
                sendCommand(secondaryCommand);
            }

            // In triode-connected pentode mode, treat S7 as the screen supply that
            // tracks the anode for every ANODE sweep point so the tube operates as
            // a triode-connected pentode without requiring a physical strap.
            if (isTriodeConnectedPentode && deviceType == PENTODE && sweepType == ANODE) {
                QString screenTrack = buildSetCommand("S7 ", sweepValue);
                qInfo("Command: %s (triode-connected screen tracking)", screenTrack.toStdString().c_str());
                sendCommand(screenTrack);
            }
        }
        if (testType == TRANSFER_CHARACTERISTICS || testType == ANODE_CHARACTERISTICS) {
            // Refire each point
            sendCommand("M6");
        }
        sendCommand("M2");
    } else {
        qInfo("=== END OF SWEEP DEBUG ===");
        qInfo("End of sweep reached, moving to next step");
        qInfo("Before increment: stepIndex=%d, sweepIndex=%d", stepIndex, sweepIndex);
        stepIndex++;
        sweepIndex = 0;
        isEndSweep = false;
        measuredIaMax = 0.0;      // ← ADD THIS
        measuredIg2Max = 0.0;     // ← ADD THIS
        qInfo("After increment: stepIndex=%d, sweepIndex=%d, isEndSweep=%d", stepIndex, sweepIndex, isEndSweep);

        if (stepIndex < stepParameter.length()) {// There is another sweep to measure
            qInfo("=== NEW SWEEP DEBUG ===");
            qInfo("Creating new sweep - stepIndex: %d, total steps: %d", stepIndex, stepParameter.length());
            double v1Nominal = stepValue.at(stepIndex);
            double v2Nominal = 0.0;
            if (deviceType == PENTODE) {
                if (testType == ANODE_CHARACTERISTICS) {
                    v2Nominal = screenStart;
                } else {
                    v2Nominal = anodeStart;
                }
            }

            // Start a new Measurement sweep for this step so that each grid
            // or screen family (e.g. each Vg2 in pentode transfer tests) is
            // represented as its own Sweep object for plotting and analysis.
            result->nextSweep(v1Nominal, v2Nominal);

            if (isDoubleTriode && stepType == ANODE) {
                // Double-triode TRANSFER: set both anodes to the new step code
                sendCommand(buildSetCommand("S3 ", stepParameter.at(stepIndex))); // primary anode step
                sendCommand(buildSetCommand("S7 ", stepParameter.at(stepIndex))); // secondary anode step
            } else {
                // Log S2 setting at the start of each new sweep (Anode Characteristics)
                if (stepCommandPrefix == "S2 ") {
                    double gridV = (stepIndex < stepValue.size()) ? stepValue.at(stepIndex) : 0.0;
                    qInfo("AnodeChar set S2 (new sweep): stepIndex=%d code=%d grid=%.3fV", stepIndex, stepParameter.at(stepIndex), gridV);
                }
                sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(stepIndex)));
            }
            // Determine target start voltages for the new sweep so verification
            // can confirm the rails are at the intended bias instead of at 0 V.
            double vaTarget = 0.0;
            bool hasVaTarget = false;
            double vg2Target = 0.0;
            bool hasVg2Target = false;

            if (testType == ANODE_CHARACTERISTICS) {
                // Anode characteristics: each sweep starts at anodeStart; pentode keeps a fixed screenStart.
                hasVaTarget = true;
                vaTarget = anodeStart;
                if (deviceType == PENTODE) {
                    hasVg2Target = true;
                    vg2Target = screenStart;
                }
            } else if (testType == TRANSFER_CHARACTERISTICS) {
                // Transfer tests:
                //  - Single/double triode: anode stepped (stepType == ANODE) or fixed (anodeStart).
                //  - Pentode: either fixed-screen + anode step, or fixed-anode + screen step.
                if (stepType == ANODE && stepIndex < stepValue.size()) {
                    hasVaTarget = true;
                    vaTarget = stepValue.at(stepIndex);
                } else {
                    hasVaTarget = true;
                    vaTarget = anodeStart;
                }

                if (deviceType == PENTODE) {
                    if (stepType == SCREEN && stepIndex < stepValue.size()) {
                        hasVg2Target = true;
                        vg2Target = stepValue.at(stepIndex);
                    } else {
                        hasVg2Target = true;
                        vg2Target = screenStart;
                    }
                }
            } else if (testType == SCREEN_CHARACTERISTICS) {
                // Screen characteristics: anode fixed at anodeStart, screen sweeps from screenStart.
                hasVaTarget = true;
                vaTarget = anodeStart;
                if (deviceType == PENTODE) {
                    hasVg2Target = true;
                    vg2Target = screenStart;
                }
            }

            // Only perform per-sweep verification for anode characteristics
            // (including pentode anode tests) and for pentode transfer tests
            // with SCREEN steps. Transfer and screen tests otherwise resume
            // directly without an extra M2 at the sweep start, so their first
            // stored sample is the real point.
            if (testType == ANODE_CHARACTERISTICS ||
                (testType == TRANSFER_CHARACTERISTICS && deviceType == PENTODE && stepType == SCREEN)) {
                isVerifyingHardware = true;
                verificationAttempts = 0;

                if (testType == ANODE_CHARACTERISTICS && isDoubleTriode) {
                    if (hasVaTarget) {
                        int vaCode = convertTargetVoltage(ANODE, vaTarget);
                        sendCommand("M1");
                        sendCommand(buildSetCommand("S3 ", vaCode));
                        sendCommand(buildSetCommand("S7 ", vaCode));
                    }
                } else {
                    if (hasVaTarget) {
                        sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, vaTarget)));
                    }
                    if (hasVg2Target) {
                        sendCommand(buildSetCommand("S7 ", convertTargetVoltage(SCREEN, vg2Target)));
                    }
                }

                // Take a verification measurement at the configured start bias for
                // the new sweep (not at 0 V). PASS/FAIL is handled in checkResponse().
                sendCommand("M2");
            } else {
                // For transfer and screen characteristics we do not perform per-sweep
                // verification; immediately start the next sweep by issuing the
                // first point.
                nextSample();
            }
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
            if (!sample) {
                qWarning("Mode(2) sample parse failed - aborting test to avoid invalid access");
                abortTest();
                return;
            }
            if (client) {
                client->updateHeater(sample->getVh(), sample->getIh());
            }

            double va = sample->getVa();
            double ia = sample->getIa();
            double va2 = sample->getVa2();
            double ia2 = sample->getIa2();

            // Handle verification measurements
            if (isVerifyingHardware) {
                // Determine the intended start anode/screen voltages for this sweep so we
                // can confirm the hardware is sitting at the configured bias before taking
                // the first real sample.
                double vaTarget = 0.0;
                bool hasVaTarget = false;
                double vg2Target = 0.0;
                bool hasVg2Target = false;

                if (testType == ANODE_CHARACTERISTICS) {
                    hasVaTarget = true;
                    vaTarget = anodeStart;
                    if (deviceType == PENTODE) {
                        hasVg2Target = true;
                        vg2Target = screenStart;
                    }
                } else if (testType == TRANSFER_CHARACTERISTICS) {
                    if (sweepType == GRID && stepType == ANODE && stepIndex < stepValue.size()) {
                        hasVaTarget = true;
                        vaTarget = stepValue.at(stepIndex);
                    } else {
                        hasVaTarget = true;
                        vaTarget = anodeStart;
                    }

                    // For pentode transfer tests, the screen rail is stepped (stepType == SCREEN)
                    // or held at screenStart. Mirror the per-sweep target logic from nextSample().
                    if (deviceType == PENTODE) {
                        if (stepType == SCREEN && stepIndex < stepValue.size()) {
                            hasVg2Target = true;
                            vg2Target = stepValue.at(stepIndex);
                        } else {
                            hasVg2Target = true;
                            vg2Target = screenStart;
                        }
                    }
                } else if (testType == SCREEN_CHARACTERISTICS) {
                    hasVaTarget = true;
                    vaTarget = anodeStart;
                    if (deviceType == PENTODE) {
                        hasVg2Target = true;
                        vg2Target = screenStart;
                    }
                }

                // Use a relative tolerance of ±1%% of the target voltage for both anode and screen.
                double epsV  = 0.0;
                double epsV2 = 0.0;
                if (hasVaTarget) {
                    epsV = std::fabs(vaTarget) * 0.01; // ±1%% of Va target
                    if (epsV < 2.0) {
                        epsV = 2.0;
                    }
                }
                if (hasVg2Target) {
                    epsV2 = std::fabs(vg2Target) * 0.01; // ±1%% of Vg2 target
                    if (epsV2 < 2.0) {
                        epsV2 = 2.0;
                    }
                }

                bool vaOk   = !hasVaTarget || std::fabs(va  - vaTarget)  <= epsV;
                bool vg2Ok  = true;
                if (deviceType == PENTODE && hasVg2Target) {
                    double vg2 = sample->getVg2();
                    vg2Ok = std::fabs(vg2 - vg2Target) <= epsV2;
                }

                if (vaOk && vg2Ok) {
                    // Verification PASSED - hardware at configured start bias
                    isVerifyingHardware = false;
                    verificationAttempts = 0;

                    // For pentode anode characteristics, ensure the screen rail
                    // is at the intended test voltage before resuming the first
                    // real sweep point. This fixes the case where the initial
                    // 0 V verification left S7 at 0 V for the first family.
                    if (deviceType == PENTODE && testType == ANODE_CHARACTERISTICS) {
                        sendCommand(buildSetCommand("S7 ", convertTargetVoltage(SCREEN, screenStart)));
                    }

                    // Proceed with the first actual sweep point as before. Guard
                    // against any out-of-range access in case sweepParameter is
                    // not populated as expected for this step.
                    if (stepIndex >= sweepParameter.size() || sweepParameter.at(stepIndex).isEmpty()) {
                        qWarning("Verification PASS but sweepParameter[%d] is out of range or empty - skipping resume and advancing", stepIndex);
                        isVerifyingHardware = false;
                        verificationAttempts = 0;
                        nextSample();
                        return;
                    }
                    int firstSampleValue = sweepParameter.at(stepIndex).at(0);

                    if (testType == TRANSFER_CHARACTERISTICS) {
                        if (stepType == ANODE) {
                            if (stepIndex < stepParameter.length()) {
                                sendCommand(buildSetCommand("S3 ", stepParameter.at(stepIndex)));
                            }
                        } else {
                            sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, anodeStart)));
                        }
                    }
                    if (testType == ANODE_CHARACTERISTICS && stepCommandPrefix == "S2 " && stepIndex < stepParameter.length()) {
                        sendCommand(buildSetCommand("S2 ", stepParameter.at(stepIndex)));
                    }
                    if (stepCommandPrefix == "S6 " && stepIndex < stepParameter.length()) {
                        const int primaryGrid = stepParameter.at(stepIndex);
                        sendCommand(buildSetCommand("S2 ", primaryGrid));
                    }

                    QString resumeCommand = buildSetCommand(sweepCommandPrefix, firstSampleValue);
                    sendCommand(resumeCommand);
                    if (isDoubleTriode) {
                        sendCommand(buildSetCommand("S7 ", firstSampleValue));
                    }
                    if (testType == TRANSFER_CHARACTERISTICS || testType == ANODE_CHARACTERISTICS) {
                        sendCommand("M6");
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
                    }

                    // Retry: discharge and re-apply start anode voltage, then verify again
                    sendCommand("M1");
                    double vaRetry = hasVaTarget ? vaTarget : anodeStart;
                    sendCommand(buildSetCommand("S3 ", convertTargetVoltage(ANODE, vaRetry)));
                    sendCommand("M2");
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
                    // Mark end-of-sweep; keep the measured sample as-is for storage
                    qInfo("AN LIMIT: end sweep on limit (va=%.3f ia=%.3f va2=%.3f ia2=%.3f p1=%.3f p2=%.3f)",
                          va, ia, va2, ia2, power1, power2);
                    isEndSweep = true;
                }

                qInfo("ADD SAMPLE: testType=%d stepIndex=%d sweepIndex=%d va=%.3f ia=%.3f",
                      (int)testType, stepIndex, sweepIndex, va, ia);
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