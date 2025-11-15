# uTmax Reference Notes

These notes summarize the architecture and core calculations of the **uTmax** application (Linux GUI for Ronald Dekker’s uTracer3), focusing on aspects that are relevant to ValveWorkbench:

- Measurement control and sweep generation
- Data structures and storage
- Optimization pipeline and model types
- The Koren / Barton and Derk / DerkE model formulas as implemented

All code references are from:

`C:\Users\lizar\Documents\ValveWorkbench\refrence code\uTmax-master`

This is a descriptive reference only; original uTmax code is GPLv3.

---

## 1. High-Level Architecture

### 1.1 Entry Point

**File:** `src/main.cpp`

```cpp
int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);

    QApplication a(argc, argv);

    QFile stylesheet("Style.qss");
    stylesheet.open(QFile::ReadOnly);
    QString setSheet = QLatin1String(stylesheet.readAll());
    a.setStyleSheet(setSheet);

    MainWindow w;

    w.uTmaxDir = QDir::homePath();
    w.uTmaxDir.append("/uTmax_files/");
    ...

    // Read default data and calibration
    w.dataFileName = w.uTmaxDir + "data.csv";
    if (!w.ReadDataFile()) return(EXIT_FAILURE);

    w.calFileName = w.uTmaxDir + "cal.txt";
    if (!w.ReadCalibration()) return(EXIT_FAILURE);

    w.SerialPortDiscovery();
    w.show();

    return a.exec();
}
```

Key points:

- uTmax expects a per-user directory `~/uTmax_files/` containing:
  - `data.csv` – tube definitions and default sweep parameters
  - `cal.txt` – calibration data
- `MainWindow` owns the serial port, data sets, optimizer, and UI.

### 1.2 Core Components

**MainWindow (mainwindow.h / .cpp)**

Responsibilities:

- User interface (Qt widgets, plots using QCustomPlot)
- Serial I/O (QSerialPort) with a custom command/response protocol
- Measurement sequencing via a large state machine (`RxData()`)
- Sweep vector generation (`CreateTestVectors()`)
- Storing measured data (`StoreData()`)
- Passing `results_t` data to a model optimizer (`dr_optimize`)
- Updating UI and plots based on measurement and model

**Optimizers / Models**

1. `dr_optimize` – main optimizer used by the current uTmax fork
   - Supports multiple model types: Koren diodes/triodes/pentodes, Derk, Derk+SE, DerkE, DerkE+SE.
   - Uses GSL Nelder–Mead (`gsl_multimin_fminimizer_nmsimplex`).

2. `kb_optimize` – older/alternative Koren/Barton model optimizer
   - Provides Koren-style Ia / Is models and fitting.

Both optimizers operate on a `QList<results_t>` data set (see typedefs below).

---

## 2. Data Structures

**File:** `src/typedefs.h`

### 2.1 Model Type Names

```cpp
#define NONE "Do Not Model"
#define DIODE "Koren Diode"
#define DUAL_DIODE "Koren Dual Diode"
#define TRIODE "Koren Triode"
#define DUAL_TRIODE "Koren Dual Triode"
#define KOREN_P "Koren Pentode"
#define DERK_P "Derk Pentode"
#define DERK_B "Derk Pent+SE"
#define DERKE_P "DerkE BTet"
#define DERKE_B "DerkE BTet+SE"
```

These strings appear in the UI (e.g. `TubeType` combobox) and map to integer enums in `dr_optimize.cpp`.

### 2.2 Measurement Results

```cpp
struct results_t
{
    float Ia;    // measured anode current (mA)
    float Is;    // measured screen current (mA)
    float Va;    // anode voltage (V)
    float Vs;    // screen voltage (V)
    float Vg;    // control grid voltage (V)
    float Vf;    // filament/ heater voltage (V)

    float IaMod; // modeled anode current (mA)
    float IsMod; // modeled screen current (mA)

    float gm_a;  // gm for section A (mA/V)
    float gm_b;  // gm for section B (dual triode etc.)
    float ra_a;  // anode resistance section A (kΩ)
    float ra_b;  // anode resistance section B (kΩ)
    float mu_a;  // mu section A
    float mu_b;  // mu section B
};
```

- Measured data (`Ia`, `Is`, `Va`, `Vs`, `Vg`, `Vf`) are filled from ADC responses.
- Modeled data (`IaMod`, `IsMod`, `gm_*`, `ra_*`, `mu_*`) are filled after optimization using the chosen model.

### 2.3 Plot Information

```cpp
struct plotInfo_t
{
    int VaSteps, VsSteps, VgSteps;
    float VaStart, VaEnd;
    float VsStart, VsEnd;
    float VgStart, VgEnd;
    QList<results_t> *dataSet;
    QString tube;     // tube ID string
    QString type;     // model type string
    QList<QPen> *penList;
    bool penChange;
    bool VsEQVa;      // triode-connected (Vs == Va) flag
};
```

This describes one plotted data set: axes ranges, steps, and a pointer to a `results_t` list.

---

## 3. Tube Data and Sweep Parameters

**File:** `src/mainwindow.h` (partial)

```cpp
struct tubeData_t
{
    QString ID;       // e.g. "6L6GC"
    QString Anode;
    QString G2;
    QString G1;
    QString G1b;
    QString Cathode;
    QString G3;
    QString HtrA;
    QString HtrB;
    float   HtrVolts;
    QString Model;     // one of NONE, TRIODE, KOREN_P, DERK_P, ...

    float VaStart, VaEnd;
    int   VaStep;      // number of steps or step count
    float VsStart, VsEnd;
    int   VsStep;
    float VgStart, VgEnd;
    int   VgStep;

    // Quick-test & emission settings
    float Vca, Dva;    // central Va and percentage delta
    float Vcs, Dvs;    // central Vs and percentage delta
    float Vcg, Dvg;    // central Vg and percentage delta
    float powerLim;    // power limit in W
    float EmmVa, EmmVs, EmmVg;  // emission test Va/Vs/Vg
    float gm1Typ, gm1Del;       // typical gm values / tolerances
    float gm2Typ, gm2Del;
};
```

- `tubeDataList` is loaded from `data.csv` at startup.
- UI edits (spinboxes etc.) update `tubeData` and ultimately drive the sweep generation.

---

## 4. Serial Protocol & Measurement Control

**File:** `src/mainwindow.cpp` (selected functions)

### 4.1 Command/Response Structure

`MainWindow` uses a custom packet format where the first nibble encodes the command type:

- `"00..."` – Start measurement
- `"10..."` – Get measurement
- `"20..."` – Hold measurement
- `"30..."` – End measurement
- `"40..."` – Filament control
- `"50..."` – ADC info

The details are encoded as hex characters (`%02X`, `%04X`, etc.).

```cpp
struct CommandResponse_t
{
    QByteArray Command;   // ASCII hex command string
    int txPos;
    TxState_t txState;
    int ExpectedRspLen;   // bytes of response expected
    QByteArray Response;  // response buffer
    int rxPos;
    RxState_t rxState;
    int timeout;          // ms
};

CommandResponse_t CmdRsp;
```

### 4.2 Sending Commands

```cpp
void MainWindow::SendStartMeasurementCommand(CommandResponse_t *p, uint8_t limits,
                                             uint8_t averaging, uint8_t screenGain,
                                             uint8_t anodeGain)
{
    char temp_buffer[30];
    std::snprintf(temp_buffer, 19, "0000000000%02X%02X%02X%02X",
                  limits, averaging, screenGain, anodeGain);
    p->Command = temp_buffer;
    p->ExpectedRspLen = 0;
    p->timeout = PING_TIMEOUT;
    SendCommand(p, true, 0);
}

void MainWindow::SendGetMeasurementCommand(CommandResponse_t *p, uint16_t anodeV,
                                           uint16_t screenV, uint16_t gridV,
                                           uint16_t filamentV)
{
    char temp_buffer[30];
    std::snprintf(temp_buffer, 19, "10%04X%04X%04X%04X",
                  anodeV, screenV, gridV, filamentV);
    p->Command = temp_buffer;
    p->ExpectedRspLen = 38;            // device returns 38-byte ADC packet
    p->timeout = ADC_READ_TIMEOUT;
    SendCommand(p, true, 0);
}

void MainWindow::SendHoldMeasurementCommand(CommandResponse_t *p, uint16_t anodeV,
                                            uint16_t screenV, uint16_t gridV,
                                            uint16_t filamentV, int delay)
{
    char temp_buffer[30];
    std::snprintf(temp_buffer, 19, "20%04X%04X%04X%04X",
                  anodeV, screenV, gridV, filamentV);
    p->Command = temp_buffer;
    p->ExpectedRspLen = 0;
    p->timeout = ADC_READ_TIMEOUT + delay;
    SendCommand(p, true, 0);
}

void MainWindow::SendEndMeasurementCommand(CommandResponse_t *p)
{
    p->Command = "300000000000000000";
    p->ExpectedRspLen = 0;
    p->timeout = PING_TIMEOUT;
    SendCommand(p, true, 0);
}

void MainWindow::SendFilamentCommand(CommandResponse_t *p, uint16_t filamentV)
{
    char temp_buffer[30];
    std::snprintf(temp_buffer, 19, "40000000000000%04X", filamentV);
    p->Command = temp_buffer;
    p->ExpectedRspLen = 0;
    p->timeout = PING_TIMEOUT;
    SendCommand(p, true, 0);
}

void MainWindow::SendADCCommand(CommandResponse_t *p)
{
    p->Command = "500000000000000000";
    p->ExpectedRspLen = 38;
    p->timeout = PING_TIMEOUT;
    SendCommand(p, true, 0);
}
```

### 4.3 State Machine in `RxData()`

`RxData()` is called from a timer or serial callback and implements a big state machine for:

- Probe / ping
- Reading ADC (calibration / status)
- Heater warmup
- Sweep measurement
- Discharge and safe shutdown
- Comms error recovery

Relevant states for a typical sweep:

- `start_sweep_heater` → initialize data store, counters, and send `SendStartMeasurementCommand`.
- `Heating_wait00` / `Heating_wait_adc` → fetch initial ADC status.
- `Heating` → ramp heater voltage via repeated `SendFilamentCommand(VfADC)`.
- `heat_done` → warmup countdown; eventually calls `CreateTestVectors()` and enters `Sweep_set`.
- `Sweep_set` → for each point in `sweepList`:
  - Convert Va/Vs/Vg to DAC codes (`GetVa/GetVs/GetVg/GetVf`).
  - Depending on `options.Delay`, either:
    - Use `SendGetMeasurementCommand` directly, or
    - Use `SendHoldMeasurementCommand` then `SendGetMeasurementCommand`.
- `Sweep_adc`:
  - On successful ADC response:
    - `saveADCInfo(&response);` – decode raw ADC into volts/amps.
    - `StoreData(true);` – push `results_t` into `dataStore`.
    - Check current limit and power limit; may abort sweep early.
    - If at end of sweep and modeling enabled:
      - Call `optimizer->Optimize(dataStore, TubeTypeIndex, Weight, VaSteps, VsSteps, VgSteps);`
      - Call `updateLcdsWithModel()` and `RePlot(dataStore);`

- `HeatOff` / `Discharge` → turn off heater, send end command, wait for HV discharge.

This is roughly the runtime equivalent of ValveWorkbench’s `Analyser` + measurement loops.

---

## 5. Sweep Vector Generation (`CreateTestVectors`)

**File:** `src/mainwindow.cpp` (excerpt)

`CreateTestVectors()` populates `sweepList` with ordered `(Va, Vs, Vg)` tuples.

### 5.1 Quick Test Mode

When `checkQuickTest` is checked, uTmax generates a small set of test points around a central operating point `(Vca, Vcs, Vcg)` and an emission check point `(EmmVa, EmmVs, EmmVg)`:

```cpp
if (ui->checkQuickTest->isChecked()) {
    if (tubeData.Model==TRIODE || tubeData.Model==DUAL_TRIODE) {
        ui->Vcs->setText(ui->Vca->text());  // Vs = Va for triode
    }

    float Vca = ui->Vca->text().toFloat();
    float Vcs = ui->Vcs->text().toFloat();
    float Vcg = ui->Vcg->text().toFloat();
    float DeltaVa = ui->DeltaVa->text().toFloat();
    float DeltaVs = ui->DeltaVs->text().toFloat();
    float DeltaVg = ui->DeltaVg->text().toFloat();

    // +/- Delta% in Va
    test_vector.Va = Vca * (1 - DeltaVa/200);
    test_vector.Vs = Vcs;
    test_vector.Vg = Vcg;
    sweepList->append(test_vector);

    test_vector.Va = Vca * (1 + DeltaVa/200);
    sweepList->append(test_vector);

    // +/- Delta% in Vs
    test_vector.Va = Vca;
    test_vector.Vs = Vcs * (1 - DeltaVs/200);
    sweepList->append(test_vector);

    test_vector.Vs = Vcs * (1 + DeltaVs/200);
    sweepList->append(test_vector);

    // +/- Delta% in Vg
    test_vector.Vs = Vcs;
    test_vector.Vg = Vcg * (1 - DeltaVg/200);
    sweepList->append(test_vector);

    test_vector.Vg = Vcg * (1 + DeltaVg/200);
    sweepList->append(test_vector);

    // Emission point
    test_vector.Va = ui->EmmVa->text().toFloat();
    test_vector.Vs = ui->EmmVs->text().toFloat();
    test_vector.Vg = ui->EmmVg->text().toFloat();
    sweepList->append(test_vector);
}
```

### 5.2 Full Sweep Mode

When not in quick-test mode, it does either:

- **Triode-connected sweep** if `VsEqVa` is checked (`checkVsEqVa`), or
- A **combined triode pre-sweep (if using pentode models)** plus a full pentode sweep grid.

For pentode models (Koren or Derk derivatives):

```cpp
if (!ui->checkQuickTest->isChecked()) {
    if (ui->checkVsEqVa->isChecked()) {
        // triode-connected: Vs == Va for all points
        for (int g = 0; g <= VgSteps; g++) {
            test_vector.Vg = VgNow;
            VaNow = VaStart;
            if (VgSteps>0) VgNow += (VgEnd - VgStart)/VgSteps;
            for (int a = 0; a <= VaSteps; a++) {
                test_vector.Vs = test_vector.Va = VaNow;
                sweepList->append(test_vector);
                if (VaSteps>0) VaNow += (VaEnd - VaStart)/VaSteps;
            }
        }
    } else {
        // If using the pentode models, do a triode-connected sweep first (Va == Vs)
        if (ui->TubeType->currentText()!=NONE) {
            if (tubeData.Model==KOREN_P || tubeData.Model==DERK_P ||
                tubeData.Model==DERK_B || tubeData.Model==DERKE_P ||
                tubeData.Model==DERKE_B) {
                VgNow = VgStart;
                for (int g = 0; g <= VgSteps; g++) {
                    test_vector.Vg = VgNow;
                    if (VgSteps>=1) VgNow += (VgEnd - VgStart)/VgSteps;
                    VaNow = VaStart;
                    for (int a = 0; a <= VaSteps; a++) {
                        test_vector.Vs = test_vector.Va = VaNow;
                        sweepList->append(test_vector);
                        if (VaSteps>0) {
                            VaNow += (VaEnd - VaStart)/VaSteps;
                        }
                    }
                }
            }
        }

        // Now the full pentode sweep
        VsNow = VsStart;
        for (int s = 0; s <= VsSteps; s++) {
            if (VsNow>400) VsNow=400;
            test_vector.Vs = VsNow;
            if (VsSteps>=1) VsNow += (VsEnd - VsStart)/VsSteps;
            VgNow = VgStart;
            for (int g = 0; g <= VgSteps; g++) {
                test_vector.Vg = VgNow;
                if (VgSteps>=1) VgNow += (VgEnd - VgStart)/VgSteps;
                VaNow = VaStart;
                for (int a = 0; a <= VaSteps; a++) {
                    if (VaNow>400) VaNow=400;
                    test_vector.Va = VaNow;
                    sweepList->append(test_vector);
                    if (VaSteps>=1) VaNow += (VaEnd - VaStart)/VaSteps;
                }
            }
        }
    }
}
```

This is an important pattern: uTmax explicitly performs a triode-connected sweep first for pentode models, then uses the resulting data to seed triode parameters before fitting pentode behavior.

---

## 6. Koren/Barton Model (kb_optimize)

**File:** `src/kb_optimize.h/.cpp`

`kb_optimize` is a GSL-based optimizer for Norman Koren’s model (with Barton’s additions). It provides **Ia** (anode current) and **Is** (screen current) models.

### 6.1 Ia Parameters

```cpp
struct IaParams_t {
    float mu, ex, kg1, kp, kvb, gp;
    float x[6];       // additional parameters; kg2 is not optimized here
    int weight;       // error weighting mode
    int penfn;        // 0 => triode, 1.. => various pentode knee functions
    float max_vg;
    float vct;        // grid voltage shift (Vcutoff-like)
    QList<results_t> * data;
};
```

### 6.2 Ia Calculation

```cpp
float kb_optimize::IaCalc(IaParams_t * p, float Va, float Vs, float Vg) {
    float pi = 3.14159, Ia, E1;
    if (p->penfn == 0) {
        // Triode model
        E1 = Va / p->kp * log(1 + exp(p->kp * (1/p->mu + (Vg + p->vct) / sqrt(p->kvb + Va*Va))));
        if (E1 >= 0)
            Ia = 2 * powf(E1, p->ex) / p->kg1;
        else
            Ia = 0;
    } else {
        // Pentode model: Vs controls the space-charge term
        E1 = Vs / p->kp * log(1 + exp(p->kp * (1/p->mu + (Vg + p->vct)/Vs)));
        if (E1 >= 0)
            Ia = (1 + p->gp*Va) * 2 * powf(E1, p->ex) / p->kg1;
        else
            Ia = 0;

        // Various anode voltage shaping functions for Va (knee / saturation)
        if (p->penfn == 2) {
            Ia *= pi/2 * (1 - exp(-2 * Va/pi/p->kvb));
        } else if (p->penfn == 3) {
            Ia *= tanh(Va / p->kvb);
        } else if (p->penfn == 4) {
            Ia *= (1 + cos(pi * (tanh(powf(Va/p->kvb, 2)) - 1))) / 2;
        } else {
            // penfn == 1: Koren’s original atan(knee)
            Ia *= atan(Va / p->kvb);
        }
    }
    return Ia * 1000.0f; // result in mA
}
```

Interpretation:

- `mu` – amplification factor (triode-like gain)
- `ex` – exponent (space-charge 3/2 law ~1.5, typically 1.3–1.5)
- `kg1` – Koren scaling factor for the Koren current
- `kp` – sharpness of the transition in the log/exp smoothing
- `kvb` – knee voltage parameter controlling saturation / plate resistance
- `gp` – linear dependence on Va (plate current slope in pentode region)
- `vct` – grid voltage offset (cutoff voltage shift)

### 6.3 Is (Screen Current) Model

The screen model uses a **space current** approach:

```cpp
struct IsParams_t {
    float I0, s, mup, mus, mug, ex;
    float x[6];
    int weight;
    int penfn;
    QList<results_t> * data;
};

float kb_optimize::IsCalc(IsParams_t * p, float Va, float Vs, float Vg) {
    // Space-charge model
    float Ix = (p->mup * Va + p->mus * Vs + p->mug * Vg);
    if (Ix < 0) Ix = 0;
    Ix = powf(fabs(Ix), p->ex);
    Ix *= (p->I0 + (1 - p->I0) * 2/pi * atan(p->s * Va/Vs));
    return Ix;  // total space current in mA
}
```

Screen current is obtained by subtracting modeled anode current from total space current:

```cpp
// IsCalcErr: error function for screen model
for each data point r in param->data:
    Ix = IsCalc(&optParam, r.Va, r.Vs, r.Vg);
    Is = Ix - r.IaMod;        // IaMod is modeled anode current
    err = Is - r.Is;          // residual vs measured screen current
```

Note: uTmax’s `kb_optimize` appears to be older; the current branch uses `dr_optimize` for most modeling, but the math is still a good reference for Koren-like behavior.

---

## 7. Derk / DerkE and Koren Models (dr_optimize)

**File:** `src/dr_optimize.h/.cpp`

`dr_optimize` is the main multi-model optimizer used in the current code. It supports:

- Koren diode / dual diode
- Koren triode / dual triode
- Koren pentode
- Derk pentode with and without secondary emission
- DerkE “BTet” pentode with and without SE

### 7.1 Model IDs and Parameters

At the top of `dr_optimize.cpp`:

```cpp
#define EK(Va,Vg,mu,kp,kvb) (Va/kp * log( 1 + exp( kp*(1/mu + Vg/sqrt(kvb + Va*Va) ) ) ) )

#define NONE        0
#define DIODE       1
#define DUAL_DIODE  2
#define TRIODE      3
#define DUAL_TRIODE 4
#define KOREN_P     5
#define DERK_P      6  // pentode + Ig2
#define DERK_B      7  // pentode + Ig2 + secondary emission
#define DERKE_P     8  // BTet + Ig2
#define DERKE_B     9  // BTet + secondary emission

// Parameter index mapping into p/o arrays
#define KP      0
#define EX      1
#define KG1     2
#define MU      3
#define KVB     4
#define KG2     5
#define AP      6
#define ALPHA_S 7
#define BETA    8
#define LAMDA   9
#define S_      10
#define NU      11
#define OMEGA   12
#define ASUBP   13
```

`IaParams_t` holds:

```cpp
struct IaParams_t {
    float p[14];   // initial / current parameters
    float o[14];   // optimized parameters (scales)
    int   penfn;   // model ID (NONE..DERKE_B)
    int   f;       // index of first parameter to optimize
    int   n;       // number of parameters (exclusive upper index)
    double (*errFun)(const gsl_vector *x, void * param);
    bool  section_b;   // for dual triodes/diodes
    int   VaSteps, VsSteps, VgSteps;
    QList<results_t> * data;
};
```

There is a function pointer table for the Ia functions:

```cpp
typedef float (*IaCalc_t)(IaParams_t *, float, float, float);
IaCalc_t iaFunc[10] = {
    0,
    &DiIa,   // 1 DIODE
    &DiIa,   // 2 DUAL_DIODE
    &TriIa,  // 3 TRIODE
    &TriIa,  // 4 DUAL_TRIODE
    &KpIa,   // 5 KOREN_P
    &DpIa,   // 6 DERK_P
    &DBIa,   // 7 DERK_B
    &DEPIa,  // 8 DERKE_P
    &DEBIa   // 9 DERKE_B
};
```

### 7.2 Optimization Flow (Optimize)

```cpp
void dr_optimize::Optimize(QList<results_t> * dataSet,
                           int Penfun, int Weight,
                           int VaSteps_, int VsSteps_, int VgSteps_)
{
    if (dataSet->length() < 5) return;

    IbParams.penfn = IaParams.penfn = Penfun;
    IbParams.data = IaParams.data = dataSet;

    // Zero p[] and o[]
    for (int i = 0; i < 13; i++) {
        IbParams.p[i] = IaParams.p[i] = IbParams.o[i] = IaParams.o[i] = 0;
    }

    IbParams.VaSteps = IaParams.VaSteps = VaSteps_;
    IbParams.VsSteps = IaParams.VsSteps = VsSteps_;
    IbParams.VgSteps = IaParams.VgSteps = VgSteps_;

    int n = IaParams.data->length();

    switch (Penfun) {
        case DIODE:       // 1
        case DUAL_DIODE:  // 2
        case TRIODE:      // 3
        case DUAL_TRIODE: // 4
        case KOREN_P:     // 5
        case DERK_P:      // 6
        case DERK_B:      // 7
        case DERKE_P:     // 8
        case DERKE_B:     // 9
            // Each case sets initial guesses (TriodeInit, EstimateKG2/AP/AB, etc.)
            // then calls OptAll() with an appropriate error function, then
            // uses findLinear() to compute gm/ra/mu from the final model.
    }
}
```

Each model case is a sequence of:

1. Triode seed (`TriodeInit`) or direct guesses.
2. One or more optimization passes over subsets of parameters via `OptAll`.
3. A final `findLinear` that uses the fitted model to compute gm/ra/mu per data point.

### 7.3 Triode Initialization (TriodeInit)

`TriodeInit()` derives initial estimates for `MU`, `EX`, `KG1`, `KP`, and `KVB` from the **triode-connected section** of the data.

- The triode section is the first `(VaSteps+1)*(VgSteps+1)` points; for pentode models this assumes the pre-sweep where `Va == Vs`.

**Steps:**

1. **Find `IaMax` over the triode section.**
2. **Estimate MU**: find points where Ia crosses `IaMax / 20` (about 5 % of max) and build a list of `(Vg_cross, Va_cross)`; average the slope `-ΔVa / ΔVg` to get `mu`.
3. **Estimate EX**: build a log–log plot
   - x = ln(Va/mu + Vg)
   - y = ln(Ia/1000)
   - average slope gives exponent `EX`.
4. **Estimate KG1**: average intercepts; `KG1 = mean(exp(EX * x - y))`.
5. **Estimate KP**: from data with Va > 100 V and non-zero Vg, create pairs
   - x = 1/mu + Vg/Va
   - y = ln(Ia/1000 * KG1) / EX
   - average slope gives `KP`.
6. **Estimate KVB**: for points where `KP*(1/MU + Vg/Va) > 2`, approximate
   - `a = Vg / ( [Ia/1000 * KG1]^(1/EX) / Va - 1/MU )`
   - `b = a^2 - Va^2`
   - accumulate `b` to find `KVB` ≈ mean(b).

This closely follows Koren’s triode method plus extensions.

### 7.4 Derk Pentode Parameter Estimators

These mirror Derk Reefman’s paper **sections 10.2.x**.

#### 7.4.1 EstimateKG2()

```cpp
void dr_optimize::EstimateKG2() {
    IaParams.p[KG2] = 0;
    int k = 0;
    for (i=0; i< data->length(); i++) {
        if (data[i].Va >= 100 && data[i].Ia != -1) {
            k++;
            IaParams.p[KG2] += IaParams.p[KG1] * data[i].Ia / data[i].Is;
        }
    }
    if (k>0) IaParams.p[KG2] /= k; else IaParams.p[KG2] = IaParams.p[KG1]*3;
}
```

Idea:

- For sufficiently high Va (≥100 V), approximate `KG2` from the ratio of anode to screen current.
- If no valid points, default `KG2 ≈ 3 * KG1`.

#### 7.4.2 EstimateAP()

```cpp
void dr_optimize::EstimateAP() {
    // sec 10.2.2: use two high-Va points differing only in Va
    int h = IaParams.VaSteps-1 + (IaParams.VaSteps+1) * (IaParams.VgSteps+1);
    while (data[h].Ia == -1) h -= 1;
    float Ga = (data[h].Ia - data[h-1].Ia)/1000
               / (data[h].Va - data[h-1].Va);
    IaParams.p[AP] = IaParams.p[KG1] * Ga /
                     pow(EK(data[h].Vs, data[h].Vg,
                          MU, KP, KVB), EX);
}
```

Interpretation:

- `AP` is the slope factor controlling linear Va dependence in Derk’s model.
- `Ga` is the derivative dIa/dVa at high voltage.

#### 7.4.3 EstimateAB() (Derk Pentode without E-type knee)

```cpp
void dr_optimize::EstimateAB() {
    QList<muPair_t> estList;
    for (int i = triodeSectionEnd; i < data->length(); i++) {
        if (data[i].Va < (data[1].Va + 5)) {  // low Va region
            pair.x = data[i].Va;
            float Ipk = pow(EK(data[i].Vs, data[i].Vg, MU, KP, KVB), EX);
            pair.y = 1 / ( KG2 * data[i].Is / Ipk / 1000 - 1 );
            estList.append(pair);
        }
    }
    // Fit straight line y = a*x + b
    ...
    ALPHA_S = 1/b;
    BETA    = a/b;
}
```

This corresponds to Reefman’s derivation of the **screen-limited** correction factor near Va ≈ 0.

#### 7.4.4 EstimateABE() (DerkE exponential knee)

```cpp
void dr_optimize::EstimateABE() {
    QList<muPair_t> estList;
    for (int i = triodeSectionEnd; i < data->length(); i++) {
        if (Va small and Ia != -1) {
            pair.x = pow(Va, 1.5);
            float Ipk = pow(EK(Vs, Vg, MU, KP, KVB), EX);
            float y = KG2 * Is / Ipk / 1000 - 1;
            y = (y > 0 ? y : 0.1);
            pair.y = log(y);
            estList.append(pair);
        }
    }
    // Fit y = a*x + b
    ALPHA_S = exp(b);
    BETA    = pow(-a, 2/3);
}
```

This corresponds to the DerkE E-type knee with `exp(-(φ·Va)^{3/2})` behaviour.

#### 7.4.5 EstimateVW() (Secondary Emission Region)

This is more involved; it inspects the **second derivative** of screen current vs Va to detect a **local maximum** (typical of secondary emission) and fits a line to locate the “virtual wall” parameters `NU` and `OMEGA`.

Key idea:

- For each (Vs, Vg) grid, compute differences of Is between adjacent Va points.
- Detect sign changes in the slope indicating a maximum of Is.
- Build a `slopeList` of `(Vg, Va - Vs/Lambda)` pairs around such maxima.
- Fit a line to derive `NU` and `OMEGA`.

Note the final override:

```cpp
IaParams.p[NU]    = -h;
IaParams.p[OMEGA] = -l;
IaParams.p[NU]    = 5;
IaParams.p[OMEGA] = 50;
```

So the code currently hard-codes NU and OMEGA to 5 and 50 after estimation, likely as a stabilizing choice.

### 7.5 Optimization Engine (OptAll)

`OptAll()` wraps the GSL Nelder–Mead simplex minimizer:

```cpp
void dr_optimize::OptAll(QByteArray name) {
    int n = IaParams.data->length();
    int num_params = IaParams.n - IaParams.f;
    if (n < num_params) return;

    // Copy p[] into o[]
    for (int i=0; i <= ASUBP; i++) IaParams.o[i] = IaParams.p[i];

    gsl_vector *x     = gsl_vector_alloc(num_params);
    gsl_vector *steps = gsl_vector_alloc(num_params);
    gsl_vector_set_all(x,     1.0);  // scale factors
    gsl_vector_set_all(steps, 0.1);  // step sizes

    gsl_multimin_function f;
    f.f      = IaParams.errFun;      // pointer to static error function
    f.n      = num_params;
    f.params = (void *)&IaParams.p;  // pointer to base parameter array

    const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;
    gsl_multimin_fminimizer *solver = gsl_multimin_fminimizer_alloc(T, num_params);
    gsl_multimin_fminimizer_set(solver, &f, x, steps);

    int iter = 0;
    int status;
    do {
        iter++;
        status = gsl_multimin_fminimizer_iterate(solver);
        if (status) break;
        double size = gsl_multimin_fminimizer_size(solver);
        status = gsl_multimin_test_size(size, SIZE); // SIZE=5e-3
    } while (status == GSL_CONTINUE && iter < max_iter);

    // Final parameters: p[i] *= |x[i-f]| for optimized indices
    for (int i = IaParams.f; i < IaParams.n; i++) {
        IaParams.p[i] = IaParams.o[i] =
            IaParams.p[i] * fabs(gsl_vector_get(solver->x, i - IaParams.f));
    }

    gsl_vector_free(x);
    gsl_vector_free(steps);
    gsl_multimin_fminimizer_free(solver);
}
```

Each error function (`TriErr`, `DpErr`, `DEPErr`, `DBErr`, `DEBErr`) applies the current scaled parameters to compute modeled currents and returns √(sum of squared residuals).

### 7.6 Model Currents: Koren, Derk, DerkE

The actual Ia formulas (`TriIa`, `KpIa`, `DpIa`, `DBIa`, `DEPIa`, `DEBIa`) are implemented later in `dr_optimize.cpp` (not fully pasted here). They closely follow:

- Koren triode / pentode forms using `EK(...)` as the **Koren base current**.
- Derk’s formulas for pentode and pentode+SE (sections 10.x of Theory.pdf), using:
  - `KG1`, `KG2`, `AP`, `ALPHA_S`, `BETA` for pentode behaviour and knee shape.
  - `LAMDA`, `S`, `NU`, `OMEGA`, `ASUBP` for secondary emission.

The mapping to ValveWorkbench is roughly:

- `KP`, `EX`, `KG1`, `MU`, `KVB` ↔ `Kp`, `x`, `Kg1`, `Mu`, `Kvb` in ValveWorkbench.
- `KG2`, `AP`, `ALPHA_S`, `BETA` ↔ `Kg2`, `A`, `Alpha`, `Beta`.
- `LAMDA`, `S`, `NU`, `OMEGA`, `ASUBP` ↔ secondary emission parameters.

`findLinear()` then uses these Ia functions to compute small-signal gm, ra, and mu by numerical differences with ±0.1 V steps in Vg and Va.

---

## 8. Model Fitting and SPICE Export

### 8.1 Fitting from Measured Sweep

When a measurement sweep completes (full data in `dataStore`), uTmax does:

```cpp
optimizer->Optimize(dataStore,
                    ui->TubeType->currentIndex(),
                    1, VaSteps, VsSteps, VgSteps);
updateLcdsWithModel();
RePlot(dataStore);
```

- `TubeType->currentIndex()` selects one of the model IDs (`NONE`, `TRIODE`, `KOREN_P`, `DERK_P`, etc.).
- `Optimize` fills `IaParams.p[]` with the final model parameters.
- `updateLcdsWithModel()` uses `RaCalc`, `GmCalc`, etc. to compute small-signal values at UI-selected biases.
- `RePlot()` overlays modeled curves on the measured data.

### 8.2 Saving SPICE Models

Both `kb_optimize` and `dr_optimize` offer `Save_Spice_Model(QWidget*, QByteArray)` to generate `.cir` files.

- They map the optimized parameters into SPICE subcircuits for use in external simulators.
- This path demonstrates how uTmax chooses parameter ordering and correlated names for Koren / Derk models.

---

## 9. Key Takeaways for ValveWorkbench

- uTmax uses **triode-connected pre-sweeps** to seed triode parameters even for pentode models. This is similar to your desire to use triode fits to initialize pentode fits.
- The Derk / DerkE logic in `dr_optimize` matches the steps in **Reefman’s Theory.pdf**, including:
  - Triode initialization (`TriodeInit`)
  - Estimating `KG2`, `AP`, `ALPHA_S`, `BETA` from Is vs Va behaviour
  - Estimating secondary emission parameters from Ig2 peaks.
- The Koren / Barton `kb_optimize::IaCalc` and `IsCalc` provide another reference implementation of Koren’s triode and pentode models and a screen current model.
- Parameter mappings between uTmax and ValveWorkbench are mostly 1:1, but some names differ (`ALPHA_S` vs `Alpha`, `ASUBP` vs `Ap`, etc.).
- uTmax’s measurement state machine and sweep vector generation (`CreateTestVectors`) are a good template for how to:
  - Integrate heating and warmup sequences.
  - Enforce power and current limits during sweep.
  - Handle triode-connected vs full pentode sweeps with a single code path.

These notes should be a stable reference as you adapt pieces of uTmax’s behavior or math into ValveWorkbench.
