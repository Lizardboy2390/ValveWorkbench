#include "simplemanualpentodedialog.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

// Relative include paths within valvemodel
#include "../model/simplemanualpentode.h"

SimpleManualPentodeDialog::SimpleManualPentodeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Simple Manual Pentode Controls"));
    setModal(false);
    setSizeGripEnabled(true);
    createLayout();
}

void SimpleManualPentodeDialog::createLayout()
{
    auto *form = new QFormLayout;

    auto makeSpin = [&](const QString &suffix) {
        auto *spin = new QDoubleSpinBox(this);
        spin->setDecimals(4);
        spin->setSingleStep(0.01);
        spin->setKeyboardTracking(false);
        spin->setSuffix(suffix);
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &SimpleManualPentodeDialog::onParameterChanged);
        return spin;
    };

    muSpin    = makeSpin(" ");
    kpSpin    = makeSpin(" ");
    kg1Spin   = makeSpin(" ");
    kg2Spin   = makeSpin(" ");
    alphaSpin = makeSpin(" ");
    betaSpin  = makeSpin(" ");
    gammaSpin = makeSpin(" ");
    aSpin     = makeSpin(" ");

    // 6L6-oriented default ranges (can be refined later or overridden from presets)
    muSpin->setRange(0.0, 100.0);
    kpSpin->setRange(0.0, 2000.0);
    kg1Spin->setRange(0.0, 5.0);
    kg2Spin->setRange(0.0, 5.0);
    alphaSpin->setRange(0.0, 5.0);
    betaSpin->setRange(0.0, 5.0);
    gammaSpin->setRange(0.0, 10.0);
    aSpin->setRange(-5.0, 5.0);

    form->addRow(tr("mu"),    muSpin);
    form->addRow(tr("kp"),    kpSpin);
    form->addRow(tr("kg1"),   kg1Spin);
    form->addRow(tr("kg2"),   kg2Spin);
    form->addRow(tr("alpha"), alphaSpin);
    form->addRow(tr("beta"),  betaSpin);
    form->addRow(tr("gamma"), gammaSpin);
    form->addRow(tr("a"),     aSpin);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *vbox = new QVBoxLayout;
    vbox->addLayout(form);
    vbox->addWidget(buttons);
    setLayout(vbox);
}

void SimpleManualPentodeDialog::setModel(SimpleManualPentode *m)
{
    model = m;
    syncFromModel();
}

void SimpleManualPentodeDialog::syncFromModel()
{
    if (!model) return;

    updatingFromModel = true;
    muSpin->setValue(model->getParameter(PAR_MU));
    kpSpin->setValue(model->getParameter(PAR_KP));
    kg1Spin->setValue(model->getParameter(PAR_KG1));
    kg2Spin->setValue(model->getParameter(PAR_KG2));
    alphaSpin->setValue(model->getParameter(PAR_ALPHA));
    betaSpin->setValue(model->getParameter(PAR_BETA));
    gammaSpin->setValue(model->getParameter(PAR_GAMMA));
    aSpin->setValue(model->getParameter(PAR_A));
    updatingFromModel = false;
}

void SimpleManualPentodeDialog::applyToModel()
{
    if (!model) return;

    model->getParameterObject(PAR_MU)->setValue(muSpin->value());
    model->getParameterObject(PAR_KP)->setValue(kpSpin->value());
    model->getParameterObject(PAR_KG1)->setValue(kg1Spin->value());
    model->getParameterObject(PAR_KG2)->setValue(kg2Spin->value());
    model->getParameterObject(PAR_ALPHA)->setValue(alphaSpin->value());
    model->getParameterObject(PAR_BETA)->setValue(betaSpin->value());
    model->getParameterObject(PAR_GAMMA)->setValue(gammaSpin->value());
    model->getParameterObject(PAR_A)->setValue(aSpin->value());
}

void SimpleManualPentodeDialog::onParameterChanged()
{
    if (updatingFromModel || !model) return;
    applyToModel();
    emit parametersChanged();
}
