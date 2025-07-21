/********************************************************************************
** Form generated from reading UI file 'preferencesdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
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
<<<<<<< Updated upstream
#include <QtWidgets/QDoubleSpinBox>
=======
>>>>>>> Stashed changes
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
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
    QCheckBox *checkScreenCurrent;
    QCheckBox *checkRemodel;
    QCheckBox *checkSecondary;
    QCheckBox *checkFixTriode;
    QCheckBox *checkFixSecondary;
<<<<<<< Updated upstream
    QLabel *labelCalibration;
    QHBoxLayout *horizontalLayoutHeaterV;
    QLabel *labelHeaterV;
    QDoubleSpinBox *heaterVoltageCalibration;
    QHBoxLayout *horizontalLayoutHeaterI;
    QLabel *labelHeaterI;
    QDoubleSpinBox *heaterCurrentCalibration;
    QHBoxLayout *horizontalLayoutAnodeV;
    QLabel *labelAnodeV;
    QDoubleSpinBox *anodeVoltageCalibration;
    QHBoxLayout *horizontalLayoutAnodeI;
    QLabel *labelAnodeI;
    QDoubleSpinBox *anodeCurrentCalibration;
    QHBoxLayout *horizontalLayoutScreenV;
    QLabel *labelScreenV;
    QDoubleSpinBox *screenVoltageCalibration;
    QHBoxLayout *horizontalLayoutScreenI;
    QLabel *labelScreenI;
    QDoubleSpinBox *screenCurrentCalibration;
    QHBoxLayout *horizontalLayoutGridV;
    QLabel *labelGridV;
    QDoubleSpinBox *gridVoltageCalibration;
    QLabel *labelCalibration1;
    QHBoxLayout *horizontalLayoutHeaterV1;
    QLabel *labelHeaterV1;
    QDoubleSpinBox *heaterVoltageCalibration1;
    QHBoxLayout *horizontalLayoutHeaterI1;
    QLabel *labelHeaterI1;
    QDoubleSpinBox *heaterCurrentCalibration1;
    QHBoxLayout *horizontalLayoutAnodeV1;
    QLabel *labelAnodeV1;
    QDoubleSpinBox *anodeVoltageCalibration1;
    QHBoxLayout *horizontalLayoutAnodeI1;
    QLabel *labelAnodeI1;
    QDoubleSpinBox *anodeCurrentCalibration1;
    QHBoxLayout *horizontalLayoutScreenV1;
    QLabel *labelScreenV1;
    QDoubleSpinBox *screenVoltageCalibration1;
    QHBoxLayout *horizontalLayoutScreenI1;
    QLabel *labelScreenI1;
    QDoubleSpinBox *screenCurrentCalibration1;
    QHBoxLayout *horizontalLayoutGridV1;
    QLabel *labelGridV1;
    QDoubleSpinBox *gridVoltageCalibration1;
    QSpacerItem *verticalSpacer;
=======
>>>>>>> Stashed changes

    void setupUi(QDialog *PreferencesDialog)
    {
        if (PreferencesDialog->objectName().isEmpty())
            PreferencesDialog->setObjectName("PreferencesDialog");
        PreferencesDialog->resize(400, 650);
        buttonBox = new QDialogButtonBox(PreferencesDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setGeometry(QRect(30, 600, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        verticalLayoutWidget = new QWidget(PreferencesDialog);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(20, 19, 361, 571));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(verticalLayoutWidget);
        label->setObjectName("label");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setMinimumSize(QSize(140, 0));

        horizontalLayout->addWidget(label);

        portSelect = new QComboBox(verticalLayoutWidget);
        portSelect->setObjectName("portSelect");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(portSelect->sizePolicy().hasHeightForWidth());
        portSelect->setSizePolicy(sizePolicy1);
        portSelect->setMinimumSize(QSize(120, 0));

        horizontalLayout->addWidget(portSelect);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

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

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

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

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout_3);

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
<<<<<<< Updated upstream
        checkFixSecondary->setChecked(true);

        verticalLayout->addWidget(checkFixSecondary);

        labelCalibration = new QLabel(verticalLayoutWidget);
        labelCalibration->setObjectName("labelCalibration");
        QFont font;
        font.setBold(true);
        labelCalibration->setFont(font);

        verticalLayout->addWidget(labelCalibration);

        horizontalLayoutHeaterV = new QHBoxLayout();
        horizontalLayoutHeaterV->setObjectName("horizontalLayoutHeaterV");
        labelHeaterV = new QLabel(verticalLayoutWidget);
        labelHeaterV->setObjectName("labelHeaterV");
        sizePolicy.setHeightForWidth(labelHeaterV->sizePolicy().hasHeightForWidth());
        labelHeaterV->setSizePolicy(sizePolicy);
        labelHeaterV->setMinimumSize(QSize(140, 0));

        horizontalLayoutHeaterV->addWidget(labelHeaterV);

        heaterVoltageCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        heaterVoltageCalibration->setObjectName("heaterVoltageCalibration");
        sizePolicy1.setHeightForWidth(heaterVoltageCalibration->sizePolicy().hasHeightForWidth());
        heaterVoltageCalibration->setSizePolicy(sizePolicy1);
        heaterVoltageCalibration->setMinimumSize(QSize(120, 0));
        heaterVoltageCalibration->setDecimals(3);
        heaterVoltageCalibration->setMinimum(0.001000000000000);
        heaterVoltageCalibration->setMaximum(10.000000000000000);
        heaterVoltageCalibration->setSingleStep(0.001000000000000);
        heaterVoltageCalibration->setValue(1.000000000000000);

        horizontalLayoutHeaterV->addWidget(heaterVoltageCalibration);


        verticalLayout->addLayout(horizontalLayoutHeaterV);

        horizontalLayoutHeaterI = new QHBoxLayout();
        horizontalLayoutHeaterI->setObjectName("horizontalLayoutHeaterI");
        labelHeaterI = new QLabel(verticalLayoutWidget);
        labelHeaterI->setObjectName("labelHeaterI");
        sizePolicy.setHeightForWidth(labelHeaterI->sizePolicy().hasHeightForWidth());
        labelHeaterI->setSizePolicy(sizePolicy);
        labelHeaterI->setMinimumSize(QSize(140, 0));

        horizontalLayoutHeaterI->addWidget(labelHeaterI);

        heaterCurrentCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        heaterCurrentCalibration->setObjectName("heaterCurrentCalibration");
        sizePolicy1.setHeightForWidth(heaterCurrentCalibration->sizePolicy().hasHeightForWidth());
        heaterCurrentCalibration->setSizePolicy(sizePolicy1);
        heaterCurrentCalibration->setMinimumSize(QSize(120, 0));
        heaterCurrentCalibration->setDecimals(3);
        heaterCurrentCalibration->setMinimum(0.001000000000000);
        heaterCurrentCalibration->setMaximum(10.000000000000000);
        heaterCurrentCalibration->setSingleStep(0.001000000000000);
        heaterCurrentCalibration->setValue(1.000000000000000);

        horizontalLayoutHeaterI->addWidget(heaterCurrentCalibration);


        verticalLayout->addLayout(horizontalLayoutHeaterI);

        horizontalLayoutAnodeV = new QHBoxLayout();
        horizontalLayoutAnodeV->setObjectName("horizontalLayoutAnodeV");
        labelAnodeV = new QLabel(verticalLayoutWidget);
        labelAnodeV->setObjectName("labelAnodeV");
        sizePolicy.setHeightForWidth(labelAnodeV->sizePolicy().hasHeightForWidth());
        labelAnodeV->setSizePolicy(sizePolicy);
        labelAnodeV->setMinimumSize(QSize(140, 0));

        horizontalLayoutAnodeV->addWidget(labelAnodeV);

        anodeVoltageCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        anodeVoltageCalibration->setObjectName("anodeVoltageCalibration");
        sizePolicy1.setHeightForWidth(anodeVoltageCalibration->sizePolicy().hasHeightForWidth());
        anodeVoltageCalibration->setSizePolicy(sizePolicy1);
        anodeVoltageCalibration->setMinimumSize(QSize(120, 0));
        anodeVoltageCalibration->setDecimals(3);
        anodeVoltageCalibration->setMinimum(0.001000000000000);
        anodeVoltageCalibration->setMaximum(10.000000000000000);
        anodeVoltageCalibration->setSingleStep(0.001000000000000);
        anodeVoltageCalibration->setValue(1.000000000000000);

        horizontalLayoutAnodeV->addWidget(anodeVoltageCalibration);


        verticalLayout->addLayout(horizontalLayoutAnodeV);

        horizontalLayoutAnodeI = new QHBoxLayout();
        horizontalLayoutAnodeI->setObjectName("horizontalLayoutAnodeI");
        labelAnodeI = new QLabel(verticalLayoutWidget);
        labelAnodeI->setObjectName("labelAnodeI");
        sizePolicy.setHeightForWidth(labelAnodeI->sizePolicy().hasHeightForWidth());
        labelAnodeI->setSizePolicy(sizePolicy);
        labelAnodeI->setMinimumSize(QSize(140, 0));

        horizontalLayoutAnodeI->addWidget(labelAnodeI);

        anodeCurrentCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        anodeCurrentCalibration->setObjectName("anodeCurrentCalibration");
        sizePolicy1.setHeightForWidth(anodeCurrentCalibration->sizePolicy().hasHeightForWidth());
        anodeCurrentCalibration->setSizePolicy(sizePolicy1);
        anodeCurrentCalibration->setMinimumSize(QSize(120, 0));
        anodeCurrentCalibration->setDecimals(3);
        anodeCurrentCalibration->setMinimum(0.001000000000000);
        anodeCurrentCalibration->setMaximum(10.000000000000000);
        anodeCurrentCalibration->setSingleStep(0.001000000000000);
        anodeCurrentCalibration->setValue(1.000000000000000);

        horizontalLayoutAnodeI->addWidget(anodeCurrentCalibration);


        verticalLayout->addLayout(horizontalLayoutAnodeI);

        horizontalLayoutScreenV = new QHBoxLayout();
        horizontalLayoutScreenV->setObjectName("horizontalLayoutScreenV");
        labelScreenV = new QLabel(verticalLayoutWidget);
        labelScreenV->setObjectName("labelScreenV");
        sizePolicy.setHeightForWidth(labelScreenV->sizePolicy().hasHeightForWidth());
        labelScreenV->setSizePolicy(sizePolicy);
        labelScreenV->setMinimumSize(QSize(140, 0));

        horizontalLayoutScreenV->addWidget(labelScreenV);

        screenVoltageCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        screenVoltageCalibration->setObjectName("screenVoltageCalibration");
        sizePolicy1.setHeightForWidth(screenVoltageCalibration->sizePolicy().hasHeightForWidth());
        screenVoltageCalibration->setSizePolicy(sizePolicy1);
        screenVoltageCalibration->setMinimumSize(QSize(120, 0));
        screenVoltageCalibration->setDecimals(3);
        screenVoltageCalibration->setMinimum(0.001000000000000);
        screenVoltageCalibration->setMaximum(10.000000000000000);
        screenVoltageCalibration->setSingleStep(0.001000000000000);
        screenVoltageCalibration->setValue(1.000000000000000);

        horizontalLayoutScreenV->addWidget(screenVoltageCalibration);


        verticalLayout->addLayout(horizontalLayoutScreenV);

        horizontalLayoutScreenI = new QHBoxLayout();
        horizontalLayoutScreenI->setObjectName("horizontalLayoutScreenI");
        labelScreenI = new QLabel(verticalLayoutWidget);
        labelScreenI->setObjectName("labelScreenI");
        sizePolicy.setHeightForWidth(labelScreenI->sizePolicy().hasHeightForWidth());
        labelScreenI->setSizePolicy(sizePolicy);
        labelScreenI->setMinimumSize(QSize(140, 0));

        horizontalLayoutScreenI->addWidget(labelScreenI);

        screenCurrentCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        screenCurrentCalibration->setObjectName("screenCurrentCalibration");
        sizePolicy1.setHeightForWidth(screenCurrentCalibration->sizePolicy().hasHeightForWidth());
        screenCurrentCalibration->setSizePolicy(sizePolicy1);
        screenCurrentCalibration->setMinimumSize(QSize(120, 0));
        screenCurrentCalibration->setDecimals(3);
        screenCurrentCalibration->setMinimum(0.001000000000000);
        screenCurrentCalibration->setMaximum(10.000000000000000);
        screenCurrentCalibration->setSingleStep(0.001000000000000);
        screenCurrentCalibration->setValue(1.000000000000000);

        horizontalLayoutScreenI->addWidget(screenCurrentCalibration);


        verticalLayout->addLayout(horizontalLayoutScreenI);

        horizontalLayoutGridV = new QHBoxLayout();
        horizontalLayoutGridV->setObjectName("horizontalLayoutGridV");
        labelGridV = new QLabel(verticalLayoutWidget);
        labelGridV->setObjectName("labelGridV");
        sizePolicy.setHeightForWidth(labelGridV->sizePolicy().hasHeightForWidth());
        labelGridV->setSizePolicy(sizePolicy);
        labelGridV->setMinimumSize(QSize(140, 0));

        horizontalLayoutGridV->addWidget(labelGridV);

        gridVoltageCalibration = new QDoubleSpinBox(verticalLayoutWidget);
        gridVoltageCalibration->setObjectName("gridVoltageCalibration");
        sizePolicy1.setHeightForWidth(gridVoltageCalibration->sizePolicy().hasHeightForWidth());
        gridVoltageCalibration->setSizePolicy(sizePolicy1);
        gridVoltageCalibration->setMinimumSize(QSize(120, 0));
        gridVoltageCalibration->setDecimals(3);
        gridVoltageCalibration->setMinimum(0.001000000000000);
        gridVoltageCalibration->setMaximum(10.000000000000000);
        gridVoltageCalibration->setSingleStep(0.001000000000000);
        gridVoltageCalibration->setValue(1.000000000000000);

        horizontalLayoutGridV->addWidget(gridVoltageCalibration);


        verticalLayout->addLayout(horizontalLayoutGridV);

        labelCalibration1 = new QLabel(verticalLayoutWidget);
        labelCalibration1->setObjectName("labelCalibration1");
        labelCalibration1->setFont(font);

        verticalLayout->addWidget(labelCalibration1);

        horizontalLayoutHeaterV1 = new QHBoxLayout();
        horizontalLayoutHeaterV1->setObjectName("horizontalLayoutHeaterV1");
        labelHeaterV1 = new QLabel(verticalLayoutWidget);
        labelHeaterV1->setObjectName("labelHeaterV1");
        sizePolicy.setHeightForWidth(labelHeaterV1->sizePolicy().hasHeightForWidth());
        labelHeaterV1->setSizePolicy(sizePolicy);
        labelHeaterV1->setMinimumSize(QSize(140, 0));

        horizontalLayoutHeaterV1->addWidget(labelHeaterV1);

        heaterVoltageCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        heaterVoltageCalibration1->setObjectName("heaterVoltageCalibration1");
        sizePolicy1.setHeightForWidth(heaterVoltageCalibration1->sizePolicy().hasHeightForWidth());
        heaterVoltageCalibration1->setSizePolicy(sizePolicy1);
        heaterVoltageCalibration1->setMinimumSize(QSize(120, 0));
        heaterVoltageCalibration1->setDecimals(3);
        heaterVoltageCalibration1->setMinimum(0.001000000000000);
        heaterVoltageCalibration1->setMaximum(10.000000000000000);
        heaterVoltageCalibration1->setSingleStep(0.001000000000000);
        heaterVoltageCalibration1->setValue(1.000000000000000);

        horizontalLayoutHeaterV1->addWidget(heaterVoltageCalibration1);


        verticalLayout->addLayout(horizontalLayoutHeaterV1);

        horizontalLayoutHeaterI1 = new QHBoxLayout();
        horizontalLayoutHeaterI1->setObjectName("horizontalLayoutHeaterI1");
        labelHeaterI1 = new QLabel(verticalLayoutWidget);
        labelHeaterI1->setObjectName("labelHeaterI1");
        sizePolicy.setHeightForWidth(labelHeaterI1->sizePolicy().hasHeightForWidth());
        labelHeaterI1->setSizePolicy(sizePolicy);
        labelHeaterI1->setMinimumSize(QSize(140, 0));

        horizontalLayoutHeaterI1->addWidget(labelHeaterI1);

        heaterCurrentCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        heaterCurrentCalibration1->setObjectName("heaterCurrentCalibration1");
        sizePolicy1.setHeightForWidth(heaterCurrentCalibration1->sizePolicy().hasHeightForWidth());
        heaterCurrentCalibration1->setSizePolicy(sizePolicy1);
        heaterCurrentCalibration1->setMinimumSize(QSize(120, 0));
        heaterCurrentCalibration1->setDecimals(3);
        heaterCurrentCalibration1->setMinimum(0.001000000000000);
        heaterCurrentCalibration1->setMaximum(10.000000000000000);
        heaterCurrentCalibration1->setSingleStep(0.001000000000000);
        heaterCurrentCalibration1->setValue(1.000000000000000);

        horizontalLayoutHeaterI1->addWidget(heaterCurrentCalibration1);


        verticalLayout->addLayout(horizontalLayoutHeaterI1);

        horizontalLayoutAnodeV1 = new QHBoxLayout();
        horizontalLayoutAnodeV1->setObjectName("horizontalLayoutAnodeV1");
        labelAnodeV1 = new QLabel(verticalLayoutWidget);
        labelAnodeV1->setObjectName("labelAnodeV1");
        sizePolicy.setHeightForWidth(labelAnodeV1->sizePolicy().hasHeightForWidth());
        labelAnodeV1->setSizePolicy(sizePolicy);
        labelAnodeV1->setMinimumSize(QSize(140, 0));

        horizontalLayoutAnodeV1->addWidget(labelAnodeV1);

        anodeVoltageCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        anodeVoltageCalibration1->setObjectName("anodeVoltageCalibration1");
        sizePolicy1.setHeightForWidth(anodeVoltageCalibration1->sizePolicy().hasHeightForWidth());
        anodeVoltageCalibration1->setSizePolicy(sizePolicy1);
        anodeVoltageCalibration1->setMinimumSize(QSize(120, 0));
        anodeVoltageCalibration1->setDecimals(3);
        anodeVoltageCalibration1->setMinimum(0.001000000000000);
        anodeVoltageCalibration1->setMaximum(10.000000000000000);
        anodeVoltageCalibration1->setSingleStep(0.001000000000000);
        anodeVoltageCalibration1->setValue(1.000000000000000);

        horizontalLayoutAnodeV1->addWidget(anodeVoltageCalibration1);


        verticalLayout->addLayout(horizontalLayoutAnodeV1);

        horizontalLayoutAnodeI1 = new QHBoxLayout();
        horizontalLayoutAnodeI1->setObjectName("horizontalLayoutAnodeI1");
        labelAnodeI1 = new QLabel(verticalLayoutWidget);
        labelAnodeI1->setObjectName("labelAnodeI1");
        sizePolicy.setHeightForWidth(labelAnodeI1->sizePolicy().hasHeightForWidth());
        labelAnodeI1->setSizePolicy(sizePolicy);
        labelAnodeI1->setMinimumSize(QSize(140, 0));

        horizontalLayoutAnodeI1->addWidget(labelAnodeI1);

        anodeCurrentCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        anodeCurrentCalibration1->setObjectName("anodeCurrentCalibration1");
        sizePolicy1.setHeightForWidth(anodeCurrentCalibration1->sizePolicy().hasHeightForWidth());
        anodeCurrentCalibration1->setSizePolicy(sizePolicy1);
        anodeCurrentCalibration1->setMinimumSize(QSize(120, 0));
        anodeCurrentCalibration1->setDecimals(3);
        anodeCurrentCalibration1->setMinimum(0.001000000000000);
        anodeCurrentCalibration1->setMaximum(10.000000000000000);
        anodeCurrentCalibration1->setSingleStep(0.001000000000000);
        anodeCurrentCalibration1->setValue(1.000000000000000);

        horizontalLayoutAnodeI1->addWidget(anodeCurrentCalibration1);


        verticalLayout->addLayout(horizontalLayoutAnodeI1);

        horizontalLayoutScreenV1 = new QHBoxLayout();
        horizontalLayoutScreenV1->setObjectName("horizontalLayoutScreenV1");
        labelScreenV1 = new QLabel(verticalLayoutWidget);
        labelScreenV1->setObjectName("labelScreenV1");
        sizePolicy.setHeightForWidth(labelScreenV1->sizePolicy().hasHeightForWidth());
        labelScreenV1->setSizePolicy(sizePolicy);
        labelScreenV1->setMinimumSize(QSize(140, 0));

        horizontalLayoutScreenV1->addWidget(labelScreenV1);

        screenVoltageCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        screenVoltageCalibration1->setObjectName("screenVoltageCalibration1");
        sizePolicy1.setHeightForWidth(screenVoltageCalibration1->sizePolicy().hasHeightForWidth());
        screenVoltageCalibration1->setSizePolicy(sizePolicy1);
        screenVoltageCalibration1->setMinimumSize(QSize(120, 0));
        screenVoltageCalibration1->setDecimals(3);
        screenVoltageCalibration1->setMinimum(0.001000000000000);
        screenVoltageCalibration1->setMaximum(10.000000000000000);
        screenVoltageCalibration1->setSingleStep(0.001000000000000);
        screenVoltageCalibration1->setValue(1.000000000000000);

        horizontalLayoutScreenV1->addWidget(screenVoltageCalibration1);


        verticalLayout->addLayout(horizontalLayoutScreenV1);

        horizontalLayoutScreenI1 = new QHBoxLayout();
        horizontalLayoutScreenI1->setObjectName("horizontalLayoutScreenI1");
        labelScreenI1 = new QLabel(verticalLayoutWidget);
        labelScreenI1->setObjectName("labelScreenI1");
        sizePolicy.setHeightForWidth(labelScreenI1->sizePolicy().hasHeightForWidth());
        labelScreenI1->setSizePolicy(sizePolicy);
        labelScreenI1->setMinimumSize(QSize(140, 0));

        horizontalLayoutScreenI1->addWidget(labelScreenI1);

        screenCurrentCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        screenCurrentCalibration1->setObjectName("screenCurrentCalibration1");
        sizePolicy1.setHeightForWidth(screenCurrentCalibration1->sizePolicy().hasHeightForWidth());
        screenCurrentCalibration1->setSizePolicy(sizePolicy1);
        screenCurrentCalibration1->setMinimumSize(QSize(120, 0));
        screenCurrentCalibration1->setDecimals(3);
        screenCurrentCalibration1->setMinimum(0.001000000000000);
        screenCurrentCalibration1->setMaximum(10.000000000000000);
        screenCurrentCalibration1->setSingleStep(0.001000000000000);
        screenCurrentCalibration1->setValue(1.000000000000000);

        horizontalLayoutScreenI1->addWidget(screenCurrentCalibration1);


        verticalLayout->addLayout(horizontalLayoutScreenI1);

        horizontalLayoutGridV1 = new QHBoxLayout();
        horizontalLayoutGridV1->setObjectName("horizontalLayoutGridV1");
        labelGridV1 = new QLabel(verticalLayoutWidget);
        labelGridV1->setObjectName("labelGridV1");
        sizePolicy.setHeightForWidth(labelGridV1->sizePolicy().hasHeightForWidth());
        labelGridV1->setSizePolicy(sizePolicy);
        labelGridV1->setMinimumSize(QSize(140, 0));

        horizontalLayoutGridV1->addWidget(labelGridV1);

        gridVoltageCalibration1 = new QDoubleSpinBox(verticalLayoutWidget);
        gridVoltageCalibration1->setObjectName("gridVoltageCalibration1");
        sizePolicy1.setHeightForWidth(gridVoltageCalibration1->sizePolicy().hasHeightForWidth());
        gridVoltageCalibration1->setSizePolicy(sizePolicy1);
        gridVoltageCalibration1->setMinimumSize(QSize(120, 0));
        gridVoltageCalibration1->setDecimals(3);
        gridVoltageCalibration1->setMinimum(0.001000000000000);
        gridVoltageCalibration1->setMaximum(10.000000000000000);
        gridVoltageCalibration1->setSingleStep(0.001000000000000);
        gridVoltageCalibration1->setValue(1.000000000000000);

        horizontalLayoutGridV1->addWidget(gridVoltageCalibration1);


        verticalLayout->addLayout(horizontalLayoutGridV1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

=======

        verticalLayout->addWidget(checkFixSecondary);

>>>>>>> Stashed changes

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
        label_3->setText(QCoreApplication::translate("PreferencesDialog", "Anode sampling:", nullptr));
        checkScreenCurrent->setText(QCoreApplication::translate("PreferencesDialog", "Show screen current on anode plots", nullptr));
        checkRemodel->setText(QCoreApplication::translate("PreferencesDialog", "Use Pentode remodelling step", nullptr));
        checkSecondary->setText(QCoreApplication::translate("PreferencesDialog", "Model secondary emission", nullptr));
        checkFixTriode->setText(QCoreApplication::translate("PreferencesDialog", "Fix triode parameters for pentode modelling", nullptr));
        checkFixSecondary->setText(QCoreApplication::translate("PreferencesDialog", "Fix secondary emission parameters for screen modelling", nullptr));
<<<<<<< Updated upstream
        labelCalibration->setText(QCoreApplication::translate("PreferencesDialog", "Calibration", nullptr));
        labelHeaterV->setText(QCoreApplication::translate("PreferencesDialog", "Heater Voltage:", nullptr));
        labelHeaterI->setText(QCoreApplication::translate("PreferencesDialog", "Heater Current:", nullptr));
        labelAnodeV->setText(QCoreApplication::translate("PreferencesDialog", "Anode Voltage:", nullptr));
        labelAnodeI->setText(QCoreApplication::translate("PreferencesDialog", "Anode Current:", nullptr));
        labelScreenV->setText(QCoreApplication::translate("PreferencesDialog", "Screen Voltage:", nullptr));
        labelScreenI->setText(QCoreApplication::translate("PreferencesDialog", "Screen Current:", nullptr));
        labelGridV->setText(QCoreApplication::translate("PreferencesDialog", "Grid Voltage:", nullptr));
        labelCalibration1->setText(QCoreApplication::translate("PreferencesDialog", "Calibration", nullptr));
        labelHeaterV1->setText(QCoreApplication::translate("PreferencesDialog", "Heater Voltage:", nullptr));
        labelHeaterI1->setText(QCoreApplication::translate("PreferencesDialog", "Heater Current:", nullptr));
        labelAnodeV1->setText(QCoreApplication::translate("PreferencesDialog", "Anode Voltage:", nullptr));
        labelAnodeI1->setText(QCoreApplication::translate("PreferencesDialog", "Anode Current:", nullptr));
        labelScreenV1->setText(QCoreApplication::translate("PreferencesDialog", "Screen Voltage:", nullptr));
        labelScreenI1->setText(QCoreApplication::translate("PreferencesDialog", "Screen Current:", nullptr));
        labelGridV1->setText(QCoreApplication::translate("PreferencesDialog", "Grid Voltage:", nullptr));
=======
>>>>>>> Stashed changes
    } // retranslateUi

};

namespace Ui {
    class PreferencesDialog: public Ui_PreferencesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PREFERENCESDIALOG_H
