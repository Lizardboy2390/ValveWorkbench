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

            // Choose model class: prefer explicit type, otherwise infer from keys.
            // This now includes explicit support for ExtractModel pentode exports
            // which use type "extractDerkE" in their JSON.
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
            } else if (modelTypeStr == "extractDerkE") {
                model = new ExtractModelPentode();
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

    // Choose grid voltage step for model sweeps. For pentodes with typical
    // power-tube ranges (e.g. 0 .. -60 V), use a finer fixed step so the
    // number of red model curves is closer to the number of analyser
    // measurement sweeps and grid families line up with the projected
    // Class B line. For other cases, fall back to the generic interval
    // heuristic.
    double vgInterval = 0.0;
    if (deviceType == PENTODE && vg1Max > 0.0 && vg1Max <= 80.0) {
        vgInterval = 2.0;
    } else {
        vgInterval = interval(vg1Max);
    }

    double vg1 = -vg1Max;
    int vgIndex = 0; // used to stagger labels to avoid overlap

    while (vg1 <= 0.0) {
        if (deviceType == TRIODE) {
            const double vaStart = 0.0;
            const double vaStop = vaMax;
            const double vaInc = (vaStop - vaStart) / 60.0;
            const double yMaxAxis = iaMax;

            double vaPrev = vaStart;
            double iaPrev = model->anodeCurrent(vaPrev, vg1);

            double va = vaStart + vaInc;
            QList<QGraphicsItem*> triodeSegments;
            while (va <= vaStop) {
                double ia = model->anodeCurrent(va, vg1);
                QGraphicsLineItem *seg = plot->createSegment(vaPrev, iaPrev, va, ia, modelPen);
                if (seg) {
                    triodeSegments.append(seg);
                }
                vaPrev = va;
                iaPrev = ia;
                va += vaInc;
            }

            const double vaLabel = vaStart + 0.7 * (vaStop - vaStart);
            const double iaLabel = model->anodeCurrent(vaLabel, vg1);
            if (std::isfinite(vaLabel) && std::isfinite(iaLabel)) {
                double iaClamped = std::min(yMaxAxis, std::max(0.0, iaLabel));
                QGraphicsItem *label = plot->createLabel(vaLabel, iaClamped, vg1, modelPen.color());
                if (label) {
                    QRectF r = label->boundingRect();
                    QPointF p = label->pos();
                    const double halfW = r.width() * 0.5;
                    const double halfH = r.height() * 0.5;
                    label->setPos(p.x() - 5.0 - halfW,
                                  p.y() + 10.0 - halfH);

                    QRectF labelSceneRect = label->sceneBoundingRect();
                    for (QGraphicsItem *seg : triodeSegments) {
                        QRectF segRect = seg->sceneBoundingRect();
                        QPointF mid = segRect.center();
                        if (labelSceneRect.contains(mid)) {
                            plot->remove(seg);
                            delete seg;
                            continue;
                        }
                        items.append(seg);
                    }

                    items.append(label);
                } else {
                    for (QGraphicsItem *seg : triodeSegments) {
                        items.append(seg);
                    }
                }
            } else {
                for (QGraphicsItem *seg : triodeSegments) {
                    items.append(seg);
                }
            }
        } else {
            const double vgLabel = vg1; // preserve for label text
            double va = 0.0;
            double ia = 0.0;
            ia = model->anodeCurrent(va, vg1, vg2Max);

            // For pentode model families, sample up to the full visible X
            // range so lines can extend to the right edge of the current
            // Plot axes (e.g., Designer's 2*VB range), rather than stopping
            // at the device's vaMax. Also use this same range for label
            // visibility scanning.
            double curveVaMax = vaMax;
            const double xScaleGlobal = plot->getXScale();
            if (xScaleGlobal > 0.0) {
                const double xStartGlobal = plot->getXStart();
                const double xStopGlobal  = xStartGlobal + static_cast<double>(PLOT_WIDTH) / xScaleGlobal;
                curveVaMax = std::max(vaMax, xStopGlobal);
            }

            // Collect this family's segments locally so we can trim out the
            // portion covered by the label rectangle, mirroring the triode
            // branch behaviour.
            QList<QGraphicsItem*> pentodeSegments;
            for (int j = 1; j < 61; j++) {
                double vaNext = (curveVaMax * j) / 60.0;
                double iaNext = model->anodeCurrent(vaNext, vg1, vg2Max);

                QGraphicsLineItem *seg = plot->createSegment(va, ia, vaNext, iaNext, modelPen);
                if (seg) {
                    pentodeSegments.append(seg);
                }

                va = vaNext;
                ia = iaNext;
            }

            // Place one label per pentode Vg1 family roughly 70% along the
            // visible Va axis, clamped into the current Plot axes so labels
            // follow the curve in both Modeller and Designer (even when the
            // Y-axis has been extended above the device's iaMax).
            const double xScale = plot->getXScale();
            const double yScale = plot->getYScale();

            double x = 0.0;
            double y = 0.0;
            bool hasVisibleSegment = false;

            if (xScale != 0.0 && yScale != 0.0) {
                const double xStart = plot->getXStart();
                const double yStart = plot->getYStart();
                const double xStop  = xStart + static_cast<double>(PLOT_WIDTH)  / xScale;
                const double yStop  = yStart + static_cast<double>(PLOT_HEIGHT) / yScale;

                // Scan along the model curve to find the portion that is
                // actually visible inside the current axes box. Use 70% of
                // that visible segment so labels sit inside the drawn line,
                // not in a region that has been clipped by X or Y limits.
                const int steps = 60;
                int firstVis = -1;
                int lastVis  = -1;
                for (int j = 0; j <= steps; ++j) {
                    const double vaProbe = curveVaMax * static_cast<double>(j) / static_cast<double>(steps);
                    double iaProbe = model->anodeCurrent(vaProbe, vg1, vg2Max);
                    if (!std::isfinite(iaProbe)) {
                        continue;
                    }
                    if (vaProbe < xStart || vaProbe > xStop) {
                        continue;
                    }
                    if (iaProbe < yStart || iaProbe > yStop) {
                        continue;
                    }
                    if (firstVis < 0) {
                        firstVis = j;
                    }
                    lastVis = j;
                }

                if (firstVis >= 0 && lastVis >= firstVis) {
                    hasVisibleSegment = true;

                    const double epsX = std::max(0.5,  std::abs(xStop - xStart) * 0.01);
                    const double epsY = std::max(0.05, std::abs(yStop - yStart) * 0.01);

                    const double vaStart = curveVaMax * static_cast<double>(firstVis) / static_cast<double>(steps);
                    const double vaEnd   = curveVaMax * static_cast<double>(lastVis)  / static_cast<double>(steps);

                    double vaLabel = vaStart;
                    if (lastVis > firstVis) {
                        vaLabel = vaStart + 0.7 * (vaEnd - vaStart);
                    }
                    vaLabel = std::min(xStop - epsX, std::max(xStart, vaLabel));
                    x = vaLabel;

                    double yAtLine = model->anodeCurrent(x, vg1, vg2Max);
                    // Fallback: mid-height of the visible axis if the model
                    // returns a non-finite value.
                    y = yStart + 0.5 * (yStop - yStart);
                    if (std::isfinite(yAtLine)) {
                        y = std::min(yStop - epsY, std::max(yStart, yAtLine));
                    }
                }
            } else {
                // Fallback to device vaMax/iaMax if plot scales are invalid.
                const double epsX = std::max(0.5, vaMax * 0.01);
                const double epsY = std::max(0.05, iaMax * 0.01);
                const double vaLabel = std::max(0.0, vaMax * 0.7);
                x = std::min(vaMax - epsX, std::max(0.0, vaLabel));
                double yAtLine = model->anodeCurrent(x, vg1, vg2Max);
                y = iaMax - epsY;
                if (std::isfinite(yAtLine)) {
                    y = std::min(iaMax - epsY, std::max(0.0, yAtLine));
                }
            }

            if (hasVisibleSegment || xScale == 0.0 || yScale == 0.0) {
                QGraphicsTextItem *label = plot->createLabel(x, y, vgLabel, QColor::fromRgb(255, 0, 0));
                if (label) {
                    label->setPlainText(QString("%1V").arg(vgLabel, 0, 'f', 1));
                    QFont f = label->font();
                    f.setPointSizeF(std::max(7.0, f.pointSizeF()));
                    label->setFont(f);

                    // Reposition so that the centre of the label rectangle
                    // sits on the chosen (x,y) point on the curve, mirroring
                    // the triode labelling behaviour.
                    QRectF r = label->boundingRect();
                    QPointF p = label->pos();
                    const double halfW = r.width() * 0.5;
                    const double halfH = r.height() * 0.5;
                    label->setPos(p.x() - 5.0 - halfW,
                                  p.y() + 10.0 - halfH);

                    // Remove any segment whose midpoint lies inside the
                    // label rectangle so the line segment touching the
                    // label is not drawn.
                    QRectF labelSceneRect = label->sceneBoundingRect();
                    for (QGraphicsItem *seg : pentodeSegments) {
                        QRectF segRect = seg->sceneBoundingRect();
                        QPointF mid = segRect.center();
                        if (labelSceneRect.contains(mid)) {
                            plot->remove(seg);
                            delete seg;
                            continue;
                        }
                        items.append(seg);
                    }

                    items.append(label);
                } else {
                    for (QGraphicsItem *seg : pentodeSegments) {
                        items.append(seg);
                    }
                }
            } else {
                for (QGraphicsItem *seg : pentodeSegments) {
                    items.append(seg);
                }
            }
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
