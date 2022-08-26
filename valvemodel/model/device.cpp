#include "device.h"

Device::Device(int _modelDeviceType) : deviceType(_modelDeviceType)
{
    if (deviceType == MODEL_TRIODE) {
        modelType = COHEN_HELIE_TRIODE;
    }
}

Device::Device(QJsonDocument modelDocument)
{
    vaMax = 400.0;
    iaMax = 6.0;
    vg1Max = 4.0;
    vg2Max = vaMax;
    paMax = 1.25;

    if (modelDocument.isObject()) {
        QJsonObject modelObject = modelDocument.object();

        if (modelObject.contains("name") && modelObject["name"].isString()) {
            name = modelObject["name"].toString();
        }

        if (modelObject.contains("vaMax") && modelObject["vaMax"].isDouble()) {
            vaMax = modelObject["vaMax"].toDouble();
        }

        if (modelObject.contains("iaMax") && modelObject["iaMax"].isDouble()) {
            iaMax = modelObject["iaMax"].toDouble();
        }

        if (modelObject.contains("paMax") && modelObject["paMax"].isDouble()) {
            paMax = modelObject["paMax"].toDouble();
        }

        if (modelObject.contains("triode") && modelObject["triode"].isObject()) {
            QJsonObject triode = modelObject["triode"].toObject();

            deviceType = MODEL_TRIODE;

            if (triode.contains("vg1Max") && triode["vg1Max"].isDouble()) {
                vg1Max = triode["vg1Max"].toDouble();
            }

            if (triode.contains("gardiner") && triode["gardiner"].isObject()) {
                Model *newModel = new GardinerTriode();
                newModel->fromJson(triode["gardiner"].toObject());
                models.append(newModel);
            }

            if (triode.contains("cohenHelie") && triode["cohenHelie"].isObject()) {
                Model *newModel = new CohenHelieTriode();
                newModel->fromJson(triode["cohenHelie"].toObject());
                models.append(newModel);
            }

            if (triode.contains("koren") && triode["koren"].isObject()) {
                Model *newModel = new KorenTriode();
                newModel->fromJson(triode["koren"].toObject());
                models.append(newModel);
            }

            if (triode.contains("simple") && triode["simple"].isObject()) {
                Model *newModel = new SimpleTriode();
                newModel->fromJson(triode["simple"].toObject());
                models.append(newModel);
            }
        }
    }
}

void Device::solve()
{
    if (currentModel != nullptr) {
        currentModel->solve();
    }
}

double Device::anodeCurrent(double va, double vg1, double vg2)
{
    if (currentModel != nullptr) {
        return currentModel->anodeCurrent(va, vg1, vg2);
    }

    return 0.0;
}

double Device::anodeVoltage(double ia, double vg1, double vg2)
{
    if (currentModel != nullptr) {
        return currentModel->anodeVoltage(ia, vg1, vg2);
    }

    return 0.0;
}

void Device::updateUI(QLabel *labels[], QLineEdit *values[])
{
    for (int i=0; i < 16; i++) { // Parameters all initially hidden
        values[i]->setVisible(false);
        labels[i]->setVisible(false);
    }
    if (currentModel != nullptr) {
        currentModel->updateUI(labels, values);
    }
}

void Device::updateModelSelect(QComboBox *select)
{
    select->clear();

    for (int i=0; i < models.size(); i++) {
        select->addItem(models.at(i)->getName());
    }

    selectModel(select->currentIndex());
}

void Device::selectModel(int index)
{
    currentModel = models.at(index);
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

    double vg1 = 0.0;

    while (vg1 < vg1Max) { // vg1 will be made -ve in order to calculate ia
        double va = 0.0;
        double ia = currentModel->anodeCurrent(va, -vg1);

        for (int j=1; j < 101; j++) {
            double vaNext = (vaMax * j) / 100.0;
            double iaNext = currentModel->anodeCurrent(vaNext, -vg1);
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
    return 0.0;
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
