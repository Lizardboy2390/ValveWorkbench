#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>

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
    bool useRemodelling();
    bool useSecondaryEmission();
    bool fixSecondaryEmission();
    bool fixTriodeParameters();
    bool showScreenCurrent();

    double getAnodeVoltageCalibration();
    double getAnodeCurrentCalibration();
    double getScreenVoltageCalibration();
    double getScreenCurrentCalibration();
    double getGrid1VoltageCalibration();
    double getGrid2VoltageCalibration();
    double grid1CommandForDesired(double desiredVoltage) const;
    double grid2CommandForDesired(double desiredVoltage) const;
    double getGrid1MeasuredLow() const;
    double getGrid1MeasuredHigh() const;
    double getGrid2MeasuredLow() const;
    double getGrid2MeasuredHigh() const;

private slots:

private:
    static constexpr double GRID_CAL_LOW_REF = 5.0;
    static constexpr double GRID_CAL_HIGH_REF = 60.0;
    static constexpr double GRID_CAL_EPSILON = 1e-6;

    Ui::PreferencesDialog *ui;

    QDoubleSpinBox *anodeVoltageSpinBox;
    QDoubleSpinBox *anodeCurrentSpinBox;
    QDoubleSpinBox *screenVoltageSpinBox;
    QDoubleSpinBox *screenCurrentSpinBox;
    QDoubleSpinBox *grid1VoltageSpinBox;
    QDoubleSpinBox *grid2VoltageSpinBox;
    QDoubleSpinBox *grid1MeasuredLowSpinBox;
    QDoubleSpinBox *grid1MeasuredHighSpinBox;
    QDoubleSpinBox *grid2MeasuredLowSpinBox;
    QDoubleSpinBox *grid2MeasuredHighSpinBox;

    double gridCommandForDesired(double desiredVoltage, double measuredLow, double measuredHigh) const;
};

#endif // PREFERENCESDIALOG_H
