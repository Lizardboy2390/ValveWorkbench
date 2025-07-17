/********************************************************************************
** Form generated from reading UI file 'comparedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COMPAREDIALOG_H
#define UI_COMPAREDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CompareDialog
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *triodeGroup;
    QGridLayout *gridLayout;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *anode;
    QLabel *label_3;
    QLineEdit *grid;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *refMu;
    QLabel *modelMu;
    QLabel *label_8;
    QLabel *refGm;
    QLabel *modelGm;
    QLabel *label_11;
    QLabel *refRa;
    QLabel *modelRa;
    QLabel *label_14;
    QLabel *refIa;
    QLabel *modelIa;
    QLabel *label_17;
    QLabel *label_18;
    QGroupBox *pentodeGroup;
    QGridLayout *gridLayout_2;
    QLabel *label_19;
    QLabel *label_20;
    QLineEdit *anodeP;
    QLabel *label_21;
    QLineEdit *screen;
    QLabel *label_22;
    QLineEdit *gridP;
    QLabel *label_23;
    QLabel *label_24;
    QLabel *pentodeRefGm;
    QLabel *pentodeModelGm;
    QLabel *label_27;
    QLabel *pentodeRefRa;
    QLabel *pentodeModelRa;
    QLabel *label_30;
    QLabel *pentodeRefIa;
    QLabel *pentodeModelIa;
    QLabel *label_33;
    QLabel *pentodeRefIs;
    QLabel *pentodeModelIs;
    QLabel *label_36;
    QLabel *label_37;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButton;

    void setupUi(QDialog *CompareDialog)
    {
        if (CompareDialog->objectName().isEmpty())
            CompareDialog->setObjectName("CompareDialog");
        CompareDialog->resize(600, 500);
        verticalLayoutWidget = new QWidget(CompareDialog);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(20, 20, 560, 460));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        triodeGroup = new QGroupBox(verticalLayoutWidget);
        triodeGroup->setObjectName("triodeGroup");
        gridLayout = new QGridLayout(triodeGroup);
        gridLayout->setObjectName("gridLayout");
        label = new QLabel(triodeGroup);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_2 = new QLabel(triodeGroup);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        anode = new QLineEdit(triodeGroup);
        anode->setObjectName("anode");

        gridLayout->addWidget(anode, 1, 1, 1, 1);

        label_3 = new QLabel(triodeGroup);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        grid = new QLineEdit(triodeGroup);
        grid->setObjectName("grid");

        gridLayout->addWidget(grid, 2, 1, 1, 1);

        label_4 = new QLabel(triodeGroup);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        label_5 = new QLabel(triodeGroup);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 4, 0, 1, 1);

        refMu = new QLabel(triodeGroup);
        refMu->setObjectName("refMu");

        gridLayout->addWidget(refMu, 4, 1, 1, 1);

        modelMu = new QLabel(triodeGroup);
        modelMu->setObjectName("modelMu");

        gridLayout->addWidget(modelMu, 4, 2, 1, 1);

        label_8 = new QLabel(triodeGroup);
        label_8->setObjectName("label_8");

        gridLayout->addWidget(label_8, 5, 0, 1, 1);

        refGm = new QLabel(triodeGroup);
        refGm->setObjectName("refGm");

        gridLayout->addWidget(refGm, 5, 1, 1, 1);

        modelGm = new QLabel(triodeGroup);
        modelGm->setObjectName("modelGm");

        gridLayout->addWidget(modelGm, 5, 2, 1, 1);

        label_11 = new QLabel(triodeGroup);
        label_11->setObjectName("label_11");

        gridLayout->addWidget(label_11, 6, 0, 1, 1);

        refRa = new QLabel(triodeGroup);
        refRa->setObjectName("refRa");

        gridLayout->addWidget(refRa, 6, 1, 1, 1);

        modelRa = new QLabel(triodeGroup);
        modelRa->setObjectName("modelRa");

        gridLayout->addWidget(modelRa, 6, 2, 1, 1);

        label_14 = new QLabel(triodeGroup);
        label_14->setObjectName("label_14");

        gridLayout->addWidget(label_14, 7, 0, 1, 1);

        refIa = new QLabel(triodeGroup);
        refIa->setObjectName("refIa");

        gridLayout->addWidget(refIa, 7, 1, 1, 1);

        modelIa = new QLabel(triodeGroup);
        modelIa->setObjectName("modelIa");

        gridLayout->addWidget(modelIa, 7, 2, 1, 1);

        label_17 = new QLabel(triodeGroup);
        label_17->setObjectName("label_17");

        gridLayout->addWidget(label_17, 0, 1, 1, 1);

        label_18 = new QLabel(triodeGroup);
        label_18->setObjectName("label_18");

        gridLayout->addWidget(label_18, 0, 2, 1, 1);


        verticalLayout->addWidget(triodeGroup);

        pentodeGroup = new QGroupBox(verticalLayoutWidget);
        pentodeGroup->setObjectName("pentodeGroup");
        gridLayout_2 = new QGridLayout(pentodeGroup);
        gridLayout_2->setObjectName("gridLayout_2");
        label_19 = new QLabel(pentodeGroup);
        label_19->setObjectName("label_19");

        gridLayout_2->addWidget(label_19, 0, 0, 1, 1);

        label_20 = new QLabel(pentodeGroup);
        label_20->setObjectName("label_20");

        gridLayout_2->addWidget(label_20, 1, 0, 1, 1);

        anodeP = new QLineEdit(pentodeGroup);
        anodeP->setObjectName("anodeP");

        gridLayout_2->addWidget(anodeP, 1, 1, 1, 1);

        label_21 = new QLabel(pentodeGroup);
        label_21->setObjectName("label_21");

        gridLayout_2->addWidget(label_21, 2, 0, 1, 1);

        screen = new QLineEdit(pentodeGroup);
        screen->setObjectName("screen");

        gridLayout_2->addWidget(screen, 2, 1, 1, 1);

        label_22 = new QLabel(pentodeGroup);
        label_22->setObjectName("label_22");

        gridLayout_2->addWidget(label_22, 3, 0, 1, 1);

        gridP = new QLineEdit(pentodeGroup);
        gridP->setObjectName("gridP");

        gridLayout_2->addWidget(gridP, 3, 1, 1, 1);

        label_23 = new QLabel(pentodeGroup);
        label_23->setObjectName("label_23");

        gridLayout_2->addWidget(label_23, 4, 0, 1, 1);

        label_24 = new QLabel(pentodeGroup);
        label_24->setObjectName("label_24");

        gridLayout_2->addWidget(label_24, 5, 0, 1, 1);

        pentodeRefGm = new QLabel(pentodeGroup);
        pentodeRefGm->setObjectName("pentodeRefGm");

        gridLayout_2->addWidget(pentodeRefGm, 5, 1, 1, 1);

        pentodeModelGm = new QLabel(pentodeGroup);
        pentodeModelGm->setObjectName("pentodeModelGm");

        gridLayout_2->addWidget(pentodeModelGm, 5, 2, 1, 1);

        label_27 = new QLabel(pentodeGroup);
        label_27->setObjectName("label_27");

        gridLayout_2->addWidget(label_27, 6, 0, 1, 1);

        pentodeRefRa = new QLabel(pentodeGroup);
        pentodeRefRa->setObjectName("pentodeRefRa");

        gridLayout_2->addWidget(pentodeRefRa, 6, 1, 1, 1);

        pentodeModelRa = new QLabel(pentodeGroup);
        pentodeModelRa->setObjectName("pentodeModelRa");

        gridLayout_2->addWidget(pentodeModelRa, 6, 2, 1, 1);

        label_30 = new QLabel(pentodeGroup);
        label_30->setObjectName("label_30");

        gridLayout_2->addWidget(label_30, 7, 0, 1, 1);

        pentodeRefIa = new QLabel(pentodeGroup);
        pentodeRefIa->setObjectName("pentodeRefIa");

        gridLayout_2->addWidget(pentodeRefIa, 7, 1, 1, 1);

        pentodeModelIa = new QLabel(pentodeGroup);
        pentodeModelIa->setObjectName("pentodeModelIa");

        gridLayout_2->addWidget(pentodeModelIa, 7, 2, 1, 1);

        label_33 = new QLabel(pentodeGroup);
        label_33->setObjectName("label_33");

        gridLayout_2->addWidget(label_33, 8, 0, 1, 1);

        pentodeRefIs = new QLabel(pentodeGroup);
        pentodeRefIs->setObjectName("pentodeRefIs");

        gridLayout_2->addWidget(pentodeRefIs, 8, 1, 1, 1);

        pentodeModelIs = new QLabel(pentodeGroup);
        pentodeModelIs->setObjectName("pentodeModelIs");

        gridLayout_2->addWidget(pentodeModelIs, 8, 2, 1, 1);

        label_36 = new QLabel(pentodeGroup);
        label_36->setObjectName("label_36");

        gridLayout_2->addWidget(label_36, 0, 1, 1, 1);

        label_37 = new QLabel(pentodeGroup);
        label_37->setObjectName("label_37");

        gridLayout_2->addWidget(label_37, 0, 2, 1, 1);


        verticalLayout->addWidget(pentodeGroup);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        pushButton = new QPushButton(verticalLayoutWidget);
        pushButton->setObjectName("pushButton");

        horizontalLayout->addWidget(pushButton);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(CompareDialog);

        QMetaObject::connectSlotsByName(CompareDialog);
    } // setupUi

    void retranslateUi(QDialog *CompareDialog)
    {
        CompareDialog->setWindowTitle(QCoreApplication::translate("CompareDialog", "Compare Model", nullptr));
        triodeGroup->setTitle(QCoreApplication::translate("CompareDialog", "Triode Parameters", nullptr));
        label->setText(QCoreApplication::translate("CompareDialog", "Test Conditions:", nullptr));
        label_2->setText(QCoreApplication::translate("CompareDialog", "Anode Voltage (V):", nullptr));
        label_3->setText(QCoreApplication::translate("CompareDialog", "Grid Voltage (V):", nullptr));
        label_4->setText(QCoreApplication::translate("CompareDialog", "Parameters:", nullptr));
        label_5->setText(QCoreApplication::translate("CompareDialog", "\316\274:", nullptr));
        refMu->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        modelMu->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_8->setText(QCoreApplication::translate("CompareDialog", "gm (A/V):", nullptr));
        refGm->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        modelGm->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_11->setText(QCoreApplication::translate("CompareDialog", "ra (\316\251):", nullptr));
        refRa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        modelRa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_14->setText(QCoreApplication::translate("CompareDialog", "Ia (A):", nullptr));
        refIa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        modelIa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_17->setText(QCoreApplication::translate("CompareDialog", "Reference", nullptr));
        label_18->setText(QCoreApplication::translate("CompareDialog", "Model", nullptr));
        pentodeGroup->setTitle(QCoreApplication::translate("CompareDialog", "Pentode Parameters", nullptr));
        label_19->setText(QCoreApplication::translate("CompareDialog", "Test Conditions:", nullptr));
        label_20->setText(QCoreApplication::translate("CompareDialog", "Anode Voltage (V):", nullptr));
        label_21->setText(QCoreApplication::translate("CompareDialog", "Screen Voltage (V):", nullptr));
        label_22->setText(QCoreApplication::translate("CompareDialog", "Grid Voltage (V):", nullptr));
        label_23->setText(QCoreApplication::translate("CompareDialog", "Parameters:", nullptr));
        label_24->setText(QCoreApplication::translate("CompareDialog", "gm (A/V):", nullptr));
        pentodeRefGm->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        pentodeModelGm->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_27->setText(QCoreApplication::translate("CompareDialog", "ra (\316\251):", nullptr));
        pentodeRefRa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        pentodeModelRa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_30->setText(QCoreApplication::translate("CompareDialog", "Ia (A):", nullptr));
        pentodeRefIa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        pentodeModelIa->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_33->setText(QCoreApplication::translate("CompareDialog", "Is (A):", nullptr));
        pentodeRefIs->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        pentodeModelIs->setText(QCoreApplication::translate("CompareDialog", "-", nullptr));
        label_36->setText(QCoreApplication::translate("CompareDialog", "Reference", nullptr));
        label_37->setText(QCoreApplication::translate("CompareDialog", "Model", nullptr));
        pushButton->setText(QCoreApplication::translate("CompareDialog", "Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CompareDialog: public Ui_CompareDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_COMPAREDIALOG_H
