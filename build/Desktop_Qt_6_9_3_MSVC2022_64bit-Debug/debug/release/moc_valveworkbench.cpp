/****************************************************************************
** Meta object code from reading C++ file 'valveworkbench.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../valveworkbench.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'valveworkbench.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14ValveWorkbenchE_t {};
} // unnamed namespace

template <> constexpr inline auto ValveWorkbench::qt_create_metaobjectdata<qt_meta_tag_ZN14ValveWorkbenchE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ValveWorkbench",
        "loadModel",
        "",
        "modelScreen",
        "remodelAnode",
        "on_actionExit_triggered",
        "on_actionPrint_triggered",
        "on_actionOptions_triggered",
        "on_actionLoad_Model_triggered",
        "on_stdDeviceSelection_currentIndexChanged",
        "index",
        "on_circuitSelection_currentIndexChanged",
        "on_cir1Value_editingFinished",
        "on_cir2Value_editingFinished",
        "on_cir3Value_editingFinished",
        "on_cir4Value_editingFinished",
        "on_cir5Value_editingFinished",
        "on_cir6Value_editingFinished",
        "on_cir7Value_editingFinished",
        "on_actionNew_Project_triggered",
        "on_projectTree_currentItemChanged",
        "QTreeWidgetItem*",
        "current",
        "previous",
        "on_heaterButton_clicked",
        "on_runButton_clicked",
        "handleReadyRead",
        "handleError",
        "QSerialPort::SerialPortError",
        "error",
        "handleTimeout",
        "on_deviceType_currentIndexChanged",
        "on_testType_currentIndexChanged",
        "on_anodeStart_editingFinished",
        "on_anodeStop_editingFinished",
        "on_anodeStep_editingFinished",
        "on_gridStart_editingFinished",
        "on_gridStop_editingFinished",
        "on_gridStep_editingFinished",
        "on_screenStart_editingFinished",
        "on_screenStop_editingFinished",
        "on_screenStep_editingFinished",
        "on_heaterVoltage_editingFinished",
        "on_iaMax_editingFinished",
        "on_pMax_editingFinished",
        "on_btnAddToProject_clicked",
        "on_fitTriodeButton_clicked",
        "on_fitPentodeButton_clicked",
        "on_tabWidget_currentChanged",
        "on_measureCheck_stateChanged",
        "arg1",
        "on_modelCheck_stateChanged",
        "on_screenCheck_stateChanged",
        "on_designerCheck_stateChanged",
        "on_symSwingCheck_stateChanged",
        "on_inputSensitivityCheck_stateChanged",
        "on_useBypassedGainCheck_stateChanged",
        "on_properties_itemChanged",
        "QTableWidgetItem*",
        "item",
        "on_actionSave_Project_triggered",
        "on_actionOpen_Project_triggered",
        "on_actionClose_Project_triggered",
        "on_compareButton_clicked",
        "on_cir8Value_editingFinished",
        "on_cir9Value_editingFinished",
        "on_cir10Value_editingFinished",
        "on_cir11Value_editingFinished",
        "on_cir12Value_editingFinished",
        "on_actionExport_Model_triggered",
        "exportFittedModelToDevices",
        "on_mes_mod_select_stateChanged",
        "state",
        "on_pushButton_3_clicked",
        "on_pushButton_4_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'loadModel'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'modelScreen'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'remodelAnode'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'on_actionExit_triggered'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionPrint_triggered'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionOptions_triggered'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionLoad_Model_triggered'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_stdDeviceSelection_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_circuitSelection_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_cir1Value_editingFinished'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir2Value_editingFinished'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir3Value_editingFinished'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir4Value_editingFinished'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir5Value_editingFinished'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir6Value_editingFinished'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir7Value_editingFinished'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionNew_Project_triggered'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_projectTree_currentItemChanged'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, QTreeWidgetItem *)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 }, { 0x80000000 | 21, 23 },
        }}),
        // Slot 'on_heaterButton_clicked'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_runButton_clicked'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleReadyRead'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleError'
        QtMocHelpers::SlotData<void(QSerialPort::SerialPortError)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 28, 29 },
        }}),
        // Slot 'handleTimeout'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_deviceType_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_testType_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_anodeStart_editingFinished'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_anodeStop_editingFinished'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_anodeStep_editingFinished'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_gridStart_editingFinished'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_gridStop_editingFinished'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_gridStep_editingFinished'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_screenStart_editingFinished'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_screenStop_editingFinished'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_screenStep_editingFinished'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_heaterVoltage_editingFinished'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_iaMax_editingFinished'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pMax_editingFinished'
        QtMocHelpers::SlotData<void()>(44, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnAddToProject_clicked'
        QtMocHelpers::SlotData<void()>(45, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_fitTriodeButton_clicked'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_fitPentodeButton_clicked'
        QtMocHelpers::SlotData<void()>(47, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_tabWidget_currentChanged'
        QtMocHelpers::SlotData<void(int)>(48, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'on_measureCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(49, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_modelCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(51, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_screenCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(52, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_designerCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(53, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_symSwingCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(54, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_inputSensitivityCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(55, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_useBypassedGainCheck_stateChanged'
        QtMocHelpers::SlotData<void(int)>(56, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 50 },
        }}),
        // Slot 'on_properties_itemChanged'
        QtMocHelpers::SlotData<void(QTableWidgetItem *)>(57, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 58, 59 },
        }}),
        // Slot 'on_actionSave_Project_triggered'
        QtMocHelpers::SlotData<void()>(60, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionOpen_Project_triggered'
        QtMocHelpers::SlotData<void()>(61, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionClose_Project_triggered'
        QtMocHelpers::SlotData<void()>(62, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_compareButton_clicked'
        QtMocHelpers::SlotData<void()>(63, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir8Value_editingFinished'
        QtMocHelpers::SlotData<void()>(64, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir9Value_editingFinished'
        QtMocHelpers::SlotData<void()>(65, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir10Value_editingFinished'
        QtMocHelpers::SlotData<void()>(66, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir11Value_editingFinished'
        QtMocHelpers::SlotData<void()>(67, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cir12Value_editingFinished'
        QtMocHelpers::SlotData<void()>(68, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionExport_Model_triggered'
        QtMocHelpers::SlotData<void()>(69, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'exportFittedModelToDevices'
        QtMocHelpers::SlotData<void()>(70, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_mes_mod_select_stateChanged'
        QtMocHelpers::SlotData<void(int)>(71, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 72 },
        }}),
        // Slot 'on_pushButton_3_clicked'
        QtMocHelpers::SlotData<void()>(73, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pushButton_4_clicked'
        QtMocHelpers::SlotData<void()>(74, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ValveWorkbench, qt_meta_tag_ZN14ValveWorkbenchE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ValveWorkbench::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ValveWorkbenchE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ValveWorkbenchE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14ValveWorkbenchE_t>.metaTypes,
    nullptr
} };

void ValveWorkbench::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ValveWorkbench *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->loadModel(); break;
        case 1: _t->modelScreen(); break;
        case 2: _t->remodelAnode(); break;
        case 3: _t->on_actionExit_triggered(); break;
        case 4: _t->on_actionPrint_triggered(); break;
        case 5: _t->on_actionOptions_triggered(); break;
        case 6: _t->on_actionLoad_Model_triggered(); break;
        case 7: _t->on_stdDeviceSelection_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->on_circuitSelection_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->on_cir1Value_editingFinished(); break;
        case 10: _t->on_cir2Value_editingFinished(); break;
        case 11: _t->on_cir3Value_editingFinished(); break;
        case 12: _t->on_cir4Value_editingFinished(); break;
        case 13: _t->on_cir5Value_editingFinished(); break;
        case 14: _t->on_cir6Value_editingFinished(); break;
        case 15: _t->on_cir7Value_editingFinished(); break;
        case 16: _t->on_actionNew_Project_triggered(); break;
        case 17: _t->on_projectTree_currentItemChanged((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[2]))); break;
        case 18: _t->on_heaterButton_clicked(); break;
        case 19: _t->on_runButton_clicked(); break;
        case 20: _t->handleReadyRead(); break;
        case 21: _t->handleError((*reinterpret_cast< std::add_pointer_t<QSerialPort::SerialPortError>>(_a[1]))); break;
        case 22: _t->handleTimeout(); break;
        case 23: _t->on_deviceType_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 24: _t->on_testType_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 25: _t->on_anodeStart_editingFinished(); break;
        case 26: _t->on_anodeStop_editingFinished(); break;
        case 27: _t->on_anodeStep_editingFinished(); break;
        case 28: _t->on_gridStart_editingFinished(); break;
        case 29: _t->on_gridStop_editingFinished(); break;
        case 30: _t->on_gridStep_editingFinished(); break;
        case 31: _t->on_screenStart_editingFinished(); break;
        case 32: _t->on_screenStop_editingFinished(); break;
        case 33: _t->on_screenStep_editingFinished(); break;
        case 34: _t->on_heaterVoltage_editingFinished(); break;
        case 35: _t->on_iaMax_editingFinished(); break;
        case 36: _t->on_pMax_editingFinished(); break;
        case 37: _t->on_btnAddToProject_clicked(); break;
        case 38: _t->on_fitTriodeButton_clicked(); break;
        case 39: _t->on_fitPentodeButton_clicked(); break;
        case 40: _t->on_tabWidget_currentChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 41: _t->on_measureCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 42: _t->on_modelCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 43: _t->on_screenCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 44: _t->on_designerCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 45: _t->on_symSwingCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 46: _t->on_inputSensitivityCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 47: _t->on_useBypassedGainCheck_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 48: _t->on_properties_itemChanged((*reinterpret_cast< std::add_pointer_t<QTableWidgetItem*>>(_a[1]))); break;
        case 49: _t->on_actionSave_Project_triggered(); break;
        case 50: _t->on_actionOpen_Project_triggered(); break;
        case 51: _t->on_actionClose_Project_triggered(); break;
        case 52: _t->on_compareButton_clicked(); break;
        case 53: _t->on_cir8Value_editingFinished(); break;
        case 54: _t->on_cir9Value_editingFinished(); break;
        case 55: _t->on_cir10Value_editingFinished(); break;
        case 56: _t->on_cir11Value_editingFinished(); break;
        case 57: _t->on_cir12Value_editingFinished(); break;
        case 58: _t->on_actionExport_Model_triggered(); break;
        case 59: _t->exportFittedModelToDevices(); break;
        case 60: _t->on_mes_mod_select_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 61: _t->on_pushButton_3_clicked(); break;
        case 62: _t->on_pushButton_4_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *ValveWorkbench::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ValveWorkbench::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ValveWorkbenchE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "Client"))
        return static_cast< Client*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int ValveWorkbench::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 63)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 63;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 63)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 63;
    }
    return _id;
}
QT_WARNING_POP
