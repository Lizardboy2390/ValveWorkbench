#include "sweep.h"
#include "measurement.h"

Sweep::Sweep(eSweepType type_) : type(type_)
{

}

Sweep::Sweep(int deviceType, int testType, double v1Nominal, double v2Nominal)
{
    if (deviceType == TRIODE) {
        switch (testType) {
        case ANODE_CHARACTERISTICS:
            type = SWEEP_TRIODE_ANODE;
            vg1Nominal = v1Nominal;
            break;
        case TRANSFER_CHARACTERISTICS:
            type = SWEEP_TRIODE_GRID;
            vaNominal = v1Nominal;
            break;
        }
    } else {
        switch (testType) {
        case ANODE_CHARACTERISTICS:
            type = SWEEP_PENTODE_ANODE;
            vg1Nominal = v1Nominal;
            vg2Nominal = v2Nominal;
            break;
        case TRANSFER_CHARACTERISTICS:
            type = SWEEP_PENTODE_GRID;
            vg2Nominal = v1Nominal;
            vaNominal = v2Nominal;
            break;
        case SCREEN_CHARACTERISTICS:
            type = SWEEP_PENTODE_SCREEN;
            vg1Nominal = v1Nominal;
            vaNominal = v2Nominal;
            break;
        }
    }
}

void Sweep::addSample(Sample *sample)
{
    samples.append(sample);
}

int Sweep::count()
{
    return samples.size();
}

Sample *Sweep::at(int index)
{
    return samples.at(index);
}

void Sweep::fromJson(QJsonObject source)
{
    switch(type) {
    case SWEEP_TRIODE_ANODE:
        if (source.contains("vg1Nominal") && source["vg1Nominal"].isDouble()) {
            vg1Nominal = source["vg1Nominal"].toDouble();
        }
        break;
    case SWEEP_TRIODE_GRID:
        if (source.contains("vaNominal") && source["vaNominal"].isDouble()) {
            vaNominal = source["vaNominal"].toDouble();
        }
        break;
    case SWEEP_PENTODE_ANODE:
        if (source.contains("vg1Nominal") && source["vg1Nominal"].isDouble()) {
            vg1Nominal = source["vg1Nominal"].toDouble();
        }

        if (source.contains("vg2Nominal") && source["vg2Nominal"].isDouble()) {
            vg2Nominal = source["vg2Nominal"].toDouble();
        }
        break;
    case SWEEP_PENTODE_GRID:
        if (source.contains("vg2Nominal") && source["vg2Nominal"].isDouble()) {
            vg2Nominal = source["vg2Nominal"].toDouble();
        }

        if (source.contains("vaNominal") && source["vaNominal"].isDouble()) {
            vaNominal = source["vaNominal"].toDouble();
        }
        break;
    case SWEEP_PENTODE_SCREEN:
        if (source.contains("vg1Nominal") && source["vg1Nominal"].isDouble()) {
            vg1Nominal = source["vg1Nominal"].toDouble();
        }

        if (source.contains("vaNominal") && source["vaNominal"].isDouble()) {
            vaNominal = source["vaNominal"].toDouble();
        }
        break;
    }

    samples.clear();

    if (source.contains("samples") && source["samples"].isArray()) {
        QJsonArray jsonSamples = source["samples"].toArray();
        for (int i = 0; i < jsonSamples.size(); i++) {
            if (jsonSamples.at(i).isObject()) {
                Sample *sample = new Sample();
                sample->fromJson(jsonSamples.at(i).toObject());

                samples.append(sample);
            }
        }
    }
}

void Sweep::toJson(QJsonObject &destination)
{
    switch(type) {
    case SWEEP_TRIODE_ANODE:
        destination["vg1Nominal"] = vg1Nominal;
        break;
    case SWEEP_TRIODE_GRID:
        destination["vaNominal"] = vaNominal;
        break;
    case SWEEP_PENTODE_ANODE:
        destination["vg1Nominal"] = vg1Nominal;
        destination["vg2Nominal"] = vg2Nominal;
        break;
    case SWEEP_PENTODE_GRID:
        destination["vg2Nominal"] = vg2Nominal;
        destination["vaNominal"] = vaNominal;
        break;
    case SWEEP_PENTODE_SCREEN:
        destination["vg1Nominal"] = vg1Nominal;
        destination["vaNominal"] = vaNominal;
        break;
    }

    QJsonArray jsonSamples;
    for (int i = 0; i < samples.length(); i++) {
        QJsonObject sample;
        samples.at(i)->toJson(sample);

        jsonSamples.append(sample);
    }
    destination["samples"] = jsonSamples;
}

QTreeWidgetItem *Sweep::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_SWEEP);

    item->setText(0, sweepName());
    item->setIcon(0, QIcon(":/icons/sweep32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    for (int i = 0; i < samples.size(); i++) {
        samples.at(i)->buildTree(item);
    }

    return item;
}

QString Sweep::sweepName()
{
    switch(type) {
    case SWEEP_TRIODE_ANODE:
        return QString("Vg1 = %1").arg(vg1Nominal);
        break;
    case SWEEP_TRIODE_GRID:
        return QString("Va = %1").arg(vaNominal);
        break;
    case SWEEP_PENTODE_ANODE:
        return QString("Vg1 = %1, Vg2 = %2").arg(vg1Nominal).arg(vg2Nominal);
        break;
    case SWEEP_PENTODE_GRID:
        return QString("Va = %1, Vg2 = %2").arg(vaNominal).arg(vg2Nominal);
        break;
    case SWEEP_PENTODE_SCREEN:
        return QString("Vg1 = %1, Va = %2").arg(vg1Nominal).arg(vaNominal);
        break;
    }

    return QString("");
}

void Sweep::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    switch(type) {
    case SWEEP_TRIODE_ANODE:
        addProperty(properties, "Type", "Triode Anode");
        addProperty(properties, "Vg1", QString("%1").arg(vg1Nominal));
        break;
    case SWEEP_TRIODE_GRID:
        addProperty(properties, "Type", "Triode Grid");
        addProperty(properties, "Va", QString("%1").arg(vaNominal));
        break;
    case SWEEP_PENTODE_ANODE:
        addProperty(properties, "Type", "Pentode Anode");
        addProperty(properties, "Vg1", QString("%1").arg(vg1Nominal));
        addProperty(properties, "Vg2", QString("%1").arg(vg2Nominal));
        break;
    case SWEEP_PENTODE_GRID:
        addProperty(properties, "Type", "Pentode Grid");
        addProperty(properties, "Va", QString("%1").arg(vaNominal));
        addProperty(properties, "Vg2", QString("%1").arg(vg2Nominal));
        break;
    case SWEEP_PENTODE_SCREEN:
        addProperty(properties, "Type", "Pentode Screen");
        addProperty(properties, "Vg1", QString("%1").arg(vg1Nominal));
        addProperty(properties, "Va", QString("%1").arg(vaNominal));
        break;
    }
}

QGraphicsItemGroup *Sweep::updatePlot(Plot *plot)
{
    if (measurement != nullptr) {
        return(measurement->updatePlot(plot, this));
    }

    return nullptr;
}

double Sweep::getVaNominal() const
{
    return vaNominal;
}

void Sweep::setVaNominal(double newVaNominal)
{
    vaNominal = newVaNominal;
}

double Sweep::getVg1Nominal() const
{
    return vg1Nominal;
}

void Sweep::setVg1Nominal(double newVg1Nominal)
{
    vg1Nominal = newVg1Nominal;
}

double Sweep::getVg2Nominal() const
{
    return vg2Nominal;
}

void Sweep::setVg2Nominal(double newVg2Nominal)
{
    vg2Nominal = newVg2Nominal;
}

void Sweep::setMeasurement(Measurement *measurement)
{
    this->measurement = measurement;
}

Measurement *Sweep::getMeasurement() const
{
    return measurement;
}

void Sweep::plotTriodeAnode(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double va = firstSample->getVa();
    double ia = firstSample->getIa();

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vaNext = sample->getVa();
         double iaNext = sample->getIa();

         segments->append(plot->createSegment(va, ia, vaNext, iaNext, *samplePen));

         va = vaNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(va, ia, vg1Nominal));
}

void Sweep::plotTriodeTransfer(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double va = firstSample->getVa();
    double ia = firstSample->getIa();

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vgNext = sample->getVg1();
         double iaNext = sample->getIa();

         segments->append(plot->createSegment(vg, ia, vgNext, iaNext, *samplePen));

         vg = vgNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(vg, ia, vaNominal));
}

void Sweep::plotPentodeAnode(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double va = firstSample->getVa();
    double ia = firstSample->getIa();

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vaNext = sample->getVa();
         double iaNext = sample->getIa();

         segments->append(plot->createSegment(va, ia, vaNext, iaNext, *samplePen));

         va = vaNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(va, ia, vg1Nominal));
}

void Sweep::plotPentodeTransfer(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double vg2 = firstSample->getVg2();
    double ia = firstSample->getIa();

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vgNext = sample->getVg1();
         double iaNext = sample->getIa();

         segments->append(plot->createSegment(vg, ia, vgNext, iaNext, *samplePen));

         vg = vgNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(vg, ia, vg2Nominal));
}

void Sweep::propertyEdited(QTableWidgetItem *item)
{

}

