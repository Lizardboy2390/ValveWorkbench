#ifndef COMPAREDIALOG_H
#define COMPAREDIALOG_H

#include <QDialog>
#include "valvemodel/model/model.h"
#include "valvemodel/constants.h"

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
    void setReferenceValues(double mu, double gm, double ra); // For triodes
    void setReferenceValues(double gm, double ra); // For pentodes
    void setReferenceCurrents(double ia); // For triodes
    void setReferenceCurrents(double ia, double screenCurrent); // For pentodes
    
private slots:
<<<<<<< Updated upstream
    void updateCalculations();
=======
    void updateComparison();
    void on_pushButton_clicked();
>>>>>>> Stashed changes

private:
    Ui::CompareDialog *ui;
    Model *m_model = nullptr; // Store the model for calculations
<<<<<<< Updated upstream
=======
    
    void updateTriodeComparison();
    void updatePentodeComparison();
>>>>>>> Stashed changes
};

#endif // COMPAREDIALOG_H
