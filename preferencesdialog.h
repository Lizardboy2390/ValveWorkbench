#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>
#include <QDoubleSpinBox>
#include <QLabel>

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

    double getHeaterVoltageCalibration();
    double getHeaterCurrentCalibration();
    double getAnodeVoltageCalibration();
    double getAnodeCurrentCalibration();
    double getScreenVoltageCalibration();
    double getScreenCurrentCalibration();
    double getGridVoltageCalibration();
    double getGridCurrentCalibration();

private slots:

private:
    Ui::PreferencesDialog *ui;

    QDoubleSpinBox *heaterVoltageSpinBox;
    QDoubleSpinBox *heaterCurrentSpinBox;
    QDoubleSpinBox *anodeVoltageSpinBox;
    QDoubleSpinBox *anodeCurrentSpinBox;
    QDoubleSpinBox *screenVoltageSpinBox;
    QDoubleSpinBox *screenCurrentSpinBox;
    QDoubleSpinBox *gridVoltageSpinBox;
    QDoubleSpinBox *gridCurrentSpinBox;

    QLabel *heaterVoltageLabel;
    QLabel *heaterCurrentLabel;
    QLabel *anodeVoltageLabel;
    QLabel *anodeCurrentLabel;
    QLabel *screenVoltageLabel;
    QLabel *screenCurrentLabel;
    QLabel *gridVoltageLabel;
    QLabel *gridCurrentLabel;
};

#endif // PREFERENCESDIALOG_H
