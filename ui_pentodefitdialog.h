/********************************************************************************
** Form generated from reading UI file 'pentodefitdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PENTODEFITDIALOG_H
#define UI_PENTODEFITDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PentodeFitDialog
{
public:
    QDialogButtonBox *buttonBox;
    QGroupBox *groupBox;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_2;
    QRadioButton *radioTruePentode;
    QSpacerItem *horizontalSpacer;
    QRadioButton *radioBeamTetrode;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *verticalSpacer_2;
    QCheckBox *checkSecondary;

    void setupUi(QDialog *PentodeFitDialog)
    {
        if (PentodeFitDialog->objectName().isEmpty())
            PentodeFitDialog->setObjectName("PentodeFitDialog");
        PentodeFitDialog->resize(400, 300);
        buttonBox = new QDialogButtonBox(PentodeFitDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setGeometry(QRect(30, 240, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        groupBox = new QGroupBox(PentodeFitDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(20, 10, 361, 61));
        layoutWidget = new QWidget(groupBox);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(3, 10, 351, 51));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        radioTruePentode = new QRadioButton(layoutWidget);
        radioTruePentode->setObjectName("radioTruePentode");

        horizontalLayout->addWidget(radioTruePentode);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        radioBeamTetrode = new QRadioButton(layoutWidget);
        radioBeamTetrode->setObjectName("radioBeamTetrode");

        horizontalLayout->addWidget(radioBeamTetrode);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        checkSecondary = new QCheckBox(PentodeFitDialog);
        checkSecondary->setObjectName("checkSecondary");
        checkSecondary->setGeometry(QRect(20, 100, 191, 20));

        retranslateUi(PentodeFitDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, PentodeFitDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, PentodeFitDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(PentodeFitDialog);
    } // setupUi

    void retranslateUi(QDialog *PentodeFitDialog)
    {
        PentodeFitDialog->setWindowTitle(QCoreApplication::translate("PentodeFitDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PentodeFitDialog", "PentodeType", nullptr));
        radioTruePentode->setText(QCoreApplication::translate("PentodeFitDialog", "True Pentode", nullptr));
        radioBeamTetrode->setText(QCoreApplication::translate("PentodeFitDialog", "Beam Tetrode", nullptr));
        checkSecondary->setText(QCoreApplication::translate("PentodeFitDialog", "Include Secondary Emission", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PentodeFitDialog: public Ui_PentodeFitDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PENTODEFITDIALOG_H
