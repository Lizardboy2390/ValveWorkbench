/********************************************************************************
** Form generated from reading UI file 'valveworkbench.ui'
**
** Created by: Qt User Interface Compiler version 6.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VALVEWORKBENCH_H
#define UI_VALVEWORKBENCH_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ValveWorkbench
{
public:
    QAction *actionLoad_Model;
    QAction *actionExit;
    QAction *actionPrint;
    QAction *actionNew_Project;
    QAction *actionOpen_Project;
    QAction *actionLoad_Measurement;
    QAction *actionSave_Project;
    QAction *actionSave_As;
    QAction *actionClose_Project;
    QAction *actionOptions;
    QAction *actionLoad_Template;
    QAction *actionLoad_Device;
    QWidget *centralwidget;
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_16;
    QLabel *label_4;
    QComboBox *stdDeviceSelection;
    QHBoxLayout *horizontalLayout_17;
    QLabel *label_5;
    QComboBox *stdModelSelection;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_3;
    QComboBox *circuitSelection;
    QHBoxLayout *horizontalLayout_5;
    QLabel *cir1Label;
    QLineEdit *cir1Value;
    QHBoxLayout *horizontalLayout_7;
    QLabel *cir3Label;
    QLineEdit *cir3Value;
    QHBoxLayout *horizontalLayout_6;
    QLabel *cir2Label;
    QLineEdit *cir2Value;
    QHBoxLayout *horizontalLayout_13;
    QLabel *cir7Label;
    QLineEdit *cir7Value;
    QHBoxLayout *horizontalLayout_10;
    QLabel *cir5Label;
    QLineEdit *cir5Value;
    QHBoxLayout *horizontalLayout_12;
    QLabel *cir6Label;
    QLineEdit *cir6Value;
    QHBoxLayout *horizontalLayout_8;
    QLabel *cir4Label;
    QLineEdit *cir4Value;
    QSpacerItem *verticalSpacer;
    QWidget *tab_2;
    QWidget *layoutWidget1;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_2;
    QTreeWidget *projectTree;
    QLabel *label;
    QTableWidget *properties;
    QSpacerItem *verticalSpacer_2;
    QWidget *tab_3;
    QWidget *layoutWidget_2;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_24;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;
    QHBoxLayout *horizontalLayout_25;
    QLabel *label_13;
    QLineEdit *deviceName;
    QHBoxLayout *horizontalLayout_26;
    QLabel *deviceTypeLabel_2;
    QComboBox *deviceType;
    QHBoxLayout *horizontalLayout_27;
    QLabel *label_14;
    QComboBox *testType;
    QSpacerItem *verticalSpacer_8;
    QHBoxLayout *horizontalLayout_28;
    QLabel *heaterLabel_2;
    QLineEdit *heaterVoltage;
    QSpacerItem *horizontalSpacer_11;
    QHBoxLayout *horizontalLayout_29;
    QSpacerItem *horizontalSpacer_16;
    QLabel *label_15;
    QLabel *label_16;
    QLabel *label_17;
    QHBoxLayout *horizontalLayout_30;
    QLabel *anodeLabel;
    QLineEdit *anodeStart;
    QLineEdit *anodeStop;
    QLineEdit *anodeStep;
    QHBoxLayout *gridGroup;
    QLabel *gridLabel;
    QLineEdit *gridStart;
    QLineEdit *gridStop;
    QLineEdit *gridStep;
    QHBoxLayout *screenGroup_2;
    QLabel *screenLabel;
    QLineEdit *screenStart;
    QLineEdit *screenStop;
    QLineEdit *screenStep;
    QVBoxLayout *verticalLayout_7;
    QHBoxLayout *horizontalLayout_31;
    QLabel *label_18;
    QLineEdit *iaMax;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_32;
    QLabel *label_19;
    QLineEdit *pMax;
    QSpacerItem *horizontalSpacer_5;
    QSpacerItem *verticalSpacer_9;
    QHBoxLayout *heaterLayout;
    QPushButton *heaterButton;
    QSpacerItem *horizontalSpacer_13;
    QHBoxLayout *horizontalLayout_33;
    QLabel *heaterVLabel;
    QSpacerItem *horizontalSpacer_14;
    QLCDNumber *heaterVlcd;
    QHBoxLayout *horizontalLayout_34;
    QLabel *heaterILabel;
    QSpacerItem *horizontalSpacer_17;
    QLCDNumber *heaterIlcd;
    QSpacerItem *verticalSpacer_10;
    QHBoxLayout *horizontalLayout_35;
    QPushButton *runButton;
    QSpacerItem *horizontalSpacer_18;
    QProgressBar *progressBar;
    QSpacerItem *verticalSpacer_11;
    QSpacerItem *horizontalSpacer;
    QVBoxLayout *verticalLayout_2;
    QGraphicsView *graphicsView;
    QSpacerItem *verticalSpacer_3;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *ValveWorkbench)
    {
        if (ValveWorkbench->objectName().isEmpty())
            ValveWorkbench->setObjectName(QString::fromUtf8("ValveWorkbench"));
        ValveWorkbench->resize(997, 716);
        actionLoad_Model = new QAction(ValveWorkbench);
        actionLoad_Model->setObjectName(QString::fromUtf8("actionLoad_Model"));
        actionExit = new QAction(ValveWorkbench);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionPrint = new QAction(ValveWorkbench);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        actionNew_Project = new QAction(ValveWorkbench);
        actionNew_Project->setObjectName(QString::fromUtf8("actionNew_Project"));
        actionOpen_Project = new QAction(ValveWorkbench);
        actionOpen_Project->setObjectName(QString::fromUtf8("actionOpen_Project"));
        actionLoad_Measurement = new QAction(ValveWorkbench);
        actionLoad_Measurement->setObjectName(QString::fromUtf8("actionLoad_Measurement"));
        actionSave_Project = new QAction(ValveWorkbench);
        actionSave_Project->setObjectName(QString::fromUtf8("actionSave_Project"));
        actionSave_As = new QAction(ValveWorkbench);
        actionSave_As->setObjectName(QString::fromUtf8("actionSave_As"));
        actionClose_Project = new QAction(ValveWorkbench);
        actionClose_Project->setObjectName(QString::fromUtf8("actionClose_Project"));
        actionOptions = new QAction(ValveWorkbench);
        actionOptions->setObjectName(QString::fromUtf8("actionOptions"));
        actionLoad_Template = new QAction(ValveWorkbench);
        actionLoad_Template->setObjectName(QString::fromUtf8("actionLoad_Template"));
        actionLoad_Device = new QAction(ValveWorkbench);
        actionLoad_Device->setObjectName(QString::fromUtf8("actionLoad_Device"));
        centralwidget = new QWidget(ValveWorkbench);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayoutWidget = new QWidget(centralwidget);
        horizontalLayoutWidget->setObjectName(QString::fromUtf8("horizontalLayoutWidget"));
        horizontalLayoutWidget->setGeometry(QRect(19, 9, 961, 661));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        tabWidget = new QTabWidget(horizontalLayoutWidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy);
        tabWidget->setMinimumSize(QSize(350, 0));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        layoutWidget = new QWidget(tab);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 10, 250, 461));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setObjectName(QString::fromUtf8("horizontalLayout_16"));
        label_4 = new QLabel(layoutWidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        horizontalLayout_16->addWidget(label_4);

        stdDeviceSelection = new QComboBox(layoutWidget);
        stdDeviceSelection->setObjectName(QString::fromUtf8("stdDeviceSelection"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(stdDeviceSelection->sizePolicy().hasHeightForWidth());
        stdDeviceSelection->setSizePolicy(sizePolicy1);
        stdDeviceSelection->setMinimumSize(QSize(160, 0));
        stdDeviceSelection->setMaximumSize(QSize(160, 16777215));

        horizontalLayout_16->addWidget(stdDeviceSelection);


        verticalLayout->addLayout(horizontalLayout_16);

        horizontalLayout_17 = new QHBoxLayout();
        horizontalLayout_17->setObjectName(QString::fromUtf8("horizontalLayout_17"));
        label_5 = new QLabel(layoutWidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        horizontalLayout_17->addWidget(label_5);

        stdModelSelection = new QComboBox(layoutWidget);
        stdModelSelection->setObjectName(QString::fromUtf8("stdModelSelection"));
        sizePolicy1.setHeightForWidth(stdModelSelection->sizePolicy().hasHeightForWidth());
        stdModelSelection->setSizePolicy(sizePolicy1);
        stdModelSelection->setMinimumSize(QSize(160, 0));
        stdModelSelection->setMaximumSize(QSize(160, 16777215));

        horizontalLayout_17->addWidget(stdModelSelection);


        verticalLayout->addLayout(horizontalLayout_17);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_3 = new QLabel(layoutWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy2);
        label_3->setMinimumSize(QSize(0, 0));
        label_3->setMaximumSize(QSize(16777215, 16777215));

        horizontalLayout_4->addWidget(label_3);

        circuitSelection = new QComboBox(layoutWidget);
        circuitSelection->setObjectName(QString::fromUtf8("circuitSelection"));
        sizePolicy1.setHeightForWidth(circuitSelection->sizePolicy().hasHeightForWidth());
        circuitSelection->setSizePolicy(sizePolicy1);
        circuitSelection->setMinimumSize(QSize(160, 0));
        circuitSelection->setMaximumSize(QSize(160, 16777215));

        horizontalLayout_4->addWidget(circuitSelection);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        cir1Label = new QLabel(layoutWidget);
        cir1Label->setObjectName(QString::fromUtf8("cir1Label"));

        horizontalLayout_5->addWidget(cir1Label);

        cir1Value = new QLineEdit(layoutWidget);
        cir1Value->setObjectName(QString::fromUtf8("cir1Value"));
        sizePolicy1.setHeightForWidth(cir1Value->sizePolicy().hasHeightForWidth());
        cir1Value->setSizePolicy(sizePolicy1);
        cir1Value->setMinimumSize(QSize(60, 0));
        cir1Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_5->addWidget(cir1Value);


        verticalLayout->addLayout(horizontalLayout_5);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        cir3Label = new QLabel(layoutWidget);
        cir3Label->setObjectName(QString::fromUtf8("cir3Label"));

        horizontalLayout_7->addWidget(cir3Label);

        cir3Value = new QLineEdit(layoutWidget);
        cir3Value->setObjectName(QString::fromUtf8("cir3Value"));
        sizePolicy1.setHeightForWidth(cir3Value->sizePolicy().hasHeightForWidth());
        cir3Value->setSizePolicy(sizePolicy1);
        cir3Value->setMinimumSize(QSize(60, 0));
        cir3Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_7->addWidget(cir3Value);


        verticalLayout->addLayout(horizontalLayout_7);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        cir2Label = new QLabel(layoutWidget);
        cir2Label->setObjectName(QString::fromUtf8("cir2Label"));

        horizontalLayout_6->addWidget(cir2Label);

        cir2Value = new QLineEdit(layoutWidget);
        cir2Value->setObjectName(QString::fromUtf8("cir2Value"));
        sizePolicy1.setHeightForWidth(cir2Value->sizePolicy().hasHeightForWidth());
        cir2Value->setSizePolicy(sizePolicy1);
        cir2Value->setMinimumSize(QSize(60, 0));
        cir2Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_6->addWidget(cir2Value);


        verticalLayout->addLayout(horizontalLayout_6);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        cir7Label = new QLabel(layoutWidget);
        cir7Label->setObjectName(QString::fromUtf8("cir7Label"));

        horizontalLayout_13->addWidget(cir7Label);

        cir7Value = new QLineEdit(layoutWidget);
        cir7Value->setObjectName(QString::fromUtf8("cir7Value"));
        sizePolicy1.setHeightForWidth(cir7Value->sizePolicy().hasHeightForWidth());
        cir7Value->setSizePolicy(sizePolicy1);
        cir7Value->setMinimumSize(QSize(60, 0));
        cir7Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_13->addWidget(cir7Value);


        verticalLayout->addLayout(horizontalLayout_13);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        cir5Label = new QLabel(layoutWidget);
        cir5Label->setObjectName(QString::fromUtf8("cir5Label"));

        horizontalLayout_10->addWidget(cir5Label);

        cir5Value = new QLineEdit(layoutWidget);
        cir5Value->setObjectName(QString::fromUtf8("cir5Value"));
        sizePolicy1.setHeightForWidth(cir5Value->sizePolicy().hasHeightForWidth());
        cir5Value->setSizePolicy(sizePolicy1);
        cir5Value->setMinimumSize(QSize(60, 0));
        cir5Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_10->addWidget(cir5Value);


        verticalLayout->addLayout(horizontalLayout_10);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        cir6Label = new QLabel(layoutWidget);
        cir6Label->setObjectName(QString::fromUtf8("cir6Label"));

        horizontalLayout_12->addWidget(cir6Label);

        cir6Value = new QLineEdit(layoutWidget);
        cir6Value->setObjectName(QString::fromUtf8("cir6Value"));
        sizePolicy1.setHeightForWidth(cir6Value->sizePolicy().hasHeightForWidth());
        cir6Value->setSizePolicy(sizePolicy1);
        cir6Value->setMinimumSize(QSize(60, 0));
        cir6Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_12->addWidget(cir6Value);


        verticalLayout->addLayout(horizontalLayout_12);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        cir4Label = new QLabel(layoutWidget);
        cir4Label->setObjectName(QString::fromUtf8("cir4Label"));

        horizontalLayout_8->addWidget(cir4Label);

        cir4Value = new QLineEdit(layoutWidget);
        cir4Value->setObjectName(QString::fromUtf8("cir4Value"));
        sizePolicy1.setHeightForWidth(cir4Value->sizePolicy().hasHeightForWidth());
        cir4Value->setSizePolicy(sizePolicy1);
        cir4Value->setMinimumSize(QSize(60, 0));
        cir4Value->setMaximumSize(QSize(60, 16777215));

        horizontalLayout_8->addWidget(cir4Value);


        verticalLayout->addLayout(horizontalLayout_8);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        layoutWidget1 = new QWidget(tab_2);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(10, 10, 258, 598));
        verticalLayout_4 = new QVBoxLayout(layoutWidget1);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(layoutWidget1);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        verticalLayout_4->addWidget(label_2);

        projectTree = new QTreeWidget(layoutWidget1);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        projectTree->setHeaderItem(__qtreewidgetitem);
        projectTree->setObjectName(QString::fromUtf8("projectTree"));
        projectTree->setMinimumSize(QSize(0, 300));
        projectTree->setHeaderHidden(true);

        verticalLayout_4->addWidget(projectTree);

        label = new QLabel(layoutWidget1);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout_4->addWidget(label);

        properties = new QTableWidget(layoutWidget1);
        if (properties->columnCount() < 2)
            properties->setColumnCount(2);
        if (properties->rowCount() < 15)
            properties->setRowCount(15);
        properties->setObjectName(QString::fromUtf8("properties"));
        properties->setMinimumSize(QSize(0, 200));
        properties->setRowCount(15);
        properties->setColumnCount(2);
        properties->horizontalHeader()->setVisible(false);
        properties->horizontalHeader()->setDefaultSectionSize(118);
        properties->horizontalHeader()->setHighlightSections(false);
        properties->horizontalHeader()->setStretchLastSection(true);
        properties->verticalHeader()->setVisible(false);
        properties->verticalHeader()->setHighlightSections(false);

        verticalLayout_4->addWidget(properties);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_2);

        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        layoutWidget_2 = new QWidget(tab_3);
        layoutWidget_2->setObjectName(QString::fromUtf8("layoutWidget_2"));
        layoutWidget_2->setGeometry(QRect(10, 10, 300, 608));
        verticalLayout_6 = new QVBoxLayout(layoutWidget_2);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_24 = new QHBoxLayout();
        horizontalLayout_24->setObjectName(QString::fromUtf8("horizontalLayout_24"));
        pushButton_3 = new QPushButton(layoutWidget_2);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));

        horizontalLayout_24->addWidget(pushButton_3);

        pushButton_4 = new QPushButton(layoutWidget_2);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));

        horizontalLayout_24->addWidget(pushButton_4);


        verticalLayout_6->addLayout(horizontalLayout_24);

        horizontalLayout_25 = new QHBoxLayout();
        horizontalLayout_25->setObjectName(QString::fromUtf8("horizontalLayout_25"));
        label_13 = new QLabel(layoutWidget_2);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        horizontalLayout_25->addWidget(label_13);

        deviceName = new QLineEdit(layoutWidget_2);
        deviceName->setObjectName(QString::fromUtf8("deviceName"));
        deviceName->setMinimumSize(QSize(160, 0));
        deviceName->setMaximumSize(QSize(160, 16777215));

        horizontalLayout_25->addWidget(deviceName);


        verticalLayout_6->addLayout(horizontalLayout_25);

        horizontalLayout_26 = new QHBoxLayout();
        horizontalLayout_26->setObjectName(QString::fromUtf8("horizontalLayout_26"));
        deviceTypeLabel_2 = new QLabel(layoutWidget_2);
        deviceTypeLabel_2->setObjectName(QString::fromUtf8("deviceTypeLabel_2"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(deviceTypeLabel_2->sizePolicy().hasHeightForWidth());
        deviceTypeLabel_2->setSizePolicy(sizePolicy3);
        deviceTypeLabel_2->setMinimumSize(QSize(100, 0));

        horizontalLayout_26->addWidget(deviceTypeLabel_2);

        deviceType = new QComboBox(layoutWidget_2);
        deviceType->setObjectName(QString::fromUtf8("deviceType"));
        sizePolicy1.setHeightForWidth(deviceType->sizePolicy().hasHeightForWidth());
        deviceType->setSizePolicy(sizePolicy1);
        deviceType->setMinimumSize(QSize(160, 0));
        deviceType->setMaximumSize(QSize(160, 16777215));

        horizontalLayout_26->addWidget(deviceType);


        verticalLayout_6->addLayout(horizontalLayout_26);

        horizontalLayout_27 = new QHBoxLayout();
        horizontalLayout_27->setObjectName(QString::fromUtf8("horizontalLayout_27"));
        label_14 = new QLabel(layoutWidget_2);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        horizontalLayout_27->addWidget(label_14);

        testType = new QComboBox(layoutWidget_2);
        testType->setObjectName(QString::fromUtf8("testType"));
        sizePolicy1.setHeightForWidth(testType->sizePolicy().hasHeightForWidth());
        testType->setSizePolicy(sizePolicy1);
        testType->setMinimumSize(QSize(160, 0));
        testType->setMaximumSize(QSize(160, 16777215));

        horizontalLayout_27->addWidget(testType);


        verticalLayout_6->addLayout(horizontalLayout_27);

        verticalSpacer_8 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_6->addItem(verticalSpacer_8);

        horizontalLayout_28 = new QHBoxLayout();
        horizontalLayout_28->setObjectName(QString::fromUtf8("horizontalLayout_28"));
        heaterLabel_2 = new QLabel(layoutWidget_2);
        heaterLabel_2->setObjectName(QString::fromUtf8("heaterLabel_2"));
        sizePolicy3.setHeightForWidth(heaterLabel_2->sizePolicy().hasHeightForWidth());
        heaterLabel_2->setSizePolicy(sizePolicy3);
        heaterLabel_2->setMinimumSize(QSize(100, 0));

        horizontalLayout_28->addWidget(heaterLabel_2);

        heaterVoltage = new QLineEdit(layoutWidget_2);
        heaterVoltage->setObjectName(QString::fromUtf8("heaterVoltage"));
        sizePolicy1.setHeightForWidth(heaterVoltage->sizePolicy().hasHeightForWidth());
        heaterVoltage->setSizePolicy(sizePolicy1);
        heaterVoltage->setMinimumSize(QSize(50, 0));
        heaterVoltage->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_28->addWidget(heaterVoltage);

        horizontalSpacer_11 = new QSpacerItem(112, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_28->addItem(horizontalSpacer_11);


        verticalLayout_6->addLayout(horizontalLayout_28);

        horizontalLayout_29 = new QHBoxLayout();
        horizontalLayout_29->setObjectName(QString::fromUtf8("horizontalLayout_29"));
        horizontalSpacer_16 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_29->addItem(horizontalSpacer_16);

        label_15 = new QLabel(layoutWidget_2);
        label_15->setObjectName(QString::fromUtf8("label_15"));
        sizePolicy1.setHeightForWidth(label_15->sizePolicy().hasHeightForWidth());
        label_15->setSizePolicy(sizePolicy1);
        label_15->setMinimumSize(QSize(50, 20));
        label_15->setMaximumSize(QSize(50, 20));
        label_15->setAlignment(Qt::AlignCenter);

        horizontalLayout_29->addWidget(label_15);

        label_16 = new QLabel(layoutWidget_2);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(label_16->sizePolicy().hasHeightForWidth());
        label_16->setSizePolicy(sizePolicy4);
        label_16->setMinimumSize(QSize(50, 0));
        label_16->setMaximumSize(QSize(50, 16777215));
        label_16->setAlignment(Qt::AlignCenter);

        horizontalLayout_29->addWidget(label_16);

        label_17 = new QLabel(layoutWidget_2);
        label_17->setObjectName(QString::fromUtf8("label_17"));
        sizePolicy4.setHeightForWidth(label_17->sizePolicy().hasHeightForWidth());
        label_17->setSizePolicy(sizePolicy4);
        label_17->setMinimumSize(QSize(50, 0));
        label_17->setMaximumSize(QSize(50, 16777215));
        label_17->setAlignment(Qt::AlignCenter);

        horizontalLayout_29->addWidget(label_17);


        verticalLayout_6->addLayout(horizontalLayout_29);

        horizontalLayout_30 = new QHBoxLayout();
        horizontalLayout_30->setObjectName(QString::fromUtf8("horizontalLayout_30"));
        anodeLabel = new QLabel(layoutWidget_2);
        anodeLabel->setObjectName(QString::fromUtf8("anodeLabel"));
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy5.setHorizontalStretch(100);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(anodeLabel->sizePolicy().hasHeightForWidth());
        anodeLabel->setSizePolicy(sizePolicy5);
        anodeLabel->setMinimumSize(QSize(100, 0));

        horizontalLayout_30->addWidget(anodeLabel);

        anodeStart = new QLineEdit(layoutWidget_2);
        anodeStart->setObjectName(QString::fromUtf8("anodeStart"));
        sizePolicy1.setHeightForWidth(anodeStart->sizePolicy().hasHeightForWidth());
        anodeStart->setSizePolicy(sizePolicy1);
        anodeStart->setMinimumSize(QSize(50, 0));
        anodeStart->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_30->addWidget(anodeStart);

        anodeStop = new QLineEdit(layoutWidget_2);
        anodeStop->setObjectName(QString::fromUtf8("anodeStop"));
        sizePolicy1.setHeightForWidth(anodeStop->sizePolicy().hasHeightForWidth());
        anodeStop->setSizePolicy(sizePolicy1);
        anodeStop->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_30->addWidget(anodeStop);

        anodeStep = new QLineEdit(layoutWidget_2);
        anodeStep->setObjectName(QString::fromUtf8("anodeStep"));
        sizePolicy1.setHeightForWidth(anodeStep->sizePolicy().hasHeightForWidth());
        anodeStep->setSizePolicy(sizePolicy1);
        anodeStep->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_30->addWidget(anodeStep);


        verticalLayout_6->addLayout(horizontalLayout_30);

        gridGroup = new QHBoxLayout();
        gridGroup->setObjectName(QString::fromUtf8("gridGroup"));
        gridLabel = new QLabel(layoutWidget_2);
        gridLabel->setObjectName(QString::fromUtf8("gridLabel"));
        sizePolicy3.setHeightForWidth(gridLabel->sizePolicy().hasHeightForWidth());
        gridLabel->setSizePolicy(sizePolicy3);
        gridLabel->setMinimumSize(QSize(100, 0));

        gridGroup->addWidget(gridLabel);

        gridStart = new QLineEdit(layoutWidget_2);
        gridStart->setObjectName(QString::fromUtf8("gridStart"));
        sizePolicy1.setHeightForWidth(gridStart->sizePolicy().hasHeightForWidth());
        gridStart->setSizePolicy(sizePolicy1);
        gridStart->setMinimumSize(QSize(50, 0));
        gridStart->setMaximumSize(QSize(50, 16777215));

        gridGroup->addWidget(gridStart);

        gridStop = new QLineEdit(layoutWidget_2);
        gridStop->setObjectName(QString::fromUtf8("gridStop"));
        sizePolicy1.setHeightForWidth(gridStop->sizePolicy().hasHeightForWidth());
        gridStop->setSizePolicy(sizePolicy1);
        gridStop->setMinimumSize(QSize(50, 0));
        gridStop->setMaximumSize(QSize(50, 16777215));

        gridGroup->addWidget(gridStop);

        gridStep = new QLineEdit(layoutWidget_2);
        gridStep->setObjectName(QString::fromUtf8("gridStep"));
        sizePolicy1.setHeightForWidth(gridStep->sizePolicy().hasHeightForWidth());
        gridStep->setSizePolicy(sizePolicy1);
        gridStep->setMinimumSize(QSize(50, 0));
        gridStep->setMaximumSize(QSize(50, 16777215));

        gridGroup->addWidget(gridStep);


        verticalLayout_6->addLayout(gridGroup);

        screenGroup_2 = new QHBoxLayout();
        screenGroup_2->setObjectName(QString::fromUtf8("screenGroup_2"));
        screenLabel = new QLabel(layoutWidget_2);
        screenLabel->setObjectName(QString::fromUtf8("screenLabel"));
        sizePolicy3.setHeightForWidth(screenLabel->sizePolicy().hasHeightForWidth());
        screenLabel->setSizePolicy(sizePolicy3);
        screenLabel->setMinimumSize(QSize(100, 0));

        screenGroup_2->addWidget(screenLabel);

        screenStart = new QLineEdit(layoutWidget_2);
        screenStart->setObjectName(QString::fromUtf8("screenStart"));
        sizePolicy1.setHeightForWidth(screenStart->sizePolicy().hasHeightForWidth());
        screenStart->setSizePolicy(sizePolicy1);
        screenStart->setMinimumSize(QSize(50, 0));
        screenStart->setMaximumSize(QSize(50, 16777215));

        screenGroup_2->addWidget(screenStart);

        screenStop = new QLineEdit(layoutWidget_2);
        screenStop->setObjectName(QString::fromUtf8("screenStop"));
        sizePolicy1.setHeightForWidth(screenStop->sizePolicy().hasHeightForWidth());
        screenStop->setSizePolicy(sizePolicy1);
        screenStop->setMinimumSize(QSize(50, 0));
        screenStop->setMaximumSize(QSize(50, 16777215));

        screenGroup_2->addWidget(screenStop);

        screenStep = new QLineEdit(layoutWidget_2);
        screenStep->setObjectName(QString::fromUtf8("screenStep"));
        sizePolicy1.setHeightForWidth(screenStep->sizePolicy().hasHeightForWidth());
        screenStep->setSizePolicy(sizePolicy1);
        screenStep->setMinimumSize(QSize(50, 0));
        screenStep->setMaximumSize(QSize(50, 16777215));

        screenGroup_2->addWidget(screenStep);


        verticalLayout_6->addLayout(screenGroup_2);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        horizontalLayout_31 = new QHBoxLayout();
        horizontalLayout_31->setObjectName(QString::fromUtf8("horizontalLayout_31"));
        label_18 = new QLabel(layoutWidget_2);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        sizePolicy3.setHeightForWidth(label_18->sizePolicy().hasHeightForWidth());
        label_18->setSizePolicy(sizePolicy3);
        label_18->setMinimumSize(QSize(120, 0));
        label_18->setMaximumSize(QSize(16777215, 16777215));

        horizontalLayout_31->addWidget(label_18);

        iaMax = new QLineEdit(layoutWidget_2);
        iaMax->setObjectName(QString::fromUtf8("iaMax"));
        sizePolicy1.setHeightForWidth(iaMax->sizePolicy().hasHeightForWidth());
        iaMax->setSizePolicy(sizePolicy1);
        iaMax->setMinimumSize(QSize(50, 0));
        iaMax->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_31->addWidget(iaMax);

        horizontalSpacer_4 = new QSpacerItem(112, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_31->addItem(horizontalSpacer_4);


        verticalLayout_7->addLayout(horizontalLayout_31);

        horizontalLayout_32 = new QHBoxLayout();
        horizontalLayout_32->setObjectName(QString::fromUtf8("horizontalLayout_32"));
        label_19 = new QLabel(layoutWidget_2);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        sizePolicy3.setHeightForWidth(label_19->sizePolicy().hasHeightForWidth());
        label_19->setSizePolicy(sizePolicy3);
        label_19->setMinimumSize(QSize(120, 0));
        label_19->setMaximumSize(QSize(16777215, 16777215));

        horizontalLayout_32->addWidget(label_19);

        pMax = new QLineEdit(layoutWidget_2);
        pMax->setObjectName(QString::fromUtf8("pMax"));
        sizePolicy1.setHeightForWidth(pMax->sizePolicy().hasHeightForWidth());
        pMax->setSizePolicy(sizePolicy1);
        pMax->setMinimumSize(QSize(50, 0));
        pMax->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_32->addWidget(pMax);

        horizontalSpacer_5 = new QSpacerItem(112, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_32->addItem(horizontalSpacer_5);


        verticalLayout_7->addLayout(horizontalLayout_32);


        verticalLayout_6->addLayout(verticalLayout_7);

        verticalSpacer_9 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_6->addItem(verticalSpacer_9);

        heaterLayout = new QHBoxLayout();
        heaterLayout->setObjectName(QString::fromUtf8("heaterLayout"));
        heaterButton = new QPushButton(layoutWidget_2);
        heaterButton->setObjectName(QString::fromUtf8("heaterButton"));
        sizePolicy1.setHeightForWidth(heaterButton->sizePolicy().hasHeightForWidth());
        heaterButton->setSizePolicy(sizePolicy1);
        heaterButton->setMinimumSize(QSize(80, 0));
        heaterButton->setCheckable(false);

        heaterLayout->addWidget(heaterButton);

        horizontalSpacer_13 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        heaterLayout->addItem(horizontalSpacer_13);


        verticalLayout_6->addLayout(heaterLayout);

        horizontalLayout_33 = new QHBoxLayout();
        horizontalLayout_33->setObjectName(QString::fromUtf8("horizontalLayout_33"));
        heaterVLabel = new QLabel(layoutWidget_2);
        heaterVLabel->setObjectName(QString::fromUtf8("heaterVLabel"));
        sizePolicy4.setHeightForWidth(heaterVLabel->sizePolicy().hasHeightForWidth());
        heaterVLabel->setSizePolicy(sizePolicy4);
        heaterVLabel->setMinimumSize(QSize(100, 0));

        horizontalLayout_33->addWidget(heaterVLabel);

        horizontalSpacer_14 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_33->addItem(horizontalSpacer_14);

        heaterVlcd = new QLCDNumber(layoutWidget_2);
        heaterVlcd->setObjectName(QString::fromUtf8("heaterVlcd"));
        QSizePolicy sizePolicy6(QSizePolicy::Fixed, QSizePolicy::Minimum);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(heaterVlcd->sizePolicy().hasHeightForWidth());
        heaterVlcd->setSizePolicy(sizePolicy6);
        heaterVlcd->setMinimumSize(QSize(133, 0));
        heaterVlcd->setDigitCount(6);
        heaterVlcd->setSegmentStyle(QLCDNumber::Flat);

        horizontalLayout_33->addWidget(heaterVlcd);


        verticalLayout_6->addLayout(horizontalLayout_33);

        horizontalLayout_34 = new QHBoxLayout();
        horizontalLayout_34->setObjectName(QString::fromUtf8("horizontalLayout_34"));
        heaterILabel = new QLabel(layoutWidget_2);
        heaterILabel->setObjectName(QString::fromUtf8("heaterILabel"));
        sizePolicy4.setHeightForWidth(heaterILabel->sizePolicy().hasHeightForWidth());
        heaterILabel->setSizePolicy(sizePolicy4);
        heaterILabel->setMinimumSize(QSize(100, 0));

        horizontalLayout_34->addWidget(heaterILabel);

        horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_34->addItem(horizontalSpacer_17);

        heaterIlcd = new QLCDNumber(layoutWidget_2);
        heaterIlcd->setObjectName(QString::fromUtf8("heaterIlcd"));
        sizePolicy6.setHeightForWidth(heaterIlcd->sizePolicy().hasHeightForWidth());
        heaterIlcd->setSizePolicy(sizePolicy6);
        heaterIlcd->setMinimumSize(QSize(133, 0));
        heaterIlcd->setLineWidth(1);
        heaterIlcd->setDigitCount(6);
        heaterIlcd->setSegmentStyle(QLCDNumber::Flat);

        horizontalLayout_34->addWidget(heaterIlcd);


        verticalLayout_6->addLayout(horizontalLayout_34);

        verticalSpacer_10 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_6->addItem(verticalSpacer_10);

        horizontalLayout_35 = new QHBoxLayout();
        horizontalLayout_35->setObjectName(QString::fromUtf8("horizontalLayout_35"));
        runButton = new QPushButton(layoutWidget_2);
        runButton->setObjectName(QString::fromUtf8("runButton"));
        runButton->setCheckable(true);

        horizontalLayout_35->addWidget(runButton);

        horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_35->addItem(horizontalSpacer_18);


        verticalLayout_6->addLayout(horizontalLayout_35);

        progressBar = new QProgressBar(layoutWidget_2);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(24);
        progressBar->setTextVisible(false);

        verticalLayout_6->addWidget(progressBar);

        verticalSpacer_11 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_11);

        tabWidget->addTab(tab_3, QString());

        horizontalLayout->addWidget(tabWidget);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        graphicsView = new QGraphicsView(horizontalLayoutWidget);
        graphicsView->setObjectName(QString::fromUtf8("graphicsView"));
        sizePolicy1.setHeightForWidth(graphicsView->sizePolicy().hasHeightForWidth());
        graphicsView->setSizePolicy(sizePolicy1);
        graphicsView->setMinimumSize(QSize(550, 500));
        graphicsView->setMaximumSize(QSize(550, 500));

        verticalLayout_2->addWidget(graphicsView);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);


        horizontalLayout->addLayout(verticalLayout_2);

        ValveWorkbench->setCentralWidget(centralwidget);
        menubar = new QMenuBar(ValveWorkbench);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 997, 22));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        ValveWorkbench->setMenuBar(menubar);
        statusbar = new QStatusBar(ValveWorkbench);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        ValveWorkbench->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionNew_Project);
        menuFile->addAction(actionOpen_Project);
        menuFile->addAction(actionSave_Project);
        menuFile->addAction(actionSave_As);
        menuFile->addAction(actionClose_Project);
        menuFile->addSeparator();
        menuFile->addAction(actionLoad_Device);
        menuFile->addAction(actionLoad_Model);
        menuFile->addAction(actionLoad_Measurement);
        menuFile->addAction(actionLoad_Template);
        menuFile->addSeparator();
        menuFile->addAction(actionPrint);
        menuFile->addAction(actionExit);
        menuFile->addSeparator();
        menuFile->addAction(actionOptions);

        retranslateUi(ValveWorkbench);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ValveWorkbench);
    } // setupUi

    void retranslateUi(QMainWindow *ValveWorkbench)
    {
        ValveWorkbench->setWindowTitle(QCoreApplication::translate("ValveWorkbench", "Valve Designer", nullptr));
        actionLoad_Model->setText(QCoreApplication::translate("ValveWorkbench", "Load Model...", nullptr));
        actionExit->setText(QCoreApplication::translate("ValveWorkbench", "Exit", nullptr));
#if QT_CONFIG(shortcut)
        actionExit->setShortcut(QCoreApplication::translate("ValveWorkbench", "Ctrl+W", nullptr));
#endif // QT_CONFIG(shortcut)
        actionPrint->setText(QCoreApplication::translate("ValveWorkbench", "Print...", nullptr));
#if QT_CONFIG(shortcut)
        actionPrint->setShortcut(QCoreApplication::translate("ValveWorkbench", "Ctrl+P", nullptr));
#endif // QT_CONFIG(shortcut)
        actionNew_Project->setText(QCoreApplication::translate("ValveWorkbench", "New Project", nullptr));
        actionOpen_Project->setText(QCoreApplication::translate("ValveWorkbench", "Open Project", nullptr));
        actionLoad_Measurement->setText(QCoreApplication::translate("ValveWorkbench", "Load Measurement...", nullptr));
        actionSave_Project->setText(QCoreApplication::translate("ValveWorkbench", "Save Project", nullptr));
        actionSave_As->setText(QCoreApplication::translate("ValveWorkbench", "Save As...", nullptr));
        actionClose_Project->setText(QCoreApplication::translate("ValveWorkbench", "Close Project", nullptr));
        actionOptions->setText(QCoreApplication::translate("ValveWorkbench", "Options", nullptr));
        actionLoad_Template->setText(QCoreApplication::translate("ValveWorkbench", "Load Template...", nullptr));
        actionLoad_Device->setText(QCoreApplication::translate("ValveWorkbench", "Load Device...", nullptr));
        label_4->setText(QCoreApplication::translate("ValveWorkbench", "Device:", nullptr));
        label_5->setText(QCoreApplication::translate("ValveWorkbench", "Model:", nullptr));
        label_3->setText(QCoreApplication::translate("ValveWorkbench", "Circuit Type:", nullptr));
        cir1Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        cir3Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        cir2Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        cir7Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        cir5Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        cir6Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        cir4Label->setText(QCoreApplication::translate("ValveWorkbench", "TextLabel", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("ValveWorkbench", "Designer", nullptr));
        label_2->setText(QCoreApplication::translate("ValveWorkbench", "Project Browser", nullptr));
        label->setText(QCoreApplication::translate("ValveWorkbench", "Properties", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("ValveWorkbench", "Modeller", nullptr));
        pushButton_3->setText(QCoreApplication::translate("ValveWorkbench", "Load Template...", nullptr));
        pushButton_4->setText(QCoreApplication::translate("ValveWorkbench", "Save Template...", nullptr));
        label_13->setText(QCoreApplication::translate("ValveWorkbench", "Device Name:", nullptr));
        deviceTypeLabel_2->setText(QCoreApplication::translate("ValveWorkbench", "Device Type: ", nullptr));
        label_14->setText(QCoreApplication::translate("ValveWorkbench", "Test Type:", nullptr));
        heaterLabel_2->setText(QCoreApplication::translate("ValveWorkbench", "Heater Voltage:", nullptr));
        label_15->setText(QCoreApplication::translate("ValveWorkbench", "Start", nullptr));
        label_16->setText(QCoreApplication::translate("ValveWorkbench", "Stop", nullptr));
        label_17->setText(QCoreApplication::translate("ValveWorkbench", "Step", nullptr));
        anodeLabel->setText(QCoreApplication::translate("ValveWorkbench", "Anode Voltage:", nullptr));
        gridLabel->setText(QCoreApplication::translate("ValveWorkbench", "-ve Grid Voltage:", nullptr));
        screenLabel->setText(QCoreApplication::translate("ValveWorkbench", "Screen Voltage:", nullptr));
        label_18->setText(QCoreApplication::translate("ValveWorkbench", "Max Ia (mA):", nullptr));
        label_19->setText(QCoreApplication::translate("ValveWorkbench", "Max P (W):", nullptr));
        heaterButton->setText(QCoreApplication::translate("ValveWorkbench", "Heater", nullptr));
        heaterVLabel->setText(QCoreApplication::translate("ValveWorkbench", "Heater Voltage (V)", nullptr));
        heaterILabel->setText(QCoreApplication::translate("ValveWorkbench", "Heater Current (mA)", nullptr));
        runButton->setText(QCoreApplication::translate("ValveWorkbench", "Run Test", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QCoreApplication::translate("ValveWorkbench", "Analyser", nullptr));
        menuFile->setTitle(QCoreApplication::translate("ValveWorkbench", "File", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("ValveWorkbench", "Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ValveWorkbench: public Ui_ValveWorkbench {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VALVEWORKBENCH_H
