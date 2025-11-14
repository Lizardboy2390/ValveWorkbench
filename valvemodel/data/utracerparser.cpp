#include "utracerparser.h"

#include "measurement.h"
#include "sample.h"
#include "sweep.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>

#include <algorithm>

namespace {
static constexpr double milliampToAmp = 1.0 / 1000.0;

struct ColumnMap
{
    int point = -1;
    int curve = -1;
    int ia = -1;
    int is = -1;
    int vg = -1;
    int va = -1;
    int vs = -1;
    int vf = -1;

    bool isValid() const
    {
        return curve >= 0 && ia >= 0 && vg >= 0 && va >= 0;
    }
};

static ColumnMap detectColumns(const QStringList &headers)
{
    ColumnMap map;
    for (int idx = 0; idx < headers.size(); ++idx) {
        const QString token = headers.at(idx).trimmed().toLower();
        if (token.startsWith("point")) map.point = idx;
        else if (token.startsWith("curve")) map.curve = idx;
        else if (token.startsWith("ia")) map.ia = idx;
        else if (token.startsWith("is")) map.is = idx;
        else if (token.startsWith("vg")) map.vg = idx;
        else if (token.startsWith("va")) map.va = idx;
        else if (token.startsWith("vs")) map.vs = idx;
        else if (token.startsWith("vf")) map.vf = idx;
    }
    return map;
}

static double toDouble(const QStringList &tokens, int index, bool *ok)
{
    if (index < 0 || index >= tokens.size()) {
        if (ok) *ok = false;
        return 0.0;
    }
    const double value = tokens.at(index).toDouble(ok);
    return value;
}
} // namespace

UTracerParser::Result UTracerParser::parse(const QStringList &filePaths, double gridOffset)
{
    Result result;
    for (const QString &path : filePaths) {
        QStringList warnings;
        std::unique_ptr<Measurement> measurement = parseFile(path, gridOffset, &warnings);
        if (!warnings.isEmpty()) {
            result.warnings.append(QStringLiteral("%1:").arg(path));
            result.warnings.append(warnings);
        }
        if (measurement) {
            Entry entry{path, std::move(measurement)};
            result.entries.emplace_back(std::move(entry));
        }
    }
    return result;
}

std::unique_ptr<Measurement> UTracerParser::parseFile(const QString &filePath,
                                                      double gridOffset,
                                                      QStringList *warnings)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (warnings) warnings->append(QStringLiteral("Failed to open file"));
        return nullptr;
    }

    QTextStream stream(&file);

    QString headerLine;
    while (!stream.atEnd()) {
        headerLine = stream.readLine().trimmed();
        if (!headerLine.isEmpty() && !headerLine.startsWith('#')) {
            break;
        }
    }

    if (headerLine.isEmpty()) {
        if (warnings) warnings->append(QStringLiteral("Missing header row"));
        return nullptr;
    }

    const QRegularExpression splitter(QStringLiteral("\t|\s+"));

    const QStringList headerTokens = headerLine.split(splitter, Qt::SkipEmptyParts);
    const ColumnMap columns = detectColumns(headerTokens);
    if (!columns.isValid()) {
        if (warnings) warnings->append(QStringLiteral("Header row does not expose required columns"));
        return nullptr;
    }

    std::unique_ptr<Measurement> measurement(new Measurement());
    measurement->setDeviceType(PENTODE);
    measurement->setTestType(ANODE_CHARACTERISTICS);

    double minGrid = std::numeric_limits<double>::infinity();
    double maxGrid = -std::numeric_limits<double>::infinity();
    double minScreen = std::numeric_limits<double>::infinity();
    double maxScreen = -std::numeric_limits<double>::infinity();
    double minAnode = std::numeric_limits<double>::infinity();
    double maxAnode = -std::numeric_limits<double>::infinity();
    double iaMax = 0.0;
    double powerMax = 0.0;

    QList<double> gridSamples;
    int previousCurve = std::numeric_limits<int>::min();

    while (!stream.atEnd()) {
        const QString rawLine = stream.readLine();
        QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }
        const QStringList tokens = trimmed.split(splitter, Qt::SkipEmptyParts);
        if (tokens.size() < headerTokens.size()) {
            if (warnings) warnings->append(QStringLiteral("Skipping malformed row: %1").arg(rawLine.trimmed()));
            continue;
        }

        bool okCurve = false;
        const int curveIndex = qRound(toDouble(tokens, columns.curve, &okCurve));
        if (!okCurve) {
            if (warnings) warnings->append(QStringLiteral("Missing curve index"));
            continue;
        }

        bool okVa = false, okVg = false, okVs = false, okIa = false, okIs = false;
        const double va = toDouble(tokens, columns.va, &okVa);
        const double vgRaw = toDouble(tokens, columns.vg, &okVg);
        const double vs = columns.vs >= 0 ? toDouble(tokens, columns.vs, &okVs) : 0.0;
        const double iaMilli = toDouble(tokens, columns.ia, &okIa);
        const double ig2Milli = columns.is >= 0 ? toDouble(tokens, columns.is, &okIs) : 0.0;

        if (!okVa || !okVg || !okIa || (columns.vs >= 0 && !okVs) || (columns.is >= 0 && !okIs)) {
            if (warnings) warnings->append(QStringLiteral("Skipping row with invalid numeric data: %1").arg(rawLine.trimmed()));
            continue;
        }

        const double vg = vgRaw + gridOffset;
        if (curveIndex != previousCurve) {
            measurement->nextSweep(vg, vs);
            previousCurve = curveIndex;
            if (!gridSamples.contains(vg)) {
                gridSamples.append(vg);
            }
        }

        Sample *sample = new Sample(vg, va, iaMilli, vs, ig2Milli, 0.0, 0.0, 0.0, 0.0, 0.0);
        measurement->addSample(sample);

        minGrid = std::min(minGrid, vg);
        maxGrid = std::max(maxGrid, vg);
        minScreen = std::min(minScreen, vs);
        maxScreen = std::max(maxScreen, vs);
        minAnode = std::min(minAnode, va);
        maxAnode = std::max(maxAnode, va);
        iaMax = std::max(iaMax, iaMilli);
        const double power = va * (iaMilli * milliampToAmp);
        powerMax = std::max(powerMax, power);
    }

    if (measurement->count() == 0) {
        if (warnings) warnings->append(QStringLiteral("File did not yield any sweeps"));
        return nullptr;
    }

    if (std::isfinite(minGrid) && std::isfinite(maxGrid)) {
        measurement->setGridStart(minGrid);
        measurement->setGridStop(maxGrid);
    }

    if (gridSamples.size() >= 2) {
        std::sort(gridSamples.begin(), gridSamples.end());
        double minStep = std::numeric_limits<double>::infinity();
        for (int i = 1; i < gridSamples.size(); ++i) {
            const double delta = qAbs(gridSamples.at(i) - gridSamples.at(i - 1));
            if (delta > 0.0) {
                minStep = std::min(minStep, delta);
            }
        }
        if (std::isfinite(minStep) && minStep > 0.0) {
            measurement->setGridStep(minStep);
        }
    }

    // Attempt to derive anode/screen step sizes from the first sweep
    if (measurement->count() > 0) {
        Sweep *firstSweep = measurement->at(0);
        if (firstSweep && firstSweep->count() >= 2) {
            const double va0 = firstSweep->at(0)->getVa();
            const double va1 = firstSweep->at(1)->getVa();
            const double anodeStep = qAbs(va1 - va0);
            if (anodeStep > 0.0) {
                measurement->setAnodeStep(anodeStep);
            }

            const double vg20 = firstSweep->at(0)->getVg2();
            for (int i = 1; i < firstSweep->count(); ++i) {
                const double vg2 = firstSweep->at(i)->getVg2();
                if (!qFuzzyCompare(vg20, vg2)) {
                    const double screenStep = qAbs(vg2 - vg20);
                    if (screenStep > 0.0) {
                        measurement->setScreenStep(screenStep);
                    }
                    break;
                }
            }
        }
    }

    if (std::isfinite(minScreen) && std::isfinite(maxScreen)) {
        measurement->setScreenStart(minScreen);
        measurement->setScreenStop(maxScreen);
    }

    if (std::isfinite(minAnode) && std::isfinite(maxAnode)) {
        measurement->setAnodeStart(minAnode);
        measurement->setAnodeStop(maxAnode);
    }

    measurement->setIaMax(iaMax);
    measurement->setPMax(powerMax);

    return measurement;
}
