#pragma once

#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QTreeWidgetItem>

#include "parameter.h"

class UIBridge
{
public:
    UIBridge();

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) = 0;

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]) = 0;

    virtual void updateProperties(QTableWidget *properties);

protected:
    void clearProperties(QTableWidget *properties);
    void addProperty(QTableWidget *properties, QString label, QString value, bool editable = false);
    void updateParameter(QLabel *uiLabel, QLineEdit *uiValue, QString label, double value);
    void updateParameter(QLabel *uiLabel, QLineEdit *uiValue, Parameter *parameter);
};
