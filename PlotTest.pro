QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = PlotTest
TEMPLATE = app

SOURCES += \
    plottest.cpp \
    qcustomplot/qcustomplot.cpp \
    valvemodel/ui/plotqcp.cpp

HEADERS += \
    qcustomplot/qcustomplot.h \
    valvemodel/ui/plotqcp.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
