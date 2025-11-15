#pragma once

#include <QDialog>

class QLabel;
class QDoubleSpinBox;
class SimpleManualPentode;

class SimpleManualPentodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleManualPentodeDialog(QWidget *parent = nullptr);

    void setModel(SimpleManualPentode *model);

signals:
    void parametersChanged();

private slots:
    void onParameterChanged();

private:
    SimpleManualPentode *model = nullptr;
    bool updatingFromModel = false;

    QDoubleSpinBox *muSpin = nullptr;
    QDoubleSpinBox *kpSpin = nullptr;
    QDoubleSpinBox *kg1Spin = nullptr;
    QDoubleSpinBox *kg2Spin = nullptr;
    QDoubleSpinBox *alphaSpin = nullptr;
    QDoubleSpinBox *betaSpin = nullptr;
    QDoubleSpinBox *gammaSpin = nullptr;
    QDoubleSpinBox *aSpin = nullptr;

    void createLayout();
    void syncFromModel();
    void applyToModel();
};
