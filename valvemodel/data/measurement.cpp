#include "measurement.h"
#include "sweep.h"
#include <QDebug>

Measurement::Measurement()
{
    reset();
}

void Measurement::reset()
{
    sweeps.clear();
}

void Measurement::addSweep(Sweep *sweep)
{
    sweep->setMeasurement(this);
    sweeps.append(sweep);
}

void Measurement::addSample(Sample *sample)
{
    int sweepIndex = sweeps.indexOf(currentSweep);
    // qInfo("=== ADDING SAMPLE TO SWEEP %d ===", sweepIndex);
    // qInfo("Sweep v1Nominal: %f, v2Nominal: %f", currentSweep->getVg1Nominal(), currentSweep->getVg2Nominal());
    // qInfo("Sample va: %f V, ia: %f mA", sample->getVa(), sample->getIa());
    currentSweep->addSample(sample);
    // qInfo("Sample added, sweep now has %d samples", currentSweep->count());
}

void Measurement::nextSweep(double v1Nominal, double v2Nominal)
{
    // qInfo("=== CREATING NEW SWEEP ===");
    // qInfo("v1Nominal: %f, v2Nominal: %f", v1Nominal, v2Nominal);
    currentSweep = new Sweep(deviceType, testType, v1Nominal, v2Nominal);
    addSweep(currentSweep);
    // qInfo("New sweep created, total sweeps: %d", sweeps.size());
}

void Measurement::fromJson(QJsonObject source)
{
    if (source.contains("deviceType") && source["deviceType"].isString()) {
        QString sDeviceType = source["deviceType"].toString();
        if (sDeviceType == "pentode") {
            deviceType = PENTODE;
        } else {
            deviceType = TRIODE;
        }
    }

    sweeps.clear();

    eSweepType sweepType = SWEEP_TRIODE_ANODE;

    if (source.contains("testType") && source["testType"].isString()) {
        QString sTestType = source["testType"].toString();
        if (sTestType == "anodeCharacteristics") {
            testType = ANODE_CHARACTERISTICS;
        } else if (sTestType == "transferCharacteristics") {
            testType = TRANSFER_CHARACTERISTICS;
        } else if (sTestType == "screenCharacteristics") {
            testType = SCREEN_CHARACTERISTICS;
        }
    }

    if (source.contains("vh") && source["vh"].isDouble()) {
        heaterVoltage = source["vh"].toDouble();
    }

    if (source.contains("iaMax") && source["iaMax"].isDouble()) {
        iaMax = source["iaMax"].toDouble();
    }

    if (source.contains("paMax") && source["paMax"].isDouble()) {
        pMax = source["paMax"].toDouble();
    }

    if (source.contains("anodeStart") && source["anodeStart"].isDouble()) {
        anodeStart = source["anodeStart"].toDouble();
    }

    if (source.contains("anodeStop") && source["anodeStop"].isDouble()) {
        anodeStop = source["anodeStop"].toDouble();
    }

    if (source.contains("anodeStep") && source["anodeStep"].isDouble()) {
        anodeStep = source["anodeStep"].toDouble();
    }

    if (source.contains("gridStart") && source["gridStart"].isDouble()) {
        gridStart = source["gridStart"].toDouble();
    }

    if (source.contains("gridStop") && source["gridStop"].isDouble()) {
        gridStop = source["gridStop"].toDouble();
    }

    if (source.contains("gridStep") && source["gridStep"].isDouble()) {
        gridStep = source["gridStep"].toDouble();
    }

    if (source.contains("screenStart") && source["screenStart"].isDouble()) {
        screenStart = source["screenStart"].toDouble();
    }

    if (source.contains("screenStop") && source["screenStop"].isDouble()) {
        screenStop = source["screenStop"].toDouble();
    }

    if (source.contains("screenStep") && source["screenStep"].isDouble()) {
        screenStep = source["screenStep"].toDouble();
    }

    if (source.contains("sweeps") && source["sweeps"].isArray()) {
        QJsonArray jsonSweeps = source["sweeps"].toArray();
        for (int i = 0; i < jsonSweeps.size(); i++) {
            if (jsonSweeps.at(i).isObject()) {
                Sweep *sweep = new Sweep(deviceType, testType);
                sweep->fromJson(jsonSweeps.at(i).toObject());
                sweeps.append(sweep);
            }
        }
    }
}

void Measurement::toJson(QJsonObject &destination)
{
    if (deviceType == TRIODE) {
        destination["deviceType"] = "triode";
    } else if (deviceType == PENTODE) {
        destination["deviceType"] = "pentode";
    }

    if (testType == ANODE_CHARACTERISTICS) {
        destination["testType"] = "anodeCharacteristics";
    }

    destination["vh"] = heaterVoltage;
    destination["iaMax"] = iaMax;
    destination["paMax"] = pMax;

    destination["anodeStart"] = anodeStart;
    destination["anodeStop"] = anodeStop;
    destination["anodeStep"] = anodeStep;

    destination["gridStart"] = gridStart;
    destination["gridStop"] = gridStop;
    destination["gridStep"] = gridStep;

    destination["screenStart"] = screenStart;
    destination["screenStop"] = screenStop;
    destination["screenStep"] = screenStep;

    QJsonArray jsonSweeps;

    for (int i = 0; i < sweeps.length(); i++) {
        Sweep *sweep = sweeps.at(i);

        QJsonObject jsonSweep;
        sweep->toJson(jsonSweep);

        jsonSweeps.append(jsonSweep);
    }

    destination["sweeps"] = jsonSweeps;
}

QTreeWidgetItem *Measurement::buildTree(QTreeWidgetItem *parent)
{
    qDebug("Measurement::buildTree called for %s %s", deviceName().toStdString().c_str(), testName().toStdString().c_str());
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_MEASUREMENT);

    item->setText(0, measurementName());
    item->setIcon(0, QIcon(":/icons/meter32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    qDebug("Measurement has %d sweeps", sweeps.size());
    for (int i = 0; i < sweeps.size(); i++) {
        qDebug("Building tree for sweep %d", i);
        if (sweeps.at(i) == nullptr) {
            qWarning("Sweep %d is null", i);
            continue;
        }
        sweeps.at(i)->buildTree(item);
    }

    parent->setExpanded(true);
    qDebug("Measurement::buildTree completed");

    return item;
}

QString Measurement::measurementName()
{
    return deviceName() + " " + testName();
}

QString Measurement::deviceName()
{
    switch(deviceType) {
    case TRIODE:
        return QString("Triode");
        break;
    case PENTODE:
        return QString("Pentode");
        break;
    }

    return QString("Unknown");
}

QString Measurement::testName()
{
    switch(testType) {
    case ANODE_CHARACTERISTICS:
        return QString("Anode Charcteristics");
        break;
    case TRANSFER_CHARACTERISTICS:
        return QString("Transfer Characteristics");
        break;
    }

    return QString("Unknown");
}

void Measurement::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Device", deviceName());
    addProperty(properties, "Test", testName());
    addProperty(properties, "Sweeps", QString("%1").arg(sweeps.size()));
}

QGraphicsItemGroup *Measurement::updatePlot(Plot *plot, Sweep *sweep)
{
    if (deviceType == TRIODE) {
        if (testType == ANODE_CHARACTERISTICS) {
            anodeAxes(plot);
            return createGroup(plotTriodeAnode(plot, sweep));
        } else if (testType == TRANSFER_CHARACTERISTICS) {
            transferAxes(plot);
            return createGroup(plotTriodeTransfer(plot, sweep));
        }
    } else if (deviceType == PENTODE) {
        if (testType == ANODE_CHARACTERISTICS) {
            anodeAxes(plot);
            return createGroup(plotPentodeAnode(plot, sweep));
        } else if (testType == TRANSFER_CHARACTERISTICS) {
            transferAxes(plot);
            return createGroup(plotPentodeTransfer(plot, sweep));
        } else if (testType == SCREEN_CHARACTERISTICS) {
            screenAxes(plot);
            return createGroup(plotPentodeScreen(sweep));
        }
    }

    return nullptr;
}

QGraphicsItemGroup *Measurement::updatePlot(Plot *plot)
{
    return updatePlot(plot, nullptr);
}

int Measurement::getDeviceType() const
{
    return deviceType;
}

void Measurement::setDeviceType(int newDeviceType)
{
    deviceType = newDeviceType;
}

int Measurement::getTestType() const
{
    return testType;
}

void Measurement::setTestType(int newTestType)
{
    testType = newTestType;
}

double Measurement::getHeaterVoltage() const
{
    return heaterVoltage;
}

void Measurement::setHeaterVoltage(double newHeaterVoltage)
{
    heaterVoltage = newHeaterVoltage;
}

double Measurement::getAnodeStart() const
{
    return anodeStart;
}

void Measurement::setAnodeStart(double newAnodeStart)
{
    anodeStart = newAnodeStart;
}

double Measurement::getAnodeStop() const
{
    return anodeStop;
}

void Measurement::setAnodeStop(double newAnodeStop)
{
    anodeStop = newAnodeStop;
}

double Measurement::getAnodeStep() const
{
    return anodeStep;
}

void Measurement::setAnodeStep(double newAnodeStep)
{
    anodeStep = newAnodeStep;
}

double Measurement::getGridStart() const
{
    return gridStart;
}

void Measurement::setGridStart(double newGridStart)
{
    gridStart = newGridStart;
}

double Measurement::getGridStop() const
{
    return gridStop;
}

void Measurement::setGridStop(double newGridStop)
{
    gridStop = newGridStop;
}

double Measurement::getGridStep() const
{
    return gridStep;
}

void Measurement::setGridStep(double newGridStep)
{
    gridStep = newGridStep;
}

double Measurement::getScreenStart() const
{
    return screenStart;
}

void Measurement::setScreenStart(double newScreenStart)
{
    screenStart = newScreenStart;
}

double Measurement::getScreenStop() const
{
    return screenStop;
}

void Measurement::setScreenStop(double newScreenStop)
{
    screenStop = newScreenStop;
}

double Measurement::getScreenStep() const
{
    return screenStep;
}

void Measurement::setScreenStep(double newScreenStep)
{
    screenStep = newScreenStep;
}

double Measurement::getIaMax() const
{
    return iaMax;
}

void Measurement::setIaMax(double newIaMax)
{
    iaMax = newIaMax;
}

double Measurement::getPMax() const
{
    return pMax;
}

void Measurement::setPMax(double newPMax)
{
    pMax = newPMax;
}

Sweep *Measurement::at(int i)
{
    return sweeps.at(i);
}

int Measurement::count()
{
    return sweeps.count();
}

bool Measurement::getShowScreen() const
{
    return showScreen;
}

void Measurement::setShowScreen(bool newShowScreen)
{
    showScreen = newShowScreen;
}

QList<QGraphicsItem *> *Measurement::plotTriodeAnode(Plot *plot, Sweep *sweep)
{
    QPen samplePen;
    samplePen.setColor(QColor::fromRgb(0, 0, 0));

    QList<QGraphicsItem *> *segments = new QList<QGraphicsItem *>();

    if (sweep == nullptr) {
        int nSweeps = sweeps.size();
        for (int i = 0; i < nSweeps; i++) {
            Sweep *thisSweep = sweeps.at(i);

            thisSweep->plotTriodeAnode(plot, &samplePen, segments);
        }
    } else {
        sweep->plotTriodeAnode(plot, &samplePen, segments);
    }
    return segments;
}

QList<QGraphicsItem *> *Measurement::plotTriodeTransfer(Plot *plot, Sweep *sweep)
{
    QPen samplePen;
    samplePen.setColor(QColor::fromRgb(0, 0, 0));

    QList<QGraphicsItem *> *segments = new QList<QGraphicsItem *>();

    if (sweep == nullptr) {
        int nSweeps = sweeps.size();
        for (int i = 0; i < nSweeps; i++) {
            Sweep *thisSweep = sweeps.at(i);

            thisSweep->plotTriodeTransfer(plot, &samplePen, segments);
        }
    } else {
        sweep->plotTriodeTransfer(plot, &samplePen, segments);
    }

    return segments;
}

QList<QGraphicsItem *> *Measurement::plotPentodeAnode(Plot *plot, Sweep *sweep)
{
    QPen samplePen;
    QPen samplePenS;
    samplePen.setColor(QColor::fromRgb(0, 0, 0));
    samplePenS.setColor(QColor::fromRgb(0, 255, 0));

    QList<QGraphicsItem *> *segments = new QList<QGraphicsItem *>();

    if (sweep == nullptr) {
        int nSweeps = sweeps.size();
        for (int i = 0; i < nSweeps; i++) {
            Sweep *thisSweep = sweeps.at(i);

            thisSweep->plotPentodeAnode(plot, &samplePen, segments);
            if (showScreen) {
                thisSweep->plotPentodeScreen(plot, &samplePenS, segments);
            }
        }
    } else {
        sweep->plotPentodeAnode(plot, &samplePen, segments);
        if (showScreen) {
            sweep->plotPentodeScreen(plot, &samplePenS, segments);
        }
    }

    return segments;
}

QList<QGraphicsItem *> *Measurement::plotPentodeTransfer(Plot *plot, Sweep *sweep)
{
    QPen samplePen;
    samplePen.setColor(QColor::fromRgb(0, 0, 0));

    QList<QGraphicsItem *> *segments = new QList<QGraphicsItem *>();

    if (sweep == nullptr) {
        int nSweeps = sweeps.size();
        for (int i = 0; i < nSweeps; i++) {
            Sweep *thisSweep = sweeps.at(i);

            thisSweep->plotPentodeTransfer(plot, &samplePen, segments);
        }
    } else {
        sweep->plotPentodeTransfer(plot, &samplePen, segments);
    }

    return segments;
}

QList<QGraphicsItem *> *Measurement::plotPentodeScreen(Sweep *sweep)
{
    QPen samplePen;
    samplePen.setColor(QColor::fromRgb(0, 0, 0));

    QList<QGraphicsItem *> *segments = new QList<QGraphicsItem *>();

    return segments;
}

void Measurement::anodeAxes(Plot *plot)
{
    plot->clear();

    double vaInterval = interval(anodeStop);
    double iaInterval = interval(iaMax);

    plot->setAxes(0.0, anodeStop, vaInterval, 0.0, iaMax, iaInterval, 2, 1);
}

void Measurement::transferAxes(Plot *plot)
{
    plot->clear();

    double vg1Interval = interval(gridStop);
    double iaInterval = interval(iaMax);

    plot->setAxes(-gridStop, 0.0, vg1Interval, 0.0, iaMax, iaInterval, 2, 1);
}

void Measurement::screenAxes(Plot *plot)
{
    plot->clear();

    double vg1Interval = interval(gridStop);
    double iaInterval = interval(iaMax);

    plot->setAxes(-gridStop, 0.0, vg1Interval, 0.0, iaMax, iaInterval, 2, 1);
}

void Measurement::propertyEdited(QTableWidgetItem *item)
{

}

double Measurement::interval(double maxValue)
{
    double interval = 0.5;

    if (maxValue > 5.0) {
        interval = 1.0;
    }
    if (maxValue > 10.0) {
        interval = 2.0;
    }
    if (maxValue > 20.0) {
        interval = 5.0;
    }
    if (maxValue > 50.0) {
        interval = 10.0;
    }
    if (maxValue > 100.0) {
        interval = 20.0;
    }
    if (maxValue > 200.0) {
        interval = 50.0;
    }
    if (maxValue > 500.0) {
        interval = 100.0;
    }

    return interval;
}
