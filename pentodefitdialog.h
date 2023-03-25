#pragma once

#include <QDialog>

namespace Ui {
class PentodeFitDialog;
}

class PentodeFitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PentodeFitDialog(QWidget *parent = nullptr);
    ~PentodeFitDialog();

    bool isTruePentode();
    bool withSecondaryEmission();

private:
    Ui::PentodeFitDialog *ui;
};

