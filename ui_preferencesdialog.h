/********************************************************************************
** Form generated from reading UI file 'preferencesdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.5.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PREFERENCESDIALOG_H
#define UI_PREFERENCESDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PreferencesDialog
{
public:
    QDialogButtonBox *buttonBox;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QComboBox *portSelect;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QComboBox *pentodeFit;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QComboBox *sampling;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_avgMode;
    QComboBox *avgMode;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_avgSamples;
    QSpinBox *avgSamples;
    QSpacerItem *horizontalSpacer_5;
    QCheckBox *checkScreenCurrent;
    QCheckBox *checkRemodel;
    QCheckBox *checkSecondary;
    QCheckBox *checkFixTriode;
    QCheckBox *checkFixSecondary;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *PreferencesDialog)
    {
        if (PreferencesDialog->objectName().isEmpty())
            PreferencesDialog->setObjectName("PreferencesDialog");
        PreferencesDialog->resize(445, 508);
        buttonBox = new QDialogButtonBox(PreferencesDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setGeometry(QRect(30, 450, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        verticalLayoutWidget = new QWidget(PreferencesDialog);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(20, 19, 406, 401));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(verticalLayoutWidget);
        label->setObjectName("label");
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setMinimumSize(QSize(140, 0));

        horizontalLayout->addWidget(label);

        portSelect = new QComboBox(verticalLayoutWidget);
        portSelect->setObjectName("portSelect");
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(portSelect->sizePolicy().hasHeightForWidth());
        portSelect->setSizePolicy(sizePolicy1);
        portSelect->setMinimumSize(QSize(120, 0));

        horizontalLayout->addWidget(portSelect);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_2 = new QLabel(verticalLayoutWidget);
        label_2->setObjectName("label_2");
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);
        label_2->setMinimumSize(QSize(140, 0));

        horizontalLayout_2->addWidget(label_2);

        pentodeFit = new QComboBox(verticalLayoutWidget);
        pentodeFit->setObjectName("pentodeFit");
        sizePolicy1.setHeightForWidth(pentodeFit->sizePolicy().hasHeightForWidth());
        pentodeFit->setSizePolicy(sizePolicy1);
        pentodeFit->setMinimumSize(QSize(120, 0));

        horizontalLayout_2->addWidget(pentodeFit);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_3 = new QLabel(verticalLayoutWidget);
        label_3->setObjectName("label_3");
        sizePolicy.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy);
        label_3->setMinimumSize(QSize(140, 0));

        horizontalLayout_3->addWidget(label_3);

        sampling = new QComboBox(verticalLayoutWidget);
        sampling->setObjectName("sampling");
        sizePolicy1.setHeightForWidth(sampling->sizePolicy().hasHeightForWidth());
        sampling->setSizePolicy(sizePolicy1);
        sampling->setMinimumSize(QSize(120, 0));

        horizontalLayout_3->addWidget(sampling);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label_avgMode = new QLabel(verticalLayoutWidget);
        label_avgMode->setObjectName("label_avgMode");
        sizePolicy.setHeightForWidth(label_avgMode->sizePolicy().hasHeightForWidth());
        label_avgMode->setSizePolicy(sizePolicy);
        label_avgMode->setMinimumSize(QSize(140, 0));

        horizontalLayout_4->addWidget(label_avgMode);

        avgMode = new QComboBox(verticalLayoutWidget);
        avgMode->setObjectName("avgMode");
        sizePolicy1.setHeightForWidth(avgMode->sizePolicy().hasHeightForWidth());
        avgMode->setSizePolicy(sizePolicy1);
        avgMode->setMinimumSize(QSize(120, 0));

        horizontalLayout_4->addWidget(avgMode);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_4);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        label_avgSamples = new QLabel(verticalLayoutWidget);
        label_avgSamples->setObjectName("label_avgSamples");
        sizePolicy.setHeightForWidth(label_avgSamples->sizePolicy().hasHeightForWidth());
        label_avgSamples->setSizePolicy(sizePolicy);
        label_avgSamples->setMinimumSize(QSize(140, 0));

        horizontalLayout_5->addWidget(label_avgSamples);

        avgSamples = new QSpinBox(verticalLayoutWidget);
        avgSamples->setObjectName("avgSamples");

        horizontalLayout_5->addWidget(avgSamples);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_5);


        verticalLayout->addLayout(horizontalLayout_5);

        checkScreenCurrent = new QCheckBox(verticalLayoutWidget);
        checkScreenCurrent->setObjectName("checkScreenCurrent");
        checkScreenCurrent->setChecked(true);

        verticalLayout->addWidget(checkScreenCurrent);

        checkRemodel = new QCheckBox(verticalLayoutWidget);
        checkRemodel->setObjectName("checkRemodel");
        checkRemodel->setChecked(false);

        verticalLayout->addWidget(checkRemodel);

        checkSecondary = new QCheckBox(verticalLayoutWidget);
        checkSecondary->setObjectName("checkSecondary");
        checkSecondary->setChecked(true);

        verticalLayout->addWidget(checkSecondary);

        checkFixTriode = new QCheckBox(verticalLayoutWidget);
        checkFixTriode->setObjectName("checkFixTriode");
        checkFixTriode->setChecked(true);

        verticalLayout->addWidget(checkFixTriode);

        checkFixSecondary = new QCheckBox(verticalLayoutWidget);
        checkFixSecondary->setObjectName("checkFixSecondary");
        checkFixSecondary->setChecked(true);

        verticalLayout->addWidget(checkFixSecondary);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(PreferencesDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, PreferencesDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, PreferencesDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(PreferencesDialog);
    } // setupUi

    void retranslateUi(QDialog *PreferencesDialog)
    {
        PreferencesDialog->setWindowTitle(QCoreApplication::translate("PreferencesDialog", "Preferences", nullptr));
        label->setText(QCoreApplication::translate("PreferencesDialog", "Analyser Port:", nullptr));
        label_2->setText(QCoreApplication::translate("PreferencesDialog", "Pentode Fit:", nullptr));
        label_3->setText(QCoreApplication::translate("PreferencesDialog", "Sweep spacing:", nullptr));
        label_avgMode->setText(QCoreApplication::translate("PreferencesDialog", "Averaging mode:", nullptr));
        label_avgSamples->setText(QCoreApplication::translate("PreferencesDialog", "Fixed samples:", nullptr));
        checkScreenCurrent->setText(QCoreApplication::translate("PreferencesDialog", "Show screen current on anode plots", nullptr));
        checkRemodel->setText(QCoreApplication::translate("PreferencesDialog", "Pentode remodel after screen modelling", nullptr));
        checkSecondary->setText(QCoreApplication::translate("PreferencesDialog", "Use secondary emissions in model", nullptr));
        checkFixTriode->setText(QCoreApplication::translate("PreferencesDialog", "Fix triode parameters for pentode modelling", nullptr));
        checkFixSecondary->setText(QCoreApplication::translate("PreferencesDialog", "Fix secondary emission parameters for screen modelling", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PreferencesDialog: public Ui_PreferencesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PREFERENCESDIALOG_H
