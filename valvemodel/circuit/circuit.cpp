#include "circuit.h"

#include <QApplication>

<<<<<<< Updated upstream
// External declarations for variables defined in ngspice_stubs.cpp
extern bool no_bg;
extern int vecgetnumber;
extern double v2dat;
extern bool has_break;
extern bool errorflag;
=======
bool no_bg = true;
int vecgetnumber = 0;
double v2dat;
bool has_break = false;
bool errorflag = false;
// Define testnumber to avoid undefined variable error
int testnumber = 0;
#ifndef _MSC_VER
// Only needed for non-Windows platforms
pthread_t mainthread;
#endif

/* Callback function called from bg thread in ngspice to transfer
any string created by printf or puts. Output to stdout in ngspice is
preceded by token stdout, same with stderr.*/
int ng_getchar(char* outputreturn, int ident, void* userdata)
{
    printf("%s\n", outputreturn);
    /* setting a flag if an error message occurred */
    if (ciprefix("stderr Error:", outputreturn))
        errorflag = true;
    return 0;
}

int ng_getstat(char* outputreturn, int ident, void* userdata)
{
    printf("%s\n", outputreturn);
    return 0;
}

int ng_thread_runs(bool noruns, int ident, void* userdata)
{
    no_bg = noruns;
    if (noruns)
        printf("bg not running\n");
    else
        printf("bg running\n");

    return 0;
}

/* Callback function called from bg thread in ngspice once per accepted data point */
int ng_data(pvecvaluesall vdata, int numvecs, int ident, void* userdata)
{
    int* ret = nullptr; // Initialize to avoid unused variable warning

    v2dat = vdata->vecsa[vecgetnumber]->creal;
    if (!has_break && (v2dat > 0.5)) {
        //        printf("Data V(2) value: %f\n", v2dat);
        /* using signal SIGTERM by sending to main thread, alterp() then is run from the main thread,
              (not on Windows though!)  */
#ifndef _MSC_VER
        // Only execute this code on non-Windows platforms
        if (testnumber == 4 && mainthread != 0) {
            // Use pthread_kill only on platforms that support it
            pthread_kill(mainthread, SIGTERM);
        }
#endif
        has_break = true;
        printf("Pause requested, setpoint reached\n");
        /* leave bg thread for a while to allow halting it from main */
#if defined(__MINGW32__) || defined(_MSC_VER)
        Sleep(100);
#else
        usleep(100000);
#endif \
        //        ret = ((int * (*)(char*)) ngSpice_Command_handle)("bg_halt");
    }
    return 0;
}

/* Callback function called from bg thread in ngspice once upon intialization
   of the simulation vectors)*/
int ng_initdata(pvecinfoall intdata, int ident, void* userdata)
{
    int i;
    int vn = intdata->veccount;
    for (i = 0; i < vn; i++) {
        printf("Vector: %s\n", intdata->vecs[i]->vecname);
        /* find the location of V(2) */
        if (cieq(intdata->vecs[i]->vecname, const_cast <char*>("V(2)")))
            vecgetnumber = i;
    }
    return 0;
}

/* Callback function called from bg thread in ngspice if fcn controlled_exit()
   is hit. Do not exit, but unload ngspice. */
int ng_exit(int exitstatus, bool immediate, bool quitexit, int ident, void* userdata)
{
    if (quitexit) {
        printf("DNote: Returned form quit with exit status %d\n", exitstatus);
        exit(exitstatus);
    }

    if (immediate) {
        printf("DNote: Unloading ngspice inmmediately is not possible\n");
        printf("DNote: Can we recover?\n");
    } else {
        printf("DNote: Unloading ngspice is not possible\n");
        printf("DNote: Can we recover? Send 'quit' command to ngspice.\n");
        errorflag = true;
        ngSpice_Command(const_cast <char*>("quit 5"));
        //        raise(SIGINT);
    }

    return exitstatus;
}

/* Case insensitive str eq.
   Like strcasecmp( ) XXX */
int cieq(char* p, char* s)
{
    while (*p) {
        if ((isupper(*p) ? tolower(*p) : *p) !=
            (isupper(*s) ? tolower(*s) : *s))
            return(false);
        p++;
        s++;
    }
    return (*s ? false : true);
}

/* Case insensitive prefix. */
int ciprefix(const char* p, const char* s)
{
    while (*p) {
        if ((isupper(*p) ? tolower(*p) : *p) !=
            (isupper(*s) ? tolower(*s) : *s))
            return(false);
        p++;
        s++;
    }
    return (true);
}
>>>>>>> Stashed changes

Circuit::Circuit()
{

}

void Circuit::setParameter(int index, double value)
{
    parameter[index]->setValue(value);

    update(index);
}

double Circuit::getParameter(int index)
{
    return parameter[index]->getValue();
}

void Circuit::setDevice1(Device *newDevice1)
{
    device1 = newDevice1;
}

void Circuit::setDevice2(Device *newDevice2)
{
    device2 = newDevice2;
}

bool Circuit::waitSimulationEnd()
{
    bool completed = false;
    for (int i=0;i < 20;i++) {
#if defined(__MINGW32__) || defined(_MSC_VER)
        Sleep (100);
#else
        usleep (100000);
#endif
        QApplication::processEvents(QEventLoop::AllEvents);

        if (no_bg) {
            completed = true;
            break;
        }
    }

    return (completed);
}
