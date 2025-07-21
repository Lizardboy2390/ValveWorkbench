#include "comparedialog.h"
#include "ui_comparedialog.h"
#include <QMessageBox>
#include <QString>

CompareDialog::CompareDialog(QWidget *parent) :
    QDialog(parent),
<<<<<<< Updated upstream
    ui(new Ui::CompareDialog)
=======
    ui(new Ui::CompareDialog),
    m_model(nullptr)
>>>>>>> Stashed changes
{
    ui->setupUi(this);
    
    // Connect the close button
    connect(ui->pushButton, &QPushButton::clicked, this, &CompareDialog::accept);
    
    // Connect text changed signals for input fields
<<<<<<< Updated upstream
    connect(ui->grid, &QLineEdit::editingFinished, this, &CompareDialog::updateCalculations);
    connect(ui->anode, &QLineEdit::editingFinished, this, &CompareDialog::updateCalculations);
    connect(ui->gridP, &QLineEdit::editingFinished, this, &CompareDialog::updateCalculations);
    connect(ui->anodeP, &QLineEdit::editingFinished, this, &CompareDialog::updateCalculations);
    connect(ui->screen, &QLineEdit::editingFinished, this, &CompareDialog::updateCalculations);
}
=======
    connect(ui->grid, &QLineEdit::editingFinished, this, &CompareDialog::updateComparison);
    connect(ui->anode, &QLineEdit::editingFinished, this, &CompareDialog::updateComparison);
    connect(ui->gridP, &QLineEdit::editingFinished, this, &CompareDialog::updateComparison);
    connect(ui->anodeP, &QLineEdit::editingFinished, this, &CompareDialog::updateComparison);
    connect(ui->screen, &QLineEdit::editingFinished, this, &CompareDialog::updateComparison);
>>>>>>> Stashed changes

CompareDialog::~CompareDialog()
{
    delete ui;
}

void CompareDialog::setModel(Model *model)
{
    if (!model) {
        QMessageBox::warning(this, "Error", "No model provided for comparison");
        return;
    }
    
    // Store the model as a member variable for later use
    m_model = model;
    
    // Get the device type (triode or pentode based on model type)
    int deviceType = (m_model->getType() == SIMPLE_TRIODE || 
                     m_model->getType() == KOREN_TRIODE || 
                     m_model->getType() == COHEN_HELIE_TRIODE) ? TRIODE : PENTODE;
    
    // For triode models
    if (deviceType == TRIODE) {
        // Show triode section, hide pentode section
        ui->groupBox->setVisible(true);
        ui->groupBox_2->setVisible(false);
        
        // Set default test conditions
        ui->anode->setText("250");
        ui->grid->setText("-8");
        
<<<<<<< Updated upstream
        // Calculate and display the anode current
        double anodeVoltage = ui->anode->text().toDouble();
        double gridVoltage = ui->grid->text().toDouble();
        double ia = model->triodeAnodeCurrent(anodeVoltage, gridVoltage);
        ui->modIa->setText(QString::number(ia, 'f', 2));
        
        // Calculate mu, gm, ra from the model
        // We'll calculate these by using small changes in voltage
        double delta = 1.0; // 1V change for calculations
        
        // Calculate mu (amplification factor)
        double ia1 = model->triodeAnodeCurrent(anodeVoltage, gridVoltage);
        double ia2 = model->triodeAnodeCurrent(anodeVoltage, gridVoltage - delta);
        double vg_effect = (ia2 - ia1) / delta;
        
        double ia3 = model->triodeAnodeCurrent(anodeVoltage + delta, gridVoltage);
        double va_effect = (ia3 - ia1) / delta;
        
        double mu = va_effect != 0 ? vg_effect / va_effect : 0;
        double gm = vg_effect * 1000; // Convert to mA/V
        double ra = va_effect != 0 ? 1.0 / va_effect : 0;
        
        // Display the parameters
        ui->modMu->setText(QString::number(mu, 'f', 2));
        ui->modGm->setText(QString::number(gm, 'f', 2));
        ui->modRa->setText(QString::number(ra, 'f', 0));
        
        // Enable triode fields, disable pentode fields
        ui->groupBox->setEnabled(true);
        ui->groupBox_2->setEnabled(false);
        
        // Calculate anode current at specified operating point
        updateCalculations();
    }
    // For pentode models
    else if (deviceType == PENTODE) {
        // Calculate pentode parameters
        // We'll calculate these by using small changes in voltage like we did for triodes
        double anodeVoltage = ui->anodeP->text().toDouble();
        double screenVoltage = ui->screen->text().toDouble();
        double gridVoltage = ui->gridP->text().toDouble();
        
        // Calculate gm and ra using voltage perturbations
        double delta = 1.0; // 1V change for calculations
        
        // Calculate gm (transconductance)
        double ia1 = model->anodeCurrent(anodeVoltage, screenVoltage, gridVoltage);
        double ia2 = model->anodeCurrent(anodeVoltage, screenVoltage, gridVoltage - delta);
        double gm = (ia2 - ia1) / delta;
        
        // Calculate ra (plate resistance)
        double ia3 = model->anodeCurrent(anodeVoltage + delta, screenVoltage, gridVoltage);
        double ra = delta / (ia3 - ia1);
        
        // Format the values with appropriate units
        // Clear triode section values since we're showing pentode
        ui->modMu->setText("");  // Pentodes don't have mu in the same way
        ui->modGm->setText("");
        ui->modRa->setText("");
        ui->modIa->setText("");
        
        // Use correct UI element names for pentode parameters
        ui->modGmP->setText(QString::number(gm * 1000.0, 'f', 1) + " mA/V");
        ui->modRaP->setText(QString::number(ra / 1000.0, 'f', 1) + " kΩ");
=======
        // Enable triode fields, disable pentode fields
        ui->groupBox->setEnabled(true);
        ui->groupBox_2->setEnabled(false);
    }
    // For pentode models
    else if (deviceType == PENTODE) {
        // Show pentode section, hide triode section
        ui->groupBox->setVisible(false);
        ui->groupBox_2->setVisible(true);
        
        // Set default test conditions
        ui->anodeP->setText("250");
        ui->gridP->setText("-8");
        ui->screen->setText("250");
>>>>>>> Stashed changes
        
        // Enable pentode fields, disable triode fields
        ui->groupBox->setEnabled(false);
        ui->groupBox_2->setEnabled(true);
<<<<<<< Updated upstream
        
        // Calculate anode current at specified operating point
        updateCalculations();
    }
=======
    }
    
    // Calculate and display values
    updateComparison();
}

void CompareDialog::on_pushButton_clicked()
{
    accept();
>>>>>>> Stashed changes
}

// Set reference values for triodes
void CompareDialog::setReferenceValues(double mu, double gm, double ra)
{
    // Format the values with appropriate units
    ui->refMu->setText(QString::number(mu, 'f', 1));
    ui->refGm->setText(QString::number(gm * 1000.0, 'f', 1) + " mA/V");
    ui->refRa->setText(QString::number(ra / 1000.0, 'f', 1) + " kΩ");
    
    // Clear pentode reference values
    ui->refIaP->setText("");
    ui->refGmP->setText("");
    ui->refRaP->setText("");
}

// Set reference values for pentodes
void CompareDialog::setReferenceValues(double gm, double ra)
{
    // Clear triode reference values
    ui->refMu->setText("");
    ui->refGm->setText("");
    ui->refRa->setText("");
    ui->refIa->setText("");
    
    // Format the values with appropriate units
    ui->refGmP->setText(QString::number(gm * 1000.0, 'f', 1) + " mA/V");
    ui->refRaP->setText(QString::number(ra / 1000.0, 'f', 1) + " kΩ");
}

// Set reference currents for triodes
void CompareDialog::setReferenceCurrents(double ia)
{
    ui->refIa->setText(QString::number(ia * 1000.0, 'f', 1) + " mA");
}

// Set reference currents for pentodes
void CompareDialog::setReferenceCurrents(double ia, double screenCurrent)
{
    // Store reference currents for comparison
    // Note: screenCurrent is only used for pentodes
    Q_UNUSED(screenCurrent); // Suppress unused parameter warning
    
    ui->refIaP->setText(QString::number(ia * 1000.0, 'f', 1) + " mA");
    // Note: Screen current isn't displayed in the current UI design
}

<<<<<<< Updated upstream
// Update calculations when input values change
void CompareDialog::updateCalculations()
=======
void CompareDialog::updateComparison()
>>>>>>> Stashed changes
{
    if (!m_model) {
        return;
    }
    
    // Get the device type (triode or pentode based on model type)
    int deviceType = (m_model->getType() == SIMPLE_TRIODE || 
                     m_model->getType() == KOREN_TRIODE || 
                     m_model->getType() == COHEN_HELIE_TRIODE) ? TRIODE : PENTODE;
    
    // For triode models
    if (deviceType == TRIODE) {
<<<<<<< Updated upstream
        // Get input values
        bool gridOk = false;
        bool anodeOk = false;
        double gridVoltage = ui->grid->text().toDouble(&gridOk);
        double anodeVoltage = ui->anode->text().toDouble(&anodeOk);
        
        if (gridOk && anodeOk && gridVoltage < 0 && anodeVoltage > 0) {
            double ia = m_model->triodeAnodeCurrent(anodeVoltage, gridVoltage);
            ui->modIa->setText(QString::number(ia, 'f', 2));
            
            // Calculate mu, gm, ra from the model
            double delta = 1.0; // 1V change for calculations
            
            // Calculate mu (amplification factor)
            double ia1 = m_model->triodeAnodeCurrent(anodeVoltage, gridVoltage);
            double ia2 = m_model->triodeAnodeCurrent(anodeVoltage, gridVoltage - delta);
            double vg_effect = (ia2 - ia1) / delta;
            
            double ia3 = m_model->triodeAnodeCurrent(anodeVoltage + delta, gridVoltage);
            double va_effect = (ia3 - ia1) / delta;
            
            double mu = va_effect != 0 ? vg_effect / va_effect : 0;
            double gm = vg_effect * 1000; // Convert to mA/V
            double ra = va_effect != 0 ? 1.0 / va_effect : 0;
            
            // Display the parameters
            ui->modMu->setText(QString::number(mu, 'f', 2));
            ui->modGm->setText(QString::number(gm, 'f', 2));
            ui->modRa->setText(QString::number(ra, 'f', 0));
        } else {
            ui->modIa->setText("N/A");
            ui->modMu->setText("N/A");
            ui->modGm->setText("N/A");
            ui->modRa->setText("N/A");
        }
    }
    // For pentode models
    else if (deviceType == PENTODE) {
        // Get input values
        bool gridOk = false;
        bool anodeOk = false;
        bool screenOk = false;
        double gridVoltage = ui->gridP->text().toDouble(&gridOk);
        double anodeVoltage = ui->anodeP->text().toDouble(&anodeOk);
        double screenVoltage = ui->screen->text().toDouble(&screenOk);
        
        if (gridOk && anodeOk && screenOk && gridVoltage < 0 && anodeVoltage > 0 && screenVoltage > 0) {
            double ia = m_model->anodeCurrent(anodeVoltage, gridVoltage, screenVoltage);
            double is = m_model->screenCurrent(anodeVoltage, gridVoltage, screenVoltage);
            
            // Display both anode and screen current in the same label
            ui->modIaP->setText(QString::number(ia, 'f', 2) + " / " + QString::number(is, 'f', 2));
            
            // Calculate gm, ra from the model
            double delta = 1.0; // 1V change for calculations
            
            double ia1 = m_model->anodeCurrent(anodeVoltage, gridVoltage, screenVoltage);
            double ia2 = m_model->anodeCurrent(anodeVoltage, gridVoltage - delta, screenVoltage);
            double vg_effect = (ia2 - ia1) / delta;
            
            double ia3 = m_model->anodeCurrent(anodeVoltage + delta, gridVoltage, screenVoltage);
            double va_effect = (ia3 - ia1) / delta;
            
            double gm = vg_effect * 1000; // Convert to mA/V
            double ra = va_effect != 0 ? 1.0 / va_effect : 0;
            
            // Display the parameters
            ui->modGmP->setText(QString::number(gm, 'f', 2));
            ui->modRaP->setText(QString::number(ra, 'f', 0));
        } else {
            ui->modIaP->setText("N/A");
            ui->modGmP->setText("N/A");
            ui->modRaP->setText("N/A");
        }
=======
        updateTriodeComparison();
    } else {
        updatePentodeComparison();
    }
}

void CompareDialog::updateTriodeComparison()
{
    // Get input values
    bool gridOk = false;
    bool anodeOk = false;
    double gridVoltage = ui->grid->text().toDouble(&gridOk);
    double anodeVoltage = ui->anode->text().toDouble(&anodeOk);
    
    if (gridOk && anodeOk && gridVoltage < 0 && anodeVoltage > 0) {
        double ia = m_model->triodeAnodeCurrent(anodeVoltage, gridVoltage);
        ui->modIa->setText(QString::number(ia, 'f', 2));
        
        // Calculate mu, gm, ra from the model
        double delta = 1.0; // 1V change for calculations
        
        // Calculate mu (amplification factor)
        double ia1 = m_model->triodeAnodeCurrent(anodeVoltage, gridVoltage);
        double ia2 = m_model->triodeAnodeCurrent(anodeVoltage, gridVoltage - delta);
        double vg_effect = (ia2 - ia1) / delta;
        
        double ia3 = m_model->triodeAnodeCurrent(anodeVoltage + delta, gridVoltage);
        double va_effect = (ia3 - ia1) / delta;
        
        double mu = va_effect != 0 ? vg_effect / va_effect : 0;
        double gm = vg_effect * 1000; // Convert to mA/V
        double ra = va_effect != 0 ? 1.0 / va_effect : 0;
        
        // Display the parameters
        ui->modMu->setText(QString::number(mu, 'f', 2));
        ui->modGm->setText(QString::number(gm, 'f', 2));
        ui->modRa->setText(QString::number(ra, 'f', 0));
    } else {
        ui->modIa->setText("N/A");
        ui->modMu->setText("N/A");
        ui->modGm->setText("N/A");
        ui->modRa->setText("N/A");
    }
}

void CompareDialog::updatePentodeComparison()
{
    // Get input values
    bool gridOk = false;
    bool anodeOk = false;
    bool screenOk = false;
    double gridVoltage = ui->gridP->text().toDouble(&gridOk);
    double anodeVoltage = ui->anodeP->text().toDouble(&anodeOk);
    double screenVoltage = ui->screen->text().toDouble(&screenOk);
    
    if (gridOk && anodeOk && screenOk && gridVoltage < 0 && anodeVoltage > 0 && screenVoltage > 0) {
        double ia = m_model->anodeCurrent(anodeVoltage, screenVoltage, gridVoltage);
        double is = m_model->screenCurrent(anodeVoltage, screenVoltage, gridVoltage);
        
        // Display both anode and screen current in the same label
        ui->modIaP->setText(QString::number(ia, 'f', 2) + " / " + QString::number(is, 'f', 2));
        
        // Calculate gm, ra from the model
        double delta = 1.0; // 1V change for calculations
        
        double ia1 = m_model->anodeCurrent(anodeVoltage, screenVoltage, gridVoltage);
        double ia2 = m_model->anodeCurrent(anodeVoltage, screenVoltage, gridVoltage - delta);
        double vg_effect = (ia2 - ia1) / delta;
        
        double ia3 = m_model->anodeCurrent(anodeVoltage + delta, screenVoltage, gridVoltage);
        double va_effect = (ia3 - ia1) / delta;
        
        double gm = vg_effect * 1000; // Convert to mA/V
        double ra = va_effect != 0 ? 1.0 / va_effect : 0;
        
        // Display the parameters
        ui->modGmP->setText(QString::number(gm, 'f', 2));
        ui->modRaP->setText(QString::number(ra, 'f', 0));
    } else {
        ui->modIaP->setText("N/A");
        ui->modGmP->setText("N/A");
        ui->modRaP->setText("N/A");
>>>>>>> Stashed changes
    }
}
