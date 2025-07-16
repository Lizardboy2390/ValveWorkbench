#ifndef COMPAREDIALOG_H
#define COMPAREDIALOG_H

#include <QDialog>
#include "valvemodel/model/model.h"

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

private:
    Ui::CompareDialog *ui;
};

#endif // COMPAREDIALOG_H
