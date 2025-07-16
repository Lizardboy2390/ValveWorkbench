#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

// Simple class to test model loading
class DeviceTest {
public:
    QString name;
    int deviceType = 0; // 0 = triode, 1 = pentode

    DeviceTest(const QString& filename) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                
                if (obj.contains("name") && obj["name"].isString()) {
                    name = obj["name"].toString();
                }
                
                if (obj.contains("model") && obj["model"].isObject()) {
                    QJsonObject modelObj = obj["model"].toObject();
                    
                    if (modelObj.contains("device") && modelObj["device"].isString()) {
                        QString deviceStr = modelObj["device"].toString();
                        if (deviceStr == "pentode") {
                            deviceType = 1;
                        }
                    }
                }
                
                qDebug() << "Loaded device:" << name << "Type:" << (deviceType == 0 ? "Triode" : "Pentode");
            } else {
                qDebug() << "Failed to parse JSON from file:" << filename;
            }
        } else {
            qDebug() << "Failed to open file:" << filename;
        }
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Get the application directory
    QString appDir = QCoreApplication::applicationDirPath();
    QString modelDir = appDir + "/models";
    
    qDebug() << "Looking for models in:" << modelDir;
    
    QDir dir(modelDir);
    if (!dir.exists()) {
        qDebug() << "ERROR: Models directory does not exist!";
        return 1;
    }
    
    // List all .vwm and .json files
    QStringList filters;
    filters << "*.vwm" << "*.json";
    dir.setNameFilters(filters);
    
    QStringList files = dir.entryList();
    qDebug() << "Found" << files.size() << "model files:";
    
    QList<DeviceTest*> devices;
    
    // Load each file
    for (const QString& file : files) {
        qDebug() << "Loading:" << file;
        QString fullPath = modelDir + "/" + file;
        DeviceTest* device = new DeviceTest(fullPath);
        devices.append(device);
    }
    
    // Display summary
    qDebug() << "\nSummary:";
    qDebug() << "Total devices loaded:" << devices.size();
    
    int triodes = 0;
    int pentodes = 0;
    
    for (DeviceTest* device : devices) {
        if (device->deviceType == 0) {
            triodes++;
        } else {
            pentodes++;
        }
    }
    
    qDebug() << "Triodes:" << triodes;
    qDebug() << "Pentodes:" << pentodes;
    
    // Clean up
    qDeleteAll(devices);
    
    return 0;
}
