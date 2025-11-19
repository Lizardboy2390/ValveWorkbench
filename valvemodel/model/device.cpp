#include "device.h"
#include <QGraphicsTextItem>

// For findBiasFromMeasurement we need access to Sweep and Sample getters,
// so include their headers here (measurement.h only forward-declares them).
#include "../data/sweep.h"
#include "../data/sample.h"

Device::Device(int _modelDeviceType) : deviceType(_modelDeviceType)
{
    if (deviceType == TRIODE) {
        modelType = COHEN_HELIE_TRIODE;
    }
}

Device::Device(QJsonDocument modelDocument)
{
    vaMax =400.0;
    iaMax = 6.0;
    vg1Max = 4.0;
    vg2Max = vaMax;
    paMax = 1.25;

    qInfo("=== DEVICE CONSTRUCTOR ===");
    qInfo("JSON is object: %s", modelDocument.isObject() ? "true" : "false");

    if (modelDocument.isObject()) {
        QJsonObject deviceObject = modelDocument.object();

        if (deviceObject.contains("name") && deviceObject["name"].isString()) {
            name = deviceObject["name"].toString();
            qInfo("Device name: %s", name.toStdString().c_str());
        }

        if (deviceObject.contains("vaMax") && deviceObject["vaMax"].isDouble()) {
            vaMax = deviceObject["vaMax"].toDouble();
        }

        if (deviceObject.contains("vg1Max") && deviceObject["vg1Max"].isDouble()) {
            vg1Max = deviceObject["vg1Max"].toDouble();
        }

        if (deviceObject.contains("vg2Max") && deviceObject["vg2Max"].isDouble()) {
            vg2Max = deviceObject["vg2Max"].toDouble();
        }

        if (deviceObject.contains("iaMax") && deviceObject["iaMax"].isDouble()) {
            iaMax = deviceObject["iaMax"].toDouble();
        }

        if (deviceObject.contains("paMax") && deviceObject["paMax"].isDouble()) {
            paMax = deviceObject["paMax"].toDouble();
        }

        if (deviceObject.contains("model") && deviceObject["model"].isObject()) {
            QJsonObject modelObject = deviceObject["model"].toObject();
            qInfo("Found model object");

            deviceType = TRIODE;  // Use main app constants

            if (modelObject.contains("device") && modelObject["device"].isString()) {
                QString deviceStr = modelObject["device"].toString();
                qInfo("Device string: %s", deviceStr.toStdString().c_str());
                if (deviceStr == "pentode") {
                    deviceType = PENTODE;  // Use main app constants
                }
            }

            QString modelTypeStr = modelObject.value("type").toString();
            if (!modelTypeStr.isEmpty()) {
                qInfo("Model type: %s", modelTypeStr.toStdString().c_str());
            }

            // Choose model class: prefer explicit type, otherwise infer from keys
            if (modelTypeStr == "simple") {
                model = new SimpleTriode();
            } else if (modelTypeStr == "koren") {
                model = new KorenTriode();
            } else if (modelTypeStr == "cohenHelie") {
                model = new CohenHelieTriode();
            } else if (modelTypeStr == "reefman") {
                model = new ReefmanPentode();
            } else if (modelTypeStr == "gardiner") {
                model = new GardinerPentode();
            } else {
                // Infer triode model from present keys
                if (modelObject.contains("kvb1")) {
                    model = new CohenHelieTriode();
                    qInfo("Inferred model: Cohen-Helie (kvb1 present)");
                } else if (modelObject.contains("kp") || modelObject.contains("kvb")) {
                    model = new KorenTriode();
                    qInfo("Inferred model: Koren (kp/kvb present)");
                } else {
                    model = new SimpleTriode();
                    qInfo("Inferred model: Simple (fallback)");
                }
            }

            if (model != nullptr) {
                model->fromJson(modelObject);
                qInfo("Model created and initialized");
                if (modelTypeStr == "gardiner") {
                    qInfo("Device '%s' Gardiner parameters after fromJson: mu=%.12f kg1=%.12f x=%.12f kp=%.12f kvb=%.12f kvb1=%.12f vct=%.12f",
                          name.toStdString().c_str(),
                          model->getParameter(PAR_MU),
                          model->getParameter(PAR_KG1),
                          model->getParameter(PAR_X),
                          model->getParameter(PAR_KP),
                          model->getParameter(PAR_KVB),
                          model->getParameter(PAR_KVB1),
                          model->getParameter(PAR_VCT));
                    qInfo("  kg2=%.12f kg2a=%.12f a=%.12f alpha=%.12f beta=%.12f gamma=%.12f os=%.12f",
                          model->getParameter(PAR_KG2),
                          model->getParameter(PAR_KG2A),
                          model->getParameter(PAR_A),
                          model->getParameter(PAR_ALPHA),
                          model->getParameter(PAR_BETA),
                          model->getParameter(PAR_GAMMA),
                          model->getParameter(PAR_OS));
                    qInfo("  tau=%.12f rho=%.12f theta=%.12f psi=%.12f omega=%.12f lambda=%.12f nu=%.12f s=%.12f ap=%.12f",
                          model->getParameter(PAR_TAU),
                          model->getParameter(PAR_RHO),
                          model->getParameter(PAR_THETA),
                          model->getParameter(PAR_PSI),
                          model->getParameter(PAR_OMEGA),
                          model->getParameter(PAR_LAMBDA),
                          model->getParameter(PAR_NU),
                          model->getParameter(PAR_S),
                          model->getParameter(PAR_AP));
                }
            }
        } else {
            qInfo("No model object found in JSON");
        }

        // Optional embedded triode seed model used when the preset was
        // exported from a combined triode+pentode fit. When present, this
        // allows Modeller to reuse the original triode seed for later pentode
        // fits without re-running the triode modelling step.
        if (deviceObject.contains("triodeModel") && deviceObject["triodeModel"].isObject()) {
            QJsonObject triodeObj = deviceObject["triodeModel"].toObject();
            triodeSeed = new CohenHelieTriode();
            triodeSeed->fromJson(triodeObj);
            qInfo("Device '%s' has embedded triode seed model", name.toStdString().c_str());
        }

        // Optional embedded measurement data exported from the analyser. When
        // present, this allows Designer helpers (e.g. SE bias calculations) to
        // reconstruct operating points directly from the original sweeps
        // instead of relying solely on the fitted model surface.
        if (deviceObject.contains("measurement") && deviceObject["measurement"].isObject()) {
            QJsonObject measObj = deviceObject["measurement"].toObject();
            measurement = new Measurement();
            measurement->fromJson(measObj);
            qInfo("Device '%s' has embedded measurement with %d sweeps",
                  name.toStdString().c_str(), measurement->count());
        }
    } else {
        qInfo("JSON document is not an object");
    }

    // Log final axis limits and device type for Designer diagnostics so we can
    // verify that JSON-specified ranges are honoured at runtime.
    qInfo("DEVICE AXES: name='%s' type=%d vaMax=%.3f iaMax=%.3f vg1Max=%.3f vg2Max=%.3f paMax=%.3f",
          name.toStdString().c_str(), deviceType, vaMax, iaMax, vg1Max, vg2Max, paMax);

    qInfo("Final device type: %d", deviceType);
}

bool Device::findBiasFromMeasurement(double vb,
                                     double vs,
                                     double targetIa_mA,
                                     double &vk_out,
                                     double &ig2_mA_out) const
{
    vk_out = 0.0;
    ig2_mA_out = 0.0;

    if (!measurement || measurement->count() <= 0) {
        return false;
    }

    struct BiasSample {
        double vg1;
        double ia_mA;
        double ig2_mA;
    };

    std::vector<BiasSample> samples;
    samples.reserve(128);

    const double vaTol = std::max(5.0, std::fabs(vb) * 0.05);  // 5 V or 5%
    const double vsTol = std::max(5.0, std::fabs(vs) * 0.05);

    for (int i = 0; i < measurement->count(); ++i) {
        Sweep *sw = measurement->at(i);
        if (!sw) continue;
        for (int j = 0; j < sw->count(); ++j) {
            Sample *s = sw->at(j);
            if (!s) continue;

            const double va    = s->getVa();
            const double vg1   = s->getVg1();
            const double vg2   = s->getVg2();
            const double ia_mA = s->getIa();
            const double ig2_mA = s->getIg2();

            if (!std::isfinite(va) || !std::isfinite(vg1) ||
                !std::isfinite(vg2) || !std::isfinite(ia_mA)) {
                continue;
            }

            if (std::fabs(va - vb) <= vaTol && std::fabs(vg2 - vs) <= vsTol) {
                BiasSample bs{vg1, ia_mA, std::isfinite(ig2_mA) ? ig2_mA : 0.0};
                samples.push_back(bs);
            }
        }
    }

    if (samples.size() < 1) {
        return false;
    }

    // Prefer interpolation when we have at least two points spanning target Ia.
    // Sort by ia so we can bracket the target current.
    std::sort(samples.begin(), samples.end(),
              [](const BiasSample &a, const BiasSample &b) {
                  return a.ia_mA < b.ia_mA;
              });

    const double target = targetIa_mA;
    int bracket = -1;
    for (int i = 0; i + 1 < static_cast<int>(samples.size()); ++i) {
        const double ia1 = samples[i].ia_mA;
        const double ia2 = samples[i + 1].ia_mA;
        if ((ia1 <= target && ia2 >= target) || (ia1 >= target && ia2 <= target)) {
            bracket = i;
            break;
        }
    }

    double vg1_sel = 0.0;
    double ig2_sel = 0.0;

    if (bracket >= 0) {
        const BiasSample &s1 = samples[bracket];
        const BiasSample &s2 = samples[bracket + 1];
        const double denom = (s2.ia_mA - s1.ia_mA);
        double t = 0.0;
        if (std::fabs(denom) > 1e-9) {
            t = (target - s1.ia_mA) / denom;
            t = std::clamp(t, 0.0, 1.0);
        }
        vg1_sel = s1.vg1 + t * (s2.vg1 - s1.vg1);
        ig2_sel = s1.ig2_mA + t * (s2.ig2_mA - s1.ig2_mA);
    } else {
        // No bracketing pair: fall back to nearest Ia sample.
        double bestErr = std::numeric_limits<double>::infinity();
        for (const auto &bs : samples) {
            const double err = std::fabs(bs.ia_mA - target);
            if (err < bestErr) {
                bestErr = err;
                vg1_sel = bs.vg1;
                ig2_sel = bs.ig2_mA;
            }
        }
    }

    if (!std::isfinite(vg1_sel)) {
        return false;
    }

    // Measurement Vg1 is negative for normal bias; Vk is its magnitude.
    vk_out = -vg1_sel;
    ig2_mA_out = ig2_sel;
    return true;
}

double Device::anodeCurrent(double va, double vg1, double vg2)
{
    if (model != nullptr) {
        return model->anodeCurrent(va, vg1, vg2);
    }

    return 0.0;
}

double Device::anodeVoltage(double ia, double vg1, double vg2)
{
    if (model != nullptr) {
        return model->anodeVoltage(ia, vg1, vg2);
    }

    return 0.0;
}

double Device::screenCurrent(double va, double vg1, double vg2)
{
    if (model != nullptr) {
        return model->screenCurrent(va, vg1, vg2);
    }

    return 0.0;
}

void Device::updateUI(QLabel *labels[], QLineEdit *values[])
{
    for (int i=0; i < 16; i++) { // Parameters all initially hidden
        values[i]->setVisible(false);
        labels[i]->setVisible(false);
    }
    if (model != nullptr) {
        model->updateUI(labels, values);
    }
}

void Device::anodeAxes(Plot *plot)
{
    plot->clear();

    double vaInterval = interval(vaMax);
    double iaInterval = interval(iaMax);

    plot->setAxes(0.0, vaMax, vaInterval, 0.0, iaMax, iaInterval, 2, 1);
}

void Device::transferAxes(Plot *plot)
{

}

QGraphicsItemGroup *Device::anodePlot(Plot *plot)
{
    // Some device presets (e.g., analyser-only exports) may not have an attached
    // fitted model. In that case, skip plotting model curves instead of
    // dereferencing a null model pointer.
    if (model == nullptr) {
        qWarning("Device::anodePlot: device '%s' has no model; skipping model plot",
                 name.toStdString().c_str());
        return nullptr;
    }

    qInfo("Device::anodePlot: name='%s' type=%d vaMax=%.3f iaMax=%.3f vg1Max=%.3f vg2Max=%.3f",
          name.toStdString().c_str(), deviceType, vaMax, iaMax, vg1Max, vg2Max);

    QList<QGraphicsItem *> items;

    QPen modelPen;
    modelPen.setColor(QColor::fromRgb(255, 0, 0));

    double vgInterval = interval(vg1Max);

    double vg1 = -vg1Max;
    int vgIndex = 0; // used to stagger labels to avoid overlap

    while (vg1 <= 0.0) {
        const double vgLabel = vg1; // preserve for label text
        double va = 0.0;
        double ia = 0.0;
        if (deviceType == PENTODE) {
            ia = model->anodeCurrent(va, vg1, vg2Max);
        } else {
            ia = model->anodeCurrent(va, vg1);
        }

        for (int j = 1; j < 61; j++) {
            double vaNext = (vaMax * j) / 60.0;
            double iaNext = 0.0;
            if (deviceType == PENTODE) {
                iaNext = model->anodeCurrent(vaNext, vg1, vg2Max);
            } else {
                iaNext = model->anodeCurrent(vaNext, vg1);
            }
            items.append(plot->createSegment(va, ia, vaNext, iaNext, modelPen));

            va = vaNext;
            ia = iaNext;
        }

        // Add a label near the right edge of the plot for this grid line.
        // Avoid calling Model::anodeVoltage here, since some steep Gardiner
        // parameter sets require many iterations to invert Ia(Va) and can
        // stall the UI. Instead, clamp the right-edge current into range and
        // place the label there.
        const double epsX = std::max(0.5, vaMax * 0.01);   // 1% inset or 0.5V minimum
        const double epsY = std::max(0.05, iaMax * 0.01);  // 1% inset or 0.05mA minimum

        double x = vaMax - epsX;
        double yAtRight = 0.0;
        if (deviceType == PENTODE) {
            yAtRight = model->anodeCurrent(vaMax, vg1, vg2Max);
        } else {
            yAtRight = model->anodeCurrent(vaMax, vg1);
        }
        double y = iaMax - epsY;
        if (std::isfinite(yAtRight)) {
            y = std::min(iaMax - epsY, std::max(0.0, yAtRight));
        }
        QGraphicsTextItem *label = plot->createLabel(x, y, vgLabel, QColor::fromRgb(255, 0, 0));
        if (label) {
            label->setPlainText(QString("%1V").arg(vgLabel, 0, 'f', 1));
            QFont f = label->font();
            f.setPointSizeF(std::max(7.0, f.pointSizeF()));
            label->setFont(f);
            items.append(label);
        }

        vg1 += vgInterval;
        ++vgIndex;
    }

    return plot->getScene()->createItemGroup(items);
}

QGraphicsItemGroup *Device::transferPlot(Plot *plot)
{
    QList<QGraphicsItem *> segments;

    return plot->getScene()->createItemGroup(segments);
}

double Device::interval(double maxValue)
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

int Device::getDeviceType() const
{
    return deviceType;
}

void Device::setDeviceType(int newDeviceType)
{
    deviceType = newDeviceType;
}

QString Device::getName()
{
    return name;
}

QTreeWidgetItem *Device::buildTree(QTreeWidgetItem *parent)
{
    return nullptr;
}

int Device::getModelType() const
{
    return modelType;
}

void Device::setModelType(int newModelType)
{
    modelType = newModelType;
}

double Device::getParameter(int index) const
{
    if (!model) {
        qWarning("Device::getParameter: device '%s' has no model (index=%d)",
                 name.toStdString().c_str(), index);
        return 0.0;
    }

    return model->getParameter(index);
}

double Device::getVaMax() const
{
    return vaMax;
}

double Device::getIaMax() const
{
    return iaMax;
}

double Device::getVg1Max() const
{
    return vg1Max;
}

double Device::getVg2Max() const
{
    return vg2Max;
}

double Device::getPaMax() const
{
    return paMax;
}

void Device::setName(const QString &newName)
{
    name = newName;
}
