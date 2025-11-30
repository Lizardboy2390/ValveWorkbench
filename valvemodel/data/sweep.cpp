#include "sweep.h"
#include "measurement.h"

Sweep::Sweep(eSweepType type_) : type(type_)
{

}

Sweep::Sweep(int deviceType, int testType, double v1Nominal, double v2Nominal)
{
    // Ensure all nominal fields are in a defined state before test-specific assignment
    type = SWEEP_TRIODE_ANODE;
    vg1Nominal = 0.0;
    vg2Nominal = 0.0;
    vaNominal = 0.0;

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
        Sample *sample = samples.at(i);
        if (!sample) {
            continue;
        }
        sample->buildTree(item);
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
    // qInfo("=== SWEEP::PLOTTRIODEANODE - Sample count: %d ===", samples.count());

    // ADD VALIDATION FOR EMPTY SWEEPS
    if (samples.count() < 2) {
        // qWarning("Sweep has insufficient samples (%d) for plotting - skipping", samples.count());
        return;
    }

    Sample *firstSample = samples.at(0);
    double vg = firstSample->getVg1();

    // qInfo("First sample: vg1=%.6fV, va=%.6fV, ia=%.6fA", vg, firstSample->getVa(), firstSample->getIa());

    // DEBUG: Show all grid voltages being processed with high precision
    // qInfo("*** DEBUG: Processing sweep with grid voltage: %.6fV (samples: %d) ***", vg, samples.count());

    // DEBUG: Check for exact -1.5V sweep with high precision
    if (qAbs(vg - (-1.5)) < 0.0001) {
        // qInfo("*** DEBUG: FOUND EXACT -1.5V SWEEP with %.6fV and %d samples ***", vg, samples.count());
    }

    // DEBUG: Track segments for problematic voltages
    if (qAbs(vg - (-2.5)) < 0.1 || qAbs(vg - (-3.0)) < 0.1 || qAbs(vg - (-3.5)) < 0.1 || qAbs(vg - (-4.0)) < 0.1) {
        // qInfo("*** DEBUG: Processing problematic voltage %.6fV - tracking segments ***", vg);
    }

    double va = firstSample->getVa();
    double ia = firstSample->getIa();

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vaNext = sample->getVa();
         double iaNext = sample->getIa();

         // qInfo("Segment %d: va=%f->%f, ia=%f->%f", j, va, vaNext, ia, iaNext);

         // DEBUG: Track segments for problematic voltages
        if (qAbs(vg - (-2.5)) < 0.1 || qAbs(vg - (-3.0)) < 0.1 || qAbs(vg - (-3.5)) < 0.1 || qAbs(vg - (-4.0)) < 0.1) {
            // qInfo("*** DEBUG: Creating segment %d for %.6fV: va=%.6f->%.6f, ia=%.6f->%.6f ***", j, vg, va, vaNext, ia, iaNext);
            // qInfo("*** DEBUG: Current segments list size before adding: %d ***", segments->size());
        }

         QGraphicsLineItem *segment = plot->createSegment(va, ia, vaNext, iaNext, *samplePen);
         if (segment != nullptr) {
             segments->append(segment);
            // qInfo("Segment %d added successfully", j);

            // DEBUG: Track successful segment addition for problematic voltages
            if (qAbs(vg - (-2.5)) < 0.1 || qAbs(vg - (-3.0)) < 0.1 || qAbs(vg - (-3.5)) < 0.1 || qAbs(vg - (-4.0)) < 0.1) {
                // qInfo("*** DEBUG: Successfully added segment %d for %.6fV - list size now: %d ***", j, vg, segments->size());
            }
         } else {
             // qWarning("Failed to create segment %d", j);
             // Handle nullptr return
             continue;
         }

         va = vaNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(va, ia, vg1Nominal, samplePen->color()));
    // qInfo("Finished plotting triode anode sweep - %d segments created", nSamples - 1);
    // qInfo("*** DEBUG: Completed plotting sweep with grid voltage: %.6fV ***", vg);

    // DEBUG: Show final segments list size for problematic voltages
    if (qAbs(vg - (-2.5)) < 0.1 || qAbs(vg - (-3.0)) < 0.1 || qAbs(vg - (-3.5)) < 0.1 || qAbs(vg - (-4.0)) < 0.1) {
        // qInfo("*** DEBUG: Final segments list size for %.6fV: %d ***", vg, segments->size());
    }
}

void Sweep::plotTriodeTransfer(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    // qInfo("=== SWEEP::PLOTTRIODETRANSFER - Sample count: %d ===", samples.count());

    // ADD VALIDATION FOR EMPTY SWEEPS
    if (samples.count() < 2) {
        // qWarning("Sweep has insufficient samples (%d) for plotting - skipping", samples.count());
        return;
    }

    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double va = firstSample->getVa();
    double ia = firstSample->getIa();

    // qInfo("First sample: vg1=%f, va=%f, ia=%f", vg, va, ia);

    int nSamples = samples.count();
    if (nSamples == 1) {
        segments->append(plot->createLabel(va, ia, vg1Nominal, samplePen->color()));
        return;
    }

    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vgNext = sample->getVg1();
         double iaNext = sample->getIa();

         // qInfo("Transfer segment %d: vg=%f->%f, ia=%f->%f", j, vg, vgNext, ia, iaNext);

         QGraphicsLineItem *segment = plot->createSegment(vg, ia, vgNext, iaNext, *samplePen);
         if (segment != nullptr) {
             segments->append(segment);
             // qInfo("Transfer segment %d added successfully", j);
         } else {
             // qWarning("Failed to create transfer segment %d", j);
             continue;
         }

         vg = vgNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(vg, ia, vaNominal, samplePen->color()));
    // qInfo("Finished plotting triode transfer sweep - %d segments created", nSamples - 1);
}

void Sweep::plotPentodeScreen(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    // qInfo("=== SWEEP::PLOTPENTODESCREEN - Sample count: %d ===", samples.count());

    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double va = firstSample->getVa();
    double ig2 = firstSample->getIg2();

    // qInfo("First sample: vg1=%f, va=%f, ig2=%f", vg, va, ig2);

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vaNext = sample->getVa();
         double ig2Next = sample->getIg2();

         // qInfo("Screen segment %d: va=%f->%f, ig2=%f->%f", j, va, vaNext, ig2, ig2Next);

         QGraphicsLineItem *segment = plot->createSegment(va, ig2, vaNext, ig2Next, *samplePen);
         if (segment != nullptr) {
             segments->append(segment);
             // qInfo("Screen segment %d added successfully", j);
         } else {
             // qWarning("Failed to create screen segment %d", j);
             continue;
         }

         va = vaNext;
         ig2 = ig2Next;
     }

    segments->append(plot->createLabel(va, ig2, vg1Nominal, samplePen->color()));
    // qInfo("Finished plotting pentode screen sweep - %d segments created", nSamples - 1);
}

void Sweep::plotPentodeAnode(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    // qInfo("=== SWEEP::PLOTPENTODEANODE - Sample count: %d ===", samples.count());

    // ADD VALIDATION FOR EMPTY SWEEPS
    if (samples.count() < 2) {
        // qWarning("Sweep has insufficient samples (%d) for plotting - skipping", samples.count());
        return;
    }

    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double va = firstSample->getVa();
    double ia = firstSample->getIa();

    // qInfo("First sample: vg1=%f, va=%f, ia=%f", vg, va, ia);

    int nSamples = samples.count();
    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vaNext = sample->getVa();
         double iaNext = sample->getIa();

         // qInfo("Pentode anode segment %d: va=%f->%f, ia=%f->%f", j, va, vaNext, ia, iaNext);

         QGraphicsLineItem *segment = plot->createSegment(va, ia, vaNext, iaNext, *samplePen);
         if (segment != nullptr) {
             segments->append(segment);
             // qInfo("Pentode anode segment %d added successfully", j);
         } else {
             // qWarning("Failed to create pentode anode segment %d", j);
             continue;
         }

         va = vaNext;
         ia = iaNext;
     }

    segments->append(plot->createLabel(va, ia, vg1Nominal, samplePen->color()));
    // qInfo("Finished plotting pentode anode sweep - %d segments created", nSamples - 1);
}

void Sweep::plotPentodeTransfer(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    // qInfo("=== SWEEP::PLOTPENTODETRANSFER - Sample count: %d ===", samples.count());

    // ADD VALIDATION FOR EMPTY SWEEPS
    if (samples.count() == 0) {
        // qWarning("Sweep has insufficient samples (%d) for plotting - skipping", samples.count());
        return;
    }

    Sample *firstSample = samples.at(0);

    double vg = firstSample->getVg1();
    double vg2 = firstSample->getVg2();
    double ia = firstSample->getIa();

    // qInfo("First sample: vg1=%f, vg2=%f, ia=%f", vg, vg2, ia);

    int nSamples = samples.count();
    if (nSamples == 1) {
        segments->append(plot->createLabel(vg, ia, vg2, samplePen->color()));
        return;
    }

    for (int j = 1; j < nSamples; j++) {
         Sample *sample = samples.at(j);

         double vgNext = sample->getVg1();
         double iaNext = sample->getIa();

         // qInfo("Pentode transfer segment %d: vg=%f->%f, ia=%f->%f", j, vg, vgNext, ia, iaNext);

         // Detect restart of the grid sweep (e.g. when moving from the end of one
         // family back to the start of the next). In that case, do not draw a
         // bridge segment; just start a new polyline at the new point.
         const double restartThreshold = 0.5; // volts
         if (vg - vgNext > restartThreshold) {
             vg = vgNext;
             ia = iaNext;
             continue;
         }

         QGraphicsLineItem *segment = plot->createSegment(vg, ia, vgNext, iaNext, *samplePen);
         if (segment != nullptr) {
             segments->append(segment);
             // qInfo("Pentode transfer segment %d added successfully", j);
         } else {
             // qWarning("Failed to create pentode transfer segment %d", j);
             continue;
         }

         vg = vgNext;
         ia = iaNext;
     }

    // Label this transfer family using the measured screen voltage from the
    // first sample so that the annotation reflects the actual Vg2 seen in
    // the data, even if metadata and configuration get out of sync.
    segments->append(plot->createLabel(vg, ia, vg2, samplePen->color()));
    // qInfo("Finished plotting pentode transfer sweep - %d segments created", nSamples - 1);
}

void Sweep::propertyEdited(QTableWidgetItem *item)
{

}

