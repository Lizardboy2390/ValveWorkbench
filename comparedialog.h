#ifndef COMPAREDIALOG_H
#define COMPAREDIALOG_H

#include <QDialog>
#include <QList>
#include <QColor>
#include <QIcon>
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
    void setAvailableModels(const QList<Model *> &models);
    void setComparisonModel(Model *model);

signals:
    void referenceModelChanged(Model *model);
    void comparisonModelChanged(Model *model);

private:
    // UI
    Ui::CompareDialog *ui;
    QGroupBox *modelSelectionGroup;
    QComboBox *referenceModelCombo;
    QComboBox *comparisonModelCombo;

    // State
    QList<Model *> availableModels;
    Model *pendingReferenceModel = nullptr;
    Model *pendingComparisonModel = nullptr;

    // Label sets for triode/pentode metric panels
    std::array<QLabel *, 4> triodeReferenceLabels{};
    std::array<QLabel *, 4> triodeComparisonLabels{};
    std::array<QLabel *, 3> pentodeReferenceLabels{};
    std::array<QLabel *, 3> pentodeComparisonLabels{};

    // Helpers
    void repopulateCombo(QComboBox *combo, Model *selectedModel);
    void selectModelInCombo(QComboBox *combo, Model *model);
    Model *modelForComboIndex(const QComboBox *combo, int comboIndex) const;
    void emitCurrentSelections();
    QIcon colorSwatch(const QColor &color) const;
    QString displayNameForModel(Model *model) const;
    void updateMetrics();

    struct ModelMetrics {
        bool valid = false;
        double mu = 0.0;
        double ia = 0.0;
        double gm = 0.0;
        double rp = 0.0;
    };

    ModelMetrics computeTriodeMetrics(Model *model, double anodeVoltage, double gridVoltage);
    ModelMetrics computePentodeMetrics(Model *model, double anodeVoltage, double gridVoltage, double screenVoltage);
    void applyMetricsToLabels(const ModelMetrics &metrics, QLabel *muLabel, QLabel *iaLabel, QLabel *gmLabel, QLabel *rpLabel);
    double valueFromLineEdit(QLineEdit *edit) const;
    QString formatValue(double value, const QString &unit, int precision) const;
    bool isPentodeModel(Model *model) const;
};

#endif // COMPAREDIALOG_H
