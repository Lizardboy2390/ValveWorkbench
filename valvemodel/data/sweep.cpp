#include "sweep.h"
#include "measurement.h"
#include <QDebug>
#include <cmath>

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
    if (samples.count() < 2) {
        return;
    }

    bool smooth = (measurement && measurement->isSmoothPlotting());

    qInfo("Sweep::plotTriodeAnode: smooth=%d samples=%d", smooth ? 1 : 0, samples.count());

    if (!smooth) {
        qInfo("Sweep::plotTriodeAnode: using raw segments (no smoothing)");
        Sample *firstSample = samples.at(0);
        double vg = firstSample->getVg1();
        double va = firstSample->getVa();
        double ia = firstSample->getIa();

        int nSamples = samples.count();
        for (int j = 1; j < nSamples; j++) {
            Sample *sample = samples.at(j);

            double vaNext = sample->getVa();
            double iaNext = sample->getIa();

            QGraphicsLineItem *segment = plot->createSegment(va, ia, vaNext, iaNext, *samplePen);
            if (segment != nullptr) {
                segments->append(segment);
            } else {
                continue;
            }

            va = vaNext;
            ia = iaNext;
        }

        segments->append(plot->createLabel(va, ia, vg1Nominal, samplePen->color()));
        return;
    }
    const int nSamples = samples.count();

    QVector<double> x(nSamples), y(nSamples);
    for (int i = 0; i < nSamples; ++i) {
        Sample *s = samples.at(i);
        x[i] = s->getVa();
        y[i] = s->getIa();
    }

    QVector<double> h(nSamples - 1), s(nSamples - 1);
    for (int i = 0; i < nSamples - 1; ++i) {
        h[i] = x[i + 1] - x[i];
        if (h[i] == 0.0) {
            s[i] = 0.0;
        } else {
            s[i] = (y[i + 1] - y[i]) / h[i];
        }
    }

    QVector<double> m(nSamples);
    if (nSamples == 2) {
        m[0] = m[1] = s[0];
    } else {
        m[0] = ((2 * h[0] + h[1]) * s[0] - h[0] * s[1]) / (h[0] + h[1]);
        if (m[0] * s[0] <= 0.0) m[0] = 0.0;
        else if (std::fabs(m[0] / s[0]) > 3.0) m[0] = 3.0 * s[0];

        for (int i = 1; i < nSamples - 1; ++i) {
            if (s[i - 1] * s[i] <= 0.0) {
                m[i] = 0.0;
            } else {
                double w1 = 2.0 * h[i] + h[i - 1];
                double w2 = h[i] + 2.0 * h[i - 1];
                m[i] = (w1 + w2) / (w1 / s[i - 1] + w2 / s[i]);
            }
        }

        m[nSamples - 1] = ((2 * h[nSamples - 2] + h[nSamples - 3]) * s[nSamples - 2] - h[nSamples - 2] * s[nSamples - 3]) / (h[nSamples - 2] + h[nSamples - 3]);
        if (m[nSamples - 1] * s[nSamples - 2] <= 0.0) m[nSamples - 1] = 0.0;
        else if (std::fabs(m[nSamples - 1] / s[nSamples - 2]) > 3.0) m[nSamples - 1] = 3.0 * s[nSamples - 2];

        for (int i = 0; i < nSamples - 1; ++i) {
            if (s[i] == 0.0) {
                m[i] = 0.0;
                m[i + 1] = 0.0;
            } else {
                double a = m[i] / s[i];
                double b = m[i + 1] / s[i];
                double sq = a * a + b * b;
                if (sq > 9.0) {
                    double t = 3.0 / std::sqrt(sq);
                    m[i] = t * a * s[i];
                    m[i + 1] = t * b * s[i];
                }
            }
        }
    }

    const int subdivisions = 4; // 4x plotting density when smoothing is enabled

    qInfo("Sweep::plotTriodeAnode: using spline smoothing, samples=%d, subdivisions=%d, expectedSegments=%d",
          nSamples, subdivisions, (nSamples - 1) * subdivisions);

    for (int i = 0; i < nSamples - 1; ++i) {
        double xi = x[i];
        double xi1 = x[i + 1];
        double hi = xi1 - xi;
        double yi = y[i];
        double yi1 = y[i + 1];
        double mi = m[i];
        double mi1 = m[i + 1];

        double xPrev = xi;
        double yPrev = yi;
        for (int k = 1; k <= subdivisions; ++k) {
            double t = static_cast<double>(k) / static_cast<double>(subdivisions);
            double t2 = t * t;
            double t3 = t2 * t;
            double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
            double h10 = t3 - 2.0 * t2 + t;
            double h01 = -2.0 * t3 + 3.0 * t2;
            double h11 = t3 - t2;

            double xT = xi + t * hi;
            double yT = h00 * yi + h10 * hi * mi + h01 * yi1 + h11 * hi * mi1;

            QGraphicsLineItem *seg = plot->createSegment(xPrev, yPrev, xT, yT, *samplePen);
            if (seg) {
                segments->append(seg);
            }
            xPrev = xT;
            yPrev = yT;
        }
    }

    segments->append(plot->createLabel(x[nSamples - 1], y[nSamples - 1], vg1Nominal, samplePen->color()));
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
    if (samples.count() < 2) {
        return;
    }

    bool smooth = (measurement && measurement->isSmoothPlotting());

    if (!smooth) {
        Sample *firstSample = samples.at(0);

        double vg = firstSample->getVg1();
        double va = firstSample->getVa();
        double ig2 = firstSample->getIg2();

        int nSamples = samples.count();
        for (int j = 1; j < nSamples; j++) {
            Sample *sample = samples.at(j);

            double vaNext = sample->getVa();
            double ig2Next = sample->getIg2();

            QGraphicsLineItem *segment = plot->createSegment(va, ig2, vaNext, ig2Next, *samplePen);
            if (segment != nullptr) {
                segments->append(segment);
            } else {
                continue;
            }

            va = vaNext;
            ig2 = ig2Next;
        }

        segments->append(plot->createLabel(va, ig2, vg1Nominal, samplePen->color()));
        return;
    }
    const int nSamples = samples.count();

    QVector<double> x(nSamples), y(nSamples);
    for (int i = 0; i < nSamples; ++i) {
        Sample *s = samples.at(i);
        x[i] = s->getVa();
        y[i] = s->getIg2();
    }

    QVector<double> h(nSamples - 1), s(nSamples - 1);
    for (int i = 0; i < nSamples - 1; ++i) {
        h[i] = x[i + 1] - x[i];
        if (h[i] == 0.0) {
            s[i] = 0.0;
        } else {
            s[i] = (y[i + 1] - y[i]) / h[i];
        }
    }

    QVector<double> m(nSamples);
    if (nSamples == 2) {
        m[0] = m[1] = s[0];
    } else {
        m[0] = ((2 * h[0] + h[1]) * s[0] - h[0] * s[1]) / (h[0] + h[1]);
        if (m[0] * s[0] <= 0.0) m[0] = 0.0;
        else if (std::fabs(m[0] / s[0]) > 3.0) m[0] = 3.0 * s[0];

        for (int i = 1; i < nSamples - 1; ++i) {
            if (s[i - 1] * s[i] <= 0.0) {
                m[i] = 0.0;
            } else {
                double w1 = 2.0 * h[i] + h[i - 1];
                double w2 = h[i] + 2.0 * h[i - 1];
                m[i] = (w1 + w2) / (w1 / s[i - 1] + w2 / s[i]);
            }
        }

        m[nSamples - 1] = ((2 * h[nSamples - 2] + h[nSamples - 3]) * s[nSamples - 2] - h[nSamples - 2] * s[nSamples - 3]) / (h[nSamples - 2] + h[nSamples - 3]);
        if (m[nSamples - 1] * s[nSamples - 2] <= 0.0) m[nSamples - 1] = 0.0;
        else if (std::fabs(m[nSamples - 1] / s[nSamples - 2]) > 3.0) m[nSamples - 1] = 3.0 * s[nSamples - 2];

        for (int i = 0; i < nSamples - 1; ++i) {
            if (s[i] == 0.0) {
                m[i] = 0.0;
                m[i + 1] = 0.0;
            } else {
                double a = m[i] / s[i];
                double b = m[i + 1] / s[i];
                double sq = a * a + b * b;
                if (sq > 9.0) {
                    double t = 3.0 / std::sqrt(sq);
                    m[i] = t * a * s[i];
                    m[i + 1] = t * b * s[i];
                }
            }
        }
    }

    const int subdivisions = 4; // 4x plotting density when smoothing is enabled

    for (int i = 0; i < nSamples - 1; ++i) {
        double xi = x[i];
        double xi1 = x[i + 1];
        double hi = xi1 - xi;
        double yi = y[i];
        double yi1 = y[i + 1];
        double mi = m[i];
        double mi1 = m[i + 1];

        double xPrev = xi;
        double yPrev = yi;
        for (int k = 1; k <= subdivisions; ++k) {
            double t = static_cast<double>(k) / static_cast<double>(subdivisions);
            double t2 = t * t;
            double t3 = t2 * t;
            double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
            double h10 = t3 - 2.0 * t2 + t;
            double h01 = -2.0 * t3 + 3.0 * t2;
            double h11 = t3 - t2;

            double xT = xi + t * hi;
            double yT = h00 * yi + h10 * hi * mi + h01 * yi1 + h11 * hi * mi1;

            QGraphicsLineItem *seg = plot->createSegment(xPrev, yPrev, xT, yT, *samplePen);
            if (seg) {
                segments->append(seg);
            }
            xPrev = xT;
            yPrev = yT;
        }
    }

    segments->append(plot->createLabel(x[nSamples - 1], y[nSamples - 1], vg1Nominal, samplePen->color()));
}

void Sweep::plotPentodeAnode(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments)
{
    if (samples.count() < 2) {
        return;
    }

    bool smooth = (measurement && measurement->isSmoothPlotting());

    if (!smooth) {
        Sample *firstSample = samples.at(0);

        double vg = firstSample->getVg1();
        double va = firstSample->getVa();
        double ia = firstSample->getIa();

        int nSamples = samples.count();
        for (int j = 1; j < nSamples; j++) {
            Sample *sample = samples.at(j);

            double vaNext = sample->getVa();
            double iaNext = sample->getIa();

            QGraphicsLineItem *segment = plot->createSegment(va, ia, vaNext, iaNext, *samplePen);
            if (segment != nullptr) {
                segments->append(segment);
            } else {
                continue;
            }

            va = vaNext;
            ia = iaNext;
        }

        segments->append(plot->createLabel(va, ia, vg1Nominal, samplePen->color()));
        return;
    }
    const int nSamples = samples.count();

    QVector<double> x(nSamples), y(nSamples);
    for (int i = 0; i < nSamples; ++i) {
        Sample *s = samples.at(i);
        x[i] = s->getVa();
        y[i] = s->getIa();
    }

    QVector<double> h(nSamples - 1), s(nSamples - 1);
    for (int i = 0; i < nSamples - 1; ++i) {
        h[i] = x[i + 1] - x[i];
        if (h[i] == 0.0) {
            s[i] = 0.0;
        } else {
            s[i] = (y[i + 1] - y[i]) / h[i];
        }
    }

    QVector<double> m(nSamples);
    if (nSamples == 2) {
        m[0] = m[1] = s[0];
    } else {
        m[0] = ((2 * h[0] + h[1]) * s[0] - h[0] * s[1]) / (h[0] + h[1]);
        if (m[0] * s[0] <= 0.0) m[0] = 0.0;
        else if (std::fabs(m[0] / s[0]) > 3.0) m[0] = 3.0 * s[0];

        for (int i = 1; i < nSamples - 1; ++i) {
            if (s[i - 1] * s[i] <= 0.0) {
                m[i] = 0.0;
            } else {
                double w1 = 2.0 * h[i] + h[i - 1];
                double w2 = h[i] + 2.0 * h[i - 1];
                m[i] = (w1 + w2) / (w1 / s[i - 1] + w2 / s[i]);
            }
        }

        m[nSamples - 1] = ((2 * h[nSamples - 2] + h[nSamples - 3]) * s[nSamples - 2] - h[nSamples - 2] * s[nSamples - 3]) / (h[nSamples - 2] + h[nSamples - 3]);
        if (m[nSamples - 1] * s[nSamples - 2] <= 0.0) m[nSamples - 1] = 0.0;
        else if (std::fabs(m[nSamples - 1] / s[nSamples - 2]) > 3.0) m[nSamples - 1] = 3.0 * s[nSamples - 2];

        for (int i = 0; i < nSamples - 1; ++i) {
            if (s[i] == 0.0) {
                m[i] = 0.0;
                m[i + 1] = 0.0;
            } else {
                double a = m[i] / s[i];
                double b = m[i + 1] / s[i];
                double sq = a * a + b * b;
                if (sq > 9.0) {
                    double t = 3.0 / std::sqrt(sq);
                    m[i] = t * a * s[i];
                    m[i + 1] = t * b * s[i];
                }
            }
        }
    }

    const int subdivisions = 4; // 4x plotting density when smoothing is enabled

    for (int i = 0; i < nSamples - 1; ++i) {
        double xi = x[i];
        double xi1 = x[i + 1];
        double hi = xi1 - xi;
        double yi = y[i];
        double yi1 = y[i + 1];
        double mi = m[i];
        double mi1 = m[i + 1];

        double xPrev = xi;
        double yPrev = yi;
        for (int k = 1; k <= subdivisions; ++k) {
            double t = static_cast<double>(k) / static_cast<double>(subdivisions);
            double t2 = t * t;
            double t3 = t2 * t;
            double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
            double h10 = t3 - 2.0 * t2 + t;
            double h01 = -2.0 * t3 + 3.0 * t2;
            double h11 = t3 - t2;

            double xT = xi + t * hi;
            double yT = h00 * yi + h10 * hi * mi + h01 * yi1 + h11 * hi * mi1;

            QGraphicsLineItem *seg = plot->createSegment(xPrev, yPrev, xT, yT, *samplePen);
            if (seg) {
                segments->append(seg);
            }
            xPrev = xT;
            yPrev = yT;
        }
    }

    segments->append(plot->createLabel(x[nSamples - 1], y[nSamples - 1], vg1Nominal, samplePen->color()));
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

