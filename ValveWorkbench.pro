# Qt include paths for MSVC
win32:INCLUDEPATH += "C:/Qt/6.9.3/msvc2022_64/include"
win32:INCLUDEPATH += "C:/Qt/6.9.3/msvc2022_64/include/QtCore"
win32:INCLUDEPATH += "C:/Qt/6.9.3/msvc2022_64/include/QtWidgets"
win32:INCLUDEPATH += "C:/Qt/6.9.3/msvc2022_64/include/QtSerialPort"

QT       += core gui printsupport serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += GLOG_NO_ABBREVIATED_SEVERITIES
DEFINES += NOMINMAX
DEFINES += WIN32_LEAN_AND_MEAN
DEFINES += GLOG_USE_GLOG_EXPORT

win32:CONFIG(release, debug|release): DESTDIR = $$OUT_PWD/release

# Force Release runtime to match Ceres
win32: QMAKE_CXXFLAGS += /MD
win32: QMAKE_CXXFLAGS_DEBUG += /MD

SOURCES += \
    analyser/analyser.cpp \
    analyser/client.cpp \
    ledindicator/ledindicator.cpp \
    main.cpp \
    preferencesdialog.cpp \
    projectdialog.cpp \
    comparedialog.cpp \
    project.cpp \
    valveworkbench.cpp \
    valvemodel/circuit/circuit.cpp \
    valvemodel/circuit/triodeaccathodefollower.cpp \
    valvemodel/circuit/triodecommoncathode.cpp \
    valvemodel/circuit/pentodecommoncathode.cpp \
    valvemodel/circuit/singleendedoutput.cpp \
    valvemodel/circuit/singleendeduloutput.cpp \
    valvemodel/circuit/pushpulloutput.cpp \
    valvemodel/circuit/pushpulluloutput.cpp \
    valvemodel/circuit/triodedccathodefollower.cpp \
    valvemodel/data/dataset.cpp \
    valvemodel/data/measurement.cpp \
    valvemodel/data/project.cpp \
    valvemodel/data/sample.cpp \
    valvemodel/data/sweep.cpp \
    valvemodel/model/cohenhelietriode.cpp \
    valvemodel/model/device.cpp \
    valvemodel/model/estimate.cpp \
    valvemodel/model/gardinerpentode.cpp \
    valvemodel/model/extractmodelpentode.cpp \
    valvemodel/model/simplemanualpentode.cpp \
    valvemodel/model/linearsolver.cpp \
    valvemodel/model/model.cpp \
    valvemodel/model/modelfactory.cpp \
    valvemodel/model/korentriode.cpp \
    valvemodel/model/reefmanpentode.cpp \
    valvemodel/model/simpletriode.cpp \
    valvemodel/model/template.cpp \
    valvemodel/model/quadraticsolver.cpp \
    valvemodel/model/moc_device.cpp \
    valvemodel/model/moc_model.cpp \
    valvemodel/model/moc_cohenhelietriode.cpp \
    valvemodel/model/moc_estimate.cpp \
    valvemodel/model/moc_gardinerpentode.cpp \
    valvemodel/model/moc_korentriode.cpp \
    valvemodel/model/moc_reefmanpentode.cpp \
    valvemodel/model/moc_simpletriode.cpp \
    valvemodel/ui/uibridge.cpp \
    valvemodel/ui/plot.cpp \
    valvemodel/ui/parameter.cpp \
    valvemodel/ui/simplemanualpentodedialog.cpp \
    valvemodel/ui/moc_uibridge.cpp \

HEADERS += \
    analyser/client.h \
    ledindicator/ledindicator.h \
    preferencesdialog.h \
    projectdialog.h \
    comparedialog.h \
    project.h \
    valveworkbench.h \
    valvemodel/circuit/circuit.h \
    valvemodel/circuit/triodeaccathodefollower.h \
    valvemodel/circuit/triodecommoncathode.h \
    valvemodel/circuit/pentodecommoncathode.h \
    valvemodel/circuit/singleendedoutput.h \
    valvemodel/circuit/singleendeduloutput.h \
    valvemodel/circuit/pushpulloutput.h \
    valvemodel/circuit/pushpulluloutput.h \
    valvemodel/circuit/triodedccathodefollower.h \
    valvemodel/ui/parameter.h \
    valvemodel/ui/simplemanualpentodedialog.h \
    valvemodel/model/quadraticsolver.h

# Using Ceres installed in C:/Ceres_Install/ceres-solver
win32:INCLUDEPATH += "C:/Ceres_Install/ceres-solver/include" \
                     "C:/Ceres_Install/gflags/include" \
                     "C:/Ceres_Install/Eigen3/include/eigen3" \
                     "C:/Ceres_Install/glog/include"

win32:CONFIG(release, debug|release): LIBS += -LC:/Ceres_Install/ceres-solver/lib/ -lceres
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/Ceres_Install/ceres-solver/lib/ -lceres

win32:CONFIG(release, debug|release): LIBS += -LC:/Ceres_Install/gflags/lib/ -lgflags_static
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/Ceres_Install/gflags/lib/ -lgflags_static

win32:CONFIG(release, debug|release): LIBS += -LC:/Ceres_Install/glog/lib/ -lglog
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/Ceres_Install/glog/lib/ -lglog

# ngSpice library
# win32:LIBS += -L"C:/Users/lizar/Downloads/ngspice-45.2_64" -lngspice

DEPENDPATH += C:/Ceres_Install/ceres-solver/include
DEPENDPATH += C:/Ceres_Install/gflags/include
DEPENDPATH += C:/Ceres_Install/Eigen3/include/eigen3

QMAKE_POST_LINK += copy "C:/Ceres_Install/ceres-solver/bin/ceres.dll" "$(DESTDIR)" &
QMAKE_POST_LINK += copy "C:/Ceres_Install/gflags/bin/gflags.dll" "$(DESTDIR)" &
QMAKE_POST_LINK += copy "C:/Ceres_Install/glog/bin/glog.dll" "$(DESTDIR)" &

FORMS += \
    valveworkbench.ui
