#include "analyser.h"
#include "valvemodel/data/sample.h"

QRegularExpression *Analyser::sampleMatcher = new QRegularExpression(R"(^OK: Mode\(2\) (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+))");
QRegularExpression *Analyser::sampleMatcher2 = new QRegularExpression(R"(^OK: Mode\(2\) (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+))");
QRegularExpression *Analyser::getMatcher = new QRegularExpression(R"(^OK: Get\((\d+)\) = (\d+))");
QRegularExpression *Analyser::infoMatcher = new QRegularExpression(R"(^OK: Info\((\d+)\) = (.*)\r)");

Analyser::Analyser(Client *client_, QSerialPort *port, QTimer *timeout, QTimer *heater) : client(client_), serialPort(port), timeoutTimer(timeout), heaterTimer(heater)
{
    heaterTimer->start(2000); // waits for 2s before starting to poll the measured heater values

    //sendCommand("I0"); // Get the hardware version of the board
    //sendCommand("I1"); // Get the software version of the board
}

Analyser::~Analyser()
{
}

Sample *Analyser::createSample(QString response)
{
    QRegularExpressionMatch match = sampleMatcher2->match(response);

    double vg1 = convertMeasuredVoltage(GRID, match.captured(3).toInt());
    double va = convertMeasuredVoltage(ANODE, match.captured(4).toInt());
    double ia = convertMeasuredCurrent(ANODE, match.captured(5).toInt(), match.captured(6).toInt(), match.captured(11).toInt()) * 1000;
    double vg2 = convertMeasuredVoltage(SCREEN, match.captured(8).toInt());
    double ig2 = convertMeasuredCurrent(SCREEN, match.captured(9).toInt(), match.captured(10).toInt(), match.captured(12).toInt()) * 1000;
    double vh = convertMeasuredVoltage(HEATER, match.captured(1).toInt());
    double ih = convertMeasuredCurrent(HEATER, match.captured(2).toInt());

    // This line adjusts the anode current measurement to account for the current that always flows through the voltage sense network
    // The resistance of the voltage sense network (3 x 470k + 2 * 4k7) is 1.4194M and the current needs to be adjusted for mA
    ia = ia - va / 1419.4;
    // For very low anode currents, this could lead to very small negative values, hence...
    if (ia < 0.0) {
        ia = 0.0;
    }

    if (ia > measuredIaMax) {
        measuredIaMax = ia;
    }

    if (ig2 > measuredIg2Max) {
        measuredIg2Max = ig2;
    }

    return new Sample(vg1, va, ia, vg2, ig2, vh, ih);
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
        value *= 1.4; // Compensate for unexplained 15% underage - could be because measurement is not buffered onto the ADC
        break;
    case ANODE:
    case SCREEN:
        if (isMega && currentHi < 1023) { // We haven't saturated the ADC
            value = ((double) currentHi) * vRefMaster / 1023 / 8.0 / 33.333333;
        } else if (current < 1000) { // We haven't saturated the ADC
                                     // (slight safety factor as the voltage at saturation is 2.048v
                                     // and this could be close to onset of diode conduction)
            value = ((double) current) * vRefMaster / 1023 / 2.0 / 33.333333;
        } else {
            value = ((double) currentLo) * vRefMaster / 1023 / 2.0 / 3.333333;
        }
        /*voltageHi = ((double) current) / 1023 / 2.0 * vRefMaster;
        if (voltageHi < 1.9) {
            value = voltageHi / 33.333333;
        } else {
            value = (((double) currentLo) / 1023 / 2.0 * vRefMaster / 3.333333);
        }*/
        break;
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

void Analyser::setDeviceType(int newDeviceType)
{
    deviceType = newDeviceType;
}

void Analyser::setSweepPoints(int newSweepPoints)
{
    sweepPoints = newSweepPoints;
}

void Analyser::setSweepParameters(double aStart, double aStop, double aStep, double gStart, double gStop, double gStep, double sStart, double sStop, double sStep)
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
        result->setGridStart(gridStart);
        result->setGridStop(gridStop);
        result->setGridStep(gridStep);

        result->nextSweep(gridStart, screenStart);

        steppedSweep(anodeStart, anodeStop, gridStart, gridStop, gridStep);

        if (deviceType == PENTODE) { // Anode swept, Grid stepped, Screen fixed
            result->setScreenStart(screenStart);

            sendCommand(buildSetCommand("S7 ", convertTargetVoltage(SCREEN, screenStart)));
        } else { // Anode swept, Grid stepped
            sendCommand("S7 0");
        }
        sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(0)));

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
        } else { // Anode stepped, Grid swept
            stepType = ANODE;
            stepCommandPrefix = "S3 ";

            result->setAnodeStart(anodeStart);
            result->setAnodeStop(anodeStop);
            result->setAnodeStep(anodeStep);

            result->nextSweep(anodeStart);

            steppedSweep(gridStop, gridStart, anodeStart, anodeStop, anodeStep); // Sweep is reversed to finish on low (absolute) value

            sendCommand("S7 0");
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
    if (!isEndSweep && sweepIndex < sweepParameter.at(stepIndex).length()) {
        // Run the next value in the sweep
        sendCommand(buildSetCommand(sweepCommandPrefix, sweepParameter.at(stepIndex).at(sweepIndex)));
        sendCommand("M2");
        sweepIndex++;
    } else { // We've reached the end of this sweep so onto the next step
        stepIndex++;
        sweepIndex = 0;
        isEndSweep = false;

        if (stepIndex < stepParameter.length()) { // There is another sweep to measure
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

            //setupCommands.append("M1"); // Discharge the capacitor banks at the end of a sweep (or it may take a while)
            sendCommand(buildSetCommand(stepCommandPrefix, stepParameter.at(stepIndex)));
            sendCommand(buildSetCommand(sweepCommandPrefix, sweepParameter.at(stepIndex).at(sweepIndex)));
            sendCommand("M2");
        } else {
            // We've reached the end of the test!
            sendCommand("M1");
            isDataSetValid = true;
            isTestRunning = false;

            client->testFinished();
        }
    }

    int progress = ((stepIndex * sweepPoints) + sweepIndex) * 100 / (sweepPoints * stepParameter.length());
    client->testProgress(progress);
}

void Analyser::abortTest()
{
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
    if (awaitingResponse) { // Need to wait for previous command to complete (or timeout) before sending next command
        commandBuffer.append(command);

        return;
    }

    qInfo(command.toStdString().c_str());

    QByteArray c = command.toLatin1();

    serialPort->write(c);
    serialPort->write("\r\n");

    timeoutTimer->start(15000);
    awaitingResponse = true;
}

void Analyser::nextCommand()
{
    if (!commandBuffer.isEmpty()) { // There is a command to send
        QString command = commandBuffer.takeFirst();
        sendCommand(command);
    }
}

void Analyser::checkResponse(QString response)
{
    timeoutTimer->stop();

    QString message = " Response received: ";
    message += response;
    qInfo(message.toStdString().c_str());

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
                //client->updateHeater(measuredHeaterVoltage, -1.0);
                client->updateHeater(aveHeaterVoltage / 4.0, -1.0);
            } else if (variable == IH) {
                double measuredHeaterCurrent = convertMeasuredCurrent(HEATER, value);
                aveHeaterCurrent = aveHeaterCurrent * 0.75 + measuredHeaterCurrent;
                //client->updateHeater(-1.0, measuredHeaterCurrent);
                client->updateHeater(-1.0, aveHeaterCurrent / 4.0);
            }
        }
    } else if (response.startsWith("OK: Info")) {
        QRegularExpressionMatch match = infoMatcher->match(response);
        if (match.lastCapturedIndex() == 2) {
            int info = match.captured(1).toInt();
            QString value = match.captured(2);

            if (info == 0) {
                hwVersion = value;
                isMega = (hwVersion == "Rev 2 (Mega Pro)");
                if (isMega) {
                    vRefSlave = 4.096;
                }
            } else if (info == 1) {
                swVersion = value;
            }
        }
    } else if (response.startsWith("OK: Mode(2)")) {
        if (isTestRunning) {
            // Store the measurement
            Sample *sample = createSample(response);
            result->addSample(sample);
            double va = sample->getVa();
            double ia = sample->getIa();

            message = QString {"Anode voltage: %1v"}.arg(va, 6, 'f', 1, '0' );
            qInfo(message.toStdString().c_str());
            message = QString {"Anode current: %1mA"}.arg(ia, 6, 'f', 4, '0' );
            qInfo(message.toStdString().c_str());

            if (ia > iaMax || (ia * va / 1000.0) > pMax) {
                isEndSweep = true;
                qInfo("Ending sweep due to exceeding power threshold");
            }

            // (Ideally) Only update the heater display periodically (i.e. not every sample)
            // Was previously done at the beginning of a sweep but could do it on a sample counter instead
            if (sampleCount++ > 29) {
                client->updateHeater(sample->getVh(), sample->getIh());
                sampleCount = 0;
            }

            nextSample();
        }
    } else if (!response.startsWith("OK:")) {
        abortTest();
    }

    // At this point, the response has been fully processed, any additional test commands have been queued, so
    // we can now flag that we're no longer waiting for a response and process the next command in the buffer
    awaitingResponse = false;
    nextCommand();
}

void Analyser::handleReadyRead()
{
    serialBuffer.append(serialPort->readAll());

    if (awaitingResponse) {
        if (serialBuffer.contains('\n') || serialBuffer.contains('\r')) {
            // We have a complete line and so can process it as a response
            qInfo(serialBuffer);

            checkResponse(serialBuffer);

            serialBuffer.clear();
        }
    } else {
        // We should log the unexpected characters
    }
}

void Analyser::handleCommandTimeout()
{
    qWarning("Test timeout");
    awaitingResponse = false;
    timeoutTimer->stop();

    abortTest();
}

void Analyser::handleHeaterTimeout()
{
    if (isHeatersOn) { // Only poll if the heaters are on
        if (!isTestRunning) { // Only poll if we're not running a test
            sendCommand("G0");
            sendCommand("G1");
        }
    } else {
        client->updateHeater(0.0, 0.0);
    }

    if (!isVersionRead) {
        sendCommand("I0");
        sendCommand("I1");

        isVersionRead = true;
    }

    heaterTimer->start(500); // Do it again in 500ms...
}

void Analyser::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ReadError) {

    }
}

void Analyser::steppedSweep(double sweepStart, double sweepStop, double stepStart, double stepStop, double step)
{
    double increment = 1.0 / sweepPoints;

    double stepVoltage = stepStart;

    stepValue.clear();
    stepParameter.clear();
    sweepParameter.clear();
    stepIndex = 0;
    sweepIndex = 0;

    while (stepVoltage <= (stepStop + 0.01)) {
        if (stepType == GRID) {
            stepValue.append(-stepVoltage);
        } else {
            stepValue.append(stepVoltage);
        }
        stepParameter.append(convertTargetVoltage(stepType, stepVoltage));

        QList<int> thisSweep;

        double sweep = 0.0;
        while (sweep <= 1.01) {
            //double sweepVoltage = sweepStart + (sweepStop - sweepStart) * sampleFunction(sweep);
            double sweepVoltage = sweepStart + (sweepStop - sweepStart) * sweep;
            thisSweep.append(convertTargetVoltage(sweepType, sweepVoltage));
            sweep += increment;
        }

        sweepParameter.append(thisSweep);

        stepVoltage += step;
    }
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
