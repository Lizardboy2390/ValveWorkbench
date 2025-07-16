#include "circuit.h"

// Global variables needed by circuit.cpp
bool no_bg = true;
int vecgetnumber = 0;
double v2dat = 0.0;
bool has_break = false;
bool errorflag = false;

// Stub implementations for ngspice functions to allow compilation without the actual ngspice library
// These functions will not be called in the actual application since we've removed the ngspice dependency

int ng_getchar(char* outputreturn, int ident, void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)outputreturn;
    (void)ident;
    (void)userdata;
    // Stub implementation
    return 0;
}

int ng_getstat(char* outputreturn, int ident, void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)outputreturn;
    (void)ident;
    (void)userdata;
    // Stub implementation
    return 0;
}

int ng_exit(int exitstatus, bool immediate, bool quitexit, int ident, void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)exitstatus;
    (void)immediate;
    (void)quitexit;
    (void)ident;
    (void)userdata;
    // Stub implementation
    return 0;
}

int ng_thread_runs(bool noruns, int ident, void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)noruns;
    (void)ident;
    (void)userdata;
    // Stub implementation
    return 0;
}

int ng_initdata(void* intdata, int ident, void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)intdata;
    (void)ident;
    (void)userdata;
    // Stub implementation
    return 0;
}

int ng_data(void* vdata, int numvecs, int ident, void* userdata) {
    // Mark parameters as used to avoid warnings
    (void)vdata;
    (void)numvecs;
    (void)ident;
    (void)userdata;
    // Stub implementation
    return 0;
}

int cieq(char* p, char* s) {
    // Mark parameters as used to avoid warnings
    (void)p;
    (void)s;
    // Stub implementation
    return 0;
}

int ciprefix(const char* p, const char* s) {
    // Mark parameters as used to avoid warnings
    (void)p;
    (void)s;
    // Stub implementation
    return 0;
}

// ngSpice_Command is now defined inline in circuit.h
