#ifndef SHAREDSPICE_H
#define SHAREDSPICE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__MINGW32__) || defined(_MSC_VER) || defined(__CYGWIN__)
#ifdef SHARED_MODULE
  #define IMPEXP __declspec(dllexport)
#else
  #define IMPEXP __declspec(dllimport)
#endif
#else
/* use with gcc flag -fvisibility=hidden */
#if __GNUC__ >= 4
  #define IMPEXP __attribute__ ((visibility ("default")))
  #define IMPEXPLOCAL  __attribute__ ((visibility ("hidden")))
#else
  #define IMPEXP
  #define IMPEXP_LOCAL
#endif
#endif

/* Complex numbers. */
struct ngcomplex {
    double cx_real;
    double cx_imag;
};

typedef struct ngcomplex ngcomplex_t;

/* vector info obtained from any vector in ngspice.dll.
   Allows direct access to the ngspice internal vector structure,
   as defined in include/ngspice/devc.h . */
typedef struct vector_info {
    char *v_name;		/* Same as so_vname. */
    int v_type;			/* Same as so_vtype. */
    short v_flags;		/* Flags (a combination of VF_*). */
    double *v_realdata;		/* Real data. */
    ngcomplex_t *v_compdata;	/* Complex data. */
    int v_length;		/* Length of the vector. */
} vector_info, *pvector_info;

typedef struct vecvalues {
    char* name;        /* name of a specific vector */
    double creal;      /* actual data value */
    double cimag;      /* actual data value */
    bool is_scale;     /* if 'name' is the scale vector */
    bool is_complex;   /* if the data are complex numbers */
} vecvalues, *pvecvalues;

typedef struct vecvaluesall {
    int veccount;      /* number of vectors in plot */
    int vecindex;      /* index of actual set of vectors. i.e. the number of accepted data points */
    pvecvalues *vecsa; /* values of actual set of vectors, indexed from 0 to veccount - 1 */
} vecvaluesall, *pvecvaluesall;

/* info for a specific vector */
typedef struct vecinfo
{
    int number;     /* number of vector, as postion in the linked list of vectors, starts with 0 */
    char *vecname;  /* name of the actual vector */
    bool is_real;   /* TRUE if the actual vector has real data */
    void *pdvec;    /* a void pointer to struct dvec *d, the actual vector */
    void *pdvecscale; /* a void pointer to struct dvec *ds, the scale vector */
} vecinfo, *pvecinfo;

/* info for the current plot */
typedef struct vecinfoall
{
    /* the plot */
    char *name;
    char *title;
    char *date;
    char *type;
    int veccount;

    /* the data as an array of vecinfo with length equal to the number of vectors in the plot */
    pvecinfo *vecs;

} vecinfoall, *pvecinfoall;


/* callback functions
addresses received from caller with ngSpice_Init() function
*/
/* sending output from stdout, stderr to caller */
typedef int (SendChar)(char*, int, void*);
/*
   char* string to be sent to caller output
   int   identification number of calling ngspice shared lib
   void* return pointer received from caller, e.g. pointer to object having sent the request
*/
/* sending simulation status to caller */
typedef int (SendStat)(char*, int, void*);
/*
   char* simulation status and value (in percent) to be sent to caller
   int   identification number of calling ngspice shared lib
   void* return pointer received from caller
*/
/* asking for controlled exit */
typedef int (ControlledExit)(int, bool, bool, int, void*);
/*
   int   exit status
   bool  if true: immediate unloading dll, if false: just set flag, unload is done when function has returned
   bool  if true: exit upon 'quit', if false: exit due to ngspice.dll error
   int   identification number of calling ngspice shared lib
   void* return pointer received from caller
*/
/* send back actual vector data */
typedef int (SendData)(pvecvaluesall, int, int, void*);
/*
   vecvaluesall* pointer to array of structs containing actual values from all vectors
   int           number of structs (one per vector)
   int           identification number of calling ngspice shared lib
   void*         return pointer received from caller
*/

/* send back initailization vector data */
typedef int (SendInitData)(pvecinfoall, int, void*);
/*
   vecinfoall* pointer to array of structs containing data from all vectors right after initialization
   int         identification number of calling ngspice shared lib
   void*       return pointer received from caller
*/

/* indicate if background thread is running */
typedef int (BGThreadRunning)(bool, int, void*);
/*
   bool        true if background thread is running
   int         identification number of calling ngspice shared lib
   void*       return pointer received from caller
*/

/* callback functions
   addresses received from caller with ngSpice_Init_Sync() function
*/

/* ask for VSRC EXTERNAL value */
typedef int (GetVSRCData)(double*, double, char*, int, void*);
/*
   double*     return voltage value
   double      actual time
   char*       node name
   int         identification number of calling ngspice shared lib
   void*       return pointer received from caller
*/

/* ask for ISRC EXTERNAL value */
typedef int (GetISRCData)(double*, double, char*, int, void*);
/*
   double*     return current value
   double      actual time
   char*       node name
   int         identification number of calling ngspice shared lib
   void*       return pointer received from caller
*/

/* ngspice initialization,
   printfcn: pointer to callback function for output from ngspice
   statfcn: pointer to callback function for the status string and percent value
   ControlledExit: pointer to callback function for setting a flag if ngspice wants to exit
   SendData: pointer to callback function for returning data values of a simulation
   SendInitData: pointer to callback function for returning initialization information
   BGThreadRunning: pointer to callback function indicating if background thread is running
   userData: pointer to user-defined data, will not be modified, but
      handed to callback functions
*/
IMPEXP
int ngSpice_Init(SendChar* printfcn, SendStat* statfcn, ControlledExit* ngexit,
                 SendData* sdata, SendInitData* sinitdata, BGThreadRunning* bgtrun, void* userData);

/* initialization of synchronizing functions
   vsrcdat: pointer to callback function for retrieving a voltage source value
   isrcdat: pointer to callback function for retrieving a current source value
   userData: pointer to user-defined data, will not be modified, but
      handed to callback functions
*/
IMPEXP
int ngSpice_Init_Sync(GetVSRCData *vsrcdat, GetISRCData *isrcdat, void* userData);

/* Retrieve ngspice command history */
IMPEXP
char** ngSpice_Command_History(void);

/* Supplies the simulator with the current value of a voltage source. */
IMPEXP
int ngSpice_Source_V_Callback(int isrc, double value);

/* Supplies the simulator with the current value of a current source. */
IMPEXP
int ngSpice_Source_I_Callback(int isrc, double value);

/* Send a command to ngspice. */
IMPEXP
int ngSpice_Command(char* command);

/* Send a ciruit to ngspice. */
IMPEXP
int ngSpice_Circ(char** circarray);

/* Return to the caller a pointer to the current plot */
IMPEXP
char* ngSpice_CurPlot(void);

/* return to the caller a pointer to the current plot's data */
IMPEXP
pvector_info ngGet_Vec_Info(char* vecname);

/* Clean up everything */
IMPEXP
int ngSpice_AllPlots(char*** plotlist);

/* Clean up everything */
IMPEXP
int ngSpice_AllVecs(char* plotname, char*** veclist);

/* Destroy all circuits and data */
IMPEXP
int ngSpice_Running(void);

/* Destroy all circuits and data */
IMPEXP
int ngSpice_Exit(void);

#ifdef __cplusplus
}
#endif

#endif /* SHAREDSPICE_H */
