#include "pentodefitdialog.h"
#include "ui_pentodefitdialog.h"

PentodeFitDialog::PentodeFitDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PentodeFitDialog)
{
    ui->setupUi(this);

    ui->radioTruePentode->setChecked(true);
}

PentodeFitDialog::~PentodeFitDialog()
{
    delete ui;
}

bool PentodeFitDialog::isTruePentode()
{
    return ui->radioTruePentode->isChecked();
}

bool PentodeFitDialog::withSecondaryEmission()
{
    return ui->checkSecondary->isChecked();
}
