#ifndef COMPAREDIALOG_H
#define COMPAREDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>
#include <limits>
#include <array>
#include "valvemodel/model/model.h"

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;

namespace Ui {
class CompareDialog;
}

class CompareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CompareDialog(QWidget *parent = nullptr);
    ~CompareDialog();

    void setModel(Model *model);
    void setAvailableModels(const QList<Model*> &models);
    void setComparisonModel(Model *model);

signals:
    void referenceModelChanged(Model *model);
    void comparisonModelChanged(Model *model);

private:
    Ui::CompareDialog *ui;
    QGroupBox *modelSelectionGroup;
    QComboBox *referenceModelCombo;
    QComboBox *comparisonModelCombo;
    QList<Model*> availableModels;
    Model *pendingReferenceModel;
    Model *pendingComparisonModel;

    struct ModelMetrics {
        bool valid = false;
        double mu = 0.0;   // unitless
        double gm = 0.0;   // native derivative units (model output)
        double rp = 0.0;   // native resistance units (model output)
        double ia = 0.0;   // native current units (model output)
    };

    std::array<QLabel *, 4> triodeReferenceLabels{};
    std::array<QLabel *, 4> triodeComparisonLabels{};
    std::array<QLabel *, 3> pentodeReferenceLabels{};
    std::array<QLabel *, 3> pentodeComparisonLabels{};

    void repopulateCombo(QComboBox *combo, Model *selectedModel);
    void selectModelInCombo(QComboBox *combo, Model *model);
    Model *modelForComboIndex(const QComboBox *combo, int comboIndex) const;
    void emitCurrentSelections();
    QIcon colorSwatch(const QColor &color) const;
    QString displayNameForModel(Model *model) const;
    void updateMetrics();
    ModelMetrics computeTriodeMetrics(Model *model, double anodeVoltage, double gridVoltage);
    ModelMetrics computePentodeMetrics(Model *model, double anodeVoltage, double gridVoltage, double screenVoltage);
    void applyMetricsToLabels(const ModelMetrics &metrics, QLabel *muLabel, QLabel *iaLabel, QLabel *gmLabel, QLabel *rpLabel);
    double valueFromLineEdit(QLineEdit *edit) const;
    QString formatValue(double value, const QString &unit, int precision = 2) const;
    bool isPentodeModel(Model *model) const;

};

#endif // COMPAREDIALOG_H
