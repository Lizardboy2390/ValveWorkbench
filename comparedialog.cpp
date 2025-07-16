#include "comparedialog.h"
#include "ui_comparedialog.h"

CompareDialog::CompareDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CompareDialog)
{
    ui->setupUi(this);
}

CompareDialog::~CompareDialog()
{
    delete ui;
}

void CompareDialog::setModel(Model *model)
{
    // Implementation will be added later as needed
    // This stub implementation allows the code to compile
}
