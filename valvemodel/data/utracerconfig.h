#ifndef VALVEMODEL_DATA_UTRACERCONFIG_H
#define VALVEMODEL_DATA_UTRACERCONFIG_H

#include <QString>
#include <QStringList>

struct UTracerConfig
{
    enum TubeType {
        TubeTriode,
        TubePentode,
        TubeUnknown
    };

    enum ModelFlavor {
        ModelDefault,
        ModelDerk,
        ModelDerkE
    };

    QStringList dataFiles;
    TubeType tubeType = TubeUnknown;
    ModelFlavor modelFlavor = ModelDefault;
    double pMaxWatts = 0.0;
    double gridOffsetVolts = 0.0;
    double icMaxMilliAmps = 0.0;

    bool hasIcLimit() const { return icMaxMilliAmps > 0.0; }
};

class UTracerConfigParser
{
public:
    static bool parse(const QString &filePath, UTracerConfig *outConfig, QStringList *warnings = nullptr);
};

#endif // VALVEMODEL_DATA_UTRACERCONFIG_H
