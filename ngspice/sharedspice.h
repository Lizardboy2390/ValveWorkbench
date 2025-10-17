#ifndef NGSHAREDSPICE_H
#define NGSHAREDSPICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ngspice shared library interface definitions */

/* Function pointer types for callback functions */
typedef int (*ng_getchar)(char* outputreturn, int ident, void* userdata);
typedef int (*ng_getstat)(char* outputreturn, int ident, void* userdata);
typedef int (*ng_exit)(int exitstatus, int immediate, int quitexit, int ident, void* userdata);
typedef int (*ng_data)(void* pvecvalues, int numvecs, int ident, void* userdata);
typedef int (*ng_initdata)(void* pvecinit, int ident, void* userdata);
typedef int (*ng_thread_runs)(int thread_id, void* userdata);

/* Main ngspice initialization function */
extern int ngSpice_Init(ng_getchar ng_getchar, ng_getstat ng_getstat, ng_exit ng_exit,
                       ng_data ng_data, ng_initdata ng_initdata, ng_thread_runs ng_thread_runs, void* userdata);

/* Other ngspice functions that might be needed */
extern int ngSpice_Command(char* command);
extern int ngSpice_Circ(char** circarray);
extern int ngSpice_running(void);
extern int ngSpice_AllPlots(char*** plots);
extern int ngSpice_AllVecs(char* plotname, char*** vecs);

#ifdef __cplusplus
}
#endif

#endif /* NGSHAREDSPICE_H */
