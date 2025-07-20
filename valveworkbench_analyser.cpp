// Implementation of ValveWorkbench methods related to the Analyser tab
// This file contains methods for model selection and parameter display

#include "valveworkbench.h"
#include "ui_valveworkbench.h"
#include <QDebug>
#include <cmath>

void ValveWorkbench::buildAnalyserModelSelection()
{
    qDebug("========== Building Analyser Model Selection ==========");
    
    // Clear the model selection combo box
    ui->modelSelectionCombo->clear();
    
    // Add default option
    ui->modelSelectionCombo->addItem("Select Model...", -1);
    
    // Get the current device type from the UI
    int uiDeviceType = ui->deviceType->currentData().toInt();
    qDebug("Current UI device type: %d", uiDeviceType);
    
    int modelCount = 0;
    
    // Add devices to the model selection combo box based on their model type
    for (int i = 0; i < devices.size(); i++) {
        Device *device = devices.at(i);
        int modelType = device->getDeviceType();
        QString name = device->getName();
        
        // Only add devices that match the current UI device type
        if ((uiDeviceType == TRIODE || uiDeviceType == DOUBLE_TRIODE) && modelType == MODEL_TRIODE) {
            ui->modelSelectionCombo->addItem(name, i);
            modelCount++;
            qDebug("  Added triode model '%s' to model selection", name.toStdString().c_str());
        } else if (uiDeviceType == PENTODE && modelType == MODEL_PENTODE) {
            ui->modelSelectionCombo->addItem(name, i);
            modelCount++;
            qDebug("  Added pentode model '%s' to model selection", name.toStdString().c_str());
        }
    }
    
    qDebug("Added %d models to model selection combo box", modelCount);
    
    // If no matching models were loaded, disable the combo box
    if (ui->modelSelectionCombo->count() <= 1) { // Only the "Select Model..." item
        ui->modelSelectionCombo->setEnabled(false);
        qDebug("No matching models for selection, disabled combo box");
    } else {
        ui->modelSelectionCombo->setEnabled(true);
    }
    
    // Reset mu and gm display
    ui->muValue->setText("--");
    ui->gmValue->setText("--");
    
    qDebug("modelSelectionCombo has %d items", ui->modelSelectionCombo->count());
    qDebug("========== Analyser Model Selection Built ==========");
}

void ValveWorkbench::updateMuGmDisplay(int modelIndex)
{
    qDebug("Updating mu/gm display for model index %d", modelIndex);
    
    // Reset values if no valid model is selected
    if (modelIndex <= 0) {
        ui->muValue->setText("--");
        ui->gmValue->setText("--");
        return;
    }
    
    // Get the selected device/model
    int deviceIndex = ui->modelSelectionCombo->itemData(modelIndex).toInt();
    if (deviceIndex < 0 || deviceIndex >= devices.size()) {
        qDebug("Invalid device index: %d", deviceIndex);
        ui->muValue->setText("--");
        ui->gmValue->setText("--");
        return;
    }
    
    Device *device = devices.at(deviceIndex);
    if (!device) {
        qDebug("Device is null");
        ui->muValue->setText("--");
        ui->gmValue->setText("--");
        return;
    }
    
    // Calculate mu and gm based on the model type
    double mu = 0.0;
    double gm = 0.0;
    
    if (device->getDeviceType() == MODEL_TRIODE) {
        // For triodes, use the model to calculate mu and gm
        // These values typically depend on the operating point
        // For now, use default operating point values
        double va = 250.0;  // Default anode voltage
        double vg = -2.0;   // Default grid voltage
        
        // Calculate mu and gm using the model
        // mu = -∂Va/∂Vg at constant Ia
        // gm = ∂Ia/∂Vg at constant Va
        
        // Small delta for numerical differentiation
        double delta = 0.1;
        
        // Calculate Ia at the operating point
        double ia1 = device->calculateIa(va, vg);
        
        // Calculate Ia with a small change in grid voltage
        double ia2 = device->calculateIa(va, vg - delta);
        
        // Calculate gm = ∂Ia/∂Vg
        gm = (ia2 - ia1) / delta;
        
        // Find Va2 such that Ia(Va2, vg-delta) = Ia1
        // This is an approximation - in a real implementation, we would use
        // a more sophisticated method to find Va2
        double va2 = va;
        double step = 1.0;
        double ia_target = ia1;
        double ia_current;
        
        // Simple iterative approach to find Va2
        for (int i = 0; i < 20; i++) {
            ia_current = device->calculateIa(va2, vg - delta);
            if (std::abs(ia_current - ia_target) < 0.0001) {
                break;
            }
            if (ia_current < ia_target) {
                va2 -= step;
            } else {
                va2 += step;
            }
            step *= 0.5;
        }
        
        // Calculate mu = -∂Va/∂Vg
        mu = (va2 - va) / delta;
        
    } else if (device->getDeviceType() == MODEL_PENTODE) {
        // For pentodes, the calculation is more complex
        // For now, just use placeholder values
        mu = 0.0;  // Pentodes don't typically have a mu value
        gm = 0.0;  // Placeholder
    }
    
    // Update the UI with the calculated values
    if (mu > 0) {
        ui->muValue->setText(QString::number(mu, 'f', 1));
    } else {
        ui->muValue->setText("--");
    }
    
    if (gm > 0) {
        ui->gmValue->setText(QString::number(gm * 1000.0, 'f', 1) + " mA/V");
    } else {
        ui->gmValue->setText("--");
    }
}

void ValveWorkbench::on_modelSelectionCombo_currentIndexChanged(int index)
{
    qDebug("Model selection changed to index %d", index);
    updateMuGmDisplay(index);
}
