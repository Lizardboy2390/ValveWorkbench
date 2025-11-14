#ifndef VALVEMODEL_DATA_UTRACERPARSER_H
#define VALVEMODEL_DATA_UTRACERPARSER_H

#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

class Measurement;

class UTracerParser
{
public:
    struct Entry
    {
        QString filePath;
        std::unique_ptr<Measurement> measurement;
    };

    struct Result
    {
        std::vector<Entry> entries;
        QStringList warnings;

        bool hasWarnings() const { return !warnings.isEmpty(); }
    };

    static Result parse(const QStringList &filePaths, double gridOffset = 0.0);

private:
    static std::unique_ptr<Measurement> parseFile(const QString &filePath,
                                                  double gridOffset,
                                                  QStringList *warnings);
};

#endif // VALVEMODEL_DATA_UTRACERPARSER_H
