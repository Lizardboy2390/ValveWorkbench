#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QCheckBox>
 #include <QSpinBox>
 #include <QVector>

class QPushButton;
class QLabel;
class QCheckBox;

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();

    void setPort(QString port);
    QString getPort();

    int getPentodeModelType();
    int getSamplingType();
    int getAveragingMode();
    int getAveragingFixedSamples();
    bool useRemodelling();
    bool useSecondaryEmission();
    bool fixSecondaryEmission();
    bool fixTriodeParameters();
    bool showScreenCurrent();
    bool smoothCurves();
    bool showDataTab();
    double getCalibrationLoadResistorOhms() const;
    double getCalibrationLoadResistorWatts() const;
    double grid1CommandForDesired(double desiredVoltage) const;
    double grid2CommandForDesired(double desiredVoltage) const;
    double getGrid1MeasuredLow() const;
    double getGrid1MeasuredHigh() const;
    double getGrid2MeasuredLow() const;
    double getGrid2MeasuredHigh() const;

    void setHvCalibrationRawAdc(int hv1Adc,
                                int iaHi1Adc,
                                int iaLo1Adc,
                                int hv2Adc,
                                int iaHi2Adc,
                                int iaLo2Adc);

    void loadFromSettings();
    void saveToSettings() const;

signals:
    void applyGridRefRequested(double commandVoltage, bool enabled);
    void requestHvCalibrationSampleOnce();
    void hvHoldRequested(int hvChannel, double volts, bool enabled);
    void hvDischargeRequested();

private slots:

private:
    static constexpr double GRID_CAL_LOW_REF = 5.0;
    static constexpr double GRID_CAL_HIGH_REF = 60.0;
    static constexpr double GRID_CAL_EPSILON = 1e-6;

    Ui::PreferencesDialog *ui;

    QDoubleSpinBox *calibrationLoadResistorSpinBox;
    QDoubleSpinBox *calibrationLoadResistorWattsSpinBox;
    QCheckBox *linkHvVoltagePointsCheckBox;
    QPushButton *hvSampleOnceButton;
    QPushButton *hvStopButton;
    QPushButton *hvDischargeButton;
    QLabel *hvCalRawLabel;
    int lastHv1Adc = 0;
    int lastIaHi1Adc = 0;
    int lastIaLo1Adc = 0;
    int lastHv2Adc = 0;
    int lastIaHi2Adc = 0;
    int lastIaLo2Adc = 0;

    QVector<QDoubleSpinBox*> hv1PointVoltsSpin;
    QVector<QSpinBox*> hv1PointHvAdcSpin;
    QVector<QSpinBox*> hv1PointIaHiAdcSpin;
    QVector<QSpinBox*> hv1PointIaLoAdcSpin;

    QVector<QDoubleSpinBox*> hv2PointVoltsSpin;
    QVector<QSpinBox*> hv2PointHvAdcSpin;
    QVector<QSpinBox*> hv2PointIaHiAdcSpin;
    QVector<QSpinBox*> hv2PointIaLoAdcSpin;

    QVector<double> hv1LastSuggestedVolts;
    QVector<double> hv2LastSuggestedVolts;

    QPushButton *hv1HoldActiveButton = nullptr;
    QPushButton *hv2HoldActiveButton = nullptr;
    QDoubleSpinBox *grid1MeasuredLowSpinBox;
    QDoubleSpinBox *grid1MeasuredHighSpinBox;
    QDoubleSpinBox *grid2MeasuredLowSpinBox;
    QDoubleSpinBox *grid2MeasuredHighSpinBox;
    QCheckBox *applyLowRefBothCheckBox;
    QCheckBox *applyHighRefBothCheckBox;

    double gridCommandForDesired(double desiredVoltage, double measuredLow, double measuredHigh) const;
};

#endif // PREFERENCESDIALOG_H
