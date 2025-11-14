#include "utracerconfig.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace {
static QString trimValue(const QString &line)
{
    const int equalsIndex = line.indexOf('=');
    if (equalsIndex < 0) {
        return QString();
    }
    return line.mid(equalsIndex + 1).trimmed();
}

static QString normalizeKey(const QString &line)
{
    const int equalsIndex = line.indexOf('=');
    if (equalsIndex < 0) {
        return QString();
    }
    QString key = line.left(equalsIndex).trimmed();
    return key.toLower();
}

static UTracerConfig::TubeType parseTube(const QString &value)
{
    const QString lower = value.toLower();
    if (lower == "triode") {
        return UTracerConfig::TubeTriode;
    }
    if (lower == "pentode") {
        return UTracerConfig::TubePentode;
    }
    return UTracerConfig::TubeUnknown;
}

static UTracerConfig::ModelFlavor parseModel(const QString &value)
{
    const QString lower = value.toLower();
    if (lower == "derk") {
        return UTracerConfig::ModelDerk;
    }
    if (lower == "derke") {
        return UTracerConfig::ModelDerkE;
    }
    return UTracerConfig::ModelDefault;
}
} // namespace

bool UTracerConfigParser::parse(const QString &filePath, UTracerConfig *outConfig, QStringList *warnings)
{
    if (!outConfig) {
        if (warnings) warnings->append(QStringLiteral("Null config pointer supplied"));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (warnings) warnings->append(QStringLiteral("Failed to open INI file"));
        return false;
    }

    QTextStream stream(&file);

    UTracerConfig config;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) {
            continue;
        }

        const QString key = normalizeKey(line);
        const QString value = trimValue(line);

        if (key == "file" || key == "datafile") {
            config.dataFiles.append(value);
        } else if (key == "tube") {
            config.tubeType = parseTube(value);
        } else if (key == "modelflavor" || key == "model") {
            config.modelFlavor = parseModel(value);
        } else if (key == "pmax" || key == "pmax_watts") {
            config.pMaxWatts = value.toDouble();
        } else if (key == "vgoffset" || key == "vg_offset") {
            config.gridOffsetVolts = value.toDouble();
        } else if (key == "icmax" || key == "ic_max") {
            config.icMaxMilliAmps = value.toDouble();
        }
    }

    *outConfig = config;

    if (config.dataFiles.isEmpty()) {
        if (warnings) warnings->append(QStringLiteral("Config did not specify any data files"));
    }

    return true;
}
