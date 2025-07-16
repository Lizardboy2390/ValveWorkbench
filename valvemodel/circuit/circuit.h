#pragma once

#include "../ui/uibridge.h"
#include "../model/device.h"
#include "../ui/plot.h"
#include "../ui/parameter.h"

// sharedspice.h dependency removed
// Minimal declarations needed for compatibility
#include <string>
#include <vector>

// Stub declarations for ngspice types and functions
struct vector_info {
    char* vecname;
    int v_length;
    double* v_realdata;
};
typedef struct vector_info* pvector_info;

inline pvector_info ngGet_Vec_Info(char* vecname) { 
    (void)vecname; // Mark parameter as used
    return nullptr; 
}

#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>
#endif

// Removed ngspice type declarations - using void* directly

enum eCircuitType {
    TRIODE_COMMON_CATHODE,
    PENTODE_COMMON_CATHODE
};

// These functions are defined in ngspice_stubs.cpp
#ifndef NGSPICE_STUBS_DEFINED
#define NGSPICE_STUBS_DEFINED
extern int ng_getchar(char* outputreturn, int ident, void* userdata);
extern int ng_getstat(char* outputreturn, int ident, void* userdata);
extern int ng_exit(int, bool, bool, int ident, void*);
extern int ng_thread_runs(bool noruns, int ident, void* userdata);
extern int ng_initdata(void* intdata, int ident, void* userdata);
extern int ng_data(void* vdata, int numvecs, int ident, void* userdata);
extern int cieq(char* p, char* s);
extern int ciprefix(const char* p, const char* s);
#endif

// Stub declaration for ngSpice_Command
inline int ngSpice_Command(char* cmd) { 
    (void)cmd; // Mark parameter as used
    return 0; 
}

// Stub declaration for ngSpice_Init
inline int ngSpice_Init(int (*getchar_ptr)(char*, int, void*),
                      int (*getstat_ptr)(char*, int, void*),
                      int (*exit_ptr)(int, bool, bool, int, void*),
                      int (*data_ptr)(void*, int, int, void*),
                      int (*initdata_ptr)(void*, int, void*),
                      int (*thread_runs_ptr)(bool, int, void*),
                      void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)getchar_ptr;
    (void)getstat_ptr;
    (void)exit_ptr;
    (void)data_ptr;
    (void)initdata_ptr;
    (void)thread_runs_ptr;
    (void)userdata;
    return 0;
}

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
