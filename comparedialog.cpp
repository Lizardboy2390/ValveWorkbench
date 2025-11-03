#include "comparedialog.h"
#include "ui_comparedialog.h"

#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVariant>
#include <QVBoxLayout>
#include <QIcon>
#include <QPixmap>

#include <cmath>

Q_DECLARE_METATYPE(Model *)

CompareDialog::CompareDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CompareDialog),
    pendingReferenceModel(nullptr),
    pendingComparisonModel(nullptr)
{
    ui->setupUi(this);

    qRegisterMetaType<Model *>("Model*");

    auto configureLineEdit = [](QLineEdit *edit) {
        edit->setMinimumWidth(90);
        edit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    };
    configureLineEdit(ui->grid);
    configureLineEdit(ui->anode);
    configureLineEdit(ui->gridP);
    configureLineEdit(ui->anodeP);
    configureLineEdit(ui->screen);

    triodeReferenceLabels = {ui->refMu, ui->refIa, ui->refGm, ui->refRa};
    triodeComparisonLabels = {ui->modMu, ui->modIa, ui->modGm, ui->modRa};
    pentodeReferenceLabels = {ui->refIaP, ui->refGmP, ui->refRaP};
    pentodeComparisonLabels = {ui->modIaP, ui->modGmP, ui->modRaP};

    ui->widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto *triodeWrapper = new QVBoxLayout();
    triodeWrapper->setContentsMargins(12, 12, 12, 12);
    triodeWrapper->setSpacing(8);
    triodeWrapper->addWidget(ui->widget);
    ui->groupBox->setLayout(triodeWrapper);

    ui->widget1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto *pentodeWrapper = new QVBoxLayout();
    pentodeWrapper->setContentsMargins(12, 12, 12, 12);
    pentodeWrapper->setSpacing(8);
    pentodeWrapper->addWidget(ui->widget1);
    ui->groupBox_2->setLayout(pentodeWrapper);

    modelSelectionGroup = new QGroupBox(tr("Model selection"), this);
    auto *modelLayout = new QVBoxLayout(modelSelectionGroup);
    modelLayout->setContentsMargins(12, 12, 12, 12);
    modelLayout->setSpacing(8);

    auto *referenceRow = new QHBoxLayout();
    auto *referenceLabel = new QLabel(tr("Reference model:"), modelSelectionGroup);
    referenceModelCombo = new QComboBox(modelSelectionGroup);
    referenceModelCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    referenceRow->addWidget(referenceLabel);
    referenceRow->addWidget(referenceModelCombo);
    referenceRow->addStretch();
    modelLayout->addLayout(referenceRow);

    auto *comparisonRow = new QHBoxLayout();
    auto *comparisonLabel = new QLabel(tr("Comparison model:"), modelSelectionGroup);
    comparisonModelCombo = new QComboBox(modelSelectionGroup);
    comparisonModelCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    comparisonRow->addWidget(comparisonLabel);
    comparisonRow->addWidget(comparisonModelCombo);
    comparisonRow->addStretch();
    modelLayout->addLayout(comparisonRow);

    connect(referenceModelCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit referenceModelChanged(modelForComboIndex(referenceModelCombo, referenceModelCombo->currentIndex()));
        updateMetrics();
    });
    connect(comparisonModelCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit comparisonModelChanged(modelForComboIndex(comparisonModelCombo, comparisonModelCombo->currentIndex()));
        updateMetrics();
    });

    const auto metricInputs = {
        ui->grid,
        ui->anode,
        ui->gridP,
        ui->anodeP,
        ui->screen
    };

    for (QLineEdit *edit : metricInputs) {
        connect(edit, &QLineEdit::editingFinished, this, [this]() {
            updateMetrics();
        });
    }

    connect(ui->pushButton, &QPushButton::clicked, this, &QWidget::close);

    updateMetrics();

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);
    mainLayout->addWidget(modelSelectionGroup);
    mainLayout->addWidget(ui->groupBox);
    mainLayout->addWidget(ui->groupBox_2);

    auto configureValueLabel = [](QLabel *label) {
        if (!label) {
            return;
        }
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setMinimumWidth(90);
    };

    for (QLabel *label : triodeReferenceLabels) {
        configureValueLabel(label);
    }
    for (QLabel *label : triodeComparisonLabels) {
        configureValueLabel(label);
    }
    for (QLabel *label : pentodeReferenceLabels) {
        configureValueLabel(label);
    }
    for (QLabel *label : pentodeComparisonLabels) {
        configureValueLabel(label);
    }

    const std::array<QString, 3> columnTitles = {
        tr("Metric"),
        tr("Reference"),
        tr("Comparison")
    };

    const std::array<QLabel *, 4> triodeMetricLabels = {
        ui->label_6, ui->label_9, ui->label_12, ui->label_15
    };

    const std::array<QLabel *, 3> pentodeMetricLabels = {
        ui->label_18, ui->label_21, ui->label_24
    };

    auto setupMetricsPanel = [&](const QString &title,
                                 const std::array<QLabel *, 4> &metricNames,
                                 const std::array<QLabel *, 4> &referenceValues,
                                 const std::array<QLabel *, 4> &comparisonValues) {
        auto *panel = new QGroupBox(title, this);
        auto *layout = new QGridLayout(panel);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setHorizontalSpacing(12);
        layout->setVerticalSpacing(6);

        for (int col = 0; col < static_cast<int>(columnTitles.size()); ++col) {
            auto *header = new QLabel(columnTitles[col], panel);
            header->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            header->setStyleSheet(QStringLiteral("font-weight: 600;"));
            layout->addWidget(header, 0, col);
        }

        for (int row = 0; row < static_cast<int>(metricNames.size()); ++row) {
            QLabel *nameLabel = metricNames[row];
            QLabel *refValue = referenceValues[row];
            QLabel *cmpValue = comparisonValues[row];

            if (nameLabel) {
                nameLabel->setParent(panel);
                nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                layout->addWidget(nameLabel, row + 1, 0);
            }
            if (refValue) {
                refValue->setParent(panel);
                layout->addWidget(refValue, row + 1, 1);
            }
            if (cmpValue) {
                cmpValue->setParent(panel);
                layout->addWidget(cmpValue, row + 1, 2);
            }
        }

        return panel;
    };

    auto setupPentodePanel = [&](const QString &title,
                                 const std::array<QLabel *, 3> &metricNames,
                                 const std::array<QLabel *, 3> &referenceValues,
                                 const std::array<QLabel *, 3> &comparisonValues) {
        auto *panel = new QGroupBox(title, this);
        auto *layout = new QGridLayout(panel);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setHorizontalSpacing(12);
        layout->setVerticalSpacing(6);

        for (int col = 0; col < static_cast<int>(columnTitles.size()); ++col) {
            auto *header = new QLabel(columnTitles[col], panel);
            header->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            header->setStyleSheet(QStringLiteral("font-weight: 600;"));
            layout->addWidget(header, 0, col);
        }

        for (int row = 0; row < static_cast<int>(metricNames.size()); ++row) {
            QLabel *nameLabel = metricNames[row];
            QLabel *refValue = referenceValues[row];
            QLabel *cmpValue = comparisonValues[row];

            if (nameLabel) {
                nameLabel->setParent(panel);
                nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                layout->addWidget(nameLabel, row + 1, 0);
            }
            if (refValue) {
                refValue->setParent(panel);
                layout->addWidget(refValue, row + 1, 1);
            }
            if (cmpValue) {
                cmpValue->setParent(panel);
                layout->addWidget(cmpValue, row + 1, 2);
            }
        }

        return panel;
    };

    auto *triodePanel = setupMetricsPanel(tr("Triode metrics"), triodeMetricLabels, triodeReferenceLabels, triodeComparisonLabels);
    auto *pentodePanel = setupPentodePanel(tr("Pentode metrics"), pentodeMetricLabels, pentodeReferenceLabels, pentodeComparisonLabels);

    mainLayout->addWidget(triodePanel);
    mainLayout->addWidget(pentodePanel);

    if (ui->widget4) {
        ui->widget4->setParent(nullptr);
        ui->widget4->hide();
        ui->widget4->deleteLater();
    }
    if (ui->groupBox_3) {
        ui->groupBox_3->setParent(nullptr);
        ui->groupBox_3->hide();
        ui->groupBox_3->deleteLater();
    }
    if (ui->groupBox_4) {
        ui->groupBox_4->setParent(nullptr);
        ui->groupBox_4->hide();
        ui->groupBox_4->deleteLater();
    }

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(ui->pushButton);
    mainLayout->addLayout(buttonLayout);
}

CompareDialog::~CompareDialog()
{
    delete ui;
}

void CompareDialog::setModel(Model *model)
{
    pendingReferenceModel = model;
    selectModelInCombo(referenceModelCombo, model);
    emitCurrentSelections();
    updateMetrics();
}

void CompareDialog::setAvailableModels(const QList<Model *> &models)
{
    availableModels = models;
    Model *referenceSelection = pendingReferenceModel ? pendingReferenceModel : modelForComboIndex(referenceModelCombo, referenceModelCombo->currentIndex());
    Model *comparisonSelection = pendingComparisonModel ? pendingComparisonModel : modelForComboIndex(comparisonModelCombo, comparisonModelCombo->currentIndex());

    repopulateCombo(referenceModelCombo, referenceSelection);
    repopulateCombo(comparisonModelCombo, comparisonSelection);

    emitCurrentSelections();
    updateMetrics();
}

void CompareDialog::setComparisonModel(Model *model)
{
    pendingComparisonModel = model;
    selectModelInCombo(comparisonModelCombo, model);
    emitCurrentSelections();
    updateMetrics();
}

void CompareDialog::repopulateCombo(QComboBox *combo, Model *selectedModel)
{
    if (combo == nullptr) {
        return;
    }

    QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem(colorSwatch(QColor()), tr("(none)"), QVariant::fromValue<Model *>(nullptr));

    int selectedIndex = 0;
    for (int i = 0; i < availableModels.size(); ++i) {
        Model *candidate = availableModels.at(i);
        combo->addItem(colorSwatch(candidate->getPlotColor()), displayNameForModel(candidate), QVariant::fromValue<Model *>(candidate));
        if (candidate == selectedModel) {
            selectedIndex = i + 1;
        }
    }

    combo->setCurrentIndex(selectedIndex);

    if (combo == referenceModelCombo) {
        pendingReferenceModel = modelForComboIndex(combo, combo->currentIndex());
    } else if (combo == comparisonModelCombo) {
        pendingComparisonModel = modelForComboIndex(combo, combo->currentIndex());
    }
}

void CompareDialog::selectModelInCombo(QComboBox *combo, Model *model)
{
    if (combo == nullptr || combo->count() == 0) {
        return;
    }

    int foundIndex = -1;
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).value<Model *>() == model) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex >= 0) {
        QSignalBlocker blocker(combo);
        combo->setCurrentIndex(foundIndex);
        if (combo == referenceModelCombo) {
            pendingReferenceModel = model;
        } else if (combo == comparisonModelCombo) {
            pendingComparisonModel = model;
        }
    }
}

Model *CompareDialog::modelForComboIndex(const QComboBox *combo, int comboIndex) const
{
    if (combo == nullptr || comboIndex < 0 || comboIndex >= combo->count()) {
        return nullptr;
    }

    return combo->itemData(comboIndex).value<Model *>();
}

void CompareDialog::emitCurrentSelections()
{
    emit referenceModelChanged(modelForComboIndex(referenceModelCombo, referenceModelCombo->currentIndex()));
    emit comparisonModelChanged(modelForComboIndex(comparisonModelCombo, comparisonModelCombo->currentIndex()));
}

QIcon CompareDialog::colorSwatch(const QColor &color) const
{
    if (!color.isValid()) {
        return QIcon();
    }

    QPixmap swatch(12, 12);
    swatch.fill(color);
    return QIcon(swatch);
}

QString CompareDialog::displayNameForModel(Model *model) const
{
    if (model == nullptr) {
        return tr("(none)");
    }

    const QVariant compareLabel = model->property("compareLabel");
    if (compareLabel.isValid()) {
        const QString labelText = compareLabel.toString();
        if (!labelText.isEmpty()) {
            return labelText;
        }
    }

    return model->getName();
}

void CompareDialog::updateMetrics()
{
    const double triodeVa = valueFromLineEdit(ui->anode);
    const double triodeVg = valueFromLineEdit(ui->grid);
    const double pentodeVa = valueFromLineEdit(ui->anodeP);
    const double pentodeVg = valueFromLineEdit(ui->gridP);
    const double pentodeVs = valueFromLineEdit(ui->screen);

    Model *referenceModel = modelForComboIndex(referenceModelCombo, referenceModelCombo->currentIndex());
    Model *comparisonModel = modelForComboIndex(comparisonModelCombo, comparisonModelCombo->currentIndex());

    const ModelMetrics referenceTriode = referenceModel && !isPentodeModel(referenceModel)
        ? computeTriodeMetrics(referenceModel, triodeVa, triodeVg)
        : ModelMetrics{};
    const ModelMetrics referencePentode = referenceModel && isPentodeModel(referenceModel)
        ? computePentodeMetrics(referenceModel, pentodeVa, pentodeVg, pentodeVs)
        : ModelMetrics{};

    const ModelMetrics comparisonTriode = comparisonModel && !isPentodeModel(comparisonModel)
        ? computeTriodeMetrics(comparisonModel, triodeVa, triodeVg)
        : ModelMetrics{};
    const ModelMetrics comparisonPentode = comparisonModel && isPentodeModel(comparisonModel)
        ? computePentodeMetrics(comparisonModel, pentodeVa, pentodeVg, pentodeVs)
        : ModelMetrics{};

    applyMetricsToLabels(referenceTriode, ui->refMu, ui->refIa, ui->refGm, ui->refRa);
    applyMetricsToLabels(comparisonTriode, ui->modMu, ui->modIa, ui->modGm, ui->modRa);

    applyMetricsToLabels(referencePentode, nullptr, ui->refIaP, ui->refGmP, ui->refRaP);
    applyMetricsToLabels(comparisonPentode, nullptr, ui->modIaP, ui->modGmP, ui->modRaP);
}

CompareDialog::ModelMetrics CompareDialog::computeTriodeMetrics(Model *model, double anodeVoltage, double gridVoltage)
{
    ModelMetrics metrics;
    if (model == nullptr) {
        return metrics;
    }

    const double deltaGrid = 0.01; // V
    const double deltaAnode = 1.0; // V

    const double iaBase = model->anodeCurrent(anodeVoltage, gridVoltage);
    const double iaGrid = model->anodeCurrent(anodeVoltage, gridVoltage + deltaGrid);
    const double iaAnode = model->anodeCurrent(anodeVoltage + deltaAnode, gridVoltage);

    const double gm = (iaGrid - iaBase) / deltaGrid; // mA per volt (if model returns mA)
    const double deltaI = iaAnode - iaBase;

    double rp = std::numeric_limits<double>::infinity();
    if (std::abs(deltaI) > 1e-6) {
        rp = deltaAnode / deltaI; // raw units consistent with model output
    }

    metrics.valid = true;
    metrics.ia = iaBase;
    metrics.gm = gm;
    metrics.rp = rp;
    metrics.mu = std::isfinite(rp) ? gm * rp : std::numeric_limits<double>::infinity();

    return metrics;
}

CompareDialog::ModelMetrics CompareDialog::computePentodeMetrics(Model *model, double anodeVoltage, double gridVoltage, double screenVoltage)
{
    ModelMetrics metrics;
    if (model == nullptr) {
        return metrics;
    }

    const double deltaGrid = 0.01; // V
    const double deltaAnode = 1.0; // V

    const double iaBase = model->anodeCurrent(anodeVoltage, gridVoltage, screenVoltage);
    const double iaGrid = model->anodeCurrent(anodeVoltage, gridVoltage + deltaGrid, screenVoltage);
    const double iaAnode = model->anodeCurrent(anodeVoltage + deltaAnode, gridVoltage, screenVoltage);

    const double gm = (iaGrid - iaBase) / deltaGrid;
    const double deltaI = iaAnode - iaBase;

    double rp = std::numeric_limits<double>::infinity();
    if (std::abs(deltaI) > 1e-6) {
        rp = deltaAnode / deltaI;
    }

    metrics.valid = true;
    metrics.ia = iaBase;
    metrics.gm = gm;
    metrics.rp = rp;
    metrics.mu = std::isfinite(rp) ? gm * rp : std::numeric_limits<double>::infinity();

    return metrics;
}

void CompareDialog::applyMetricsToLabels(const ModelMetrics &metrics, QLabel *muLabel, QLabel *iaLabel, QLabel *gmLabel, QLabel *rpLabel)
{
    const QString dash = QString::fromUtf8("—");

    if (muLabel != nullptr) {
        muLabel->setText(metrics.valid ? formatValue(metrics.mu, QString(), 2) : dash);
    }
    if (iaLabel != nullptr) {
        iaLabel->setText(metrics.valid ? formatValue(metrics.ia, tr("mA"), 2) : dash);
    }
    if (gmLabel != nullptr) {
        gmLabel->setText(metrics.valid ? formatValue(metrics.gm, tr("mA/V"), 3) : dash);
    }
    if (rpLabel != nullptr) {
        rpLabel->setText(metrics.valid ? formatValue(metrics.rp, QString::fromUtf8("kΩ"), 2) : dash);
    }
}

double CompareDialog::valueFromLineEdit(QLineEdit *edit) const
{
    if (edit == nullptr) {
        return 0.0;
    }

    bool ok = false;
    const double value = edit->text().toDouble(&ok);
    return ok ? value : 0.0;
}

QString CompareDialog::formatValue(double value, const QString &unit, int precision) const
{
    if (!std::isfinite(value)) {
        return QString::fromUtf8("∞") + (unit.isEmpty() ? QString() : QStringLiteral(" %1").arg(unit));
    }

    QLocale locale;
    QString text = locale.toString(value, 'f', precision);
    if (unit.isEmpty()) {
        return text;
    }
    return QStringLiteral("%1 %2").arg(text, unit);
}

bool CompareDialog::isPentodeModel(Model *model) const
{
    if (model == nullptr) {
        return false;
    }

    switch (model->getType()) {
    case REEFMAN_DERK_PENTODE:
    case REEFMAN_DERK_E_PENTODE:
    case GARDINER_PENTODE:
        return true;
    default:
        return false;
    }
}
