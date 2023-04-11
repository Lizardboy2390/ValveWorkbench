/********************************************************************************
** Form generated from reading UI file 'comparedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.5.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COMPAREDIALOG_H
#define UI_COMPAREDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
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
    QGroupBox *groupBox;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *grid;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QLineEdit *anode;
    QSpacerItem *horizontalSpacer_2;
    QGroupBox *groupBox_2;
    QWidget *widget1;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLineEdit *gridP;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_4;
    QLineEdit *anodeP;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_5;
    QLineEdit *screen;
    QSpacerItem *horizontalSpacer_5;
    QGroupBox *groupBox_3;
    QWidget *widget2;
    QVBoxLayout *verticalLayout_9;
    QVBoxLayout *verticalLayout_5;
    QLabel *refMu;
    QLabel *refIa;
    QLabel *refGm;
    QLabel *refRa;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout_8;
    QLabel *refIaP;
    QLabel *refGmP;
    QLabel *refRaP;
    QGroupBox *groupBox_4;
    QWidget *widget3;
    QVBoxLayout *verticalLayout_10;
    QVBoxLayout *verticalLayout_4;
    QLabel *modMu;
    QLabel *modIa;
    QLabel *modGm;
    QLabel *modRa;
    QSpacerItem *verticalSpacer_2;
    QVBoxLayout *verticalLayout_7;
    QLabel *modIaP;
    QLabel *modGmP;
    QLabel *modRaP;
    QPushButton *pushButton;
    QWidget *widget4;
    QVBoxLayout *verticalLayout_11;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_6;
    QLabel *label_9;
    QLabel *label_12;
    QLabel *label_15;
    QSpacerItem *verticalSpacer_3;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_18;
    QLabel *label_21;
    QLabel *label_24;

    void setupUi(QDialog *CompareDialog)
    {
        if (CompareDialog->objectName().isEmpty())
            CompareDialog->setObjectName("CompareDialog");
        CompareDialog->resize(400, 630);
        groupBox = new QGroupBox(CompareDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(30, 30, 341, 101));
        widget = new QWidget(groupBox);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(20, 30, 301, 56));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(widget);
        label->setObjectName("label");
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setMinimumSize(QSize(120, 0));

        horizontalLayout->addWidget(label);

        grid = new QLineEdit(widget);
        grid->setObjectName("grid");
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(grid->sizePolicy().hasHeightForWidth());
        grid->setSizePolicy(sizePolicy1);
        grid->setMinimumSize(QSize(80, 0));
        grid->setMaximumSize(QSize(80, 16777215));

        horizontalLayout->addWidget(grid);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_2 = new QLabel(widget);
        label_2->setObjectName("label_2");
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);
        label_2->setMinimumSize(QSize(120, 0));

        horizontalLayout_2->addWidget(label_2);

        anode = new QLineEdit(widget);
        anode->setObjectName("anode");
        sizePolicy1.setHeightForWidth(anode->sizePolicy().hasHeightForWidth());
        anode->setSizePolicy(sizePolicy1);
        anode->setMinimumSize(QSize(80, 0));
        anode->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_2->addWidget(anode);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_2);

        groupBox_2 = new QGroupBox(CompareDialog);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setGeometry(QRect(30, 140, 341, 131));
        widget1 = new QWidget(groupBox_2);
        widget1->setObjectName("widget1");
        widget1->setGeometry(QRect(20, 30, 301, 86));
        verticalLayout_2 = new QVBoxLayout(widget1);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_3 = new QLabel(widget1);
        label_3->setObjectName("label_3");
        sizePolicy.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy);
        label_3->setMinimumSize(QSize(120, 0));

        horizontalLayout_3->addWidget(label_3);

        gridP = new QLineEdit(widget1);
        gridP->setObjectName("gridP");
        sizePolicy1.setHeightForWidth(gridP->sizePolicy().hasHeightForWidth());
        gridP->setSizePolicy(sizePolicy1);
        gridP->setMinimumSize(QSize(80, 0));
        gridP->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_3->addWidget(gridP);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        verticalLayout_2->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label_4 = new QLabel(widget1);
        label_4->setObjectName("label_4");
        sizePolicy.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy);
        label_4->setMinimumSize(QSize(120, 0));

        horizontalLayout_4->addWidget(label_4);

        anodeP = new QLineEdit(widget1);
        anodeP->setObjectName("anodeP");
        sizePolicy1.setHeightForWidth(anodeP->sizePolicy().hasHeightForWidth());
        anodeP->setSizePolicy(sizePolicy1);
        anodeP->setMinimumSize(QSize(80, 0));
        anodeP->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_4->addWidget(anodeP);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_4);


        verticalLayout_2->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        label_5 = new QLabel(widget1);
        label_5->setObjectName("label_5");
        sizePolicy.setHeightForWidth(label_5->sizePolicy().hasHeightForWidth());
        label_5->setSizePolicy(sizePolicy);
        label_5->setMinimumSize(QSize(120, 0));

        horizontalLayout_5->addWidget(label_5);

        screen = new QLineEdit(widget1);
        screen->setObjectName("screen");
        sizePolicy1.setHeightForWidth(screen->sizePolicy().hasHeightForWidth());
        screen->setSizePolicy(sizePolicy1);
        screen->setMinimumSize(QSize(80, 0));
        screen->setMaximumSize(QSize(80, 16777215));

        horizontalLayout_5->addWidget(screen);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_5);


        verticalLayout_2->addLayout(horizontalLayout_5);

        groupBox_3 = new QGroupBox(CompareDialog);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setGeometry(QRect(270, 280, 101, 221));
        widget2 = new QWidget(groupBox_3);
        widget2->setObjectName("widget2");
        widget2->setGeometry(QRect(20, 30, 61, 171));
        verticalLayout_9 = new QVBoxLayout(widget2);
        verticalLayout_9->setObjectName("verticalLayout_9");
        verticalLayout_9->setContentsMargins(0, 0, 0, 0);
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName("verticalLayout_5");
        refMu = new QLabel(widget2);
        refMu->setObjectName("refMu");

        verticalLayout_5->addWidget(refMu);

        refIa = new QLabel(widget2);
        refIa->setObjectName("refIa");

        verticalLayout_5->addWidget(refIa);

        refGm = new QLabel(widget2);
        refGm->setObjectName("refGm");

        verticalLayout_5->addWidget(refGm);

        refRa = new QLabel(widget2);
        refRa->setObjectName("refRa");

        verticalLayout_5->addWidget(refRa);


        verticalLayout_9->addLayout(verticalLayout_5);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_9->addItem(verticalSpacer);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setObjectName("verticalLayout_8");
        refIaP = new QLabel(widget2);
        refIaP->setObjectName("refIaP");

        verticalLayout_8->addWidget(refIaP);

        refGmP = new QLabel(widget2);
        refGmP->setObjectName("refGmP");

        verticalLayout_8->addWidget(refGmP);

        refRaP = new QLabel(widget2);
        refRaP->setObjectName("refRaP");

        verticalLayout_8->addWidget(refRaP);


        verticalLayout_9->addLayout(verticalLayout_8);

        groupBox_4 = new QGroupBox(CompareDialog);
        groupBox_4->setObjectName("groupBox_4");
        groupBox_4->setGeometry(QRect(150, 280, 101, 221));
        widget3 = new QWidget(groupBox_4);
        widget3->setObjectName("widget3");
        widget3->setGeometry(QRect(20, 30, 61, 171));
        verticalLayout_10 = new QVBoxLayout(widget3);
        verticalLayout_10->setObjectName("verticalLayout_10");
        verticalLayout_10->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName("verticalLayout_4");
        modMu = new QLabel(widget3);
        modMu->setObjectName("modMu");

        verticalLayout_4->addWidget(modMu);

        modIa = new QLabel(widget3);
        modIa->setObjectName("modIa");

        verticalLayout_4->addWidget(modIa);

        modGm = new QLabel(widget3);
        modGm->setObjectName("modGm");

        verticalLayout_4->addWidget(modGm);

        modRa = new QLabel(widget3);
        modRa->setObjectName("modRa");

        verticalLayout_4->addWidget(modRa);


        verticalLayout_10->addLayout(verticalLayout_4);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_10->addItem(verticalSpacer_2);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName("verticalLayout_7");
        modIaP = new QLabel(widget3);
        modIaP->setObjectName("modIaP");

        verticalLayout_7->addWidget(modIaP);

        modGmP = new QLabel(widget3);
        modGmP->setObjectName("modGmP");

        verticalLayout_7->addWidget(modGmP);

        modRaP = new QLabel(widget3);
        modRaP->setObjectName("modRaP");

        verticalLayout_7->addWidget(modRaP);


        verticalLayout_10->addLayout(verticalLayout_7);

        pushButton = new QPushButton(CompareDialog);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(290, 570, 75, 24));
        widget4 = new QWidget(CompareDialog);
        widget4->setObjectName("widget4");
        widget4->setGeometry(QRect(50, 310, 81, 171));
        verticalLayout_11 = new QVBoxLayout(widget4);
        verticalLayout_11->setObjectName("verticalLayout_11");
        verticalLayout_11->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName("verticalLayout_3");
        label_6 = new QLabel(widget4);
        label_6->setObjectName("label_6");

        verticalLayout_3->addWidget(label_6);

        label_9 = new QLabel(widget4);
        label_9->setObjectName("label_9");

        verticalLayout_3->addWidget(label_9);

        label_12 = new QLabel(widget4);
        label_12->setObjectName("label_12");

        verticalLayout_3->addWidget(label_12);

        label_15 = new QLabel(widget4);
        label_15->setObjectName("label_15");

        verticalLayout_3->addWidget(label_15);


        verticalLayout_11->addLayout(verticalLayout_3);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_11->addItem(verticalSpacer_3);

        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName("verticalLayout_6");
        label_18 = new QLabel(widget4);
        label_18->setObjectName("label_18");

        verticalLayout_6->addWidget(label_18);

        label_21 = new QLabel(widget4);
        label_21->setObjectName("label_21");

        verticalLayout_6->addWidget(label_21);

        label_24 = new QLabel(widget4);
        label_24->setObjectName("label_24");

        verticalLayout_6->addWidget(label_24);


        verticalLayout_11->addLayout(verticalLayout_6);


        retranslateUi(CompareDialog);

        QMetaObject::connectSlotsByName(CompareDialog);
    } // setupUi

    void retranslateUi(QDialog *CompareDialog)
    {
        CompareDialog->setWindowTitle(QCoreApplication::translate("CompareDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("CompareDialog", "Triode test conditions", nullptr));
        label->setText(QCoreApplication::translate("CompareDialog", "-ve Grid Voltage:", nullptr));
        label_2->setText(QCoreApplication::translate("CompareDialog", "Anode Voltage:", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("CompareDialog", "Pentode test conditions", nullptr));
        label_3->setText(QCoreApplication::translate("CompareDialog", "-ve Grid Voltage:", nullptr));
        label_4->setText(QCoreApplication::translate("CompareDialog", "Anode Voltage:", nullptr));
        label_5->setText(QCoreApplication::translate("CompareDialog", "Screen Voltage:", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("CompareDialog", "Reference", nullptr));
        refMu->setText(QString());
        refIa->setText(QString());
        refGm->setText(QString());
        refRa->setText(QString());
        refIaP->setText(QString());
        refGmP->setText(QString());
        refRaP->setText(QString());
        groupBox_4->setTitle(QCoreApplication::translate("CompareDialog", "Model", nullptr));
        modMu->setText(QString());
        modIa->setText(QString());
        modGm->setText(QString());
        modRa->setText(QString());
        modIaP->setText(QString());
        modGmP->setText(QString());
        modRaP->setText(QString());
        pushButton->setText(QCoreApplication::translate("CompareDialog", "Close", nullptr));
        label_6->setText(QCoreApplication::translate("CompareDialog", "Mu:", nullptr));
        label_9->setText(QCoreApplication::translate("CompareDialog", "Ia:", nullptr));
        label_12->setText(QCoreApplication::translate("CompareDialog", "gm:", nullptr));
        label_15->setText(QCoreApplication::translate("CompareDialog", "Ra:", nullptr));
        label_18->setText(QCoreApplication::translate("CompareDialog", "Ia:", nullptr));
        label_21->setText(QCoreApplication::translate("CompareDialog", "gm:", nullptr));
        label_24->setText(QCoreApplication::translate("CompareDialog", "Ra:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CompareDialog: public Ui_CompareDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_COMPAREDIALOG_H
