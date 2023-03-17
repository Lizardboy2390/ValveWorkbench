/********************************************************************************
** Form generated from reading UI file 'projectdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROJECTDIALOG_H
#define UI_PROJECTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ProjectDialog
{
public:
    QDialogButtonBox *buttonBox;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *projectName;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QComboBox *deviceType;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QComboBox *pentodeType;
    QCheckBox *secondaryEmission;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *ProjectDialog)
    {
        if (ProjectDialog->objectName().isEmpty())
            ProjectDialog->setObjectName("ProjectDialog");
        ProjectDialog->resize(400, 300);
        buttonBox = new QDialogButtonBox(ProjectDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setGeometry(QRect(30, 240, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        verticalLayoutWidget = new QWidget(ProjectDialog);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(19, 19, 361, 201));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(verticalLayoutWidget);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        projectName = new QLineEdit(verticalLayoutWidget);
        projectName->setObjectName("projectName");
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(projectName->sizePolicy().hasHeightForWidth());
        projectName->setSizePolicy(sizePolicy);
        projectName->setMinimumSize(QSize(150, 0));

        horizontalLayout->addWidget(projectName);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_2 = new QLabel(verticalLayoutWidget);
        label_2->setObjectName("label_2");

        horizontalLayout_2->addWidget(label_2);

        deviceType = new QComboBox(verticalLayoutWidget);
        deviceType->setObjectName("deviceType");
        sizePolicy.setHeightForWidth(deviceType->sizePolicy().hasHeightForWidth());
        deviceType->setSizePolicy(sizePolicy);
        deviceType->setMinimumSize(QSize(150, 0));

        horizontalLayout_2->addWidget(deviceType);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_3 = new QLabel(verticalLayoutWidget);
        label_3->setObjectName("label_3");

        horizontalLayout_3->addWidget(label_3);

        pentodeType = new QComboBox(verticalLayoutWidget);
        pentodeType->setObjectName("pentodeType");
        sizePolicy.setHeightForWidth(pentodeType->sizePolicy().hasHeightForWidth());
        pentodeType->setSizePolicy(sizePolicy);
        pentodeType->setMinimumSize(QSize(150, 0));

        horizontalLayout_3->addWidget(pentodeType);


        verticalLayout->addLayout(horizontalLayout_3);

        secondaryEmission = new QCheckBox(verticalLayoutWidget);
        secondaryEmission->setObjectName("secondaryEmission");

        verticalLayout->addWidget(secondaryEmission);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(ProjectDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, ProjectDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, ProjectDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(ProjectDialog);
    } // setupUi

    void retranslateUi(QDialog *ProjectDialog)
    {
        ProjectDialog->setWindowTitle(QCoreApplication::translate("ProjectDialog", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("ProjectDialog", "Project Name", nullptr));
        label_2->setText(QCoreApplication::translate("ProjectDialog", "Device Type", nullptr));
        label_3->setText(QCoreApplication::translate("ProjectDialog", "Pentode Type", nullptr));
        secondaryEmission->setText(QCoreApplication::translate("ProjectDialog", "With Secondary Emission", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ProjectDialog: public Ui_ProjectDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROJECTDIALOG_H
