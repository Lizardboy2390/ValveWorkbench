#pragma once

#include "../ui/uibridge.h"
#include "../model/device.h"
#include "../ui/plot.h"
#include "../ui/parameter.h"

#include "sharedspice.h"

#include <tchar.h>
#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>
#endif

enum eCircuitType {
    TRIODE_COMMON_CATHODE,
    PENTODE_COMMON_CATHODE
};

int ng_getchar(char* outputreturn, int ident, void* userdata);

int ng_getstat(char* outputreturn, int ident, void* userdata);

int ng_exit(int, bool, bool, int ident, void*);

int ng_thread_runs(bool noruns, int ident, void* userdata);

int ng_initdata(pvecinfoall intdata, int ident, void* userdata);

int ng_data(pvecvaluesall vdata, int numvecs, int ident, void* userdata);

int cieq(char* p, char* s);

int ciprefix(const char* p, const char* s);

#define spice(X) (ngSpice_Command(const_cast <char *>(X)))

class Circuit : public UIBridge
{
    Q_OBJECT
public:
    Circuit();

    virtual void plot(Plot *plot) = 0;
    virtual int getDeviceType(int index) = 0;

    void setParameter(int index, double value);
    double getParameter(int index);

    void setDevice1(Device *newDevice1);

    void setDevice2(Device *newDevice2);

protected:
    Parameter *parameter[16];
    QGraphicsItemGroup *anodeLoadLine = nullptr;
    QGraphicsItemGroup *cathodeLoadLine = nullptr;

    Device *device1;
    Device *device2;

    virtual void update(int index) = 0;

    bool waitSimulationEnd();
};
