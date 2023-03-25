#include "projectdialog.h"
#include "ui_projectdialog.h"

/**
 * @brief ProjectDialog::ProjectDialog
 * @param parent
 */
ProjectDialog::ProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectDialog)
{
    ui->setupUi(this);

    ui->radioTriode->setChecked(true);
}

/**
 * @brief ProjectDialog::~ProjectDialog
 */
ProjectDialog::~ProjectDialog()
{
    delete ui;
}

QString ProjectDialog::getName()
{
    return ui->projectName->text();
}

int ProjectDialog::getDeviceType()
{
    if (ui->radioTriode->isChecked()) {
        return TRIODE;
    }

    return PENTODE;
}
