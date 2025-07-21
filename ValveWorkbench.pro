QT       += core gui printsupport serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    analyser/analyser.cpp \
    analyser/client.cpp \
    comparedialog.cpp \
    ledindicator/ledindicator.cpp \
    main.cpp \
    preferencesdialog.cpp \
    projectdialog.cpp \
<<<<<<< Updated upstream
    qcustomplot/qcustomplot.cpp \
    valvemodel/circuit/ngspice_stubs.cpp \
=======
#    valvemodel/circuit/pentodecommoncathode.cpp \
>>>>>>> Stashed changes
    valvemodel/data/dataset.cpp \
    valvemodel/data/measurement.cpp \
    valvemodel/data/project.cpp \
    valvemodel/data/sample.cpp \
    valvemodel/data/sweep.cpp \
    valvemodel/model/cohenhelietriode.cpp \
    valvemodel/model/device.cpp \
    valvemodel/model/estimate.cpp \
    valvemodel/model/gardinerpentode.cpp \
    valvemodel/model/linearsolver.cpp \
    valvemodel/model/model.cpp \
    valvemodel/model/modelfactory.cpp \
    valvemodel/model/korentriode.cpp \
    valvemodel/model/quadraticsolver.cpp \
    valvemodel/model/reefmanpentode.cpp \
    valvemodel/model/simpletriode.cpp \
    valvemodel/circuit/circuit.cpp \
    valvemodel/circuit/triodeaccathodefollower.cpp \
    valvemodel/circuit/triodecommoncathode.cpp \
    valvemodel/circuit/pentodecommoncathode.cpp \
    valvemodel/circuit/triodedccathodefollower.cpp \
    valvemodel/model/template.cpp \
    valvemodel/ui/uibridge.cpp \
    valvemodel/ui/plot.cpp \
    valvemodel/ui/plotqcp.cpp \
    valvemodel/ui/parameter.cpp \
    valveworkbench.cpp

HEADERS += \
    analyser/analyser.h \
    analyser/client.h \
    comparedialog.h \
    ledindicator/ledindicator.h \
<<<<<<< Updated upstream
    preferencesdialog.h \
    projectdialog.h \
    qcustomplot/qcustomplot.h \
=======
#    ngspice/sharedspice.h \
    preferencesdialog.h \
    projectdialog.h \
#    valvemodel/circuit/pentodecommoncathode.h \
>>>>>>> Stashed changes
    valvemodel/constants.h \
    valvemodel/data/dataset.h \
    valvemodel/data/measurement.h \
    valvemodel/data/project.h \
    valvemodel/data/sample.h \
    valvemodel/data/sweep.h \
    valvemodel/model/cohenhelietriode.h \
    valvemodel/model/device.h \
    valvemodel/model/estimate.h \
    valvemodel/model/gardinerpentode.h \
    valvemodel/model/korentriode.h \
    valvemodel/model/linearsolver.h \
    valvemodel/model/model.h \
    valvemodel/model/modelfactory.h \
    valvemodel/model/quadraticsolver.h \
    valvemodel/model/reefmanpentode.h \
    valvemodel/model/simpletriode.h \
    valvemodel/circuit/circuit.h \
    valvemodel/circuit/triodeaccathodefollower.h \
    valvemodel/circuit/triodecommoncathode.h \
    valvemodel/circuit/pentodecommoncathode.h \
    valvemodel/circuit/triodedccathodefollower.h \
    valvemodel/model/template.h \
    valvemodel/ui/uibridge.h \
    valvemodel/ui/plot.h \
    valvemodel/ui/plotqcp.h \
    valvemodel/ui/parameter.h \
#    valvemodeller.h \
    valveworkbench.h

FORMS += \
#    comparedialog.ui \
    preferencesdialog.ui \
    projectdialog.ui \
    valveworkbench.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

<<<<<<< Updated upstream
# Ceres Solver dependency has been removed
# Direct mathematical calculations are now used instead

# Ceres Solver pre-target dependencies have been removed

# glog and gflags dependencies have been removed as they were only needed by Ceres
=======
# Link against Ceres library
win32:CONFIG(release, debug|release): LIBS += -LC:/ceres_install/install_final/lib/ -lceres
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/ceres_install/install_final/lib/ -lceres
else:unix: LIBS += -LC:/ceres_install/install_final/lib/ -lceres

INCLUDEPATH += C:/ceres_install/install_final/include
INCLUDEPATH += C:/ceres_install/eigen
DEPENDPATH += C:/ceres_install/install_final/include
DEPENDPATH += C:/ceres_install/eigen

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += C:/ceres_install/install_final/lib/ceres.lib
win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += C:/ceres_install/install_final/lib/ceres.lib

# Library already linked above

# Define preprocessor macro to use internal miniglog
DEFINES += CERES_USE_INTERNAL_GLOG

INCLUDEPATH += $$(CMAKE_PREFIX_PATH)/glog/include
DEPENDPATH += $$(CMAKE_PREFIX_PATH)/glog/include

# Disable gflags dependency since it's not available
# win32: LIBS += -LC:/ceres_install/install_final/lib/ -lgflags_static
# else:unix: LIBS += -L$$(CMAKE_PREFIX_PATH)/gflags/lib/ -lgflags_static

# Define preprocessor macro to disable gflags
DEFINES += CERES_NO_GFLAGS

INCLUDEPATH += $$(CMAKE_PREFIX_PATH)/gflags/include
DEPENDPATH += $$(CMAKE_PREFIX_PATH)/gflags/include
>>>>>>> Stashed changes

INCLUDEPATH += $$(CMAKE_PREFIX_PATH)/eigen3/include/eigen3
DEPENDPATH += $$(CMAKE_PREFIX_PATH)/eigen3/include/eigen3

<<<<<<< Updated upstream
# ngspice dependency has been removed
# win32: LIBS += -L$$PWD/ngspice/ -lngspice
=======
# Temporarily disable ngspice until the library is available
# win32: LIBS += -L$$PWD/ngspice/ -lngspice
# Define a preprocessor macro to conditionally exclude ngspice-dependent code
DEFINES += DISABLE_NGSPICE
>>>>>>> Stashed changes

INCLUDEPATH += $$PWD/ngspice
DEPENDPATH += $$PWD/ngspice

RESOURCES += \
    icons.qrc
