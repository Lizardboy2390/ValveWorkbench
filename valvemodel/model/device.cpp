#include "device.h"

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

            if (modelObject.contains("type") && modelObject["type"].isString()) {
                QString modelType = modelObject["type"].toString();
                qInfo("Model type: %s", modelType.toStdString().c_str());

                if (modelType == "simple") {
                    model = new SimpleTriode();
                } else if (modelType == "koren") {
                    model = new KorenTriode();
                } else if (modelType == "cohenHelie") {
                    model = new CohenHelieTriode();
                } else if (modelType == "reefman") {
                    model = new ReefmanPentode();
                } else if (modelType == "gardiner") {
                    model = new GardinerPentode();
                }

                if (model != nullptr) {
                    model->fromJson(modelObject);
                    qInfo("Model created and initialized");
                } else {
                    qInfo("Model type not recognized: %s", modelType.toStdString().c_str());
                }
            }
        } else {
            qInfo("No model object found in JSON");
        }
    } else {
        qInfo("JSON document is not an object");
    }

    qInfo("Final device type: %d", deviceType);
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
    QList<QGraphicsItem *> segments;

    QPen modelPen;
    modelPen.setColor(QColor::fromRgb(255, 0, 0));

    double vgInterval = interval(vg1Max);

    double vg1 = -vg1Max;

    while (vg1 <= 0.0) {
        double va = 0.0;
        double ia = model->anodeCurrent(va, vg1);

        for (int j = 1; j < 61; j++) {
            double vaNext = (vaMax * j) / 60.0;
            double iaNext = model->anodeCurrent(vaNext, vg1);
            segments.append(plot->createSegment(va, ia, vaNext, iaNext, modelPen));

            va = vaNext;
            ia = iaNext;
        }

        vg1 += vgInterval;
    }

    return plot->getScene()->createItemGroup(segments);
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
