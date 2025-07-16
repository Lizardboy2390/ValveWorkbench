#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "valvemodel/model/device.h"
#include "valvemodel/constants.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    // Get the models directory
    QString modelPath = QCoreApplication::applicationDirPath() + QDir::separator() + "models";
    qDebug() << "Looking for model files in:" << modelPath;
    
    QDir modelDir(modelPath);
    if (!modelDir.exists()) {
        qDebug() << "ERROR: Models directory does not exist!";
        return 1;
    }
    
    // Set up filters for .vwm and .json files
    QStringList filters;
    filters << "*.vwm" << "*.json";
    modelDir.setNameFilters(filters);
    
    // Get list of model files
    QStringList models = modelDir.entryList();
    qDebug() << "Found" << models.size() << "model files";
    
    // Load each model file
    QList<Device*> devices;
    
    for (int i = 0; i < models.size(); i++) {
        QString modelFile = modelPath + QDir::separator() + models.at(i);
        qDebug() << "Loading model file:" << modelFile;
        
        QFile file(modelFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray modelData = file.readAll();
            QJsonDocument modelDoc = QJsonDocument::fromJson(modelData);
            
            if (modelDoc.isNull() || !modelDoc.isObject()) {
                qDebug() << "ERROR: Invalid JSON in model file:" << modelFile;
                continue;
            }
            
            Device *device = new Device(modelDoc);
            int type = device->getDeviceType();
            QString name = device->getName();
            
            qDebug() << "Created device:" << name << "(Type:" << type << ")";
            
            // Check if the device type is valid
            if (type != MODEL_TRIODE && type != MODEL_PENTODE) {
                qDebug() << "WARNING: Invalid device type" << type << "for" << name;
            }
            
            devices.append(device);
        } else {
            qDebug() << "ERROR: Could not open model file:" << modelFile;
        }
    }
    
    // Print summary
    qDebug() << "\nSummary:";
    qDebug() << "Total devices loaded:" << devices.size();
    
    int triodes = 0;
    int pentodes = 0;
    int unknown = 0;
    
    for (Device* device : devices) {
        int type = device->getDeviceType();
        if (type == MODEL_TRIODE) {
            triodes++;
            qDebug() << "Triode:" << device->getName();
        } else if (type == MODEL_PENTODE) {
            pentodes++;
            qDebug() << "Pentode:" << device->getName();
        } else {
            unknown++;
            qDebug() << "Unknown type" << type << ":" << device->getName();
        }
    }
    
    qDebug() << "Triodes:" << triodes;
    qDebug() << "Pentodes:" << pentodes;
    qDebug() << "Unknown:" << unknown;
    
    // Clean up
    qDeleteAll(devices);
    
    return 0;
}
