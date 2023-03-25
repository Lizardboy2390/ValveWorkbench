#pragma once

#include <QDialog>

#include "valvemodel/constants.h"

namespace Ui {
class ProjectDialog;
}

class ProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(QWidget *parent = nullptr);
    ~ProjectDialog();

    QString getName();
    int getDeviceType();

private slots:

private:
    Ui::ProjectDialog *ui;
};

