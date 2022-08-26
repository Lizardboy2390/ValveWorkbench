#include "measurement.h"

Measurement::Measurement()
{
}

void Measurement::reset()
{
    sweeps.clear();
    nextSweep();
}

void Measurement::addSweep(Sweep *sweep)
{
    sweeps.append(sweep);
}

void Measurement::addSample(Sample *sample)
{
    currentSweep->addSample(sample);
}

void Measurement::nextSweep()
{
    currentSweep = new Sweep(deviceType, testType);
    addSweep(currentSweep);
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
            sweepType = SWEEP_TRIODE_ANODE;
        } else {
            testType = ANODE_CHARACTERISTICS;
            sweepType = SWEEP_TRIODE_ANODE;
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
                Sweep *sweep = new Sweep(sweepType);
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
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_MEASUREMENT);

    item->setText(0, measurementName());
    item->setIcon(0, QIcon(":/icons/meter32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    for (int i = 0; i < sweeps.size(); i++) {
        sweeps.at(i)->buildTree(item);
    }

    parent->setExpanded(true);

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
        return QString("Anode Sweep");
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

void Measurement::updatePlot(Plot *plot)
{
    anodeAxes(plot);

    plotTriodeAnode(plot);
}

int Measurement::getDeviceType() const
{
    return deviceType;
}

int Measurement::getTestType() const
{
    return testType;
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

QList<QGraphicsItem *> *Measurement::plotTriodeAnode(Plot *plot)
{
    QPen samplePen;
    samplePen.setColor(QColor::fromRgb(0, 0, 0));

    QList<QGraphicsItem *> *segments = new QList<QGraphicsItem *>();

    int nSweeps = sweeps.size();
    for (int i = 0; i < nSweeps; i++) {
        Sweep *thisSweep = sweeps.at(i);

        Sample *firstSample = thisSweep->at(0);

        double vg = firstSample->getVg1();
        double va = firstSample->getVa();
        double ia = firstSample->getIa();

        int samples = thisSweep->count();
        for (int j = 1; j < samples; j++) {
             Sample *sample = thisSweep->at(j);

             double vaNext = sample->getVa();
             double iaNext = sample->getIa();

             segments->append(plot->createSegment(va, ia, vaNext, iaNext, samplePen));

             va = vaNext;
             ia = iaNext;
         }

        plot->createLabel(va, ia, vg);
    }

    return segments;
}

void Measurement::anodeAxes(Plot *plot)
{
    plot->clear();

    double vaInterval = interval(anodeStop);
    double iaInterval = interval(iaMax);

    plot->setAxes(0.0, anodeStop, vaInterval, 0.0, iaMax, iaInterval, 2, 1);
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
