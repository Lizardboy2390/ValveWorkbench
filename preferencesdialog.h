#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>

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
    
    // Calibration getters
    double getHeaterVoltageCalibration() const;
    double getAnodeVoltageCalibration() const;
    double getScreenVoltageCalibration() const;
    double getGridVoltageCalibration() const;
    
    double getHeaterCurrentCalibration() const;
    double getAnodeCurrentCalibration() const;
    double getScreenCurrentCalibration() const;
    
    // Calibration setters
    void setHeaterVoltageCalibration(double value);
    void setAnodeVoltageCalibration(double value);
    void setScreenVoltageCalibration(double value);
    void setGridVoltageCalibration(double value);
    
    void setHeaterCurrentCalibration(double value);
    void setAnodeCurrentCalibration(double value);
    void setScreenCurrentCalibration(double value);

private slots:

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
