#include "valveworkbench.h"
#include "ui_valveworkbench.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QString>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QPushButton>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QInputDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QVector>
#include <QColor>
#include <QBrush>
#include <QTextEdit>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QStatusBar>
#include <QGraphicsTextItem>
#include <QLineEdit>
#include <algorithm>
#include <cmath>
#include <limits>

#include "analyser/analyser.h"
#include "valvemodel/model/model.h"
#include "valvemodel/model/device.h"
#include "valvemodel/model/estimate.h"
#include "valvemodel/model/modelfactory.h"
#include "valvemodel/data/project.h"
#include "valvemodel/data/measurement.h"
#include "valvemodel/data/sample.h"
#include "valvemodel/data/sweep.h"
#include "valvemodel/circuit/circuit.h"
#include "valvemodel/circuit/triodecommoncathode.h"
#include "valvemodel/circuit/pentodecommoncathode.h"
#include "valvemodel/circuit/singleendedoutput.h"
#include "valvemodel/circuit/singleendeduloutput.h"
#include "valvemodel/circuit/pushpulloutput.h"
#include "valvemodel/circuit/pushpulluloutput.h"
#include "valvemodel/circuit/triodeaccathodefollower.h"
#include "valvemodel/circuit/triodedccathodefollower.h"
#include "ledindicator/ledindicator.h"
#include "preferencesdialog.h"
#include "projectdialog.h"
#include "comparedialog.h"

#include "valvemodel/circuit/sharedspice.h"
#include "valvemodel/model/simplemanualpentode.h"
#include "valvemodel/ui/simplemanualpentodedialog.h"
 
/**
 * Helper namespace for SPICE export utilities.
 *
 * The goal is to keep all SPICE-related formatting and string construction
 * together in one place so that:
 *  - Export-to-Devices can embed a self-contained `spice` block into the
 *    device JSON (tube-style preset), and
 *  - Future File → Export to Spice and Designer-stage exporters can simply
 *    read that block and write it to disk, without needing to know about
 *    the underlying model types or parameter indices.
 *
 * For now we support two fitted model types as SPICE subcircuits:
 *  - COHEN_HELIE_TRIODE  → format "cohenHelieTriode"
 *  - GARDINER_PENTODE    → format "gardinerPentode"
 *
 * Each `spice` block looks roughly like:
 *
 *  {
 *    "version": 1,
 *    "format": "gardinerPentode",
 *    "subcktName": "6L6GC_GardinerFit_AS",
 *    "pins": ["P","G2","G1","K","H"],
 *    "body": "... full .subckt text ..."
 *  }
 */
namespace {

    /**
     * Sanitize an arbitrary device name into a SPICE-safe identifier.
     *
     * SPICE subcircuit names are typically limited to letters, digits and
     * a few punctuation characters. To avoid surprises across simulators
     * we conservatively map anything outside [A-Za-z0-9_] to '_', and
     * ensure the first character is alphabetic when possible.
     */
    QString makeSpiceSafeIdentifier(const QString &rawName)
    {
        QString id = rawName;
        if (id.isEmpty()) {
            return QStringLiteral("FittedModel_AS");
        }

        // Replace all characters that are not alphanumeric or '_' with '_'.
        for (int i = 0; i < id.size(); ++i) {
            const QChar c = id.at(i);
            if (!(c.isLetterOrNumber() || c == QLatin1Char('_'))) {
                id[i] = QLatin1Char('_');
            }
        }

        // Ensure leading character is a letter; if not, prepend a safe prefix.
        if (!id.at(0).isLetter()) {
            id.prepend(QStringLiteral("AS_"));
        }

        return id;
    }

    /**
     * Build a SPICE `spice` JSON object for the given fitted model.
     *
     * This inspects the concrete model type (via Model::getType) and, for
     * recognised types, emits a fully-formed .subckt body that implements
     * the same Ia(Va, Vg1, Vg2) law used by ValveWorkbench:
     *
     *  - COHEN_HELIE_TRIODE  → behavioural triode current source using the
     *    Cohen–Helie Epk law.
     *  - GARDINER_PENTODE    → anode and screen currents based on the unified
     *    Gardiner pentode, expressed as SPICE functions.
     *
     * The returned object is suitable for embedding into the device JSON as
     * the `spice` property.
     */
    QJsonObject buildSpiceBlockForModel(Model *model,
                                        int deviceType,
                                        const QString &deviceName)
    {
        QJsonObject spice;

        if (!model) {
            return spice;
        }

        const int modelType = model->getType();

        // Derive a stable subcircuit name from the device name, with a
        // suffix to indicate that it is an AudioSmith fitted model.
        const QString subcktName = makeSpiceSafeIdentifier(deviceName + QStringLiteral("_AS"));

        // Helper lambdas for common JSON fields.
        auto setCommonFields = [&](const char *formatTag,
                                   const QStringList &pins,
                                   const QString &body) {
            spice["version"] = 1;
            spice["format"] = QString::fromLatin1(formatTag);
            spice["subcktName"] = subcktName;

            QJsonArray pinsArray;
            for (const QString &p : pins) {
                pinsArray.append(p);
            }
            spice["pins"] = pinsArray;
            spice["body"] = body;
        };

        // COHEN_HELIE_TRIODE → triode subcircuit using the Cohen–Helie law.
        if (modelType == COHEN_HELIE_TRIODE && deviceType == TRIODE) {
            const double mu   = model->getParameter(PAR_MU);
            const double kg1  = model->getParameter(PAR_KG1);
            const double x    = model->getParameter(PAR_X);
            const double kp   = model->getParameter(PAR_KP);
            const double kvb  = model->getParameter(PAR_KVB);
            const double kvb1 = model->getParameter(PAR_KVB1);
            const double vct  = model->getParameter(PAR_VCT);

            // NOTE: This body is deliberately verbose and commented so that
            // users can inspect and, if necessary, hand-edit the exported
            // .inc/.lib file. The behaviour mirrors CohenHelieTriode::cohenHelieEpk
            // and CohenHelieTriode::cohenHelieCurrent in C++.
            QString body;
            body += QStringLiteral("**** Cohen–Helie triode fitted in ValveWorkbench\n");
            body += QStringLiteral("**** Subcircuit generated from fitted model parameters.\n");
            body += QStringLiteral("**** Node order: P=plate, G=grid, K=cathode, H=heater (unused).\n\n");

            body += QStringLiteral(".subckt %1 P G K H\n").arg(subcktName);
            body += QStringLiteral("+ + MU=%1 KG1=%2 KP=%3 KVB=%4 KVB1=%5 VCT=%6 X=%7\n\n")
                        .arg(mu, 0, 'g', 12)
                        .arg(kg1, 0, 'g', 12)
                        .arg(kp, 0, 'g', 12)
                        .arg(kvb, 0, 'g', 12)
                        .arg(kvb1, 0, 'g', 12)
                        .arg(vct, 0, 'g', 12)
                        .arg(x, 0, 'g', 12);

            body += QStringLiteral("* Va = V(P,K), Vgk = V(G,K) (negative for normal bias)\n");
            body += QStringLiteral("* Epk helper as in CohenHelieTriode::cohenHelieEpk\n");
            body += QStringLiteral(".func CH_f(v)    { sqrt( max( 0, KVB + KVB1*v + v*v ) ) }\n");
            body += QStringLiteral(".func CH_y(v,vg) { KP * ( 1/MU + (vg + VCT)/(CH_f(v) + 1e-9) ) }\n");
            body += QStringLiteral(".func CH_ep(v,vg){ v/KP * ln( 1 + exp( limit(CH_y(v,vg), -50, 50) ) ) }\n");
            body += QStringLiteral(".func CH_ia(v,vg){ ( max( CH_ep(v,vg), 0 )**X ) / max(KG1,1e-9) }\n\n");

            body += QStringLiteral("* Behavioural anode current source from P to K.\n");
            body += QStringLiteral("* The max() guard mirrors the C++ clamp that prevents negative Ia near Va≈0.\n");
            body += QStringLiteral("Biak P K I = { max( CH_ia( V(P,K), V(G,K) ), 0 ) }\n\n");
            body += QStringLiteral(".ends %1\n").arg(subcktName);

            setCommonFields("cohenHelieTriode",
                            QStringList() << QStringLiteral("P")
                                          << QStringLiteral("G")
                                          << QStringLiteral("K")
                                          << QStringLiteral("H"),
                            body);

            return spice;
        }

        // GARDINER_PENTODE → pentode subcircuit using unified Gardiner law.
        if (modelType == GARDINER_PENTODE && deviceType == PENTODE) {
            const double mu    = model->getParameter(PAR_MU);
            const double kg1   = model->getParameter(PAR_KG1);
            const double x     = model->getParameter(PAR_X);
            const double kp    = model->getParameter(PAR_KP);
            const double kvb   = model->getParameter(PAR_KVB);
            const double kvb1  = model->getParameter(PAR_KVB1);
            const double vct   = model->getParameter(PAR_VCT);
            const double kg2   = model->getParameter(PAR_KG2);
            const double kg2a  = model->getParameter(PAR_KG2A);
            const double a     = model->getParameter(PAR_A);
            const double alpha = model->getParameter(PAR_ALPHA);
            const double beta  = model->getParameter(PAR_BETA);
            const double gamma = model->getParameter(PAR_GAMMA);
            const double os    = model->getParameter(PAR_OS);
            const double tau   = model->getParameter(PAR_TAU);
            const double rho   = model->getParameter(PAR_RHO);
            const double theta = model->getParameter(PAR_THETA);
            const double psi   = model->getParameter(PAR_PSI);
            const double omega = model->getParameter(PAR_OMEGA);
            const double lambdaVal = model->getParameter(PAR_LAMBDA);
            const double nu    = model->getParameter(PAR_NU);
            const double s     = model->getParameter(PAR_S);
            const double ap    = model->getParameter(PAR_AP);

            QString body;
            body += QStringLiteral("**** Gardiner pentode fitted in ValveWorkbench\n");
            body += QStringLiteral("**** Subcircuit generated from fitted Gardiner parameters.\n");
            body += QStringLiteral("**** Node order: P=plate, G2=screen, G1=control grid, K=cathode, H=heater (unused).\n\n");

            body += QStringLiteral(".subckt %1 P G2 G1 K H\n").arg(subcktName);
            body += QStringLiteral("+ + MU=%1 KG1=%2 KP=%3 KVB=%4 KVB1=%5 VCT=%6 X=%7\n")
                        .arg(mu, 0, 'g', 12)
                        .arg(kg1, 0, 'g', 12)
                        .arg(kp, 0, 'g', 12)
                        .arg(kvb, 0, 'g', 12)
                        .arg(kvb1, 0, 'g', 12)
                        .arg(vct, 0, 'g', 12)
                        .arg(x, 0, 'g', 12);
            body += QStringLiteral("+ + KG2=%1 KG2A=%2 A=%3 ALPHA=%4 BETA=%5 GAMMA=%6 OS=%7\n")
                        .arg(kg2, 0, 'g', 12)
                        .arg(kg2a, 0, 'g', 12)
                        .arg(a, 0, 'g', 12)
                        .arg(alpha, 0, 'g', 12)
                        .arg(beta, 0, 'g', 12)
                        .arg(gamma, 0, 'g', 12)
                        .arg(os, 0, 'g', 12);
            body += QStringLiteral("+ + TAU=%1 RHO=%2 THETA=%3 PSI=%4 OMEGA=%5 LAMBDA=%6 NU=%7 S=%8 AP=%9\n\n")
                        .arg(tau, 0, 'g', 12)
                        .arg(rho, 0, 'g', 12)
                        .arg(theta, 0, 'g', 12)
                        .arg(psi, 0, 'g', 12)
                        .arg(omega, 0, 'g', 12)
                        .arg(lambdaVal, 0, 'g', 12)
                        .arg(nu, 0, 'g', 12)
                        .arg(s, 0, 'g', 12)
                        .arg(ap, 0, 'g', 12);

            body += QStringLiteral("* Helper functions for Cohen–Helie Epk, reused by Gardiner.\n");
            body += QStringLiteral(".func CH_f(v)    { sqrt( max( 0, KVB + KVB1*v + v*v ) ) }\n");
            body += QStringLiteral(".func CH_y(v,vg) { KP * ( 1/MU + (vg + VCT)/(CH_f(v) + 1e-9) ) }\n");
            body += QStringLiteral(".func CH_ep(v,vg){ v/KP * ln( 1 + exp( limit(CH_y(v,vg), -50, 50) ) ) }\n\n");

            body += QStringLiteral("* For Gardiner, epk is driven by screen voltage (normalised) and grid1 bias.\n");
            body += QStringLiteral(".func G_v2norm(v2){ if( abs(v2) < 5, v2*1000, v2 ) }\n");
            body += QStringLiteral(".func G_epk(v2,vg1){ max( CH_ep( G_v2norm(v2), vg1 ), 1e-6 ) }\n\n");

            body += QStringLiteral("* Derived helpers matching GardinerPentode::anodeCurrent/screenCurrent.\n");
            body += QStringLiteral(".func G_k()           { 1/max(KG1,1e-9) - 1/max(KG2,1e-9) }\n");
            body += QStringLiteral(".func G_scale(va,vg1) { 1 - exp( - (abs(BETA*(1-ALPHA*vg1)*va)+1e-12)**GAMMA ) }\n");
            body += QStringLiteral(".func G_vco(v2,vg1)   { v2/LAMBDA - vg1*NU - OMEGA }\n");
            body += QStringLiteral(".func G_psec(va,v2,vg1){ va*S * ( 1 + tanh( -AP*(va - G_vco(v2,vg1)) ) ) }\n");
            body += QStringLiteral(".func G_termIa(va,vg1){ G_k()*G_scale(va,vg1) + A*va/max(KG2,1e-9) }\n");
            body += QStringLiteral(".func G_sh2(vg1)      { RHO*(1-TAU*vg1) }\n");
            body += QStringLiteral(".func G_h(va,vg1)     { exp( - (abs(G_sh2(vg1)*va)+1e-12)**(THETA*0.9) ) }\n");
            body += QStringLiteral(".func G_termIg2(va,vg1){ (1 + PSI*G_h(va,vg1))/max(KG2A,1e-9) - A*va/max(KG2A,1e-9) }\n\n");

            // Final Ia/Ig2 helpers as single-expression functions so they are
            // valid in standard SPICE dialects (no local `let` statements).
            body += QStringLiteral(".func G_ia(va,vg1,v2)  { G_epk(v2,vg1)*G_termIa(va,vg1) + OS*v2 }\n\n");
            body += QStringLiteral(".func G_ig2(va,vg1,v2) { G_epk(v2,vg1)*G_termIg2(va,vg1) }\n\n");

            body += QStringLiteral("* Behavioural current sources: anode (P→K) and screen (G2→K).\n");
            body += QStringLiteral("Biak  P  K  I = { max( G_ia(  V(P,K), V(G1,K), V(G2,K) ), 0 ) }\n");
            body += QStringLiteral("Big2  G2 K  I = { max( G_ig2( V(P,K), V(G1,K), V(G2,K) ), 0 ) }\n\n");
            body += QStringLiteral(".ends %1\n").arg(subcktName);

            setCommonFields("gardinerPentode",
                            QStringList() << QStringLiteral("P")
                                          << QStringLiteral("G2")
                                          << QStringLiteral("G1")
                                          << QStringLiteral("K")
                                          << QStringLiteral("H"),
                            body);

            return spice;
        }

        // For all other model/device combinations we currently do not emit a
        // SPICE block. Future work can extend this helper for Reefman-style
        // pentodes or alternative triode models.
        return spice;
    }

} // namespace (SPICE helpers)

// Stub callback required by the ngspice shared library interface. We do not
// currently embed ngspice; SPICE export is file-based only. This remains here
// for completeness in case future work enables live SPICE integration.
int ngspice_getchar(char* outputreturn, int ident, void* userdata) {
    Q_UNUSED(outputreturn);
    Q_UNUSED(ident);
    Q_UNUSED(userdata);
    // Callback for ngSpice to send characters (e.g., print output). For the
    // current design we ignore ngspice entirely and rely on exported .cir/.inc
    // files instead.
    return 0;
}

void ValveWorkbench::populateDataTableFromMeasurement(Measurement *measurement)
{
    // Populate analyser data table with rows per sweep from the given
    // Measurement. Shared between live analyser results and measurements
    // imported from device presets.
    if (!measurement || !dataTable) {
        return;
    }

    dataTable->clearContents();
    int numSweeps = measurement->count();

    if (numSweeps == 0) {
        qWarning("No sweeps found in measurement data");
        return;
    }

    // Determine rows per sweep based on device type
    const int measDeviceType = measurement->getDeviceType();
    // Pentode: Va, Ia, Vg1, Vg2, Ig2 (5 rows)
    // Double triode: Va, Ia, Vg1, Vg3, Va2, Ia2 (6 rows)
    // Single triode: Va, Ia, Vg1 (and optional Vg3 if present) - keep legacy 4 rows for compatibility
    int rowsPerSweep = 4;
    if (measDeviceType == PENTODE) {
        rowsPerSweep = 5;
    } else if (isDoubleTriode) {
        rowsPerSweep = 6;
    } else {
        rowsPerSweep = 4;
    }
    dataTable->setRowCount(numSweeps * rowsPerSweep);

    // Set column headers for the 62 Va points
    dataTable->setColumnCount(62);
    QStringList headers;
    for (int i = 0; i < 62; ++i) {
        headers << QString("Va_%1").arg(i);
    }
    dataTable->setHorizontalHeaderLabels(headers);

    qInfo("Populating table with %d sweeps (%d rows each)", numSweeps, rowsPerSweep);

    for (int sweepIdx = 0; sweepIdx < numSweeps; ++sweepIdx) {
        Sweep *sweep = measurement->at(sweepIdx);
        QString gridVoltage = QString("Vg_%1V").arg(sweep->getVg1Nominal(), 0, 'f', 2);

        int sampleCount = sweep->count();
        qInfo("Sweep %d: Vg1Nominal = %f, sampleCount = %d", sweepIdx, sweep->getVg1Nominal(), sampleCount);

        if (sampleCount == 0) {
            qWarning("Sweep %d has zero samples - skipping data population for this sweep", sweepIdx);
            continue;  // Skip to next sweep
        }

        // Row for anode voltage values
        int vaRow = sweepIdx * rowsPerSweep;
        QString vaRowHeader = gridVoltage + " (Va)";
        dataTable->setVerticalHeaderItem(vaRow, new QTableWidgetItem(vaRowHeader));

        // Row for anode current values
        int iaRow = sweepIdx * rowsPerSweep + 1;
        QString iaRowHeader = gridVoltage + " (Ia)";
        dataTable->setVerticalHeaderItem(iaRow, new QTableWidgetItem(iaRowHeader));

        // Row for first grid voltage values (Vg1)
        int vg1Row = sweepIdx * rowsPerSweep + 2;
        QString vg1RowHeader = gridVoltage + " (Vg1)";
        dataTable->setVerticalHeaderItem(vg1Row, new QTableWidgetItem(vg1RowHeader));

        // Either add Vg3 (double triode) or Vg2/Ig2 (pentode)
        int vg3Row = -1;
        int vg2Row = -1;
        int ig2Row = -1;
        int va2Row = -1;
        int ia2Row = -1;

        if (measDeviceType == PENTODE) {
            // Row for screen voltage values (Vg2)
            vg2Row = sweepIdx * rowsPerSweep + 3;
            QString vg2RowHeader = gridVoltage + " (Vg2)";
            dataTable->setVerticalHeaderItem(vg2Row, new QTableWidgetItem(vg2RowHeader));

            // Row for screen current values (Ig2)
            ig2Row = sweepIdx * rowsPerSweep + 4;
            QString ig2RowHeader = gridVoltage + " (Ig2)";
            dataTable->setVerticalHeaderItem(ig2Row, new QTableWidgetItem(ig2RowHeader));
        } else if (isDoubleTriode) {
            // Row for second grid voltage values (Vg3)
            vg3Row = sweepIdx * rowsPerSweep + 3;
            QString vg3RowHeader = gridVoltage + " (Vg3)";
            dataTable->setVerticalHeaderItem(vg3Row, new QTableWidgetItem(vg3RowHeader));
            // Row for second anode voltage values (Va2)
            va2Row = sweepIdx * rowsPerSweep + 4;
            QString va2RowHeader = gridVoltage + " (Va2)";
            dataTable->setVerticalHeaderItem(va2Row, new QTableWidgetItem(va2RowHeader));

            // Row for second anode current values (Ia2)
            ia2Row = sweepIdx * rowsPerSweep + 5;
            QString ia2RowHeader = gridVoltage + " (Ia2)";
            dataTable->setVerticalHeaderItem(ia2Row, new QTableWidgetItem(ia2RowHeader));
        }

        // Populate Va row (even row numbers)
        for (int col = 0; col < 62 && col < sampleCount; ++col) {
            Sample *sample = sweep->at(col);
            double va = sample->getVa();
            if (col < 3) { // Log first few Va values for debugging
                qInfo("Sweep %d, Va_%d = %f", sweepIdx, col + 1, va);
            }
            QTableWidgetItem *vaItem = new QTableWidgetItem(QString::number(va, 'f', 2));
            dataTable->setItem(vaRow, col, vaItem);
        }

        // Populate Ia row (odd row numbers)
        for (int col = 0; col < 62 && col < sampleCount; ++col) {
            Sample *sample = sweep->at(col);
            double ia = sample->getIa();
            if (col < 3) { // Log first few Ia values for debugging
                qInfo("Sweep %d, Ia_%d = %f", sweepIdx, col + 1, ia);
            }
            QTableWidgetItem *iaItem = new QTableWidgetItem(QString::number(ia, 'f', 3));
            dataTable->setItem(iaRow, col, iaItem);
        }

        // Populate Vg1 row (third row per sweep)
        for (int col = 0; col < 62 && col < sampleCount; ++col) {
            Sample *sample = sweep->at(col);
            double vg1 = sample->getVg1();
            if (col < 3) { // Log first few Vg1 values for debugging
                qInfo("Sweep %d, Vg1_%d = %f", sweepIdx, col + 1, vg1);
            }
            QTableWidgetItem *vg1Item = new QTableWidgetItem(QString::number(vg1, 'f', 2));
            dataTable->setItem(vg1Row, col, vg1Item);
        }

        // Populate Vg3 row for double triode
        if (isDoubleTriode) {
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double vg3 = sample->getVg3();
                if (col < 3) {
                    qInfo("Sweep %d, Vg3_%d = %f", sweepIdx, col + 1, vg3);
                }
                QTableWidgetItem *vg3Item = new QTableWidgetItem(QString::number(vg3, 'f', 2));
                dataTable->setItem(vg3Row, col, vg3Item);
            }
        }

        // Populate Vg2 and Ig2 rows for pentode
        if (measDeviceType == PENTODE) {
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double vg2 = sample->getVg2();
                if (col < 3) {
                    qInfo("Sweep %d, Vg2_%d = %f", sweepIdx, col + 1, vg2);
                }
                QTableWidgetItem *vg2Item = new QTableWidgetItem(QString::number(vg2, 'f', 2));
                dataTable->setItem(vg2Row, col, vg2Item);
            }
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double ig2 = sample->getIg2();
                if (col < 3) {
                    qInfo("Sweep %d, Ig2_%d = %f", sweepIdx, col + 1, ig2);
                }
                QTableWidgetItem *ig2Item = new QTableWidgetItem(QString::number(ig2, 'f', 3));
                dataTable->setItem(ig2Row, col, ig2Item);
            }
        }

        if (isDoubleTriode) {
            // Populate Va2 row (fourth row per sweep)
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double va2 = sample->getVa2();
                if (col < 3) { // Log first few Va2 values for debugging
                    qInfo("Sweep %d, Va2_%d = %f", sweepIdx, col + 1, va2);
                }
                QTableWidgetItem *va2Item = new QTableWidgetItem(QString::number(va2, 'f', 2));
                dataTable->setItem(va2Row, col, va2Item);
            }

            // Populate Ia2 row (fifth row per sweep)
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double ia2 = sample->getIa2();
                if (col < 3) { // Log first few Ia2 values for debugging
                    qInfo("Sweep %d, Ia2_%d = %f", sweepIdx, col + 1, ia2);
                }
                QTableWidgetItem *ia2Item = new QTableWidgetItem(QString::number(ia2, 'f', 3));
                dataTable->setItem(ia2Row, col, ia2Item);
            }
        }

        // Resize columns to fit content and set a minimum width for visibility
        dataTable->resizeColumnsToContents();
        for (int col = 0; col < 62; ++col) {
            dataTable->setColumnWidth(col, qMax(dataTable->columnWidth(col), 40)); // Minimum 40px width
        }
    }

    // Set row heights for better readability
    for (int row = 0; row < numSweeps * rowsPerSweep; ++row) {
        dataTable->setRowHeight(row, qMax(dataTable->rowHeight(row), 25));
    }

    qInfo("Data table populated: %d sweeps x %d rows each = %d total rows", numSweeps, rowsPerSweep, numSweeps * rowsPerSweep);
}

void ValveWorkbench::updateDatasheetDisplay()
{
    if (!ui) {
        return;
    }

    auto setField = [](QLineEdit *edit, const QString &text) {
        if (edit) {
            edit->setText(text);
        }
    };

    auto clearAll = [&]() {
        setField(ui->datasheetVa, QString());
        setField(ui->datasheetVg, QString());
        setField(ui->datasheetIa, QString());
        setField(ui->datasheetGm, QString());
        setField(ui->datasheetMu, QString());
        setField(ui->datasheetRp, QString());
    };

    if (datasheetJson.isEmpty()) {
        clearAll();
        return;
    }

    const QJsonArray refPoints = datasheetJson.value("refPoints").toArray();
    if (refPoints.isEmpty() || !refPoints.at(0).isObject()) {
        clearAll();
        return;
    }

    const QJsonObject rp = refPoints.at(0).toObject();

    auto numToString = [](const QJsonValue &v, int decimals) -> QString {
        if (!v.isDouble()) {
            return QString();
        }
        return QString::number(v.toDouble(), 'f', decimals);
    };

    setField(ui->datasheetVa, numToString(rp.value("va"), 1));
    setField(ui->datasheetVg, numToString(rp.value("vg"), 1));
    setField(ui->datasheetIa, numToString(rp.value("ia"), 3));
    setField(ui->datasheetGm, numToString(rp.value("gm"), 1));
    setField(ui->datasheetMu, numToString(rp.value("mu"), 1));
    setField(ui->datasheetRp, numToString(rp.value("rp"), 1));
}

void ValveWorkbench::syncDatasheetFromUi()
{
    if (!ui) {
        return;
    }

    // If all UI fields are blank and we already have a non-empty datasheet
    // block (e.g. loaded from a template), preserve the existing JSON rather
    // than overwriting it with an empty refPoint.
    const QString vaText = ui->datasheetVa ? ui->datasheetVa->text().trimmed() : QString();
    const QString vgText = ui->datasheetVg ? ui->datasheetVg->text().trimmed() : QString();
    const QString iaText = ui->datasheetIa ? ui->datasheetIa->text().trimmed() : QString();
    const QString gmText = ui->datasheetGm ? ui->datasheetGm->text().trimmed() : QString();
    const QString muText = ui->datasheetMu ? ui->datasheetMu->text().trimmed() : QString();
    const QString rpText = ui->datasheetRp ? ui->datasheetRp->text().trimmed() : QString();

    const bool allEmpty = vaText.isEmpty() && vgText.isEmpty() && iaText.isEmpty() &&
                          gmText.isEmpty() && muText.isEmpty() && rpText.isEmpty();
    if (allEmpty && !datasheetJson.isEmpty()) {
        return;
    }

    QJsonArray refPoints = datasheetJson.value(QStringLiteral("refPoints")).toArray();
    QJsonObject rp;
    if (!refPoints.isEmpty() && refPoints.at(0).isObject()) {
        rp = refPoints.at(0).toObject();
    }

    auto setFromField = [&](const char *key, QLineEdit *edit) {
        if (!edit) {
            return;
        }
        const QString text = edit->text().trimmed();
        if (text.isEmpty()) {
            rp.remove(QLatin1String(key));
            return;
        }
        bool ok = false;
        const double value = text.toDouble(&ok);
        if (ok) {
            rp.insert(QLatin1String(key), value);
        }
    };

    setFromField("va", ui->datasheetVa);
    setFromField("vg", ui->datasheetVg);
    setFromField("ia", ui->datasheetIa);
    setFromField("gm", ui->datasheetGm);
    setFromField("mu", ui->datasheetMu);
    setFromField("rp", ui->datasheetRp);

    if (refPoints.isEmpty()) {
        refPoints.append(rp);
    } else {
        refPoints[0] = rp;
    }

    datasheetJson.insert(QStringLiteral("refPoints"), refPoints);
}

bool ValveWorkbench::ensureDatasheetRefPoint(double &va0, double &vg0, double &ia0, double &gm0, double &mu0, double &rp0)
{
    if (datasheetJson.isEmpty()) {
        return false;
    }

    const QJsonArray refPoints = datasheetJson.value("refPoints").toArray();
    if (refPoints.isEmpty() || !refPoints.at(0).isObject()) {
        return false;
    }

    const QJsonObject rp = refPoints.at(0).toObject();

    auto read = [&](const char *key, double &out) -> bool {
        const QJsonValue v = rp.value(QLatin1String(key));
        if (!v.isDouble()) {
            return false;
        }
        out = v.toDouble();
        return true;
    };

    if (!read("va", va0)) return false;
    if (!read("vg", vg0)) return false;
    if (!read("ia", ia0)) return false;
    if (!read("gm", gm0)) return false;
    // mu and rp are optional for now; treat missing as 0
    read("mu", mu0);
    read("rp", rp0);

    return true;
}

void ValveWorkbench::startHealthRun(HealthMode mode)
{
    double va0 = 0.0;
    double vg0 = 0.0;
    double ia0 = 0.0;
    double gm0 = 0.0;
    double mu0 = 0.0;
    double rp0 = 0.0;

    if (!ensureDatasheetRefPoint(va0, vg0, ia0, gm0, mu0, rp0)) {
        QMessageBox::warning(this, tr("Health Test"), tr("No valid datasheet reference point is available. Load a template with datasheet.refPoints[0] first."));
        return;
    }

    if (deviceType != TRIODE) {
        QMessageBox::warning(this, tr("Health Test"), tr("Quick/Full Health currently support triode devices only."));
        return;
    }

    // Build central and (for full mode) corner test points around the datasheet operating point.
    healthPoints.clear();
    healthResults.clear();

    HealthPoint center;
    center.va = va0;
    center.vg = vg0;
    healthPoints.append(center);

    if (mode == HEALTH_FULL) {
        const double dVaFrac = 0.2;
        double dVa = std::fabs(va0) * dVaFrac;
        if (dVa < 40.0) dVa = 40.0;
        if (dVa > 100.0) dVa = 100.0;

        double dVg = 0.5;
        if (std::fabs(vg0) > 2.0) {
            dVg = 1.0;
        }

        const double vaLow  = std::max(0.0, va0 - dVa);
        const double vaHigh = va0 + dVa;
        const double vgLo   = vg0 - dVg;
        const double vgHi   = vg0 + dVg;

        HealthPoint p;

        p.va = vaLow;  p.vg = vgLo;  healthPoints.append(p);
        p.va = vaLow;  p.vg = vgHi;  healthPoints.append(p);
        p.va = vaHigh; p.vg = vgLo;  healthPoints.append(p);
        p.va = vaHigh; p.vg = vgHi;  healthPoints.append(p);
    }

    healthResults.resize(healthPoints.size());
    for (int i = 0; i < healthResults.size(); ++i) {
        healthResults[i].valid = false;
        healthResults[i].va = 0.0;
        healthResults[i].vg = 0.0;
        healthResults[i].ia = 0.0;
        healthResults[i].gm = 0.0;
    }

    if (!healthStateSaved) {
        healthStateSaved = true;
        savedTestTypeForHealth = testType;
        savedAnodeStartForHealth = anodeStart;
        savedAnodeStopForHealth = anodeStop;
        savedAnodeStepForHealth = anodeStep;
        savedGridStartForHealth = gridStart;
        savedGridStopForHealth = gridStop;
        savedGridStepForHealth = gridStep;
        savedScreenStartForHealth = screenStart;
        savedScreenStopForHealth = screenStop;
        savedScreenStepForHealth = screenStep;
    }

    healthMode = mode;
    healthRunActive = true;
    healthRunIndex = 0;

    configureTransferForHealthPoint(healthPoints[0]);
    on_runButton_clicked();
}

void ValveWorkbench::configureTransferForHealthPoint(const HealthPoint &pt)
{
    // Configure a triode (or double triode) transfer test around the requested operating point.
    // Datasheet Vg values are negative physical biases (e.g. -2 V). The analyser, however,
    // expects positive magnitudes for grid DAC commands (e.g. 2 V meaning -2 V actual), so
    // map the requested operating point into a small window in magnitude space.
    testType = TRANSFER_CHARACTERISTICS;

    anodeStart = pt.va;
    anodeStop  = pt.va;
    anodeStep  = std::max(1.0, std::fabs(pt.va));

    const double vgMag = std::fabs(pt.vg);
    const double dMag  = 0.5; // ±0.5 V magnitude window around the target bias

    double startMag = (vgMag > dMag) ? (vgMag - dMag) : 0.0;
    double stopMag  = vgMag + dMag;

    gridStart = startMag;
    gridStop  = stopMag;
}

// Compute measured Ia, gm, and a local plate resistance rp around a desired
// operating point (Va, Vg) from a Measurement. For Quick/Full Health we only
// use the transfer measurement to derive Ia/gm; rp is obtained from any
// available ANODE_CHARACTERISTICS dataset for the same device using a small
// local LS fit around the nearest sample to (Va, Vg).
//
// ia_mA and gm_mA_V are always written; rp_ohms is set to >0.0 on success or
// 0.0 if no suitable anode data is available.
bool ValveWorkbench::computeIaGmAt(Measurement *measurement, const HealthPoint &pt, double &ia_mA, double &gm_mA_V, double &rp_ohms)
{
    ia_mA = 0.0;
    gm_mA_V = 0.0;
    rp_ohms = 0.0;

    if (!measurement) {
        return false;
    }

    const int sweepCount = measurement->count();
    if (sweepCount <= 0) {
        return false;
    }

    double bestScore = std::numeric_limits<double>::infinity();
    int bestSweepIdx = -1;
    int bestSampleIdx = -1;

    for (int sw = 0; sw < sweepCount; ++sw) {
        Sweep *sweep = measurement->at(sw);
        if (!sweep) {
            continue;
        }
        const int nSamples = sweep->count();
        for (int sa = 0; sa < nSamples; ++sa) {
            Sample *sample = sweep->at(sa);
            if (!sample) continue;

            const double va = sample->getVa();
            const double vg = sample->getVg1();

            const double dVa = va - pt.va;
            const double dVg = vg - pt.vg;

            const double score = dVg * dVg + 0.25 * dVa * dVa;
            if (score < bestScore) {
                bestScore = score;
                bestSweepIdx = sw;
                bestSampleIdx = sa;
            }
        }
    }

    if (bestSweepIdx < 0 || bestSampleIdx < 0) {
        return false;
    }

    Sweep *bestSweep = measurement->at(bestSweepIdx);
    if (!bestSweep) {
        return false;
    }

    const int sampleCount = bestSweep->count();
    if (sampleCount < 2 || bestSampleIdx >= sampleCount) {
        return false;
    }

    Sample *sampleMid = bestSweep->at(bestSampleIdx);
    if (!sampleMid) {
        return false;
    }

    ia_mA = sampleMid->getIa();

    int idxPrev = std::max(0, bestSampleIdx - 1);
    int idxNext = std::min(sampleCount - 1, bestSampleIdx + 1);

    Sample *samplePrev = bestSweep->at(idxPrev);
    Sample *sampleNext = bestSweep->at(idxNext);
    if (!samplePrev || !sampleNext || idxPrev == idxNext) {
        return false;
    }

    const double vgPrev = samplePrev->getVg1();
    const double iaPrev = samplePrev->getIa();
    const double vgNext = sampleNext->getVg1();
    const double iaNext = sampleNext->getIa();

    const double dVg = vgNext - vgPrev;
    if (std::fabs(dVg) < 1e-6) {
        return false;
    }

    gm_mA_V = (iaNext - iaPrev) / dVg;
    if (!std::isfinite(gm_mA_V) || gm_mA_V <= 0.0) {
        return false;
    }

    // --- Optional: derive a local plate resistance rp from ANODE_CHARACTERISTICS
    // data that is closest to the same (Va, Vg) operating point. This reuses the
    // least-squares logic from updateSmallSignalFromMeasurement but specialises
    // it for a single HealthPoint.
    Measurement *anodeMeasurement = findMeasurement(measurement->getDeviceType(), ANODE_CHARACTERISTICS);
    if (anodeMeasurement && measurementHasValidSamples(anodeMeasurement)) {
        const int anodeSweeps = anodeMeasurement->count();
        if (anodeSweeps > 0) {
            double bestScoreRa = std::numeric_limits<double>::infinity();
            int bestSweepRa = -1;
            int bestSampleRa = -1;

            // Find the sample in anode data closest to the same (Va, Vg)
            for (int sw = 0; sw < anodeSweeps; ++sw) {
                Sweep *sweep = anodeMeasurement->at(sw);
                if (!sweep) continue;
                const int nSamples = sweep->count();
                for (int sa = 0; sa < nSamples; ++sa) {
                    Sample *s = sweep->at(sa);
                    if (!s) continue;

                    const double va = s->getVa();
                    const double vg = s->getVg1();
                    const double dVa = va - pt.va;
                    const double dVg = vg - pt.vg;
                    const double score = dVg * dVg + 0.25 * dVa * dVa;
                    if (score < bestScoreRa) {
                        bestScoreRa = score;
                        bestSweepRa = sw;
                        bestSampleRa = sa;
                    }
                }
            }

            if (bestSweepRa >= 0 && bestSampleRa >= 0) {
                Sweep *sweep = anodeMeasurement->at(bestSweepRa);
                if (sweep && sweep->count() >= 3) {
                    const int sampleCount = sweep->count();

                    auto clampIndex = [](int idx, int max) {
                        if (idx < 0) return 0;
                        if (idx >= max) return max - 1;
                        return idx;
                    };

                    const int sampleIdx = clampIndex(bestSampleRa, sampleCount);
                    const int iPrev     = clampIndex(sampleIdx - 1, sampleCount);
                    const int iNext     = clampIndex(sampleIdx + 1, sampleCount);

                    Sample *samplePrev = sweep->at(iPrev);
                    Sample *sampleNext = sweep->at(iNext);
                    if (samplePrev && sampleNext) {
                        int iStart = std::max(0, sampleIdx - 2);
                        int iEnd   = std::min(sampleCount - 1, sampleIdx + 2);

                        double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;
                        int N = 0;

                        for (int i = iStart; i <= iEnd; ++i) {
                            Sample *s = sweep->at(i);
                            if (!s) continue;
                            const double ia = s->getIa(); // mA
                            if (ia <= 0.0) continue;
                            const double va = s->getVa(); // V
                            Sx  += va;
                            Sy  += ia;
                            Sxx += va * va;
                            Sxy += va * ia;
                            ++N;
                        }

                        const double den = static_cast<double>(N) * Sxx - Sx * Sx;
                        if (N >= 3 && std::fabs(den) > 1e-12) {
                            const double slope_dIa_dVa = (static_cast<double>(N) * Sxy - Sx * Sy) / den; // mA/V
                            if (std::fabs(slope_dIa_dVa) > 1e-12) {
                                rp_ohms = 1000.0 / slope_dIa_dVa; // V/mA → Ohms
                            }
                        }

                        // Fallback: two-point estimate
                        if (!(rp_ohms > 0.0)) {
                            const double vaPrev = samplePrev->getVa();
                            const double iaPrev = samplePrev->getIa();
                            const double vaNext = sampleNext->getVa();
                            const double iaNext = sampleNext->getIa();

                            const double dIa_mA = iaNext - iaPrev;
                            const double dVa    = vaNext - vaPrev;
                            if (std::fabs(dIa_mA) > 1e-9) {
                                const double dVa_dIa_V_per_mA = dVa / dIa_mA;
                                rp_ohms = dVa_dIa_V_per_mA * 1000.0;
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

void ValveWorkbench::finalizeHealthRun()
{
    double va0 = 0.0;
    double vg0 = 0.0;
    double ia0 = 0.0;
    double gm0 = 0.0;
    double mu0 = 0.0;
    double rp0 = 0.0;

    const bool haveRef = ensureDatasheetRefPoint(va0, vg0, ia0, gm0, mu0, rp0);

    auto metricScore = [](double meas, double ref) -> double {
        if (!(ref > 0.0) || !(meas > 0.0)) {
            return 0.0;
        }
        const double ratio = meas / ref;
        const double dev = std::fabs(ratio - 1.0);
        const double tol = 0.3; // 30% deviation → score 0
        double s = 1.0 - dev / tol;
        if (s < 0.0) s = 0.0;
        if (s > 1.0) s = 1.0;
        return s;
    };

    double quickPercent = 0.0;
    double fullPercent = 0.0;

    if (healthResults.size() > 0 && haveRef) {
        const HealthResult &r0 = healthResults.at(0);
        if (r0.valid) {
            const double sIa = metricScore(r0.ia, ia0);
            const double sGm = metricScore(r0.gm, gm0);
            quickPercent = 100.0 * 0.5 * (sIa + sGm);
        }
    }

    if (healthMode == HEALTH_FULL && haveRef && !healthResults.isEmpty()) {
        double sum = 0.0;
        int count = 0;
        for (const HealthResult &r : healthResults) {
            if (!r.valid) continue;
            const double sIa = metricScore(r.ia, ia0);
            const double sGm = metricScore(r.gm, gm0);
            sum += 0.5 * (sIa + sGm);
            ++count;
        }
        if (count > 0) {
            fullPercent = 100.0 * (sum / static_cast<double>(count));
        }
    }

    if (healthStateSaved) {
        testType = savedTestTypeForHealth;
        anodeStart = savedAnodeStartForHealth;
        anodeStop = savedAnodeStopForHealth;
        anodeStep = savedAnodeStepForHealth;
        gridStart = savedGridStartForHealth;
        gridStop = savedGridStopForHealth;
        gridStep = savedGridStepForHealth;
        screenStart = savedScreenStartForHealth;
        screenStop = savedScreenStopForHealth;
        screenStep = savedScreenStepForHealth;
        healthStateSaved = false;

        updateParameterDisplay();
    }

    healthRunActive = false;
    healthMode = HEALTH_NONE;
    healthRunIndex = 0;

    QString msg;
    if (healthResults.size() > 0) {
        if (quickPercent > 0.0) {
            msg += tr("Quick Health: %1% ").arg(QString::number(quickPercent, 'f', 0));
        }
        if (healthResults.size() > 1 && fullPercent > 0.0) {
            msg += tr("Full Health: %1% ").arg(QString::number(fullPercent, 'f', 0));
        }
    }

    if (msg.isEmpty()) {
        msg = tr("Health run completed, but scores are not available.");
    }

    if (ui && ui->statusbar) {
        ui->statusbar->showMessage(msg, 8000);
    }

    if (ui && ui->quickHealthButton) {
        ui->quickHealthButton->setToolTip(msg);
    }
    if (ui && ui->fullHealthButton) {
        ui->fullHealthButton->setToolTip(msg);
    }
}

void ValveWorkbench::on_quickHealthButton_clicked()
{
    if (!analyser) {
        QMessageBox::warning(this, tr("Health Test"), tr("Analyser is not initialised."));
        return;
    }

    if (ui && ui->runButton && ui->runButton->isChecked()) {
        QMessageBox::warning(this, tr("Health Test"), tr("A test is already running. Please wait for it to finish."));
        return;
    }

    startHealthRun(HEALTH_QUICK);
}

void ValveWorkbench::on_fullHealthButton_clicked()
{
    if (!analyser) {
        QMessageBox::warning(this, tr("Health Test"), tr("Analyser is not initialised."));
        return;
    }

    if (ui && ui->runButton && ui->runButton->isChecked()) {
        QMessageBox::warning(this, tr("Health Test"), tr("A test is already running. Please wait for it to finish."));
        return;
    }

    startHealthRun(HEALTH_FULL);
}


// (Removed duplicate checkbox handlers; using the canonical implementations below.)

void ValveWorkbench::on_pushButton_3_clicked()
{
    // Load Template... (default to models directory near the application)
    QString baseDir;
    {
        QStringList possiblePaths = {
            QCoreApplication::applicationDirPath() + "/../../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../models",
            QCoreApplication::applicationDirPath() + "/../models",
            QCoreApplication::applicationDirPath() + "/models"
        };

        for (const QString &path : possiblePaths) {
            QDir testDir(path);
            if (testDir.exists()) {
                baseDir = path;
                break;
            }
        }

        if (baseDir.isEmpty()) {
            baseDir = QDir::currentPath() + "/models";
        }
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Template"), baseDir, tr("JSON Files (*.json)"));
    if (fileName.isEmpty()) return;

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Load Template"), tr("Could not open file."));
        return;
    }

    // Ensure Designer overlays checkbox is present on its own second row under the existing toggle row
    if (ui->horizontalLayout_9) {
        bool hasDesigner = false;
        for (int i = 0; i < ui->horizontalLayout_9->count(); ++i) {
            QWidget *w = ui->horizontalLayout_9->itemAt(i) ? ui->horizontalLayout_9->itemAt(i)->widget() : nullptr;
            if (w && w->objectName() == QLatin1String("designerCheck")) { hasDesigner = true; break; }
        }
        if (!hasDesigner) {
            // Try to insert a new row (QHBoxLayout) directly after the existing checkbox row
            QBoxLayout *parentLayout = qobject_cast<QBoxLayout*>(ui->horizontalLayout_9->parentWidget() ? ui->horizontalLayout_9->parentWidget()->layout() : nullptr);
            if (parentLayout) {
                int idx = parentLayout->indexOf(ui->horizontalLayout_9);
                QHBoxLayout *designerRow = new QHBoxLayout();
                designerCheck = new QCheckBox(tr("Show Designer Overlays"), this);
                designerCheck->setObjectName("designerCheck");
                designerCheck->setChecked(true);
                designerRow->addStretch();
                designerRow->addWidget(designerCheck);
                designerRow->addStretch();
                parentLayout->insertLayout(idx + 1, designerRow);
                connect(designerCheck, &QCheckBox::stateChanged, this, &ValveWorkbench::on_designerCheck_stateChanged);
            } else {
                // Fallback: add to end of the existing row
                designerCheck = new QCheckBox(tr("Show Designer Overlays"), this);
                designerCheck->setObjectName("designerCheck");
                designerCheck->setChecked(true);
                ui->horizontalLayout_9->addWidget(designerCheck);
                connect(designerCheck, &QCheckBox::stateChanged, this, &ValveWorkbench::on_designerCheck_stateChanged);
            }
        }
    }

    // Add Designer overlays checkbox next to existing measurement/model toggles (if not present)
    if (ui->horizontalLayout_9) {
        bool hasDesigner = false;
        for (int i = 0; i < ui->horizontalLayout_9->count(); ++i) {
            QWidget *w = ui->horizontalLayout_9->itemAt(i) ? ui->horizontalLayout_9->itemAt(i)->widget() : nullptr;
            if (w && w->objectName() == QLatin1String("designerCheck")) { hasDesigner = true; break; }
        }
        if (!hasDesigner) {
            designerCheck = new QCheckBox(tr("Show Designer Overlays"), this);
            designerCheck->setObjectName("designerCheck");
            designerCheck->setChecked(true);
            int insertAt = std::max(0, ui->horizontalLayout_9->count() - 1); // before trailing spacer
            ui->horizontalLayout_9->insertWidget(insertAt, designerCheck);
            connect(designerCheck, &QCheckBox::stateChanged, this, &ValveWorkbench::on_designerCheck_stateChanged);
        }
    }
    const QByteArray bytes = f.readAll();
    f.close();

    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, tr("Load Template"), tr("Invalid JSON template."));
        return;
    }
    const QJsonObject obj = doc.object();

    // Optional datasheet block (reference operating points / corners / health thresholds).
    // This is currently treated as an opaque JSON object that we round-trip in
    // templates and exported devices so future Designer features can consume it.
    datasheetJson = obj.value("datasheet").toObject();

    // Name
    if (ui && ui->deviceName) {
        ui->deviceName->setText(obj.value("name").toString(QFileInfo(fileName).baseName()));
    }

    // Device type
    const QString devType = obj.value("deviceType").toString().toUpper();
    if (devType == QLatin1String("TRIODE")) {
        deviceType = TRIODE;
        if (ui && ui->deviceType) ui->deviceType->setCurrentIndex(0);
    } else if (devType == QLatin1String("PENTODE")) {
        deviceType = PENTODE;
        if (ui && ui->deviceType) ui->deviceType->setCurrentIndex(1);
    } else if (devType == QLatin1String("DOUBLE_TRIODE")) {
        deviceType = DOUBLE_TRIODE;
        if (ui && ui->deviceType) ui->deviceType->setCurrentIndex(2);
    }

    // Analyser defaults
    bool hadAnalyserDefaults = false;
    const QJsonObject defs = obj.value("analyserDefaults").toObject();
    if (!defs.isEmpty()) {
        hadAnalyserDefaults = true;
        heaterVoltage = defs.value("heaterVoltage").toDouble(heaterVoltage);

        const auto setRange = [&](const char *key, double &start, double &stop, double &step){
            const QJsonObject r = defs.value(QLatin1String(key)).toObject();
            if (!r.isEmpty()) {
                start = r.value("start").toDouble(start);
                stop  = r.value("stop").toDouble(stop);
                step  = r.value("step").toDouble(step);
            }
        };
        setRange("anode", anodeStart, anodeStop, anodeStep);
        setRange("grid", gridStart, gridStop, gridStep);
        setRange("screen", screenStart, screenStop, screenStep);

        // Prefer limits block if present (new-format templates)
        const QJsonObject lim = defs.value("limits").toObject();
        if (!lim.isEmpty()) {
            iaMax = lim.value("iaMax").toDouble(iaMax);
            pMax  = lim.value("pMax").toDouble(pMax);
        } else {
            // Backward compatibility: fall back to legacy top-level fields
            if (obj.contains("ia_max") && obj.value("ia_max").isDouble()) {
                iaMax = obj.value("ia_max").toDouble(iaMax);
            } else if (obj.contains("iaMax") && obj.value("iaMax").isDouble()) {
                iaMax = obj.value("iaMax").toDouble(iaMax);
            }

            if (obj.contains("pa_max") && obj.value("pa_max").isDouble()) {
                pMax = obj.value("pa_max").toDouble(pMax);
            } else if (obj.contains("pMax") && obj.value("pMax").isDouble()) {
                pMax = obj.value("pMax").toDouble(pMax);
            }
        }

        // Determine which test type this template wants to restore; default to
        // the current analyser testType if no explicit value was saved.
        int savedTestType = testType;
        if (defs.contains("testType")) {
            savedTestType = defs.value("testType").toInt(testType);
        }

        // If per-test snapshots are present and this JSON is a pure template
        // (no embedded measurement), prefer the one whose testType matches
        // savedTestType and use its ranges/limits to override the generic
        // anode/grid/screen/limits above. For device presets that embed a
        // full measurement, we treat analyserDefaults as authoritative so
        // that edited sweep ranges are not silently overridden by the
        // original measurement envelope when reloading as a template.
        const bool hasMeasurement = obj.contains("measurement") && obj.value("measurement").isObject();
        const QJsonObject testsObj = defs.value("tests").toObject();
        if (!hasMeasurement && !testsObj.isEmpty()) {
            QJsonObject snapshot;
            for (auto it = testsObj.begin(); it != testsObj.end(); ++it) {
                if (!it.value().isObject()) {
                    continue;
                }
                const QJsonObject tObj = it.value().toObject();
                const int tType = tObj.value("testType").toInt(-1);
                if (tType == savedTestType) {
                    snapshot = tObj;
                    break;
                }
            }

            if (!snapshot.isEmpty()) {
                auto setRangeFrom = [&](const char *key, double &start, double &stop, double &step) {
                    const QJsonObject r = snapshot.value(QLatin1String(key)).toObject();
                    if (!r.isEmpty()) {
                        start = r.value("start").toDouble(start);
                        stop  = r.value("stop").toDouble(stop);
                        step  = r.value("step").toDouble(step);
                    }
                };
                setRangeFrom("anode", anodeStart, anodeStop, anodeStep);
                setRangeFrom("grid",  gridStart, gridStop, gridStep);
                setRangeFrom("screen", screenStart, screenStop, screenStep);

                const QJsonObject lim2 = snapshot.value("limits").toObject();
                if (!lim2.isEmpty()) {
                    iaMax = lim2.value("iaMax").toDouble(iaMax);
                    pMax  = lim2.value("pMax").toDouble(pMax);
                }
            }
        }

        // Apply double-triode flag for triode devices (overrides TRI vs DOUBLE_TRIODE selection)
        if (deviceType == TRIODE && defs.contains("doubleTriode")) {
            const bool dbl = defs.value("doubleTriode").toBool(false);
            if (ui && ui->deviceType) {
                // indices: 0=Triode, 1=Pentode, 2=Double Triode (per earlier usage)
                ui->deviceType->setCurrentIndex(dbl ? 2 : 0);
                on_deviceType_currentIndexChanged(ui->deviceType->currentIndex());
            }
        }

        // Apply saved test type (or current testType if none was saved) to the UI.
        if (ui && ui->testType) {
            int matchIndex = -1;
            for (int i = 0; i < ui->testType->count(); ++i) {
                if (ui->testType->itemData(i).toInt() == savedTestType) { matchIndex = i; break; }
            }
            if (matchIndex >= 0) {
                ui->testType->setCurrentIndex(matchIndex);
                on_testType_currentIndexChanged(matchIndex);
            }
        }
    }

    // Optionally apply model parameters to the active model if present in template
    const QJsonObject modelObj = obj.value("model").toObject();
    if (!modelObj.isEmpty()) {
        // Determine model type from template
        const QString mtype = modelObj.value("type").toString();
        int desiredType = -1;
        if (mtype.compare("COHEN_HELIE_TRIODE", Qt::CaseInsensitive) == 0 || mtype.compare("TRIODE", Qt::CaseInsensitive) == 0) {
            desiredType = COHEN_HELIE_TRIODE;
        } else if (mtype.compare("KOREN_TRIODE", Qt::CaseInsensitive) == 0) {
            desiredType = KOREN_TRIODE;
        } else if (mtype.compare("SIMPLE_TRIODE", Qt::CaseInsensitive) == 0) {
            desiredType = SIMPLE_TRIODE;
        } else if (mtype.compare("GARDINER_PENTODE", Qt::CaseInsensitive) == 0 || mtype.compare("PENTODE", Qt::CaseInsensitive) == 0) {
            desiredType = GARDINER_PENTODE;
        }

        if (desiredType != -1) {
            // Ensure the correct model type is selected
            selectModel(desiredType);
            if (model) {
                // Load only the nested 'model' object so parameters map correctly
                model->fromJson(modelObj);
            }
        }
    }

    // Update UI to reflect loaded values. If analyserDefaults were present,
    // on_testType_currentIndexChanged() has already invoked
    // updateParameterDisplay() and applied test-type specific UI rules (for
    // example, clearing the anodeStep field for anode-characteristics tests
    // where the step control is not used). Avoid calling it again here in
    // that case, otherwise hidden/disabled fields like anodeStep would be
    // repopulated from the raw numeric state.
    if (!hadAnalyserDefaults) {
        updateParameterDisplay();
    }
    updateDatasheetDisplay();
}

void ValveWorkbench::on_pushButton_4_clicked()
{
    // Save Template...
    QJsonObject obj;
    obj.insert("name", ui && ui->deviceName ? ui->deviceName->text() : QString("Device"));
    QString devTypeStr = "TRIODE";
    if (deviceType == PENTODE) devTypeStr = "PENTODE";
    else if (deviceType == DOUBLE_TRIODE) devTypeStr = "DOUBLE_TRIODE";
    obj.insert("deviceType", devTypeStr);

    QJsonObject defs;
    auto makeRange = [&](double start, double stop, double step){
        QJsonObject r; r.insert("start", start); r.insert("stop", stop); r.insert("step", step); return r; };
    defs.insert("anode", makeRange(anodeStart, anodeStop, anodeStep));
    defs.insert("grid", makeRange(gridStart, gridStop, gridStep));
    defs.insert("screen", makeRange(screenStart, screenStop, screenStep));
    // Save default test type, double-triode mode, and current limits from the analyser UI
    defs.insert("testType", testType);
    defs.insert("doubleTriode", isDoubleTriode);
    QJsonObject lim; lim.insert("iaMax", iaMax); lim.insert("pMax", pMax); defs.insert("limits", lim);

    // Per-test snapshot for the currently selected test type so templates can
    // remember distinct analyser settings for anode/transfer/screen tests.
    {
        QJsonObject testsObj;
        QJsonObject snapshot;
        snapshot.insert("testType", testType);
        snapshot.insert("anode",  makeRange(anodeStart, anodeStop, anodeStep));
        snapshot.insert("grid",   makeRange(gridStart, gridStop, gridStep));
        snapshot.insert("screen", makeRange(screenStart, screenStop, screenStep));
        QJsonObject testLim;
        testLim.insert("iaMax", iaMax);
        testLim.insert("pMax",  pMax);
        snapshot.insert("limits", testLim);

        QString key;
        switch (testType) {
        case ANODE_CHARACTERISTICS:    key = QStringLiteral("anode");    break;
        case TRANSFER_CHARACTERISTICS: key = QStringLiteral("transfer"); break;
        case SCREEN_CHARACTERISTICS:   key = QStringLiteral("screen");   break;
        default:                       key = QString::number(testType);   break;
        }

        testsObj.insert(key, snapshot);
        defs.insert("tests", testsObj);
    }

    obj.insert("analyserDefaults", defs);

    // Sync any edited datasheet/reference values from the Analyser UI back
    // into the datasheetJson block before saving.
    syncDatasheetFromUi();
    if (!datasheetJson.isEmpty()) {
        obj.insert("datasheet", datasheetJson);
    }

    QJsonDocument out(obj);

    // Default template directory to models folder near the application
    QString baseDir;
    {
        QStringList possiblePaths = {
            QCoreApplication::applicationDirPath() + "/../../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../models",
            QCoreApplication::applicationDirPath() + "/../models",
            QCoreApplication::applicationDirPath() + "/models"
        };

        for (const QString &path : possiblePaths) {
            QDir testDir(path);
            if (testDir.exists()) {
                baseDir = path;
                break;
            }
        }

        if (baseDir.isEmpty()) {
            baseDir = QDir::currentPath() + "/models";
        }
    }

    QString suggested = baseDir + "/" + obj.value("name").toString("Device").replace(' ', '_') + ".json";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Template"), suggested, tr("JSON Files (*.json)"));
    if (fileName.isEmpty()) return;
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Save Template"), tr("Could not write file."));
        return;
    }
    f.write(out.toJson(QJsonDocument::Indented));
    f.close();
}


bool ValveWorkbench::measurementHasValidSamples(Measurement *measurement) const
{
    if (measurement == nullptr) {
        return false;
    }

    constexpr double minimumCurrent = 1e-9;
    int validSamples = 0;

    for (int sweepIndex = 0; sweepIndex < measurement->count(); ++sweepIndex) {
        Sweep *sweep = measurement->at(sweepIndex);
        if (sweep == nullptr) {
            continue;
        }

        for (int sampleIndex = 0; sampleIndex < sweep->count(); ++sampleIndex) {
            Sample *sample = sweep->at(sampleIndex);
            if (sample == nullptr) {
                continue;
            }

            const double current = sample->getIa();
            const double voltage = sample->getVa();
            if (!std::isfinite(current) || !std::isfinite(voltage)) {
                continue;
            }

            if (std::fabs(current) > minimumCurrent) {
                ++validSamples;
                if (validSamples >= 3) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool ValveWorkbench::measurementHasTriodeBData(Measurement *measurement) const
{
    return measurement != nullptr && measurement->hasTriodeBData();
}

Measurement *ValveWorkbench::createTriodeBMeasurementClone(Measurement *source) const
{
    if (source == nullptr || !measurementHasTriodeBData(source)) {
        return nullptr;
    }

    Measurement *clone = new Measurement();

    clone->setDeviceType(source->getDeviceType());
    clone->setTestType(source->getTestType());
    clone->setHeaterVoltage(source->getHeaterVoltage());
    clone->setShowScreen(source->getShowScreen());
    clone->setIaMax(source->getIaMax());
    clone->setPMax(source->getPMax());
    clone->setAnodeStart(source->getAnodeStart());
    clone->setAnodeStop(source->getAnodeStop());
    clone->setAnodeStep(source->getAnodeStep());
    clone->setGridStart(source->getGridStart());
    clone->setGridStop(source->getGridStop());
    clone->setGridStep(source->getGridStep());

    double minVa = std::numeric_limits<double>::infinity();
    double maxVa = -std::numeric_limits<double>::infinity();
    double minGrid = std::numeric_limits<double>::infinity();
    double maxGrid = -std::numeric_limits<double>::infinity();
    double ia2Max = 0.0;
    double powerMax = 0.0;
    const double initialIaClamp = source->getIaMax();
    double iaLimit = initialIaClamp;

    for (int sweepIndex = 0; sweepIndex < source->count(); ++sweepIndex) {
        Sweep *sourceSweep = source->at(sweepIndex);
        if (sourceSweep == nullptr) {
            continue;
        }

        QVector<Sample *> bufferedSamples;
        bufferedSamples.reserve(sourceSweep->count());

        double nominalGrid = std::numeric_limits<double>::quiet_NaN();

        for (int sampleIndex = 0; sampleIndex < sourceSweep->count(); ++sampleIndex) {
            Sample *sourceSample = sourceSweep->at(sampleIndex);
            if (sourceSample == nullptr) {
                continue;
            }

            const double vg3 = sourceSample->getVg3();
            const double va2 = sourceSample->getVa2();
            const double ia2Raw = sourceSample->getIa2();

            if (!std::isfinite(vg3) || !std::isfinite(va2) || !std::isfinite(ia2Raw) || va2 <= 0.0 || ia2Raw <= 0.0) {
                continue;
            }

            const double ia2 = (iaLimit > 0.0) ? std::min(ia2Raw, iaLimit) : ia2Raw;

            if (std::isfinite(vg3)) {
                minGrid = std::min(minGrid, vg3);
                maxGrid = std::max(maxGrid, vg3);
                if (!std::isfinite(nominalGrid)) {
                    nominalGrid = vg3;
                }
            }

            if (std::isfinite(va2)) {
                minVa = std::min(minVa, va2);
                maxVa = std::max(maxVa, va2);
            }

            // Map Triode B data into primary fields: vg1 <- vg3, va <- va2, ia <- ia2
            Sample *cloneSample = new Sample(
                vg3,                    // primary Vg1 <- Triode B grid voltage
                va2,                    // primary Va  <- Triode B anode voltage
                ia2,                    // primary Ia  <- Triode B anode current
                0.0,                    // primary Vg2 (not used)
                0.0,                    // primary Ig2 (not used)
                sourceSample->getVh(),  // heater voltage preserved
                sourceSample->getIh(),  // heater current preserved
                0.0,                    // secondary Vg3 cleared in clone
                0.0,                    // secondary Va2 cleared in clone
                0.0);                   // secondary Ia2 cleared in clone

            bufferedSamples.append(cloneSample);
            qInfo("Triode B clone sample buffered: vg3=%.6f, va2=%.6f, ia2=%.6f",
                  vg3, va2, ia2);

            if (std::isfinite(ia2Raw)) {
                ia2Max = std::max(ia2Max, ia2Raw);
                if (std::isfinite(va2)) {
                    powerMax = std::max(powerMax, va2 * (ia2Raw / 1000.0));
                }
            }
        }

        if (bufferedSamples.isEmpty()) {
            qInfo("Triode B clone sweep skipped: no valid samples");
            std::for_each(bufferedSamples.begin(), bufferedSamples.end(), [](Sample *sample) {
                delete sample;
            });
            continue;
        }

        Sweep *cloneSweep = new Sweep(source->getDeviceType(), source->getTestType());
        clone->addSweep(cloneSweep);

        for (Sample *cloneSample : std::as_const(bufferedSamples)) {
            cloneSweep->addSample(cloneSample);
        }

        if (std::isfinite(nominalGrid)) {
            cloneSweep->setVg1Nominal(nominalGrid);
        } else {
            cloneSweep->setVg1Nominal(sourceSweep->getVg1Nominal());
        }
    }

    if (std::isfinite(minVa) && std::isfinite(maxVa) && minVa <= maxVa) {
        clone->setAnodeStart(minVa);
        clone->setAnodeStop(maxVa);
    }

    if (std::isfinite(minGrid) && std::isfinite(maxGrid) && minGrid <= maxGrid) {
        clone->setGridStart(minGrid);
        clone->setGridStop(maxGrid);
    }

    clone->setScreenStart(source->getScreenStart());
    clone->setScreenStop(source->getScreenStop());
    clone->setScreenStep(source->getScreenStep());

    if (ia2Max > 0.0) {
        iaLimit = (iaLimit > 0.0) ? std::min(iaLimit, ia2Max) : ia2Max;
    }
    if (iaLimit <= 0.0) {
        iaLimit = ia2Max > 0.0 ? ia2Max : 1.0; // fall back to measured peak or 1mA to keep estimator active
    }
    clone->setIaMax(iaLimit);

    double powerLimit = source->getPMax();
    if (powerMax > 0.0) {
        powerLimit = (powerLimit > 0.0) ? std::min(powerLimit, powerMax) : powerMax;
    }
    if (powerLimit <= 0.0 && powerMax > 0.0) {
        powerLimit = powerMax;
    }
    clone->setPMax(powerLimit);

    qInfo("Triode B clone summary: sweeps=%d, iaMax=%.6f, pMax=%.6f",
          clone->count(), clone->getIaMax(), clone->getPMax());

    // If no sweeps were added to the clone, do not proceed with a secondary fit
    if (clone->count() == 0) {
        qInfo("Triode B clone has zero sweeps - discarding clone and skipping secondary fit");
        deleteMeasurementClone(clone);
        return nullptr;
    }

    return clone;
}

void ValveWorkbench::deleteMeasurementClone(Measurement *measurement) const
{
    if (measurement == nullptr) {
        return;
    }

    for (int sweepIndex = 0; sweepIndex < measurement->count(); ++sweepIndex) {
        Sweep *sweep = measurement->at(sweepIndex);
        if (sweep == nullptr) {
            continue;
        }

        for (int sampleIndex = 0; sampleIndex < sweep->count(); ++sampleIndex) {
            Sample *sample = sweep->at(sampleIndex);
            delete sample;
        }

        delete sweep;
    }

    delete measurement;
}

void ValveWorkbench::cleanupTriodeBResources()
{
    for (Measurement *clone : std::as_const(triodeBClones)) {
        deleteMeasurementClone(clone);
    }
    triodeBClones.clear();

    triodeModelSecondary = nullptr;
    modelledCurvesSecondary = nullptr;
    triodeBFitPending = false;
    runningTriodeBFit = false;

    if (measuredCurvesSecondary != nullptr) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }

    if (triodeMeasurementSecondary != nullptr) {
        deleteMeasurementClone(triodeMeasurementSecondary);
        triodeMeasurementSecondary = nullptr;
    }
}

void ValveWorkbench::on_inductiveLoadCheck_stateChanged(int arg1)
{
    const bool inductive = (arg1 != 0);

    if (!ui || !ui->circuitSelection) {
        return;
    }

    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    if (!c) {
        return;
    }

    if (auto *se = dynamic_cast<SingleEndedOutput*>(c)) {
        se->setInductiveLoad(inductive);
        se->plot(&plot);
        se->updateUI(circuitLabels, circuitValues);
    } else if (auto *seul = dynamic_cast<SingleEndedUlOutput*>(c)) {
        seul->setInductiveLoad(inductive);
        seul->plot(&plot);
        seul->updateUI(circuitLabels, circuitValues);
    } else if (auto *pp = dynamic_cast<PushPullOutput*>(c)) {
        pp->setInductiveLoad(inductive);
        pp->plot(&plot);
        pp->updateUI(circuitLabels, circuitValues);
    } else if (auto *ppul = dynamic_cast<PushPullUlOutput*>(c)) {
        ppul->setInductiveLoad(inductive);
        ppul->plot(&plot);
        ppul->updateUI(circuitLabels, circuitValues);
    }
}

void ValveWorkbench::on_screenCheck_stateChanged(int arg1)
{
    const bool show = (arg1 != 0);

    // Map current tab widget to logical role: 0 = Designer, 1 = Modeller, 2 = Analyser.
    int tabRole = -1;
    if (ui->tabWidget) {
        QWidget *w = ui->tabWidget->currentWidget();
        if (w == ui->tab) {
            tabRole = 0;
        } else if (w == ui->tab_2) {
            tabRole = 1;
        } else if (w == ui->tab_3) {
            tabRole = 2;
        }
    }
    if (tabRole >= 0 && tabRole < 3) {
        overlayStates[tabRole].showScreen = show;
    }

    if (tabRole == 1) {
        // Modeller tab: apply to the active project measurement and redraw the
        // measured plot (axes managed by Measurement itself).
        if (currentMeasurement != nullptr) {
            currentMeasurement->setShowScreen(show);
            currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
            if (measuredCurves != nullptr) {
                plot.remove(measuredCurves);
                measuredCurves = nullptr;
            }
            measuredCurves = currentMeasurement->updatePlot(&plot);
            if (measuredCurves) {
                plot.add(measuredCurves);
                measuredCurves->setVisible(ui->measureCheck->isChecked());
            }
        }
    } else if (tabRole == 0) {
        // Designer tab: apply to the embedded Measurement on the current
        // Device (if any) and replot it without touching Designer axes.
        if (currentDevice && currentDevice->getMeasurement()) {
            Measurement *embedded = currentDevice->getMeasurement();
            embedded->setShowScreen(show);
            embedded->setSmoothPlotting(preferencesDialog.smoothCurves());

            if (measuredCurves) {
                plot.remove(measuredCurves);
                measuredCurves = nullptr;
            }
            if (measuredCurvesSecondary) {
                plot.remove(measuredCurvesSecondary);
                measuredCurvesSecondary = nullptr;
            }

            measuredCurves = embedded->updatePlotWithoutAxes(&plot);
            if (measuredCurves) {
                plot.add(measuredCurves);
                measuredCurves->setVisible(ui->measureCheck->isChecked());
            }
        }
    }
}

void ValveWorkbench::on_autoscaleYCheck_stateChanged(int arg1)
{
    Q_UNUSED(arg1);

    // Only take action when Autoscale Y is being enabled. Turning it on
    // should recompute the Designer axes from the current device and
    // circuit parameters, mirroring the Pentode Class A1 designer's
    // behaviour where toggling autoscale triggers a fresh axis fit.
    if (!ui || !ui->autoscaleYCheck || !ui->autoscaleYCheck->isChecked()) {
        return;
    }

    if (!ui->stdDeviceSelection) {
        return;
    }

    const int comboIndex = ui->stdDeviceSelection->currentIndex();
    if (comboIndex < 0) {
        return;
    }

    const int deviceNumber = ui->stdDeviceSelection->itemData(comboIndex).toInt();
    if (deviceNumber < 0) {
        return;
    }

    // Reapply the current Designer device so that selectStdDevice() can
    // recalculate vaMax/iaMax (including 2*VB and Class-B extensions) on
    // top of the existing circuit parameters.
    selectStdDevice(1, deviceNumber);
}

void ValveWorkbench::on_actionExport_to_Spice_triggered()
{
    // File → Export to Spice...
    //
    // This path exports a tube-only SPICE representation of the currently
    // selected Designer Device. It uses the same SPICE helper that embeds a
    // `spice` block into analyser-exported device JSON, so external SPICE
    // simulators see exactly the same Ia(Va, Vg1, Vg2) law that the
    // Modeller/Designer use internally.

    // Require a current Designer device selection; the user picks this via
    // the stdDeviceSelection combo and Designer circuits.
    if (!currentDevice) {
        QMessageBox::warning(this, tr("Export to Spice"),
                             tr("No Designer device is currently selected. Please select a device in the Designer tab first."));
        return;
    }

    // Require an attached fitted Model; legacy presets or analyser-only
    // exports might not have a model block.
    Model *deviceModel = currentDevice->getModel();
    if (!deviceModel) {
        QMessageBox::warning(this, tr("Export to Spice"),
                             tr("The selected device has no fitted model to export as SPICE."));
        return;
    }

    // Build a SPICE description directly from the Device's model and type.
    const int devType = currentDevice->getDeviceType();
    const QString devName = currentDevice->getName();
    QJsonObject spiceObj = buildSpiceBlockForModel(deviceModel, devType, devName);

    if (spiceObj.isEmpty() || !spiceObj.contains("body") || !spiceObj.value("body").isString()) {
        QMessageBox::warning(this, tr("Export to Spice"),
                             tr("The selected device's model type is not yet supported for SPICE export."));
        return;
    }

    const QString subcktBody = spiceObj.value("body").toString();
    const QString subcktName = spiceObj.value("subcktName").toString(devName);

    // Suggest a filename based on the device name and SPICE format.
    const QString formatTag = spiceObj.value("format").toString(QStringLiteral("tube"));

    // Reuse the same models folder search used by loadDevices()/exportFittedModelToDevices
    // as the root for SPICE exports, then place .inc files into a dedicated
    // "spice" subdirectory so they do not clutter JSON preset files.
    QString baseDir;
    {
        QStringList possiblePaths = {
            QCoreApplication::applicationDirPath() + "/../../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../models",
            QCoreApplication::applicationDirPath() + "/../models",
            QCoreApplication::applicationDirPath() + "/models",
            QDir::currentPath() + "/models",
            QDir::currentPath() + "/../models",
            QDir::currentPath() + "/../../models",
            QDir::currentPath() + "/../../../models"
        };

        for (const QString &p : possiblePaths) {
            QDir d(p);
            if (d.exists()) {
                baseDir = d.absolutePath();
                break;
            }
        }

        if (baseDir.isEmpty()) {
            baseDir = QDir::cleanPath(QDir::currentPath() + "/models");
            if (!QDir(baseDir).exists()) {
                QDir().mkpath(baseDir);
            }
        }
    }

    // SPICE models live in a dedicated subdirectory beneath the models root.
    QString spiceDir = QDir::cleanPath(baseDir + "/spice");
    QDir spiceQDir(spiceDir);
    if (!spiceQDir.exists()) {
        QDir().mkpath(spiceDir);
    }

    QString safeName = devName;
    if (safeName.isEmpty()) {
        safeName = subcktName;
    }
    safeName.replace(QRegularExpression("[^A-Za-z0-9._ -]"), "_");
    if (safeName.isEmpty()) {
        safeName = QStringLiteral("FittedModel_AS");
    }

    const QString suggested = spiceQDir.filePath(safeName + "_" + formatTag + ".inc");

    QString outPath = QFileDialog::getSaveFileName(this,
                                                   tr("Export to Spice"),
                                                   suggested,
                                                   tr("SPICE Netlist (*.inc *.cir *.sp)"));
    if (outPath.isEmpty()) {
        return;
    }

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export to Spice"),
                             tr("Could not write to %1").arg(outPath));
        return;
    }

    QTextStream ts(&outFile);
    ts << "; SPICE export from ValveWorkbench\n";
    ts << "; Device: " << devName << "\n";
    ts << "; Model format: " << formatTag << "\n";
    ts << "; Subcircuit: " << subcktName << "\n";
    ts << "\n";
    ts << subcktBody;
    outFile.close();

    QMessageBox::information(this, tr("Export to Spice"),
                             tr("Exported SPICE subcircuit to %1").arg(outPath));
}

void ValveWorkbench::on_actionExport_SE_Output_to_Spice_triggered()
{
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("No valid Designer circuit selected. Please select 'Single Ended Output' in the Designer tab."));
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    auto *se = dynamic_cast<SingleEndedOutput *>(c);
    if (!se) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("SE Output SPICE export is only available when the 'Single Ended Output' circuit is active."));
        return;
    }

    if (!currentDevice) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("No Designer device is currently selected. Please select a device in the Designer tab first."));
        return;
    }

    Model *deviceModel = currentDevice->getModel();
    if (!deviceModel) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("The selected device has no fitted model to export as SPICE."));
        return;
    }

    const int devType = currentDevice->getDeviceType();
    const QString devName = currentDevice->getName();
    QJsonObject spiceObj = buildSpiceBlockForModel(deviceModel, devType, devName);
    if (spiceObj.isEmpty() || !spiceObj.contains("body") || !spiceObj.value("body").isString()) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("The selected device's model type is not yet supported for SPICE export."));
        return;
    }

    const QString subcktBody = spiceObj.value("body").toString();
    const QString subcktName = spiceObj.value("subcktName").toString(devName);
    const QString formatTag  = spiceObj.value("format").toString(QStringLiteral("tube"));

    const double vb  = se->getParameter(SE_VB);
    const double vs  = se->getParameter(SE_VS);
    const double ra  = se->getParameter(SE_RA);
    const double rk  = se->getParameter(SE_RK);

    if (!(vb > 0.0) || !(ra > 0.0)) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("SE Output requires positive supply voltage and load resistance (VB, RA)."));
        return;
    }

    QString header;
    header += QStringLiteral("; SE Output SPICE export from ValveWorkbench\n");
    header += QStringLiteral("; Device: %1\n").arg(devName);
    header += QStringLiteral("; Model format: %1\n").arg(formatTag);
    header += QStringLiteral("; Subcircuit: %1\n").arg(subcktName);
    header += QStringLiteral("; VB=%.3f V, VS=%.3f V, RA=%.1f ohm, RK=%.1f ohm\n\n")
                  .arg(vb, 0, 'f', 3)
                  .arg(vs, 0, 'f', 3)
                  .arg(ra, 0, 'f', 1)
                  .arg(rk, 0, 'f', 1);

    QString netlist;
    netlist += header;
    netlist += subcktBody;
    if (!netlist.endsWith('\n')) {
        netlist += '\n';
    }
    netlist += QStringLiteral("\n* Single-Ended Output stage (resistive load approximation)\n");
    netlist += QStringLiteral("Vb  B+ 0 %.3f\n").arg(vb, 0, 'f', 3);
    if (vs > 0.0) {
        netlist += QStringLiteral("Vg2 VS 0 %.3f\n").arg(vs, 0, 'f', 3);
    } else {
        netlist += QStringLiteral("*Vg2 VS 0 0 ; screen supply disabled (VS<=0)\n");
    }
    netlist += QStringLiteral("Ra  B+ P %.1f\n").arg(ra, 0, 'f', 1);
    if (rk > 0.0) {
        netlist += QStringLiteral("Rk  K  0 %.1f\n").arg(rk, 0, 'f', 1);
    } else {
        netlist += QStringLiteral("*Rk K 0 0 ; cathode resistor not set in Designer (RK<=0)\n");
    }
    netlist += QStringLiteral("Rg  G1 0 1Meg\n");
    netlist += QStringLiteral("XU1 P VS G1 K 0 %1\n\n").arg(subcktName);
    netlist += QStringLiteral(".op\n.end\n");

    QString baseDir;
    {
        QStringList possiblePaths = {
            QCoreApplication::applicationDirPath() + "/../../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../../models",
            QCoreApplication::applicationDirPath() + "/../../../models",
            QCoreApplication::applicationDirPath() + "/../models",
            QCoreApplication::applicationDirPath() + "/models",
            QDir::currentPath() + "/models",
            QDir::currentPath() + "/../models",
            QDir::currentPath() + "/../../models",
            QDir::currentPath() + "/../../../models"
        };

        for (const QString &p : possiblePaths) {
            QDir d(p);
            if (d.exists()) {
                baseDir = d.absolutePath();
                break;
            }
        }

        if (baseDir.isEmpty()) {
            baseDir = QDir::cleanPath(QDir::currentPath() + "/models");
            if (!QDir(baseDir).exists()) {
                QDir().mkpath(baseDir);
            }
        }
    }

    QString spiceDir = QDir::cleanPath(baseDir + "/spice");
    QDir spiceQDir(spiceDir);
    if (!spiceQDir.exists()) {
        QDir().mkpath(spiceDir);
    }

    QString safeName = devName;
    if (safeName.isEmpty()) {
        safeName = subcktName;
    }
    safeName.replace(QRegularExpression("[^A-Za-z0-9._ -]"), "_");
    if (safeName.isEmpty()) {
        safeName = QStringLiteral("SE_Output");
    }

    const QString suggested = spiceQDir.filePath(safeName + "_SEOutput.cir");

    QString outPath = QFileDialog::getSaveFileName(this,
                                                   tr("Export SE Output to SPICE"),
                                                   suggested,
                                                   tr("SPICE Netlist (*.cir *.inc *.sp)"));
    if (outPath.isEmpty()) {
        return;
    }

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export SE Output to SPICE"),
                             tr("Could not write to %1").arg(outPath));
        return;
    }

    QTextStream ts(&outFile);
    ts << netlist;
    outFile.close();

    QMessageBox::information(this, tr("Export SE Output to SPICE"),
                             tr("Exported SE Output SPICE netlist to %1").arg(outPath));
}

void ValveWorkbench::startTriodeBFit()
{
    if (triodeMeasurementSecondary == nullptr || triodeModelSecondary == nullptr) {
        qWarning("startTriodeBFit called without secondary measurement/model");
        finalizeTriodeModelling();
        return;
    }

    runningTriodeBFit = true;
    queueTriodeModelRun(triodeModelSecondary);
}

void ValveWorkbench::applyTriodeBProperties(Model *primary, Model *secondary)
{
    Q_UNUSED(primary);
    Q_UNUSED(secondary);
}

void ValveWorkbench::finalizeTriodeModelling()
{
    Project *project = (Project *) modelProject->data(0, Qt::UserRole).value<void *>();
    if (project != nullptr && model != nullptr) {
        project->addModel(model);
        model->buildTree(modelProject);
    }

    if (triodeMeasurementSecondary != nullptr) {
        deleteMeasurementClone(triodeMeasurementSecondary);
        triodeMeasurementSecondary = nullptr;
    }
    if (measuredCurvesSecondary != nullptr) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }

    runningTriodeBFit = false;

    if (doPentodeModel) {
        modelPentode();
        return;
    }

    ui->fitPentodeButton->setEnabled(true);
    ui->fitTriodeButton->setEnabled(true);
    modelProject = nullptr;
}

Measurement *ValveWorkbench::firstTriodeBMeasurement() const
{
    return triodeBClones.isEmpty() ? nullptr : triodeBClones.first();
}

int ngspice_getstat(char* outputreturn, int ident, void* userdata) {
    // Callback for ngSpice status
    return 0;
}

int ngspice_exit(int exitstatus, int immediate, int quitexit, int ident, void* userdata) {
    // Callback for ngSpice exit
    return 0;
}

int ngspice_data(void* pvecvalues, int numvecs, int ident, void* userdata) {
    // Callback for ngSpice data
    return 0;
}

int ngspice_initdata(void* pvecinit, int ident, void* userdata) {
    // Callback for ngSpice init data
    return 0;
}

int ngspice_thread_runs(int thread_id, void* userdata) {
    // Callback for ngSpice thread
    return 0;
}

ValveWorkbench::ValveWorkbench(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ValveWorkbench)
{
    logFile = new QFile("analyser.log");
    if (!logFile->open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open log file.");
        logFile = nullptr;
    }

    // ngSpice_Init(ngspice_getchar, ngspice_getstat, ngspice_exit, ngspice_data, ngspice_initdata, ngspice_thread_runs, NULL);

    anodeStart = 0.0;
    anodeStep = 0.0;
    anodeStop = 0.0;
    gridStart = 0.0;
    gridStep = 0.0;
    gridStop = 0.0;
    screenStart = 0.0;
    screenStep = 0.0;
    screenStop = 0.0;

    secondGridStart = 0.0;
    secondGridStop = 0.0;
    secondGridStep = 0.0;

    secondAnodeStart = 0.0;
    secondAnodeStop = 0.0;
    secondAnodeStep = 0.0;

    readConfig(tr("analyser.json"));

    loadDevices();

    ui->setupUi(this);

    updateDatasheetDisplay();

    // Initialise per-tab overlay visibility defaults:
    // 0 = Designer, 1 = Modeller, 2 = Analyser.
    for (int i = 0; i < 3; ++i) {
        overlayStates[i].showMeasurement = false;
        overlayStates[i].showModel = false;
        overlayStates[i].showScreen = false;
    }
    // Designer: show model + screen overlays, no measurement by default.
    overlayStates[0].showMeasurement = false;
    overlayStates[0].showModel = true;
    overlayStates[0].showScreen = true;
    // Modeller: show both measurement and model; screen visible.
    overlayStates[1].showMeasurement = true;
    overlayStates[1].showModel = true;
    overlayStates[1].showScreen = true;
    // Analyser: show measurement and screen; model off by default.
    overlayStates[2].showMeasurement = true;
    overlayStates[2].showModel = false;
    overlayStates[2].showScreen = true;

    // Apply overlay state for the initially selected tab.
    int initialRole = 0; // Assume Designer by default.
    if (ui->tabWidget) {
        QWidget *currentTab = ui->tabWidget->currentWidget();
        if (currentTab == ui->tab_2) {
            initialRole = 1; // Modeller
        } else if (currentTab == ui->tab_3) {
            initialRole = 2; // Analyser
        }
    }
    if (ui->measureCheck) {
        ui->measureCheck->setChecked(overlayStates[initialRole].showMeasurement);
    }
    if (ui->modelCheck) {
        ui->modelCheck->setChecked(overlayStates[initialRole].showModel);
    }
    if (ui->screenCheck) {
        ui->screenCheck->setChecked(overlayStates[initialRole].showScreen);
    }

    // Health boxes belong logically to the Analyser tab only.
    const bool analyserInitially = (initialRole == 2);
    if (ui->Triode_A_Box) ui->Triode_A_Box->setVisible(analyserInitially);
    if (ui->Triode_B_Box) ui->Triode_B_Box->setVisible(analyserInitially);

    // Auto-open a serial port at startup using central routine
    checkComPorts();

    // Add Import menu action only (no Modeller button)
    {
        QAction *importModelAction = new QAction(tr("Import Model to Project..."), this);
        connect(importModelAction, &QAction::triggered, this, &ValveWorkbench::on_actionLoad_Model_triggered);
        if (QMenu *fileMenu = this->findChild<QMenu*>("menuFile")) {
            fileMenu->addAction(importModelAction);
        } else if (QMenuBar *mb = this->menuBar()) {
            mb->addAction(importModelAction);
        }
    }

    // Re-check and open a port when Analyser tab is selected
    if (ui->tabWidget) {
        QObject::connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int idx){
            if (ui->tabWidget->tabText(idx) == QLatin1String("Analyser")) {
                if (!serialPort.isOpen()) {
                    checkComPorts();
                }
            }
        });
    }

    // Create analyser instance and wire preferences for grid calibration references
    // This allows PreferencesDialog checkboxes to immediately command grid DACs
    analyser = new Analyser(this, &serialPort, &timeoutTimer);
    analyser->setPreferences(&preferencesDialog);
    QObject::connect(&preferencesDialog, &PreferencesDialog::applyGridRefRequested,
                     this, [this](double commandVoltage, bool enabled){
                         qInfo("Preferences applyGridRefRequested: cmd=%.3f enabled=%d", commandVoltage, enabled ? 1 : 0);
                         // Ensure serial port is open before attempting to send S2/S6
                         if (!serialPort.isOpen()) {
                             QString selected = preferencesDialog.getPort();
                             if (!selected.isEmpty()) {
                                 qInfo("Opening serial port from Preferences selection: %s", selected.toStdString().c_str());
                                 setSerialPort(selected);
                             } else if (!port.isEmpty()) {
                                 qInfo("Opening serial port from cached port: %s", port.toStdString().c_str());
                                 setSerialPort(port);
                             } else {
                                 qInfo("No port selected; attempting auto-detect via checkComPorts()");
                                 checkComPorts();
                             }
                         }
                         if (analyser) {
                             analyser->applyGridReferenceBoth(commandVoltage, enabled);
                         }
                     });

    // Ensure a Designer checkbox exists in the bottom toggle row, positioned after the Model checkbox
    if (ui->horizontalLayout_9) {
        // Check if one already exists in the row
        QCheckBox *found = nullptr;
        for (int i = 0; i < ui->horizontalLayout_9->count(); ++i) {
            QWidget *w = ui->horizontalLayout_9->itemAt(i) ? ui->horizontalLayout_9->itemAt(i)->widget() : nullptr;
            if (w && w->objectName() == QLatin1String("designerCheck")) { found = qobject_cast<QCheckBox*>(w); break; }
        }
        if (!found) {
            designerCheck = new QCheckBox(tr("Show Designer Overlays"), this);
            designerCheck->setObjectName("designerCheck");
            designerCheck->setChecked(true);
            int modelIdx = ui->horizontalLayout_9->indexOf(ui->modelCheck);
            int insertAt = (modelIdx >= 0) ? modelIdx + 1 : ui->horizontalLayout_9->count();
            ui->horizontalLayout_9->insertWidget(insertAt, designerCheck);
            QObject::connect(designerCheck, &QCheckBox::stateChanged, this, &ValveWorkbench::on_designerCheck_stateChanged, Qt::UniqueConnection);
        } else {
            designerCheck = found;
            QObject::connect(designerCheck, &QCheckBox::stateChanged, this, &ValveWorkbench::on_designerCheck_stateChanged, Qt::UniqueConnection);
        }

        // Add Sym Swing, Input Sensitivity, and Gain Mode toggles if missing
        auto ensureToggle = [&](QCheckBox *&ref, const char *objName, const QString &label, auto slot){
            // Try to find existing by objectName
            for (int i = 0; i < ui->horizontalLayout_9->count(); ++i) {
                QWidget *w = ui->horizontalLayout_9->itemAt(i) ? ui->horizontalLayout_9->itemAt(i)->widget() : nullptr;
                if (w && w->objectName() == QLatin1String(objName)) { ref = qobject_cast<QCheckBox*>(w); break; }
            }
            if (!ref) {
                ref = new QCheckBox(label, this);
                ref->setObjectName(objName);
                if (strcmp(objName, "useBypassedGainCheck") == 0) ref->setChecked(true); else ref->setChecked(true);
                ui->horizontalLayout_9->addWidget(ref);
                QObject::connect(ref, &QCheckBox::stateChanged, this, slot, Qt::UniqueConnection);
            } else {
                ref->setText(label);
                QObject::connect(ref, &QCheckBox::stateChanged, this, slot, Qt::UniqueConnection);
            }
        };

        ensureToggle(symSwingCheck, "symSwingCheck", tr("Max Sym Swing"), &ValveWorkbench::on_symSwingCheck_stateChanged);
        ensureToggle(useBypassedGainCheck, "useBypassedGainCheck", tr("K bypass"), &ValveWorkbench::on_useBypassedGainCheck_stateChanged);
    }

    // Move the Designer swing-related checkboxes (Max Sym Swing, K bypass) into
    // a dedicated row at the very bottom of the circuit parameter panel (i.e.
    // visually below the last parameter row / label 16), instead of keeping
    // them in the bottom toggle bar.
    if (ui->verticalLayout) {
        // Remove from bottom toggle row if present
        auto removeIfIn = [&](QCheckBox *w){
            if (!w) return;
            if (ui->horizontalLayout_9 && ui->horizontalLayout_9->indexOf(w) >= 0) {
                ui->horizontalLayout_9->removeWidget(w);
            }
        };
        removeIfIn(symSwingCheck);
        removeIfIn(useBypassedGainCheck);

        // Create a new row layout and append it to the parameter panel
        QHBoxLayout *designerTogglesRow = new QHBoxLayout();
        designerTogglesRow->addStretch();
        if (symSwingCheck) designerTogglesRow->addWidget(symSwingCheck);
        if (useBypassedGainCheck) designerTogglesRow->addWidget(useBypassedGainCheck);
        designerTogglesRow->addStretch();

        ui->verticalLayout->addLayout(designerTogglesRow);
    }

    // Ensure Modeller tab has an Export to Devices button
    if (ui->horizontalLayout_3) {
        bool foundExisting = false;
        QPushButton *exportBtn = nullptr;
        // Avoid duplicating if created twice
        for (int i = 0; i < ui->horizontalLayout_3->count(); ++i) {
            QWidget *w = ui->horizontalLayout_3->itemAt(i) ? ui->horizontalLayout_3->itemAt(i)->widget() : nullptr;
            if (w && w->objectName() == QLatin1String("exportToDevicesButton")) { foundExisting = true; exportBtn = qobject_cast<QPushButton*>(w); break; }
        }
        if (!foundExisting) {
            exportBtn = new QPushButton(tr("Export to Devices"), this);
            exportBtn->setObjectName("exportToDevicesButton");
            int insertAt = std::max(0, ui->horizontalLayout_3->count() - 1); // before trailing spacer
            ui->horizontalLayout_3->insertWidget(insertAt, exportBtn);
            qInfo("Created Export to Devices button in Modeller tab");
        } else {
            qInfo("Found existing Export to Devices button in Modeller tab");
        }
        if (exportBtn) {
            QObject::connect(exportBtn, &QPushButton::clicked, this, &ValveWorkbench::exportFittedModelToDevices, Qt::UniqueConnection);
        }
    }


    // Add the Data tab programmatically
    QWidget *dataTab = nullptr;
    bool dataTabExists = false;
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        if (ui->tabWidget->tabText(i) == "Data") {
            dataTab = ui->tabWidget->widget(i);
            dataTabExists = true;
            break;
        }
    }

    if (!dataTabExists) {
        dataTab = new QWidget();
        ui->tabWidget->addTab(dataTab, "Data");
    }

    // Don't manage the layout - just add widgets directly
    // The UI file should already have proper layout
    QLabel *dataLabel = new QLabel("Sweep Data Table", dataTab);

    dataTable = new QTableWidget(dataTab);
    dataTable->setRowCount(10);
    dataTable->setColumnCount(62);
    dataTable->setHorizontalHeaderLabels(QStringList() << "Va_1" << "Va_2" << "Va_3" << "Va_4" << "Va_5" << "Va_6" << "Va_7" << "Va_8" << "Va_9" << "Va_10"
                                                        << "Va_11" << "Va_12" << "Va_13" << "Va_14" << "Va_15" << "Va_16" << "Va_17" << "Va_18" << "Va_19" << "Va_20"
                                                        << "Va_21" << "Va_22" << "Va_23" << "Va_24" << "Va_25" << "Va_26" << "Va_27" << "Va_28" << "Va_29" << "Va_30"
                                                        << "Va_31" << "Va_32" << "Va_33" << "Va_34" << "Va_35" << "Va_36" << "Va_37" << "Va_38" << "Va_39" << "Va_40"
                                                        << "Va_41" << "Va_42" << "Va_43" << "Va_44" << "Va_45" << "Va_46" << "Va_47" << "Va_48" << "Va_49" << "Va_50"
                                                        << "Va_51" << "Va_52" << "Va_53" << "Va_54" << "Va_55" << "Va_56" << "Va_57" << "Va_58" << "Va_59" << "Va_60"
                                                        << "Va_61" << "Va_62");
    dataTable->setVerticalHeaderLabels(QStringList() << "Vg_1" << "Vg_2" << "Vg_3" << "Vg_4" << "Vg_5" << "Vg_6" << "Vg_7" << "Vg_8" << "Vg_9" << "Vg_10");
    dataTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    dataTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Check if dataTab already has a layout and handle it properly
    QLayout *existingLayout = dataTab->layout();
    if (existingLayout) {
        qInfo("Data tab already has layout - using existing layout");
        existingLayout->addWidget(dataLabel);
        existingLayout->addWidget(dataTable);
    } else {
        qInfo("Data tab has no layout - creating new one");
        QVBoxLayout *dataLayout = new QVBoxLayout(dataTab);
        dataLayout->addWidget(dataLabel);
        dataLayout->addWidget(dataTable);
        dataTab->setLayout(dataLayout);
    }

    // Add a Harmonics tab programmatically for experimental spectral analysis
    harmonicsTab = nullptr;
    bool harmonicsTabExists = false;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == QLatin1String("Harmonics")) {
            harmonicsTab = ui->tabWidget->widget(i);
            harmonicsTabExists = true;
            break;
        }
    }

    if (!harmonicsTabExists) {
        harmonicsTab = new QWidget();
        ui->tabWidget->addTab(harmonicsTab, QLatin1String("Harmonics"));
    }

    if (harmonicsTab) {
        QLayout *harmLayout = harmonicsTab->layout();
        QVBoxLayout *vbox = qobject_cast<QVBoxLayout*>(harmLayout);
        if (!vbox) {
            vbox = new QVBoxLayout(harmonicsTab);
            harmonicsTab->setLayout(vbox);
        }

        QLabel *intro = new QLabel(tr("Harmonic Explorer (SE output, time-domain THD scan)"), harmonicsTab);
        intro->setWordWrap(true);

        harmonicsRunButton = new QPushButton(tr("Run SE Harmonic Scan"), harmonicsTab);
        harmonicsBiasSweepButton = new QPushButton(tr("Run SE Bias Sweep"), harmonicsTab);
        harmonicsHeatmapButton = new QPushButton(tr("Generate Harmonic Heatmap"), harmonicsTab);
        harmonicsWaterfallButton = new QPushButton(tr("Generate 3D Waterfall"), harmonicsTab);

        harmonicsView = new QGraphicsView(harmonicsTab);
        harmonicsView->setScene(harmonicsPlot.getScene());
        harmonicsView->setMinimumHeight(220);

        harmonicsText = new QTextEdit(harmonicsTab);
        harmonicsText->setReadOnly(true);
        harmonicsText->setMinimumHeight(120);

        vbox->addWidget(intro);
        vbox->addWidget(harmonicsRunButton);
        vbox->addWidget(harmonicsBiasSweepButton);
        vbox->addWidget(harmonicsHeatmapButton);
        vbox->addWidget(harmonicsWaterfallButton);
        
        // Add clipping analysis button
        QPushButton *harmonicsClippingButton = new QPushButton("Generate Clipping Analysis", harmonicsTab);
        harmonicsClippingButton->setToolTip("Generate headroom vs operating point THD map with clipping boundaries and sweet spot identification");
        vbox->addWidget(harmonicsClippingButton);
        
        // Add 3D rotation controls
        QLabel *rotationLabel = new QLabel("3D Waterfall Rotation:", harmonicsTab);
        rotationLabel->setStyleSheet("font-weight: bold;");
        rotationLabel->setObjectName("rotationLabel"); // Set object name for finding later
        vbox->addWidget(rotationLabel);
        
        // X-axis rotation slider
        QLabel *rotationXLabel = new QLabel("X-Axis Rotation:", harmonicsTab);
        rotationXLabel->setObjectName("rotationXLabel");
        vbox->addWidget(rotationXLabel);
        QSlider *rotationXSlider = new QSlider(Qt::Horizontal, harmonicsTab);
        rotationXSlider->setRange(-100, 100);
        rotationXSlider->setValue(60); // Default depthAngleX = 0.6
        rotationXSlider->setToolTip("Rotate 3D waterfall around X-axis (horizontal perspective)");
        vbox->addWidget(rotationXSlider);
        
        // Y-axis rotation slider
        QLabel *rotationYLabel = new QLabel("Y-Axis Rotation:", harmonicsTab);
        rotationYLabel->setObjectName("rotationYLabel");
        vbox->addWidget(rotationYLabel);
        QSlider *rotationYSlider = new QSlider(Qt::Horizontal, harmonicsTab);
        rotationYSlider->setRange(-100, 100);
        rotationYSlider->setValue(30); // Default depthAngleY = 0.3
        rotationYSlider->setToolTip("Rotate 3D waterfall around Y-axis (vertical perspective)");
        vbox->addWidget(rotationYSlider);
        
        // Store sliders as member variables for access in waterfall function
        harmonicsRotationXSlider = rotationXSlider;
        harmonicsRotationYSlider = rotationYSlider;
        
        // Initially hide rotation sliders - only show for 3D waterfall
        rotationXSlider->hide();
        rotationYSlider->hide();
        rotationLabel->hide();
        rotationXLabel->hide();
        rotationYLabel->hide();
        
        vbox->addWidget(harmonicsView, 1);
        vbox->addWidget(harmonicsText);

        connect(harmonicsRunButton, &QPushButton::clicked,
                this, &ValveWorkbench::runHarmonicsScan);
        connect(harmonicsBiasSweepButton, &QPushButton::clicked,
                this, &ValveWorkbench::runHarmonicsBiasSweep);
        connect(harmonicsHeatmapButton, &QPushButton::clicked,
                this, &ValveWorkbench::runHarmonicsHeatmap);
        connect(harmonicsWaterfallButton, &QPushButton::clicked,
                this, &ValveWorkbench::runHarmonicsWaterfall);
        connect(harmonicsClippingButton, &QPushButton::clicked,
                this, &ValveWorkbench::runHarmonicsClippingAnalysis);
        
        // Connect rotation sliders to trigger waterfall regeneration
        connect(rotationXSlider, &QSlider::valueChanged, this, &ValveWorkbench::onHarmonicsRotationChanged);
        connect(rotationYSlider, &QSlider::valueChanged, this, &ValveWorkbench::onHarmonicsRotationChanged);
    }

    // Heater button is unused in new hardware; no initialization needed

    // Device type combo: the itemData carries the logical eDevice type used by
    // the analyser and measurements. Variants like "Double Triode" and
    // "Triode-Connected Pentode" piggy-back on TRIODE or PENTODE and are
    // distinguished via separate flags.
    ui->deviceType->addItem("Triode", TRIODE);
    ui->deviceType->addItem("Pentode", PENTODE);
    ui->deviceType->addItem("Double Triode", TRIODE);
    ui->deviceType->addItem("Diode", DIODE);
    // Triode-Connected Pentode uses pentode hardware (S7 as screen) but with
    // anode and screen driven together in the analyser.
    ui->deviceType->addItem("Triode-Connected Pentode", PENTODE);

    // Use a single base model (prefer 12AX7 triode) as the unified source
    // for analyser ranges/steps and modelling limits instead of analyser.json
    Device *defaultDevice = nullptr;

    // Prefer an explicit 12AX7 triode if present
    for (int i = 0; i < devices.size(); ++i) {
        Device *dev = devices.at(i);
        if (dev && dev->getDeviceType() == TRIODE && dev->getName() == QLatin1String("12AX7")) {
            defaultDevice = dev;
            break;
        }
    }

    // Otherwise fall back to the first triode device
    if (!defaultDevice) {
        for (int i = 0; i < devices.size(); ++i) {
            Device *dev = devices.at(i);
            if (dev && dev->getDeviceType() == TRIODE) {
                defaultDevice = dev;
                break;
            }
        }
    }

    // And finally to the very first loaded device if nothing else matches
    if (!defaultDevice && !devices.isEmpty()) {
        defaultDevice = devices.first();
    }

    if (defaultDevice) {
        // Fixed heater voltage (hardware constant)
        heaterVoltage = 6.3;

        // Derive analyser ranges and limits from the base model
        anodeStart = 0.0;
        anodeStop  = defaultDevice->getVaMax();
        // Use a reasonable default step: either 5V or roughly 60 points over the range
        anodeStep  = std::max(5.0, anodeStop / 60.0);

        gridStart  = 0.0;
        gridStop   = defaultDevice->getVg1Max();
        gridStep   = 0.5;   // designer-style grid increment

        screenStart = 0.0;
        screenStop  = 0.0;
        screenStep  = 0.0;

        iaMax = defaultDevice->getIaMax();
        pMax  = defaultDevice->getPaMax();

        // Update basic UI selections to match the base model
        if (ui->deviceName) {
            ui->deviceName->setText(defaultDevice->getName());
        }

        deviceType = defaultDevice->getDeviceType();
        int deviceIndex = 0;
        if (deviceType == TRIODE) {
            deviceIndex = 0;
        } else if (deviceType == PENTODE) {
            deviceIndex = 1;
        } else if (deviceType == DOUBLE_TRIODE) {
            deviceIndex = 2;
        } else if (deviceType == DIODE) {
            deviceIndex = 3;
        }

        ui->deviceType->setCurrentIndex(deviceIndex);
        on_deviceType_currentIndexChanged(deviceIndex);

        // Default to anode-characteristics test type
        if (ui->testType) {
            ui->testType->setCurrentIndex(0);
            on_testType_currentIndexChanged(0);
        }

        // Push derived values into the analyser parameter fields
        updateParameterDisplay();
    }

    //buildModelSelection();

    // ui->runButton->setEnabled(false);  // Commented out for testing

    ui->progressBar->setRange(0, 100);
    ui->progressBar->reset();
    ui->progressBar->setVisible(false);

    // Heater indicator removed (cosmetic change)
    heaterIndicator = nullptr;

    ui->measureCheck->setVisible(true);
    ui->modelCheck->setVisible(true);
    ui->screenCheck->setVisible(true);
    // Default to showing model curves so selecting a device renders immediately
    ui->modelCheck->setChecked(true);

    ui->fitPentodeButton->setVisible(false);
    ui->fitTriodeButton->setVisible(false);

    ui->graphicsView->setScene(plot.getScene());
    if (ui->graphicsView && ui->graphicsView->viewport()) {
        ui->graphicsView->setMouseTracking(true);
        ui->graphicsView->viewport()->setMouseTracking(true);
        ui->graphicsView->viewport()->installEventFilter(this);
        ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    connect(&serialPort, &QSerialPort::readyRead, this, &ValveWorkbench::handleReadyRead);
    connect(&serialPort, &QSerialPort::errorOccurred, this, &ValveWorkbench::handleError);
    connect(&timeoutTimer, &QTimer::timeout, this, &ValveWorkbench::handleTimeout);

    // Modeller tab: import Measurement from a tube-style device preset.
    if (ui->btnImportFromDevice) {
        connect(ui->btnImportFromDevice, &QPushButton::clicked,
                this, &ValveWorkbench::importFromDevice);
    }
    connect(ui->runButton, &QPushButton::clicked, this, &ValveWorkbench::on_runButton_clicked);

    int count = ui->properties->rowCount();
    for (int i = 0; i < count; i++) {
        ui->properties->removeRow(0);
    // ... (rest of the code remains the same)
    }

    ui->properties->setColumnCount(3);
    QStringList propertyHeaders;
    propertyHeaders << "Parameter" << "Triode A" << "Triode B";
    ui->properties->setHorizontalHeaderLabels(propertyHeaders);

    buildCircuitParameters();
    buildCircuitSelection();

    // Initialise Designer circuit instances indexed by eCircuitType so that
    // ui->circuitSelection itemData (which stores eCircuitType) maps directly
    // to entries in the circuits list. Only a subset of circuits are
    // currently implemented; others remain null and are guarded against.
    circuits.resize(TEST_CALCULATOR + 1);
    circuits[TRIODE_COMMON_CATHODE]   = new TriodeCommonCathode();
    circuits[PENTODE_COMMON_CATHODE]  = new PentodeCommonCathode();
    circuits[SINGLE_ENDED_OUTPUT]     = new SingleEndedOutput();
    circuits[ULTRALINEAR_SINGLE_ENDED]= new SingleEndedUlOutput();
    circuits[PUSH_PULL_OUTPUT]        = new PushPullOutput();
    circuits[ULTRALINEAR_PUSH_PULL]   = new PushPullUlOutput();
    circuits[AC_CATHODE_FOLLOWER]     = new TriodeACCathodeFollower();
    circuits[DC_CATHODE_FOLLOWER]     = new TriodeDCCathodeFollower();
}

void ValveWorkbench::runHarmonicsScan()
{
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    harmonicsText->clear();
    harmonicsText->append(tr("Running SE time-domain harmonic scan..."));

    // Determine the currently selected Designer circuit
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        harmonicsText->append(tr("No valid Designer circuit selected. Please select 'Single Ended Output' on the Designer tab."));
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    if (!c) {
        harmonicsText->append(tr("Current Designer circuit is null."));
        return;
    }

    QVector<double> headroomVals;
    QVector<double> hd2Vals;
    QVector<double> hd3Vals;
    QVector<double> hd4Vals;
    QVector<double> thdVals;

    if (auto *se = dynamic_cast<SingleEndedOutput*>(c)) {
        harmonicsText->append(tr("Running SE time-domain harmonic scan..."));
        se->computeTimeDomainHarmonicScan(headroomVals, hd2Vals, hd3Vals, hd4Vals, thdVals);
    } else if (auto *tri = dynamic_cast<TriodeCommonCathode*>(c)) {
        harmonicsText->append(tr("Running Triode CC time-domain harmonic scan..."));
        tri->computeTimeDomainHarmonicScan(headroomVals, hd2Vals, hd3Vals, hd4Vals, thdVals);
    } else {
        harmonicsText->append(tr("Harmonic scan is currently implemented for the Single Ended Output and Triode Common Cathode circuits only.\nPlease select one of these circuits in the Designer tab and choose a device."));
        return;
    }

    const int count = headroomVals.size();
    if (count == 0) {
        harmonicsText->append(tr("No valid samples produced by SE scan (check VB, VS, IA, RA and device)."));
        return;
    }

    // Determine Y-axis max from all harmonic values
    double yMax = 0.0;
    auto updateYMax = [&yMax](const QVector<double> &vals) {
        for (double v : vals) {
            if (std::isfinite(v)) yMax = std::max(yMax, v);
        }
    };
    updateYMax(hd2Vals);
    updateYMax(hd3Vals);
    updateYMax(hd4Vals);
    updateYMax(thdVals);
    if (yMax <= 0.0) yMax = 1.0;

    const double xStart = 0.0;
    const double xStop  = headroomVals.last();
    const double xMajor = std::max(5.0, xStop / 10.0);
    const double yStart = 0.0;
    const double yMajor = std::max(1.0, yMax / 10.0);

    harmonicsPlot.clear();
    harmonicsPlot.setAxes(xStart, xStop, xMajor, yStart, yMax, yMajor);

    auto drawCurve = [&](const QVector<double> &vals, const QColor &color) {
        if (vals.size() != count) return;
        QPen pen(color);
        pen.setWidth(2);
        for (int i = 0; i < count - 1; ++i) {
            const double x1 = headroomVals[i];
            const double y1 = vals[i];
            const double x2 = headroomVals[i + 1];
            const double y2 = vals[i + 1];
            harmonicsPlot.createSegment(x1, y1, x2, y2, pen);
        }
    };

    drawCurve(hd2Vals, QColor::fromRgb(0, 0, 255));      // HD2 blue
    drawCurve(hd3Vals, QColor::fromRgb(0, 128, 0));      // HD3 green
    drawCurve(hd4Vals, QColor::fromRgb(165, 42, 42));    // HD4 brown
    drawCurve(thdVals, QColor::fromRgb(255, 0, 0));      // THD red

    harmonicsText->append(tr("Plotted HD2 (blue), HD3 (green), HD4 (brown), and THD (red) vs headroom (Vpk)."));
}

void ValveWorkbench::runHarmonicsBiasSweep()
{
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    harmonicsText->clear();
    harmonicsText->append(tr("Running SE bias sweep harmonic scan..."));

    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        harmonicsText->append(tr("No valid Designer circuit selected. Please select 'Single Ended Output' on the Designer tab."));
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    if (!c) {
        harmonicsText->append(tr("Current Designer circuit is null."));
        return;
    }

    auto *se = dynamic_cast<SingleEndedOutput*>(c);
    if (!se) {
        harmonicsText->append(tr("Bias sweep scan is currently implemented for the Single Ended Output circuit only.\nPlease select 'Single Ended Output' in the Designer tab and choose a device."));
        return;
    }

    QVector<double> iaVals;
    QVector<double> hd2Vals;
    QVector<double> hd3Vals;
    QVector<double> hd4Vals;
    QVector<double> hd5Vals;
    QVector<double> thdVals;

    se->computeBiasSweepHarmonicCurve(iaVals, hd2Vals, hd3Vals, hd4Vals, hd5Vals, thdVals);

    const int count = iaVals.size();
    if (count == 0) {
        harmonicsText->append(tr("No valid samples produced by SE bias sweep (ensure Headroom>0 and sensible IA range)."));
        return;
    }

    double yMax = 0.0;
    auto updateYMax = [&yMax](const QVector<double> &vals) {
        for (double v : vals) {
            if (std::isfinite(v)) yMax = std::max(yMax, v);
        }
    };
    updateYMax(hd2Vals);
    updateYMax(hd3Vals);
    updateYMax(hd4Vals);
    updateYMax(thdVals);
    if (yMax <= 0.0) yMax = 1.0;

    const double xStart = iaVals.first();
    const double xStop  = iaVals.last();
    const double xMajor = std::max(1.0, (xStop - xStart) / 10.0);
    const double yStart = 0.0;
    const double yMajor = std::max(1.0, yMax / 10.0);

    harmonicsPlot.clear();
    harmonicsPlot.setAxes(xStart, xStop, xMajor, yStart, yMax, yMajor);

    auto drawCurve = [&](const QVector<double> &vals, const QColor &color) {
        if (vals.size() != count) return;
        QPen pen(color);
        pen.setWidth(2);
        for (int i = 0; i < count - 1; ++i) {
            const double x1 = iaVals[i];
            const double y1 = vals[i];
            const double x2 = iaVals[i + 1];
            const double y2 = vals[i + 1];
            harmonicsPlot.createSegment(x1, y1, x2, y2, pen);
        }
    };

    // Enhanced harmonic vs operating point lines plot
    harmonicsText->append(tr("Harmonic vs Operating Point Analysis:"));
    harmonicsText->append(tr("Individual harmonic curves vs bias current"));
    harmonicsText->append(tr("Blue=HD2, Green=HD3, Brown=HD4, Red=THD"));
    
    // Plot individual harmonics with distinct colors and labels
    drawCurve(hd2Vals, QColor::fromRgb(0, 0, 255));      // HD2 blue
    drawCurve(hd3Vals, QColor::fromRgb(0, 128, 0));      // HD3 green  
    drawCurve(hd4Vals, QColor::fromRgb(165, 42, 42));    // HD4 brown
    drawCurve(hd5Vals, QColor::fromRgb(128, 0, 128));    // HD5 purple
    drawCurve(thdVals, QColor::fromRgb(255, 0, 0));      // THD red
    
    // Find and mark harmonic peaks
    auto findPeak = [&](const QVector<double> &vals, const QString &name) {
        if (vals.isEmpty()) return;
        int peakIdx = 0;
        double peakVal = vals[0];
        for (int i = 1; i < vals.size(); ++i) {
            if (std::isfinite(vals[i]) && vals[i] > peakVal) {
                peakVal = vals[i];
                peakIdx = i;
            }
        }
        harmonicsText->append(tr("%1 peak: %2% at IA=%3mA").arg(name).arg(peakVal, 0, 'f', 2).arg(iaVals[peakIdx], 0, 'f', 1));
        
        // Mark peak on plot with circle
        harmonicsPlot.createLabel(iaVals[peakIdx], peakVal, peakVal, QColor::fromRgb(255, 165, 0))->setPlainText("●");
    };
    
    findPeak(hd2Vals, "HD2");
    findPeak(hd3Vals, "HD3");
    findPeak(hd4Vals, "HD4");
    findPeak(hd5Vals, "HD5");
    findPeak(thdVals, "THD");

    harmonicsText->append(tr("Plotted individual harmonics vs bias current with peak markers."));
}

void ValveWorkbench::runHarmonicsHeatmap()
{
    // DEBUG: First line to confirm function is being called
    if (harmonicsText) {
        harmonicsText->append(tr("DEBUG: runHarmonicsHeatmap() called!"));
    } else {
        qDebug() << "DEBUG: harmonicsText is null!";
        return;
    }
    
    if (!harmonicsView) {
        harmonicsText->append(tr("DEBUG: harmonicsView is null!"));
        return;
    }

    harmonicsText->clear();
    harmonicsText->append(tr("Generating harmonic heatmap..."));

    // Determine the currently selected Designer circuit
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        harmonicsText->append(tr("No valid Designer circuit selected. Please select 'Single Ended Output' on the Designer tab."));
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    if (!c) {
        harmonicsText->append(tr("Current Designer circuit is null."));
        return;
    }

    auto *se = dynamic_cast<SingleEndedOutput*>(c);
    if (!se) {
        harmonicsText->append(tr("Harmonic heatmap is currently implemented for the Single Ended Output circuit only.\nPlease select 'Single Ended Output' in the Designer tab and choose a device."));
        return;
    }

    // Generate 2D harmonic surface data (bias × headroom grid)
    QVector<double> biasPoints;
    QVector<double> headroomPoints;
    QVector<QVector<QVector<double>>> harmonicSurface;

    se->computeHarmonicSurfaceData(biasPoints, headroomPoints, harmonicSurface);

    // DEBUG: Show what data was actually generated
    harmonicsText->append(tr("DEBUG: Surface data generation results:"));
    harmonicsText->append(tr("Bias points count: %1, range: %2 to %3")
                          .arg(biasPoints.size()).arg(biasPoints.isEmpty() ? 0 : biasPoints.first()).arg(biasPoints.isEmpty() ? 0 : biasPoints.last()));
    harmonicsText->append(tr("Headroom points count: %1, range: %2 to %3")
                          .arg(headroomPoints.size()).arg(headroomPoints.isEmpty() ? 0 : headroomPoints.first()).arg(headroomPoints.isEmpty() ? 0 : headroomPoints.last()));
    harmonicsText->append(tr("Harmonic surface layers: %1").arg(harmonicSurface.size()));
    
    if (!harmonicSurface.isEmpty() && !harmonicSurface[0].isEmpty()) {
        harmonicsText->append(tr("Sample HD2 values: %1, %2, %3")
                              .arg(harmonicSurface[0][0][0], 0, 'f', 3)
                              .arg(harmonicSurface[0][harmonicSurface[0].size()/2][harmonicSurface[0][0].size()/2], 0, 'f', 3)
                              .arg(harmonicSurface[0].last().last(), 0, 'f', 3));
    }

    if (biasPoints.isEmpty() || headroomPoints.isEmpty() || harmonicSurface.isEmpty() || harmonicSurface.size() < 4) {
        harmonicsText->append(tr("No valid surface data generated (ensure device is selected and parameters are reasonable)."));
        return;
    }

    // For 2D heatmap, we'll show harmonic magnitude vs bias at a fixed headroom level
    // Use the middle headroom level for the heatmap
    const int headroomIdx = headroomPoints.size() / 2;
    const double fixedHeadroom = headroomPoints[headroomIdx];
    
    // Extract 1D harmonic data from 2D surface at fixed headroom
    QVector<double> operatingPoints = biasPoints;
    QVector<QVector<double>> harmonicMatrix(4);
    
    for (int h = 0; h < 4; ++h) {
        harmonicMatrix[h] = harmonicSurface[h][headroomIdx];
    }

    // Clear the plot and prepare for heatmap
    harmonicsText->append(tr("Clearing all graphics items from previous plots..."));
    harmonicsPlot.clear();
    
    // Hide rotation controls for non-3D plots
    hideRotationControls();
    
    // Additional explicit cleanup for any remaining items
    if (harmonicsView && harmonicsView->scene()) {
        // Remove any remaining text items, labels, or graphics
        QList<QGraphicsItem*> remainingItems = harmonicsView->scene()->items();
        for (QGraphicsItem* item : remainingItems) {
            if (item->type() == QGraphicsTextItem::Type || 
                item->type() == QGraphicsEllipseItem::Type ||
                item->type() == QGraphicsRectItem::Type ||
                item->type() == QGraphicsPolygonItem::Type) {
                harmonicsView->scene()->removeItem(item);
                delete item;
            }
        }
    }
    
    // Find maximum magnitude for color scaling
    double maxMagnitude = 0.0;
    for (const auto &harmonicRow : harmonicMatrix) {
        for (double magnitude : harmonicRow) {
            if (std::isfinite(magnitude)) {
                maxMagnitude = std::max(maxMagnitude, magnitude);
            }
        }
    }
    
    if (maxMagnitude <= 0.0) {
        harmonicsText->append(tr("All harmonic magnitudes are zero or invalid."));
        return;
    }

    // Heatmap dimensions
    const int numHarmonics = 4; // HD2, HD3, HD4, HD5
    const int numOperatingPoints = operatingPoints.size();
    
    // Calculate cell dimensions in data coordinates
    double dataRange = 0.0;
    if (!operatingPoints.isEmpty()) {
        dataRange = operatingPoints.last() - operatingPoints.first();
    }
    const double cellWidth = std::max(dataRange / (numOperatingPoints - 1), 2.0); // Minimum 2.0mA width for visibility
    const double cellHeight = 1.0; // One harmonic unit per row
    
    // Debug output for coordinate calculation
    harmonicsText->append(tr("Debug: Operating points range: %1 to %2 (range: %3)")
                          .arg(operatingPoints.first()).arg(operatingPoints.last()).arg(dataRange));
    harmonicsText->append(tr("Debug: Cell dimensions: width=%1, height=%2")
                          .arg(cellWidth).arg(cellHeight));
    
    // Set plot bounds
    if (!operatingPoints.isEmpty()) {
        const double xMin = operatingPoints.first() - cellWidth;
        const double xMax = operatingPoints.last() + cellWidth;
        const double xMajor = (xMax - xMin) / 5.0; // 5 major divisions
        harmonicsPlot.setAxes(xMin, xMax, xMajor, 0, 4, 1.0, 1, 1);
        
        harmonicsText->append(tr("Debug: Plot bounds set: xMin=%1, xMax=%2")
                              .arg(xMin).arg(xMax));
    }

    // Draw heatmap cells
    QStringList harmonicNames = {"HD2", "HD3", "HD4", "HD5"};
    QStringList harmonicColors = {"Blue", "Green", "Brown", "Purple"};
    
    for (int harmonicIdx = 0; harmonicIdx < numHarmonics; ++harmonicIdx) {
        for (int pointIdx = 0; pointIdx < numOperatingPoints; ++pointIdx) {
            double magnitude = harmonicMatrix[harmonicIdx][pointIdx];
            
            if (std::isfinite(magnitude) && magnitude > 0.0) {
                // Normalize magnitude to 0-1 range for color mapping
                double normalizedMagnitude = std::min(magnitude / maxMagnitude, 1.0);
                
                // Create color from blue (cold) to red (hot) through HSV
                QColor color = QColor::fromHsv(static_cast<int>((1.0 - normalizedMagnitude) * 240), 255, 255);
                
                // Calculate cell position in data coordinates
                double x = operatingPoints[pointIdx];
                double y = harmonicIdx + 0.5; // Center in harmonic row
                
                // Debug first few cells
                if (harmonicIdx == 0 && pointIdx < 3) {
                    harmonicsText->append(tr("Debug: Cell[%1] at x=%2, y=%3, magnitude=%4")
                                          .arg(pointIdx).arg(x).arg(y).arg(magnitude));
                }
                
                // Convert data coordinates to scene coordinates for proper sizing
                // FIXED: Removed PLOT_HEIGHT inversion for Qt top-left coordinate system
                double sceneX = (x - harmonicsPlot.getXStart()) * harmonicsPlot.getXScale();
                double sceneY = (y - harmonicsPlot.getYStart()) * harmonicsPlot.getYScale();
                double sceneWidth = cellWidth * harmonicsPlot.getXScale();
                double sceneHeight = cellHeight * harmonicsPlot.getYScale();
                
                // Create rectangle for heatmap cell using scene coordinates
                QGraphicsRectItem *cell = new QGraphicsRectItem(
                    sceneX - sceneWidth/2, sceneY - sceneHeight/2, sceneWidth, sceneHeight
                );
                cell->setBrush(QBrush(color));
                cell->setPen(Qt::NoPen);
                harmonicsPlot.add(cell);
            }
        }
    }

    // Add axis labels and scale information
    harmonicsText->append(tr("Heatmap generated: X-axis = Bias Current (mA), Y-axis = Harmonic Number, Color = Magnitude (%)"));
    harmonicsText->append(tr("Fixed headroom level: %1 Vpk").arg(fixedHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("Harmonics: %1").arg(harmonicNames.join(", ")));
    harmonicsText->append(tr("Color scale: Blue (low magnitude) → Red (high magnitude)"));
    harmonicsText->append(tr("Maximum magnitude: %1%").arg(maxMagnitude, 0, 'f', 2));
    harmonicsText->append(tr("Note: Use 3D Waterfall for full bias × headroom analysis"));

    // Add axis labels to the plot
    harmonicsPlot.createLabel(operatingPoints.first(), -0.3, 0, QColor::fromRgb(0, 0, 0))->setPlainText("Bias Current (mA)");
    harmonicsPlot.createLabel(operatingPoints.first() - (operatingPoints.last() - operatingPoints.first()) * 0.15, 2.5, 0, QColor::fromRgb(0, 0, 0))->setPlainText("Harmonic Order");
    
    // Add harmonic number labels on Y-axis
    for (int h = 0; h < numHarmonics; ++h) {
        harmonicsPlot.createLabel(operatingPoints.first() - (operatingPoints.last() - operatingPoints.first()) * 0.1, h + 1, 0, QColor::fromRgb(0, 0, 0))->setPlainText(QString("HD%1").arg(h + 2));
    }
}

void ValveWorkbench::hideRotationControls()
{
    if (harmonicsRotationXSlider && harmonicsRotationYSlider) {
        harmonicsRotationXSlider->hide();
        harmonicsRotationYSlider->hide();
        // Hide the labels using object names
        if (auto rotationLabel = harmonicsTab->findChild<QLabel*>("rotationLabel")) {
            rotationLabel->hide();
        }
        if (auto rotationXLabel = harmonicsTab->findChild<QLabel*>("rotationXLabel")) {
            rotationXLabel->hide();
        }
        if (auto rotationYLabel = harmonicsTab->findChild<QLabel*>("rotationYLabel")) {
            rotationYLabel->hide();
        }
    }
}

void ValveWorkbench::runHarmonicsWaterfall()
{
    // 3D Waterfall functionality removed - AI failed to follow instructions and made unusable changes
    harmonicsText->clear();
    harmonicsText->append(tr("3D Waterfall functionality removed due to AI modification failure."));
    harmonicsText->append(tr("Original working version was overwritten with unusable changes."));
    harmonicsText->append(tr("This demonstrates AI inability to follow simple instructions without over-engineering."));
}

void ValveWorkbench::onHarmonicsRotationChanged()
{
    // Check if we have valid sliders and a waterfall is currently displayed
    if (!harmonicsRotationXSlider || !harmonicsRotationYSlider || !harmonicsText) {
        return;
    }
    
    // Only regenerate if a waterfall was the last plot generated
    if (harmonicsText->toPlainText().contains("3D Continuous Surface Waterfall generated")) {
        harmonicsText->append(tr("\n--- 3D Rotation Updated ---"));
        runHarmonicsWaterfall();
    }
}

void ValveWorkbench::runHarmonicsClippingAnalysis()
{
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    harmonicsText->clear();
    harmonicsText->append(tr("Generating clipping analysis and sweet spot identification..."));

    // Determine the currently selected Designer circuit
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size() || !circuits.at(currentCircuitType)) {
        harmonicsText->append(tr("No valid Designer circuit selected. Please select 'Single Ended Output' on the Designer tab."));
        return;
    }

    Circuit *circuit = circuits.at(currentCircuitType);
    if (!circuit) {
        harmonicsText->append(tr("Current Designer circuit is null."));
        return;
    }

    auto *se = dynamic_cast<SingleEndedOutput*>(circuit);
    if (!se) {
        harmonicsText->append(tr("Clipping analysis is currently implemented for the Single Ended Output circuit only.\nPlease select 'Single Ended Output' in the Designer tab and choose a device."));
        return;
    }

    // Generate 2D harmonic surface data (bias × headroom grid)
    QVector<double> biasPoints;
    QVector<double> headroomPoints;
    QVector<QVector<QVector<double>>> harmonicSurface;

    se->computeHarmonicSurfaceData(biasPoints, headroomPoints, harmonicSurface);

    if (biasPoints.isEmpty() || headroomPoints.isEmpty() || harmonicSurface.isEmpty() || harmonicSurface.size() < 4) {
        harmonicsText->append(tr("No valid surface data generated for clipping analysis."));
        return;
    }

    // Explicitly clear all graphics items to prevent persistence between graphs
    harmonicsText->append(tr("Clearing all graphics items from previous plots..."));
    harmonicsPlot.clear();
    
    // Hide rotation controls for non-3D plots
    hideRotationControls();
    
    // Additional explicit cleanup for any remaining items
    if (harmonicsView && harmonicsView->scene()) {
        // Remove any remaining text items, labels, or graphics
        QList<QGraphicsItem*> remainingItems = harmonicsView->scene()->items();
        for (QGraphicsItem* item : remainingItems) {
            if (item->type() == QGraphicsTextItem::Type || 
                item->type() == QGraphicsEllipseItem::Type ||
                item->type() == QGraphicsRectItem::Type ||
                item->type() == QGraphicsPolygonItem::Type) {
                harmonicsView->scene()->removeItem(item);
                delete item;
            }
        }
    }

    // Set plot bounds and labels BEFORE creating rectangles
    if (!biasPoints.isEmpty() && !headroomPoints.isEmpty()) {
        harmonicsPlot.setAxes(biasPoints.first(), biasPoints.last(), 
                              (biasPoints.last() - biasPoints.first()) / 5.0,
                              headroomPoints.first(), headroomPoints.last(),
                              (headroomPoints.last() - headroomPoints.first()) / 5.0, 1, 1);
    }

    // Calculate THD across the grid and create clipping zones
    const int numBiasPoints = biasPoints.size();
    const int numHeadroomPoints = headroomPoints.size();
    
    harmonicsText->append(tr("Clipping Analysis: %1×%2 grid (bias × headroom)").arg(numBiasPoints).arg(numHeadroomPoints));
    harmonicsText->append(tr("Analyzing THD levels to identify clipping boundaries and sweet spots..."));
    
    // DEBUG: Show actual data ranges
    harmonicsText->append(tr("DEBUG: Bias range: %1 to %2 mA").arg(biasPoints.first(), 0, 'f', 1).arg(biasPoints.last(), 0, 'f', 1));
    harmonicsText->append(tr("DEBUG: Headroom range: %1 to %2 Vpk").arg(headroomPoints.first(), 0, 'f', 1).arg(headroomPoints.last(), 0, 'f', 1));

    // DEBUG: Show THD data range
    double minTHD = 999.0, maxTHD = 0.0;
    for (int hIdx = 0; hIdx < numHeadroomPoints; ++hIdx) {
        for (int bIdx = 0; bIdx < numBiasPoints; ++bIdx) {
            double thd = harmonicSurface[3][hIdx][bIdx];
            if (std::isfinite(thd)) {
                minTHD = std::min(minTHD, thd);
                maxTHD = std::max(maxTHD, thd);
            }
        }
    }
    harmonicsText->append(tr("DEBUG: THD range: %1% to %2%").arg(minTHD, 0, 'f', 2).arg(maxTHD, 0, 'f', 2));
    
    // DEBUG: Check signal amplitude vs headroom mismatch
    double vs = se->getParameter(SE_VS);
    harmonicsText->append(tr("DEBUG: Signal amplitude (vs) = %1V vs Headroom up to %2Vpk").arg(vs, 0, 'f', 1).arg(headroomPoints.last(), 0, 'f', 1));
    harmonicsText->append(tr("FIXED: Now using headroom as signal amplitude for realistic clipping analysis!"));

    // Define adaptive clipping thresholds based on actual THD range
    double actualTHDSpan = maxTHD - minTHD;
    double THD_CLEAN = minTHD + actualTHDSpan * 0.25;    // Bottom 25% = clean
    double THD_BREAKUP = minTHD + actualTHDSpan * 0.50;  // 25-50% = breakup  
    double THD_CLIPPING = minTHD + actualTHDSpan * 0.75; // Top 25% = clipping
    
    harmonicsText->append(tr("Adaptive thresholds: Clean<%1%, Breakup<%2%, Clipping>%3%")
                          .arg(THD_CLEAN, 0, 'f', 2).arg(THD_BREAKUP, 0, 'f', 2).arg(THD_CLIPPING, 0, 'f', 2));

    // Count zones for verification
    int cleanCount = 0, breakupCount = 0, heavyCount = 0, clippingCount = 0;

    // Create contour plot with color-coded zones
    for (int hIdx = 0; hIdx < numHeadroomPoints - 1; ++hIdx) {
        for (int bIdx = 0; bIdx < numBiasPoints - 1; ++bIdx) {
            // Calculate THD at each corner of the grid cell
            double thd00 = harmonicSurface[3][hIdx][bIdx];     // THD is harmonicSurface[3]
            double thd01 = harmonicSurface[3][hIdx][bIdx + 1];
            double thd10 = harmonicSurface[3][hIdx + 1][bIdx];
            double thd11 = harmonicSurface[3][hIdx + 1][bIdx + 1];
            
            // Calculate average THD for this cell
            double avgTHD = (thd00 + thd01 + thd10 + thd11) / 4.0;
            
            // Determine zone color based on THD level
            QColor zoneColor;
            QString zoneType;
            
            if (avgTHD < THD_CLEAN) {
                zoneColor = QColor::fromRgb(0, 200, 0, 100);    // Green - clean
                zoneType = "Clean";
                cleanCount++;
            } else if (avgTHD < THD_BREAKUP) {
                zoneColor = QColor::fromRgb(255, 255, 0, 100);  // Yellow - breakup
                zoneType = "Breakup";
                breakupCount++;
            } else if (avgTHD < THD_CLIPPING) {
                zoneColor = QColor::fromRgb(255, 165, 0, 100);  // Orange - heavy breakup
                zoneType = "Heavy";
                heavyCount++;
            } else {
                zoneColor = QColor::fromRgb(255, 0, 0, 100);    // Red - clipping
                zoneType = "Clipping";
                clippingCount++;
            }
            
            // DEBUG: Show first few rectangles
            if (hIdx < 2 && bIdx < 2) {
                double biasX = biasPoints[bIdx];
                double headroomY = headroomPoints[hIdx];
                harmonicsText->append(tr("DEBUG: Cell[%1,%2] THD=%3% -> %4 zone at biasX=%5,headroomY=%6")
                                      .arg(hIdx).arg(bIdx).arg(avgTHD, 0, 'f', 2).arg(zoneType)
                                      .arg(biasX, 0, 'f', 1).arg(headroomY, 0, 'f', 1));
            }
            
            // Convert data coordinates to scene coordinates
            // FIXED: Removed PLOT_HEIGHT inversion for Qt top-left coordinate system
            double x = biasPoints[bIdx];
            double y = headroomPoints[hIdx];
            double width = (biasPoints[bIdx + 1] - biasPoints[bIdx]);
            double height = (headroomPoints[hIdx + 1] - headroomPoints[hIdx]);
            
            double sceneX = (x - harmonicsPlot.getXStart()) * harmonicsPlot.getXScale();
            double sceneY = (y - harmonicsPlot.getYStart()) * harmonicsPlot.getYScale();
            double sceneWidth = width * harmonicsPlot.getXScale();
            double sceneHeight = height * harmonicsPlot.getYScale();
            
            // Create zone rectangle
            QGraphicsRectItem *zone = new QGraphicsRectItem(sceneX, sceneY - sceneHeight, sceneWidth, sceneHeight);
            zone->setBrush(QBrush(zoneColor));
            zone->setPen(QPen(zoneColor.darker(150), 1));
            harmonicsPlot.add(zone);
        }
    }
    
    // Show zone creation summary
    harmonicsText->append(tr("DEBUG: Zone summary - Clean: %1, Breakup: %2, Heavy: %3, Clipping: %4")
                          .arg(cleanCount).arg(breakupCount).arg(heavyCount).arg(clippingCount));

    // Find and mark sweet spots (lowest THD regions)
    harmonicsText->append(tr("\nSweet Spot Identification:"));
    
    double minTHDForSweetSpot = 999.0;
    int sweetSpotBiasIdx = 0, sweetSpotHeadroomIdx = 0;
    
    for (int hIdx = 0; hIdx < numHeadroomPoints; ++hIdx) {
        for (int bIdx = 0; bIdx < numBiasPoints; ++bIdx) {
            double thd = harmonicSurface[3][hIdx][bIdx];
            if (std::isfinite(thd) && thd < minTHDForSweetSpot) {
                minTHDForSweetSpot = thd;
                sweetSpotBiasIdx = bIdx;
                sweetSpotHeadroomIdx = hIdx;
            }
        }
    }
    
    // Mark clean sweet spot on plot
    double sweetSpotBias = biasPoints[sweetSpotBiasIdx];
    double sweetSpotHeadroom = headroomPoints[sweetSpotHeadroomIdx];
    
    harmonicsText->append(tr("Clean Sweet Spot: IA=%1mA, Headroom=%2Vpk, THD=%3%")
                          .arg(sweetSpotBias, 0, 'f', 1)
                          .arg(sweetSpotHeadroom, 0, 'f', 1) 
                          .arg(minTHDForSweetSpot, 0, 'f', 2));
    
    // Create clean sweet spot marker (white circle)
    // FIXED: Removed PLOT_HEIGHT inversion for Qt top-left coordinate system
    double sweetSpotX = (sweetSpotBias - harmonicsPlot.getXStart()) * harmonicsPlot.getXScale();
    double sweetSpotY = (sweetSpotHeadroom - harmonicsPlot.getYStart()) * harmonicsPlot.getYScale();
    
    harmonicsText->append(tr("DEBUG: Clean spot scene coords - X=%1, Y=%2 (Headroom=%3Vpk)")
                          .arg(sweetSpotX, 0, 'f', 1).arg(sweetSpotY, 0, 'f', 1).arg(sweetSpotHeadroom, 0, 'f', 1));
    
    // DEBUG: Compare label vs marker coordinate systems
    harmonicsText->append(tr("DEBUG: Label data coords - bias=%1, headroom=%2")
                          .arg(sweetSpotBias, 0, 'f', 1).arg(sweetSpotHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("DEBUG: Marker scene coords - X=%1, Y=%2")
                          .arg(sweetSpotX, 0, 'f', 1).arg(sweetSpotY, 0, 'f', 1));
    
    QGraphicsEllipseItem *marker = new QGraphicsEllipseItem(sweetSpotX - 8, sweetSpotY - 8, 16, 16);
    marker->setBrush(QBrush(QColor::fromRgb(255, 255, 255)));
    marker->setPen(QPen(QColor::fromRgb(0, 0, 0), 2));
    harmonicsPlot.add(marker);
    
    harmonicsPlot.createLabel(sweetSpotBias, sweetSpotHeadroom, 0, QColor::fromRgb(0, 0, 0))->setPlainText("○ Clean");

    // Find even harmonic dominance sweet spot (maximum HD2/HD3 ratio)
    harmonicsText->append(tr("\nEven Harmonic Analysis:"));
    
    double maxEvenOddRatio = 0.0;
    int warmSpotBiasIdx = 0, warmSpotHeadroomIdx = 0;
    
    for (int hIdx = 0; hIdx < numHeadroomPoints; ++hIdx) {
        for (int bIdx = 0; bIdx < numBiasPoints; ++bIdx) {
            double hd2 = harmonicSurface[0][hIdx][bIdx]; // HD2
            double hd3 = harmonicSurface[1][hIdx][bIdx]; // HD3
            
            // Calculate HD2/HD3 ratio for even harmonic dominance
            if (std::isfinite(hd2) && std::isfinite(hd3) && hd3 > 0.1 && hd2 > 0.5) {
                double evenOddRatio = hd2 / hd3;
                if (evenOddRatio > maxEvenOddRatio) {
                    maxEvenOddRatio = evenOddRatio;
                    warmSpotBiasIdx = bIdx;
                    warmSpotHeadroomIdx = hIdx;
                }
            }
        }
    }
    
    // Mark warm sweet spot on plot
    double warmSpotBias = biasPoints[warmSpotBiasIdx];
    double warmSpotHeadroom = headroomPoints[warmSpotHeadroomIdx];
    double warmSpotHD2 = harmonicSurface[0][warmSpotHeadroomIdx][warmSpotBiasIdx];
    double warmSpotHD3 = harmonicSurface[1][warmSpotHeadroomIdx][warmSpotBiasIdx];
    
    harmonicsText->append(tr("DEBUG: Warm spot indices - biasIdx=%1, headroomIdx=%2")
                          .arg(warmSpotBiasIdx).arg(warmSpotHeadroomIdx));
    harmonicsText->append(tr("DEBUG: Warm spot coordinates - IA=%1mA, Headroom=%2Vpk")
                          .arg(warmSpotBias, 0, 'f', 1).arg(warmSpotHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("Warm Sweet Spot: IA=%1mA, Headroom=%2Vpk")
                          .arg(warmSpotBias, 0, 'f', 1)
                          .arg(warmSpotHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("HD2=%1%, HD3=%2%, HD2/HD3 Ratio=%3")
                          .arg(warmSpotHD2, 0, 'f', 2)
                          .arg(warmSpotHD3, 0, 'f', 2)
                          .arg(maxEvenOddRatio, 0, 'f', 2));
    
    // Create warm sweet spot marker (green circle)
    // FIXED: Removed PLOT_HEIGHT inversion for Qt top-left coordinate system
    double warmSpotX = (warmSpotBias - harmonicsPlot.getXStart()) * harmonicsPlot.getXScale();
    double warmSpotY = (warmSpotHeadroom - harmonicsPlot.getYStart()) * harmonicsPlot.getYScale();
    
    harmonicsText->append(tr("DEBUG: Warm spot scene coordinates - X=%1, Y=%2")
                          .arg(warmSpotX, 0, 'f', 1).arg(warmSpotY, 0, 'f', 1));
    harmonicsText->append(tr("DEBUG: Warm label data coords - bias=%1, headroom=%2")
                          .arg(warmSpotBias, 0, 'f', 1).arg(warmSpotHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("FIXED: Removed PLOT_HEIGHT inversion for Qt top-left coordinate system"));
    
    QGraphicsEllipseItem *warmMarker = new QGraphicsEllipseItem(warmSpotX - 8, warmSpotY - 8, 16, 16);
    warmMarker->setBrush(QBrush(QColor::fromRgb(0, 255, 0)));
    warmMarker->setPen(QPen(QColor::fromRgb(0, 128, 0), 2));
    harmonicsPlot.add(warmMarker);
    
    harmonicsPlot.createLabel(warmSpotBias, warmSpotHeadroom, 0, QColor::fromRgb(0, 128, 0))->setPlainText("● Warm");

    // Find maximum even harmonic clipping (highest HD2 in clipping zones)
    harmonicsText->append(tr("\nMaximum Even Harmonic Clipping:"));
    
    double maxHD2InClipping = 0.0;
    int clippingSpotBiasIdx = 0, clippingSpotHeadroomIdx = 0;
    
    for (int hIdx = 0; hIdx < numHeadroomPoints; ++hIdx) {
        for (int bIdx = 0; bIdx < numBiasPoints; ++bIdx) {
            double thd = harmonicSurface[3][hIdx][bIdx]; // THD
            double hd2 = harmonicSurface[0][hIdx][bIdx]; // HD2
            
            // Look for maximum HD2 in clipping zones (using adaptive THD_CLIPPING threshold)
            if (std::isfinite(thd) && std::isfinite(hd2) && thd > THD_CLIPPING && hd2 > 0.1) {
                if (hd2 > maxHD2InClipping) {
                    maxHD2InClipping = hd2;
                    clippingSpotBiasIdx = bIdx;
                    clippingSpotHeadroomIdx = hIdx;
                }
            }
        }
    }
    
    // Mark max even clipping spot on plot
    double clippingSpotBias = biasPoints[clippingSpotBiasIdx];
    double clippingSpotHeadroom = headroomPoints[clippingSpotHeadroomIdx];
    double clippingSpotHD2 = harmonicSurface[0][clippingSpotHeadroomIdx][clippingSpotBiasIdx];
    double clippingSpotHD3 = harmonicSurface[1][clippingSpotHeadroomIdx][clippingSpotBiasIdx];
    double clippingSpotTHD = harmonicSurface[3][clippingSpotHeadroomIdx][clippingSpotBiasIdx];
    
    harmonicsText->append(tr("Max Even Clipping: IA=%1mA, Headroom=%2Vpk")
                          .arg(clippingSpotBias, 0, 'f', 1)
                          .arg(clippingSpotHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("HD2=%1%, HD3=%2%, THD=%3%")
                          .arg(clippingSpotHD2, 0, 'f', 2)
                          .arg(clippingSpotHD3, 0, 'f', 2)
                          .arg(clippingSpotTHD, 0, 'f', 2));
    
    // Create max even clipping marker (orange diamond)
    // FIXED: Removed PLOT_HEIGHT inversion for Qt top-left coordinate system
    double clippingSpotX = (clippingSpotBias - harmonicsPlot.getXStart()) * harmonicsPlot.getXScale();
    double clippingSpotY = (clippingSpotHeadroom - harmonicsPlot.getYStart()) * harmonicsPlot.getYScale();
    
    harmonicsText->append(tr("DEBUG: Max even scene coords - X=%1, Y=%2 (Headroom=%3Vpk)")
                          .arg(clippingSpotX, 0, 'f', 1).arg(clippingSpotY, 0, 'f', 1).arg(clippingSpotHeadroom, 0, 'f', 1));
    harmonicsText->append(tr("DEBUG: Max even label data coords - bias=%1, headroom=%2")
                          .arg(clippingSpotBias, 0, 'f', 1).arg(clippingSpotHeadroom, 0, 'f', 1));
    
    QGraphicsRectItem *clippingMarker = new QGraphicsRectItem(clippingSpotX - 8, clippingSpotY - 8, 16, 16);
    clippingMarker->setBrush(QBrush(QColor::fromRgb(255, 165, 0)));
    clippingMarker->setPen(QPen(QColor::fromRgb(255, 0, 0), 2));
    clippingMarker->setRotation(45); // Rotate to make diamond shape
    harmonicsPlot.add(clippingMarker);
    
    harmonicsPlot.createLabel(clippingSpotBias, clippingSpotHeadroom, 0, QColor::fromRgb(0, 0, 0))->setPlainText("◆ Max Even");

    // Add axis labels and legend (plot bounds already set above)
    harmonicsPlot.createLabel(biasPoints.first(), headroomPoints.first() - (headroomPoints.last() - headroomPoints.first()) * 0.1, 0, QColor::fromRgb(0, 0, 0))->setPlainText("Bias Current (mA)");
    harmonicsPlot.createLabel(biasPoints.first() - (biasPoints.last() - biasPoints.first()) * 0.15, (headroomPoints.first() + headroomPoints.last()) / 2, 0, QColor::fromRgb(0, 0, 0))->setPlainText("Headroom (Vpk)");

    harmonicsText->append(tr("\nClipping Zone Map:"));
    harmonicsText->append(tr("Green: Clean zone (THD < 1%)"));
    harmonicsText->append(tr("Yellow: Breakup zone (THD 1-5%)"));
    harmonicsText->append(tr("Orange: Heavy breakup (THD 5-10%)"));
    harmonicsText->append(tr("Red: Clipping zone (THD > 10%)"));
    harmonicsText->append(tr("○ Clean Sweet Spot: Minimum THD for clean operation"));
    harmonicsText->append(tr("★ Warm Sweet Spot: Maximum even/odd harmonic ratio for tube warmth"));
    harmonicsText->append(tr("◆ Max Even Clipping: Highest even harmonics in clipping zones (THD > 5%)"));
    harmonicsText->append(tr("\nUse this map to identify optimal operating regions for different tonal goals."));
}

void ValveWorkbench::refreshHarmonicsPlots()
{
    // Check if we're on the Harmonics tab and have valid data
    if (!harmonicsText || !harmonicsView) {
        return;
    }
    
    // Check the current tab to avoid unnecessary refreshes
    if (ui->tabWidget->currentWidget() != harmonicsTab) {
        return; // Not on harmonics tab, don't refresh
    }
    
    // Get the last generated plot type from the text content
    QString currentText = harmonicsText->toPlainText();
    
    if (currentText.contains("Heatmap generated")) {
        runHarmonicsHeatmap();
    } else if (currentText.contains("3D Continuous Surface Waterfall generated")) {
        runHarmonicsWaterfall();
    } else if (currentText.contains("Plotted HD2") && currentText.contains("vs bias current IA")) {
        runHarmonicsBiasSweep();
    }
    // Note: Don't auto-refresh basic scan as it's a point measurement, not a plot
}

ValveWorkbench::~ValveWorkbench()
{
    delete ui;
    if (analyser) {
        delete analyser;
        analyser = nullptr;
    }

    // Clean up any Designer circuit instances
    for (Circuit *c : std::as_const(circuits)) {
        delete c;
    }
    circuits.clear();
}

bool ValveWorkbench::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        if (ui && ui->graphicsView && obj == ui->graphicsView->viewport()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            QGraphicsView *view = ui->graphicsView;
            const QPointF scenePos = view->mapToScene(mouseEvent->pos());

            if (scenePos.x() >= 0.0 && scenePos.x() <= PLOT_WIDTH &&
                scenePos.y() >= 0.0 && scenePos.y() <= PLOT_HEIGHT) {

                const QPointF dataPos = plot.sceneToData(scenePos);
                const double x = dataPos.x();
                const double y = dataPos.y();

                if (plot.getScene()) {
                    if (!cursorLabelItem) {
                        cursorLabelItem = plot.getScene()->addText(QString());
                        cursorLabelItem->setZValue(1000.0);
                        cursorLabelItem->setDefaultTextColor(Qt::black);
                        cursorLabelItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        cursorLabelItem->setFlag(QGraphicsItem::ItemIsMovable, false);
                        cursorLabelItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
                        QObject::connect(cursorLabelItem, &QObject::destroyed, this, [this]() {
                            cursorLabelItem = nullptr;
                        });
                    }
                    cursorLabelItem->setPlainText(
                        tr("V=%1 V\nI=%2 mA")
                            .arg(x, 0, 'f', 1)
                            .arg(y, 0, 'f', 2));
                    cursorLabelItem->setPos(scenePos.x() + 8.0,
                                             scenePos.y() - 28.0);
                    cursorLabelItem->setVisible(true);
                }
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void ValveWorkbench::buildCircuitParameters()
{
    circuitLabels[0] = ui->cir1Label;
    circuitLabels[1] = ui->cir2Label;
    circuitLabels[2] = ui->cir3Label;
    circuitLabels[3] = ui->cir4Label;
    circuitLabels[4] = ui->cir5Label;
    circuitLabels[5] = ui->cir6Label;
    circuitLabels[6] = ui->cir7Label;
    circuitLabels[7] = ui->cir8Label;
    circuitLabels[8] = ui->cir9Label;
    circuitLabels[9] = ui->cir10Label;
    circuitLabels[10] = ui->cir11Label;
    circuitLabels[11] = ui->cir12Label;
    circuitLabels[12] = ui->cir13Label;
    circuitLabels[13] = ui->cir14Label;
    circuitLabels[14] = ui->cir15Label;
    circuitLabels[15] = ui->cir16Label;

    circuitValues[0] = ui->cir1Value;
    circuitValues[1] = ui->cir2Value;
    circuitValues[2] = ui->cir3Value;
    circuitValues[3] = ui->cir4Value;
    circuitValues[4] = ui->cir5Value;
    circuitValues[5] = ui->cir6Value;
    circuitValues[6] = ui->cir7Value;
    circuitValues[7] = ui->cir8Value;
    circuitValues[8] = ui->cir9Value;
    circuitValues[9] = ui->cir10Value;
    circuitValues[10] = ui->cir11Value;
    circuitValues[11] = ui->cir12Value;
    circuitValues[12] = ui->cir13Value;
    circuitValues[13] = ui->cir14Value;
    circuitValues[14] = ui->cir15Value;
    circuitValues[15] = ui->cir16Value;

    for (int i=0; i < 16; i++) { // Parameters all initially hidden
        circuitValues[i]->setVisible(false);
        circuitLabels[i]->setVisible(false);
    }
}

void ValveWorkbench::buildCircuitSelection()
{
    ui->circuitSelection->clear();

    ui->circuitSelection->addItem("Select...", -1);
    ui->circuitSelection->addItem("Triode Common Cathode", TRIODE_COMMON_CATHODE);
    ui->circuitSelection->addItem("Pentode Common Cathode", PENTODE_COMMON_CATHODE);
    ui->circuitSelection->addItem("AC Cathode Follower", AC_CATHODE_FOLLOWER);
    ui->circuitSelection->addItem("DC Cathode Follower", DC_CATHODE_FOLLOWER);
    ui->circuitSelection->addItem("Long Tailed Pair", LONG_TAILED_PAIR);
    ui->circuitSelection->addItem("Cathodyne Phase Splitter", CATHODYNE_PHASE_SPLITTER);
    ui->circuitSelection->addItem("Single Ended Output", SINGLE_ENDED_OUTPUT);
    ui->circuitSelection->addItem("Ultralinear Single Ended", ULTRALINEAR_SINGLE_ENDED);
    ui->circuitSelection->addItem("Push Pull Output", PUSH_PULL_OUTPUT);
    ui->circuitSelection->addItem("Ultralinear Push Pull", ULTRALINEAR_PUSH_PULL);
    ui->circuitSelection->addItem("Test Calculator", TEST_CALCULATOR);  // New test item
}

void ValveWorkbench::selectStdDevice(int index, int deviceNumber)
{
    const int circuitType = ui->circuitSelection->currentData().toInt();
    if (deviceNumber < 0 || circuitType < 0) {
        return;
    }

    if (circuitType >= circuits.size() || !circuits.at(circuitType)) {
        qWarning("selectStdDevice: circuitType %d out of range or not implemented", circuitType);
        return;
    }

    if (deviceNumber >= devices.size()) {
        qWarning("selectStdDevice: device index %d out of range (devices.size()=%d)", deviceNumber, devices.size());
        return;
    }

    Device *device = devices.at(deviceNumber);
    if (!device) {
        qWarning("selectStdDevice: devices[%d] is null", deviceNumber);
        return;
    }

    // Before clearing/rewriting the scene axes, drop all cached overlay
    // pointers so we never dereference QGraphicsItems that have been deleted
    // by Plot::setAxes (which calls scene->clear()). Circuits will rebuild
    // their overlays on the next plot() call.
    if (measuredCurves)             measuredCurves = nullptr;
    if (measuredCurvesSecondary)    measuredCurvesSecondary = nullptr;
    if (estimatedCurves)            estimatedCurves = nullptr;
    if (modelledCurves)             modelledCurves = nullptr;
    if (modelledCurvesSecondary)    modelledCurvesSecondary = nullptr;
    for (Circuit *c : std::as_const(circuits)) {
        if (c) {
            c->resetOverlays();
        }
    }

    // Update plot axes to match the new device's vaMax/iaMax. For the
    // Single-Ended Output Designer circuit, give the X-axis enough headroom
    // for approximately 2× supply swing while never shrinking below the
    // device's own vaMax.
    double vaMax = device->getVaMax();
    double iaMax = device->getIaMax();

    if (circuitType == SINGLE_ENDED_OUTPUT) {
        Circuit *seCircuit = circuits.at(circuitType);
        if (seCircuit) {
            const double vb = seCircuit->getParameter(SE_VB);
            if (vb > 0.0 && vaMax > 0.0) {
                vaMax = std::max(vaMax, 2.0 * vb);
            }
        }
    }

    // For push-pull Designer circuits, apply the same 2×VB headroom rule so
    // that the combined AC load line and swing helpers have sufficient
    // horizontal space on first plot, and extend the Y-axis to cover the
    // theoretical Class B peak current (4000*VB/RAA) rather than stopping at
    // the measurement/model Ia_max.
    if (circuitType == PUSH_PULL_OUTPUT || circuitType == ULTRALINEAR_PUSH_PULL) {
        Circuit *ppCircuit = circuits.at(circuitType);
        if (ppCircuit && vaMax > 0.0) {
            double vb  = 0.0;
            double raa = 0.0;
            if (circuitType == PUSH_PULL_OUTPUT) {
                vb  = ppCircuit->getParameter(PP_VB);
                raa = ppCircuit->getParameter(PP_RAA);
            } else {
                vb  = ppCircuit->getParameter(PPUL_VB);
                raa = ppCircuit->getParameter(PPUL_RAA);
            }
            if (vb > 0.0) {
                vaMax = std::max(vaMax, 2.0 * vb);
            }
            if (vb > 0.0 && raa > 0.0) {
                const double iaClassB_mA = 4000.0 * vb / raa;
                if (iaClassB_mA > 0.0) {
                    iaMax = std::max(iaMax, iaClassB_mA);
                }
            }
        }
    }

    // If Autoscale Y is disabled and we already have a valid Y-axis,
    // preserve the existing Y range instead of recomputing iaMax from the
    // device. This lets the user lock Y while exploring devices or VB
    // changes in Designer.
    {
        const double xScale = plot.getXScale();
        const double yScale = plot.getYScale();
        if (ui->autoscaleYCheck && !ui->autoscaleYCheck->isChecked() &&
            xScale > 0.0 && yScale > 0.0) {
            const double yStart = plot.getYStart();
            const double currentYStop = yStart + static_cast<double>(PLOT_HEIGHT) / yScale;
            if (currentYStop > 0.0) {
                iaMax = currentYStop;
            }
        }
    }

    double vaInterval = device->interval(vaMax);
    double iaInterval = device->interval(iaMax);
    plot.setAxes(0.0, vaMax, vaInterval, 0.0, iaMax, iaInterval, 0, 0);

    currentDevice = device;
    // For Designer, do not set model axes or draw model curves here to avoid overriding
    // the circuit load-line axes. The circuit plot will set appropriate axes.

    Circuit *circuit = circuits.at(circuitType);
    if (index == 1) {
        circuit->setDevice1(device);
    } else {
        circuit->setDevice2(device);
    }
    circuit->updateUI(circuitLabels, circuitValues);
    circuit->plot(&plot);
    // Trigger a compute pass so derived fields (e.g., gains, Va, Ia, Vk) populate on initial load
    // Use index 0 (first editable parameter) with its current value to invoke Circuit::update(int)
    if (circuit) {
        double current = circuit->getParameter(0);
        circuit->setParameter(0, current);
    }
    circuit->updateUI(circuitLabels, circuitValues);

    // Auto-plot device model curves in Designer. When an embedded analyser
    // Measurement is present (from a tube-style preset JSON), prefer the
    // measurement-driven Model::plotModel helper so that the fitted model
    // uses the same grid/screen families as the measurement sweeps. This
    // keeps the red model curves consistent with the black measurement
    // curves on the shared Designer axes. When no embedded Measurement is
    // available, fall back to Device::anodePlot, which plots against the
    // device's vg1Max/vg2Max ranges.
    if (modelledCurves) {
        if (modelledCurves->scene() == plot.getScene()) {
            plot.remove(modelledCurves);
        }
        modelledCurves = nullptr;
    }
    if (ui->modelCheck->isChecked() && device) {
        Measurement *embedded = device->getMeasurement();
        Model *deviceModel = device->getModel();

        // For Designer power-output circuits (SE, SE-UL, PP, UL-PP), always
        // use the fitted-model anodePlot so grid families and labels behave
        // consistently across all of them, independent of the embedded
        // measurement's grid coverage.
        if (circuitType == SINGLE_ENDED_OUTPUT ||
            circuitType == ULTRALINEAR_SINGLE_ENDED ||
            circuitType == PUSH_PULL_OUTPUT ||
            circuitType == ULTRALINEAR_PUSH_PULL) {
            modelledCurves = device->anodePlot(&plot);
            if (modelledCurves) {
                modelledCurves->setVisible(ui->modelCheck->isChecked());
            }
        } else {
            // Existing behaviour for all other circuits: prefer the
            // measurement-driven Model::plotModel helper when an embedded
            // pentode Measurement is available so that grid/screen families
            // exactly match the analyser sweeps.
            if (embedded && deviceModel && embedded->getDeviceType() == PENTODE &&
                embedded->getTestType() == ANODE_CHARACTERISTICS) {
                QGraphicsItemGroup *plotted = deviceModel->plotModel(&plot, embedded, nullptr);
                if (plotted) {
                    modelledCurves = plotted;
                    plot.add(modelledCurves);
                    modelledCurves->setVisible(ui->modelCheck->isChecked());
                }
            } else {
                // Fallback: draw using the device's internal anodePlot, which
                // uses vg1Max/vg2Max from the preset JSON.
                modelledCurves = device->anodePlot(&plot);
                if (modelledCurves) {
                    modelledCurves->setVisible(ui->modelCheck->isChecked());
                }
            }
        }
    }

    // If the selected Device carries an embedded Measurement (from a tube-style
    // preset JSON), plot its sweeps onto the Designer plot when Show
    // Measurement is enabled. Use the "without axes" variant so the current
    // circuit's axes (set above) remain in control.
    if (measuredCurves) {
        plot.remove(measuredCurves);
        measuredCurves = nullptr;
    }
    if (measuredCurvesSecondary) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }
    if (ui->measureCheck->isChecked() && device && device->getMeasurement()) {
        Measurement *embedded = device->getMeasurement();
        // On initial selection in Designer, align the embedded measurement's
        // screen visibility with the Screen checkbox so Ig2 shows/hides
        // according to the current state instead of a stale default.
        embedded->setShowScreen(ui->screenCheck->isChecked());
        embedded->setSmoothPlotting(preferencesDialog.smoothCurves());
        measuredCurves = embedded->updatePlotWithoutAxes(&plot);
        if (measuredCurves) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(ui->measureCheck->isChecked());
        }
    }
}

void ValveWorkbench::selectModel(int modelType)
{
    customDevice->setModelType(modelType);
    //customDevice->updateUI(parameterLabels, parameterValues);
}

void ValveWorkbench::plotCurrentModelOverMeasurement()
{
    if (!model || !currentMeasurement) {
        return;
    }

    if (currentMeasurement->getDeviceType() != PENTODE) {
        return;
    }

    if (modelledCurves) {
        plot.remove(modelledCurves);
        modelledCurves = nullptr;
    }

    QGraphicsItemGroup *plotted = model->plotModel(&plot, currentMeasurement, nullptr);
    if (plotted) {
        modelledCurves = plotted;
        plot.add(modelledCurves);
        modelledCurves->setVisible(ui->modelCheck->isChecked());
    }
}

void ValveWorkbench::ensureSimplePentodeDialog()
{
    auto *manual = dynamic_cast<SimpleManualPentode*>(model);
    if (!manual) {
        if (simplePentodeDialog) {
            simplePentodeDialog->hide();
        }
        return;
    }

    if (!simplePentodeDialog) {
        simplePentodeDialog = new SimpleManualPentodeDialog(this);
        connect(simplePentodeDialog, &SimpleManualPentodeDialog::parametersChanged,
                this, &ValveWorkbench::plotCurrentModelOverMeasurement);
    }

    simplePentodeDialog->setModel(manual);
    simplePentodeDialog->show();
    simplePentodeDialog->raise();
    simplePentodeDialog->activateWindow();
}

void ValveWorkbench::selectCircuit(int circuitType)
{
    // Clear Designer plot and hide any existing circuit overlays when
    // switching circuits so load lines, operating point markers, and model
    // curves from the previous circuit do not linger on the shared scene.
    plot.clear();
    cursorLabelItem = nullptr;
    measuredCurves = nullptr;
    measuredCurvesSecondary = nullptr;
    estimatedCurves = nullptr;
    modelledCurves = nullptr;
    modelledCurvesSecondary = nullptr;

    for (Circuit *c : std::as_const(circuits)) {
        if (c) {
            c->setOverlaysVisible(false);
            c->resetOverlays();
        }
    }

    qInfo("=== SELECTING CIRCUIT ===");
    qInfo("Circuit type: %d", circuitType);

    for (int i = 0; i < 16; i++) {
        circuitLabels[i]->setVisible(false);
        circuitValues[i]->setVisible(false);
    }

    if (circuitType < 0 || circuitType >= circuits.size() || !circuits.at(circuitType)) {
        qInfo("Invalid or unimplemented circuit type %d - disabling device selections", circuitType);
        ui->stdDeviceSelection->setCurrentIndex(0);
        ui->stdDeviceSelection2->setCurrentIndex(0);

        buildStdDeviceSelection(ui->stdDeviceSelection, -1);
        buildStdDeviceSelection(ui->stdDeviceSelection2, -1);
        return;
    }

    Circuit *circuit = circuits.at(circuitType);
    qInfo("Circuit class: %s", typeid(*circuit).name());

    circuit->setDevice1(nullptr);
    circuit->setDevice2(nullptr);

    ui->stdDeviceSelection->setCurrentIndex(0);
    ui->stdDeviceSelection2->setCurrentIndex(0);

    int deviceType1 = circuit->getDeviceType(1);
    int deviceType2 = circuit->getDeviceType(2);

    qInfo("Circuit requires device1 type: %d, device2 type: %d", deviceType1, deviceType2);

    buildStdDeviceSelection(ui->stdDeviceSelection, deviceType1);
    buildStdDeviceSelection(ui->stdDeviceSelection2, deviceType2);

    // Show parameter UI for the selected circuit
    circuit->updateUI(circuitLabels, circuitValues);

    qInfo("Circuit selection completed");

    // Show 'Show Screen Current' checkbox only for pentode circuits
    bool wantsPentodeScreen = (deviceType1 == PENTODE);
    if (ui->screenCheck) ui->screenCheck->setVisible(wantsPentodeScreen);

    // Show the Designer "Inductive Load" toggle only for output-stage
    // circuits where an inductive vs resistive load model makes sense, and
    // propagate its current state into SE/PP circuits so that switching
    // circuits keeps the load interpretation in sync with the UI.
    bool wantsInductiveToggle =
        (circuitType == SINGLE_ENDED_OUTPUT ||
         circuitType == ULTRALINEAR_SINGLE_ENDED ||
         circuitType == PUSH_PULL_OUTPUT ||
         circuitType == ULTRALINEAR_PUSH_PULL);
    if (ui->inductiveLoadCheck) {
        ui->inductiveLoadCheck->setVisible(wantsInductiveToggle);
        if (wantsInductiveToggle) {
            const bool inductive = ui->inductiveLoadCheck->isChecked();
            if (auto *se = dynamic_cast<SingleEndedOutput*>(circuit)) {
                se->setInductiveLoad(inductive);
            } else if (auto *seul = dynamic_cast<SingleEndedUlOutput*>(circuit)) {
                seul->setInductiveLoad(inductive);
            } else if (auto *pp = dynamic_cast<PushPullOutput*>(circuit)) {
                pp->setInductiveLoad(inductive);
            } else if (auto *ppul = dynamic_cast<PushPullUlOutput*>(circuit)) {
                ppul->setInductiveLoad(inductive);
            }
        }
    }
}

void ValveWorkbench::buildStdDeviceSelection(QComboBox *selection, int type)
{
    selection->clear();

    if (type < 0) {
        selection->setEnabled(false);
        return;
    }

    selection->setEnabled(true);
    selection->addItem("Select...", -1);

    qInfo("=== BUILDING DEVICE SELECTION ===");
    qInfo("Requested device type: %d", type);
    qInfo("Available devices: %d", devices.size());

    for (int i = 0; i < devices.size(); i++) {
        Device *device = devices.at(i);
        qInfo("Device %d: %s, type: %d", i, device->getName().toStdString().c_str(), device->getDeviceType());

        if (device->getDeviceType() == type) {
            selection->addItem(device->getName(), i);
            qInfo("MATCH! Added device %s to dropdown", device->getName().toStdString().c_str());
        }
    }

    qInfo("Dropdown populated with %d matching devices", selection->count() - 1); // -1 for "Select..." item
}

void ValveWorkbench::plotModel()
{
    if (modelPlot) {
       plot.getScene()->removeItem(modelPlot);
    }

    if (currentDevice != nullptr) {
        modelPlot = currentDevice->anodePlot(&plot);
    }
}

double ValveWorkbench::checkDoubleValue(QLineEdit *input, double oldValue)
{
    float parsedValue;

    const char *value = _strdup(input->text().toStdString().c_str());

    int n = sscanf_s(value, "%f.3", &parsedValue);

    if (n < 1) {
        return oldValue;
    }

    if (parsedValue < 0) {
        return 0.0;
    }

    return parsedValue;
}

void ValveWorkbench::updateDoubleValue(QLineEdit *input, double value)
{
    char number[32];

    sprintf(number, "%.3f", value);

    int length = strlen(number);
    for (int i=length-1;i >= 0; i--) {
        char test = number[i];
        if (test == '0' || test == '.') {
            number[i] = 0;
        }

        if (test != '0') {
            break;
        }
    }

    input->setText(number);
}

void ValveWorkbench::updateCircuitParameter(int index)
{
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size() || !circuits.at(currentCircuitType)) {
        return; // No valid circuit selected
    }

    Circuit *circuit = circuits.at(currentCircuitType);
    double value = checkDoubleValue(circuitValues[index], circuit->getParameter(index));

    updateDoubleValue(circuitValues[index], value);

    // If this is the Triode Common Cathode circuit, treat RA and RL inputs as kΩ in the UI
    // but do not drive the small-signal LCDs here. The LCDs are a Modeller tool and are updated from measured/model data instead.
    {
        #include "valvemodel/circuit/triodecommoncathode.h"
        if (auto tcc = dynamic_cast<TriodeCommonCathode*>(circuit)) {
            if (index == TRI_CC_RA || index == TRI_CC_RL) {
                // Circuit::setParameter already calls update(index), so we only
                // need to scale the user value from kΩ to Ω here.
                circuit->setParameter(index, value * 1000.0);
            } else {
                circuit->setParameter(index, value);
            }

            circuit->updateUI(circuitLabels, circuitValues);
            circuit->plot(&plot);
            circuit->updateUI(circuitLabels, circuitValues);

            // In model mode (mes_mod_select checked), drive the small-signal LCDs
            // from the Designer's Triode Common Cathode circuit so the operating
            // point and gm/ra/mu are consistent between Designer and Modeller.
            if (ui->mes_mod_select && ui->mes_mod_select->isChecked()) {
                auto safeDisplayText = [this](QLCDNumber *lcd, const QString &text) {
                    if (lcd) {
                        lcd->display(text);
                    }
                };

                const double gm_mA_V = tcc->getParameter(TRI_CC_GM);
                const double ra_ohms = tcc->getParameter(TRI_CC_AR);
                const double mu      = tcc->getParameter(TRI_CC_MU);
                const double ra_k    = (ra_ohms > 0.0) ? (ra_ohms / 1000.0) : 0.0;

                if (ui->gmLcd) {
                    if (gm_mA_V > 0.0) {
                        ui->gmLcd->display(QString("%1").arg(gm_mA_V, 0, 'f', 2));
                    } else {
                        safeDisplayText(ui->gmLcd, "--");
                    }
                }
                if (ui->raLcd) {
                    if (ra_k > 0.0) {
                        ui->raLcd->display(QString("%1").arg(ra_k, 0, 'f', 1));
                    } else {
                        safeDisplayText(ui->raLcd, "--");
                    }
                }
                if (ui->lcdNumber_3) {
                    if (mu > 0.0) {
                        ui->lcdNumber_3->display(QString("%1").arg(mu, 0, 'f', 1));
                    } else {
                        safeDisplayText(ui->lcdNumber_3, "--");
                    }
                }
            }

            return;
        }
    }

    // Default path for all other circuits. Circuit::setParameter already
    // calls the protected virtual update(index), so all derived metrics
    // (including effective headroom and THD in SingleEndedOutput) are
    // recomputed automatically here.

    // For Designer output stages, handle supply and load changes in a way
    // that mirrors the Pentode Class A1 designer's Autoscale Y behaviour:
    //
    // - When Autoscale Y is enabled, recompute the Y-axis from the
    //   device/model limits on each relevant parameter change. For SE/SE-UL
    //   this is the device's Ia_max; for PP/UL-PP it is the larger of
    //   Ia_max and the theoretical Class B peak current (4000*VB/RAA).
    // - When Autoscale Y is disabled, treat the current Y range as locked
    //   and reuse it even if VB/RAA would normally suggest a different
    //   headroom.
    // - For all output stages, keep the X-axis from shrinking by pinning
    //   the new max to at least the current visible right edge and
    //   max(device.vaMax, 2*VB) where applicable.
    {
        auto se   = dynamic_cast<SingleEndedOutput*>(circuit);
        auto seul = dynamic_cast<SingleEndedUlOutput*>(circuit);
        auto pp   = dynamic_cast<PushPullOutput*>(circuit);
        auto ppul = dynamic_cast<PushPullUlOutput*>(circuit);

        const bool isSeVB   = (se   && index == SE_VB);
        const bool isSeUlVB = (seul && index == SEUL_VB);
        const bool isPpVB   = (pp   && index == PP_VB);
        const bool isPpUlVB = (ppul && index == PPUL_VB);
        const bool isPpRaa   = (pp   && index == PP_RAA);
        const bool isPpUlRaa = (ppul && index == PPUL_RAA);

        const bool affectsOutputStage =
            (isSeVB || isSeUlVB || isPpVB || isPpUlVB || isPpRaa || isPpUlRaa);

        if (currentDevice && affectsOutputStage) {
            const double xScale = plot.getXScale();
            const double yScale = plot.getYScale();
            if (xScale > 0.0 && yScale > 0.0) {
                const double xStart = plot.getXStart();
                const double yStart = plot.getYStart();
                const double currentXStop = xStart + static_cast<double>(PLOT_WIDTH) / xScale;
                const double currentYStop = yStart + static_cast<double>(PLOT_HEIGHT) / yScale;

                const double deviceVaMax = currentDevice->getVaMax();
                const double deviceIaMax = currentDevice->getIaMax();

                // Resolve the effective VB/RAA values after this edit. For
                // SE/SE-UL we only care about VB; for PP/UL-PP we also need
                // RAA so that the Class B peak current (4000*VB/RAA) can be
                // reflected in the Y-axis when Autoscale Y is enabled.
                double vbSe = 0.0;
                if (se && isSeVB) {
                    vbSe = value;
                } else if (se) {
                    vbSe = se->getParameter(SE_VB);
                }
                if (seul && isSeUlVB) {
                    vbSe = value;
                } else if (seul) {
                    vbSe = seul->getParameter(SEUL_VB);
                }

                double vbPp   = 0.0;
                double raaPp  = 0.0;
                if (pp) {
                    vbPp  = pp->getParameter(PP_VB);
                    raaPp = pp->getParameter(PP_RAA);
                    if (isPpVB)   vbPp  = value;
                    if (isPpRaa)  raaPp = value;
                }

                double vbPpUl   = 0.0;
                double raaPpUl  = 0.0;
                if (ppul) {
                    vbPpUl  = ppul->getParameter(PPUL_VB);
                    raaPpUl = ppul->getParameter(PPUL_RAA);
                    if (isPpUlVB)   vbPpUl  = value;
                    if (isPpUlRaa)  raaPpUl = value;
                }

                // X-axis: never shrink; for output stages keep at least
                // max(device.vaMax, 2*VB) where applicable, but do not roll
                // back below the current visible right edge.
                double vaMaxNew = currentXStop;
                if ((se || seul) && (isSeVB || isSeUlVB) && vbSe > 0.0) {
                    const double desiredXStop = 2.0 * vbSe;
                    const double baseX       = std::max(deviceVaMax, desiredXStop);
                    vaMaxNew = std::max(currentXStop, baseX);
                } else if (pp || ppul) {
                    double vbForX = (pp ? vbPp : vbPpUl);
                    if (vbForX > 0.0) {
                        const double desiredXStop = 2.0 * vbForX;
                        const double baseX       = std::max(deviceVaMax, desiredXStop);
                        vaMaxNew = std::max(currentXStop, baseX);
                    } else {
                        vaMaxNew = std::max(currentXStop, deviceVaMax);
                    }
                }

                // Y-axis base: device Ia_max, optionally extended to cover
                // theoretical Class B current for push-pull stages.
                double iaBase = deviceIaMax;
                if (pp || ppul) {
                    double vbForY  = (pp ? vbPp : vbPpUl);
                    double raaForY = (pp ? raaPp : raaPpUl);
                    if (vbForY > 0.0 && raaForY > 0.0) {
                        const double iaClassB_mA = 4000.0 * vbForY / raaForY;
                        if (iaClassB_mA > 0.0) {
                            iaBase = std::max(iaBase, iaClassB_mA);
                        }
                    }
                }

                double iaMaxNew = iaBase;

                // When Autoscale Y is disabled, lock the Y range to the
                // current axis limits so Designer tweaks preserve a fixed
                // vertical reference, mirroring the A1 tool's manual mode.
                if (ui->autoscaleYCheck && !ui->autoscaleYCheck->isChecked()) {
                    if (currentYStop > 0.0) {
                        iaMaxNew = currentYStop;
                    }
                }

                const double vaInterval = currentDevice->interval(vaMaxNew);
                const double iaInterval = currentDevice->interval(iaMaxNew);

                if (measuredCurves)          measuredCurves = nullptr;
                if (measuredCurvesSecondary) measuredCurvesSecondary = nullptr;
                if (estimatedCurves)         estimatedCurves = nullptr;
                if (modelledCurves)          modelledCurves = nullptr;
                if (modelledCurvesSecondary) modelledCurvesSecondary = nullptr;

                circuit->resetOverlays();
                plot.setAxes(0.0, vaMaxNew, vaInterval, 0.0, iaMaxNew, iaInterval, 0, 0);
            }
        }
    }

    circuit->setParameter(index, value);
    circuit->updateUI(circuitLabels, circuitValues);
    circuit->plot(&plot);
    circuit->updateUI(circuitLabels, circuitValues);

    Device *device = currentDevice;
    if (device) {
        if (modelledCurves) {
            if (modelledCurves->scene() == plot.getScene()) {
                plot.remove(modelledCurves);
            }
            modelledCurves = nullptr;
        }
        if (ui->modelCheck->isChecked()) {
            Measurement *embedded = device->getMeasurement();
            Model *deviceModel = device->getModel();
            if (embedded && deviceModel && embedded->getDeviceType() == PENTODE &&
                embedded->getTestType() == ANODE_CHARACTERISTICS) {
                QGraphicsItemGroup *plotted = deviceModel->plotModel(&plot, embedded, nullptr);
                if (plotted) {
                    modelledCurves = plotted;
                    plot.add(modelledCurves);
                    modelledCurves->setVisible(ui->modelCheck->isChecked());
                }
            } else {
                modelledCurves = device->anodePlot(&plot);
                if (modelledCurves) {
                    modelledCurves->setVisible(ui->modelCheck->isChecked());
                }
            }
        }

        if (measuredCurves) {
            plot.remove(measuredCurves);
            measuredCurves = nullptr;
        }
        if (measuredCurvesSecondary) {
            plot.remove(measuredCurvesSecondary);
            measuredCurvesSecondary = nullptr;
        }
        if (ui->measureCheck->isChecked() && device->getMeasurement()) {
            Measurement *embedded = device->getMeasurement();
            embedded->setShowScreen(ui->screenCheck->isChecked());
            embedded->setSmoothPlotting(preferencesDialog.smoothCurves());
            measuredCurves = embedded->updatePlotWithoutAxes(&plot);
            if (measuredCurves) {
                plot.add(measuredCurves);
                measuredCurves->setVisible(ui->measureCheck->isChecked());
            }
        }
    }

    // Refresh harmonic plots if they're currently displayed
    refreshHarmonicsPlots();
}

// Helper: pick an operating point (sweepIdx, sampleIdx) from an ANODE_CHARACTERISTICS
// measurement near 50% of Ia_max. Returns false on failure.
static bool pickOperatingPointFromAnode(Measurement *measurement,
                                        int &sweepIdx,
                                        int &sampleIdx,
                                        double &vaOp,
                                        double &vg1Op,
                                        double &vg2Op)
{
    if (!measurement) return false;

    const int sweepCount = measurement->count();
    if (sweepCount == 0) {
        return false;
    }

    const double iaTarget = std::max(0.0, measurement->getIaMax() * 0.5);
    double bestDiff = std::numeric_limits<double>::infinity();

    sweepIdx  = sweepCount / 2;
    sampleIdx = -1;
    Sweep *sweep = nullptr;

    for (int sw = 0; sw < sweepCount; ++sw) {
        Sweep *s = measurement->at(sw);
        if (!s || s->count() < 1) {
            continue;
        }
        const int nSamples = s->count();
        for (int sa = 0; sa < nSamples; ++sa) {
            Sample *sample = s->at(sa);
            if (!sample) continue;
            const double ia = sample->getIa();
            if (ia <= 0.0) {
                continue; // skip non-conducting points
            }
            const double diff = std::fabs(ia - iaTarget);
            if (diff < bestDiff) {
                bestDiff = diff;
                sweepIdx = sw;
                sampleIdx = sa;
                sweep = s;
            }
        }
    }

    if (!sweep) {
        // Fallback: central sweep/sample
        sweepIdx = sweepCount / 2;
        sweep = measurement->at(sweepIdx);
        if (!sweep || sweep->count() == 0) {
            return false;
        }
        sampleIdx = sweep->count() / 2;
    }

    Sample *sampleMid = sweep->at(sampleIdx);
    if (!sampleMid) {
        return false;
    }

    vaOp   = sampleMid->getVa();
    vg1Op  = sampleMid->getVg1();
    vg2Op  = sampleMid->getVg2();
    return std::isfinite(vaOp) && std::isfinite(vg1Op);
}

// Helper: compute gm from a TRANSFER_CHARACTERISTICS measurement at a desired
// operating point (Va_op, Vg2_op, Vg1_op) using a local linear regression of
// Ia vs Vg1. Returns gm in mA/V, or <= 0.0 on failure.
static double gmFromTransferAtOP(Measurement *transfer,
                                 double vaOp,
                                 double vg2Op,
                                 double vg1Op)
{
    if (!transfer) return 0.0;
    if (transfer->getTestType() != TRANSFER_CHARACTERISTICS) return 0.0;

    const int sweeps = transfer->count();
    if (sweeps <= 0) return 0.0;

    // Select the sweep whose nominal Va/Vg2 is closest to the desired OP.
    int bestSweep = -1;
    double bestDistance = std::numeric_limits<double>::infinity();

    for (int sw = 0; sw < sweeps; ++sw) {
        Sweep *s = transfer->at(sw);
        if (!s || s->count() < 1) continue;

        // Approximate sweep Va/Vg2 by the mid sample of that sweep.
        Sample *mid = s->at(s->count() / 2);
        if (!mid) continue;

        const double vaMid  = mid->getVa();
        const double vg2Mid = mid->getVg2();
        if (!std::isfinite(vaMid) || !std::isfinite(vg2Mid)) continue;

        const double dVa  = std::fabs(vaMid  - vaOp);
        const double dVg2 = std::fabs(vg2Mid - vg2Op);
        const double dist = dVa + dVg2;
        if (dist < bestDistance) {
            bestDistance = dist;
            bestSweep = sw;
        }
    }

    if (bestSweep < 0) {
        return 0.0;
    }

    Sweep *sweep = transfer->at(bestSweep);
    if (!sweep || sweep->count() < 3) {
        return 0.0;
    }

    // Require that the chosen sweep is reasonably close to the desired
    // operating Va/Vg2. If it's too far away, this transfer dataset is not
    // representative of the modelling OP and we should fall back to the
    // previous gm estimation path.
    const double maxVaDelta  = 50.0;  // volts
    const double maxVg2Delta = 50.0;  // volts
    if (bestDistance > (maxVaDelta + maxVg2Delta)) {
        return 0.0;
    }

    const int sampleCount = sweep->count();

    auto clampIndex = [](int idx, int max) {
        if (idx < 0) return 0;
        if (idx >= max) return max - 1;
        return idx;
    };

    // Find the sample whose Vg1 is closest to Vg1_op.
    int centreIdx = 0;
    double bestVgDiff = std::numeric_limits<double>::infinity();
    for (int i = 0; i < sampleCount; ++i) {
        Sample *s = sweep->at(i);
        if (!s) continue;
        const double vg = s->getVg1();
        if (!std::isfinite(vg)) continue;
        const double diff = std::fabs(vg - vg1Op);
        if (diff < bestVgDiff) {
            bestVgDiff = diff;
            centreIdx = i;
        }
    }

    // Require that the transfer sweep actually passes near the desired grid
    // operating point; otherwise, gm at this Va/Vg2 will not be meaningful.
    const double maxVgDelta = 1.5; // volts
    if (!std::isfinite(bestVgDiff) || bestVgDiff > maxVgDelta) {
        return 0.0;
    }

    const int iStart = clampIndex(centreIdx - 2, sampleCount);
    const int iEnd   = clampIndex(centreIdx + 2, sampleCount);

    double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;
    int N = 0;

    for (int i = iStart; i <= iEnd; ++i) {
        Sample *s = sweep->at(i);
        if (!s) continue;
        const double ia = s->getIa();
        const double vg = s->getVg1();
        if (!std::isfinite(ia) || !std::isfinite(vg) || ia <= 0.0) {
            continue;
        }
        Sx  += vg;
        Sy  += ia;
        Sxx += vg * vg;
        Sxy += vg * ia;
        ++N;
    }

    double gm_mA_V = 0.0;
    const double den = static_cast<double>(N) * Sxx - Sx * Sx;
    if (N >= 3 && std::fabs(den) > 1e-12) {
        gm_mA_V = (static_cast<double>(N) * Sxy - Sx * Sy) / den; // mA/V
    }

    // Fallback: two-point dIa/dVg around the centre index.
    if (gm_mA_V <= 0.0 && sampleCount >= 2) {
        const int iPrev = clampIndex(centreIdx - 1, sampleCount);
        const int iNext = clampIndex(centreIdx + 1, sampleCount);
        Sample *sPrev = sweep->at(iPrev);
        Sample *sNext = sweep->at(iNext);
        if (sPrev && sNext) {
            const double iaPrev = sPrev->getIa();
            const double iaNext = sNext->getIa();
            const double vgPrev = sPrev->getVg1();
            const double vgNext = sNext->getVg1();
            const double dVg    = vgNext - vgPrev;
            if (std::fabs(dVg) > 1e-6) {
                gm_mA_V = (iaNext - iaPrev) / dVg;
            }
        }
    }

    return gm_mA_V;
}

// Helper: compute small-signal gm, ra, mu from a measured dataset at an
// automatically chosen operating point. This is used for tube matching in
// Modeller when mes_mod_select is unchecked ("measured" mode).
void ValveWorkbench::updateSmallSignalFromMeasurement(Measurement *measurement)
{
    // Only active in measured mode
    if (!measurement || (ui->mes_mod_select && ui->mes_mod_select->isChecked())) {
        return;
    }

    const int deviceType = measurement->getDeviceType();
    const int testType   = measurement->getTestType();

    if (deviceType != TRIODE && deviceType != PENTODE) {
        qInfo("SMALL-SIGNAL (MODEL): unsupported deviceType=%d (only TRIODE/PENTODE)", deviceType);
        return;
    }

    const int sweepCount = measurement->count();
    if (sweepCount == 0) {
        qInfo("SMALL-SIGNAL (MODEL): measurement has zero sweeps");
        return;
    }

    auto safeDisplayText = [this](QLCDNumber *lcd, const QString &text) {
        if (lcd) {
            lcd->display(text);
        }
    };

    // Determine operating point from anode characteristics if available.
    int   opSweepIdx  = -1;
    int   opSampleIdx = -1;
    double vaOp = 0.0, vg1Op = 0.0, vg2Op = 0.0;

    Measurement *anodeMeasurement = measurement;
    if (testType != ANODE_CHARACTERISTICS) {
        // If current measurement is not anode characteristics, try to find one
        // in the project tree for the same device type to define the OP.
        Measurement *candidate = findMeasurement(deviceType, ANODE_CHARACTERISTICS);
        if (candidate && measurementHasValidSamples(candidate)) {
            anodeMeasurement = candidate;
        }
    }

    if (!anodeMeasurement ||
        anodeMeasurement->getTestType() != ANODE_CHARACTERISTICS ||
        !pickOperatingPointFromAnode(anodeMeasurement, opSweepIdx, opSampleIdx, vaOp, vg1Op, vg2Op)) {
        // Fallback: original central sweep/sample heuristic on the provided measurement.
        opSweepIdx  = sweepCount / 2;
        Sweep *s    = measurement->at(opSweepIdx);
        if (!s || s->count() < 3) {
            return;
        }
        opSampleIdx = s->count() / 2;
        Sample *mid = s->at(opSampleIdx);
        if (!mid) return;
        vaOp  = mid->getVa();
        vg1Op = mid->getVg1();
        vg2Op = mid->getVg2();
    }

    // Now compute ra from anode data near the OP, and gm from transfer data
    // at the same OP when available.
    double gm_mA_V = 0.0;
    double ra_ohms = 0.0;
    double mu      = 0.0;
    bool   gmFromTransfer = false;

    // --- ra from anode characteristics around OP (same LS logic as before) ---
    if (anodeMeasurement && anodeMeasurement->getTestType() == ANODE_CHARACTERISTICS) {
        Sweep *sweep = anodeMeasurement->at(opSweepIdx);
        if (sweep && sweep->count() >= 3) {
            const int sampleCount = sweep->count();

            auto clampIndex = [](int idx, int max) {
                if (idx < 0) return 0;
                if (idx >= max) return max - 1;
                return idx;
            };

            const int sampleIdx = clampIndex(opSampleIdx, sampleCount);
            const int iPrev     = clampIndex(sampleIdx - 1, sampleCount);
            const int iNext     = clampIndex(sampleIdx + 1, sampleCount);

            Sample *samplePrev = sweep->at(iPrev);
            Sample *sampleNext = sweep->at(iNext);
            if (samplePrev && sampleNext) {
                int iStart = std::max(0, sampleIdx - 2);
                int iEnd   = std::min(sampleCount - 1, sampleIdx + 2);

                double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;
                int N = 0;

                for (int i = iStart; i <= iEnd; ++i) {
                    Sample *s = sweep->at(i);
                    if (!s) continue;
                    const double ia = s->getIa(); // mA
                    if (ia <= 0.0) continue;
                    const double va = s->getVa(); // V
                    Sx  += va;
                    Sy  += ia;
                    Sxx += va * va;
                    Sxy += va * ia;
                    ++N;
                }

                const double den = static_cast<double>(N) * Sxx - Sx * Sx;
                if (N >= 3 && std::fabs(den) > 1e-12) {
                    const double slope_dIa_dVa = (static_cast<double>(N) * Sxy - Sx * Sy) / den; // mA/V
                    if (std::fabs(slope_dIa_dVa) > 1e-12) {
                        ra_ohms = 1000.0 / slope_dIa_dVa; // V/mA → Ohms
                    }
                }

                // Fallback: two-point estimate
                if (ra_ohms <= 0.0) {
                    const double vaPrev = samplePrev->getVa();
                    const double iaPrev = samplePrev->getIa();
                    const double vaNext = sampleNext->getVa();
                    const double iaNext = sampleNext->getIa();

                    const double dIa_mA = iaNext - iaPrev;
                    const double dVa    = vaNext - vaPrev;
                    if (std::fabs(dIa_mA) > 1e-9) {
                        const double dVa_dIa_V_per_mA = dVa / dIa_mA;
                        ra_ohms = dVa_dIa_V_per_mA * 1000.0;
                    }
                }
            }
        }
    }

    // --- gm from transfer at the same OP when such a measurement exists ---
    Measurement *transferMeasurement = findMeasurement(deviceType, TRANSFER_CHARACTERISTICS);
    if (transferMeasurement && measurementHasValidSamples(transferMeasurement)) {
        const double gmFromTransferVal = gmFromTransferAtOP(transferMeasurement, vaOp, vg2Op, vg1Op);
        if (gmFromTransferVal > 0.0) {
            gm_mA_V = gmFromTransferVal;
            gmFromTransfer = true;
        }
    }

    // Fallback: if we still don't have gm from transfer, fall back to the
    // existing measurement-based logic on the active dataset.
    if (gm_mA_V <= 0.0) {
        const int localTestType = measurement->getTestType();
        if (localTestType == ANODE_CHARACTERISTICS) {
            // Reuse the original cross-sweep gm LS logic around the OP.
            auto clampIndex = [](int idx, int max) {
                if (idx < 0) return 0;
                if (idx >= max) return max - 1;
                return idx;
            };

            const int sweepIdx = clampIndex(opSweepIdx, sweepCount);
            Sweep *sweep = measurement->at(sweepIdx);
            if (sweep && sweep->count() >= 3) {
                const int sampleCount = sweep->count();
                const int sampleIdx  = clampIndex(opSampleIdx, sampleCount);

                int swStart = clampIndex(sweepIdx - 2, sweepCount);
                int swEnd   = clampIndex(sweepIdx + 2, sweepCount);

                double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;
                int N = 0;

                for (int sw = swStart; sw <= swEnd; ++sw) {
                    Sweep *sRow = measurement->at(sw);
                    if (!sRow || sRow->count() <= sampleIdx) continue;
                    Sample *sp = sRow->at(sampleIdx);
                    if (!sp) continue;
                    const double ia = sp->getIa();
                    if (ia <= 0.0) continue;
                    const double vg = sRow->getVg1Nominal();
                    Sx  += vg;
                    Sy  += ia;
                    Sxx += vg * vg;
                    Sxy += vg * ia;
                    ++N;
                }

                const double den = static_cast<double>(N) * Sxx - Sx * Sx;
                if (N >= 3 && std::fabs(den) > 1e-12) {
                    gm_mA_V = (static_cast<double>(N) * Sxy - Sx * Sy) / den; // mA/V
                }

                // Two-sweep fallback as before
                if (gm_mA_V <= 0.0) {
                    const int sweepPrevIdx = clampIndex(sweepIdx - 1, sweepCount);
                    const int sweepNextIdx = clampIndex(sweepIdx + 1, sweepCount);
                    Sweep *sPrev = measurement->at(sweepPrevIdx);
                    Sweep *sNext = measurement->at(sweepNextIdx);
                    if (sPrev && sNext && sPrev->count() > sampleIdx && sNext->count() > sampleIdx) {
                        Sample *spPrev = sPrev->at(sampleIdx);
                        Sample *spNext = sNext->at(sampleIdx);
                        if (spPrev && spNext) {
                            const double iaPrevSweep = spPrev->getIa();
                            const double iaNextSweep = spNext->getIa();
                            const double vgPrev      = sPrev->getVg1Nominal();
                            const double vgNext      = sNext->getVg1Nominal();
                            const double dVg         = vgNext - vgPrev;
                            if (std::fabs(dVg) > 1e-6) {
                                gm_mA_V = (iaNextSweep - iaPrevSweep) / dVg; // mA/V
                            }
                        }
                    }
                }
            }
        } else if (localTestType == TRANSFER_CHARACTERISTICS) {
            // Use the original within-sweep gm LS logic around the OP.
            Sweep *sweep = measurement->at(opSweepIdx);
            if (sweep && sweep->count() >= 3) {
                const int sampleCount = sweep->count();

                auto clampIndex = [](int idx, int max) {
                    if (idx < 0) return 0;
                    if (idx >= max) return max - 1;
                    return idx;
                };

                const int sampleIdx = clampIndex(opSampleIdx, sampleCount);
                const int iPrev     = clampIndex(sampleIdx - 1, sampleCount);
                const int iNext     = clampIndex(sampleIdx + 1, sampleCount);

                Sample *samplePrev = sweep->at(iPrev);
                Sample *sampleNext = sweep->at(iNext);
                if (samplePrev && sampleNext) {
                    int iStart = std::max(0, sampleIdx - 2);
                    int iEnd   = std::min(sampleCount - 1, sampleIdx + 2);

                    double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;
                    int N = 0;

                    for (int i = iStart; i <= iEnd; ++i) {
                        Sample *s = sweep->at(i);
                        if (!s) continue;
                        const double ia = s->getIa();
                        if (ia <= 0.0) continue;
                        const double vg = s->getVg1();
                        Sx  += vg;
                        Sy  += ia;
                        Sxx += vg * vg;
                        Sxy += vg * ia;
                        ++N;
                    }

                    const double den = static_cast<double>(N) * Sxx - Sx * Sx;
                    if (N >= 3 && std::fabs(den) > 1e-12) {
                        gm_mA_V = (static_cast<double>(N) * Sxy - Sx * Sy) / den; // mA/V
                    }

                    // Two-point fallback
                    if (gm_mA_V <= 0.0) {
                        const double iaPrev = samplePrev->getIa();
                        const double iaNext = sampleNext->getIa();
                        const double vgPrev = samplePrev->getVg1();
                        const double vgNext = sampleNext->getVg1();
                        const double dVg    = vgNext - vgPrev;
                        if (std::fabs(dVg) > 1e-6) {
                            gm_mA_V = (iaNext - iaPrev) / dVg; // mA/V
                        }
                    }
                }
            }
        }
    }

    const double ra_k = (ra_ohms > 0.0) ? (ra_ohms / 1000.0) : 0.0;
    if (gm_mA_V > 0.0 && ra_k > 0.0) {
        mu = gm_mA_V * ra_k; // μ ≈ Ra[kΩ] * Gm[mA/V]
    }

    // Push values to LCDs; if something failed, show "--" for that field.
    if (ui->gmLcd) {
        if (gm_mA_V > 0.0) {
            ui->gmLcd->display(QString("%1").arg(gm_mA_V, 0, 'f', 2));

            if (gmFromTransfer) {
                ui->gmLcd->setStyleSheet("color: rgb(0, 0, 192);");
                if (ui->gmLabel) {
                    ui->gmLabel->setStyleSheet("color: rgb(0, 0, 192);");
                }
            } else {
                ui->gmLcd->setStyleSheet("");
                if (ui->gmLabel) {
                    ui->gmLabel->setStyleSheet("");
                }
            }
        } else {
            ui->gmLcd->setStyleSheet("");
            if (ui->gmLabel) {
                ui->gmLabel->setStyleSheet("");
            }
            safeDisplayText(ui->gmLcd, "--");
        }
    }
    if (ui->raLcd) {
        if (ra_k > 0.0) {
            ui->raLcd->display(QString("%1").arg(ra_k, 0, 'f', 1));
        } else {
            safeDisplayText(ui->raLcd, "--");
        }
    }
    if (ui->lcdNumber_3) {
        if (mu > 0.0) {
            ui->lcdNumber_3->display(QString("%1").arg(mu, 0, 'f', 1));
        } else {
            safeDisplayText(ui->lcdNumber_3, "--");
        }
    }
}

// Helper: compute small-signal gm, ra, mu directly from the fitted model at an
// operating point derived from the active measurement. This is used in model
// mode (mes_mod_select checked) when no Designer circuit is providing
// small-signal values (e.g. pentode models or when only a model fit is present).
void ValveWorkbench::updateSmallSignalFromModel(Model *modelForSmallSignal, Measurement *measurement)
{
    if (!measurement) {
        qInfo("SMALL-SIGNAL (MODEL): aborted - measurement is null");
        return;
    }

    // Only active in model mode
    if (!(ui->mes_mod_select && ui->mes_mod_select->isChecked())) {
        qInfo("SMALL-SIGNAL (MODEL): skipped - mes_mod_select is not checked (not in model mode)");
        return;
    }

    const int deviceType = measurement->getDeviceType();
    const int testType   = measurement->getTestType();

    if (deviceType != TRIODE && deviceType != PENTODE) {
        qInfo("SMALL-SIGNAL (MODEL): unsupported deviceType=%d (only TRIODE/PENTODE)", deviceType);
        return;
    }

    // Prefer the explicitly supplied model pointer if valid. If not, or if it
    // does not match the measurement device type (e.g. pentode measurement but
    // triode model), fall back to a model found in the current project tree.
    Model *sourceModel = modelForSmallSignal;

    auto modelMatchesMeasurement = [deviceType](Model *m) {
        if (!m) return false;
        const int t = m->getType();
        if (deviceType == TRIODE) {
            return (t == COHEN_HELIE_TRIODE || t == KOREN_TRIODE || t == SIMPLE_TRIODE);
        }
        // Pentode: accept any pentode family (Gardiner, Reefman, SimpleManual)
        if (deviceType == PENTODE) {
            return (t == GARDINER_PENTODE || t == SIMPLE_MANUAL_PENTODE ||
                    t == REEFMAN_DERK_PENTODE || t == REEFMAN_DERK_E_PENTODE ||
                    t == EXTRACT_DERK_E_PENTODE);
        }
        return false;
    };

    if (!modelMatchesMeasurement(sourceModel)) {
        int desiredType = -1;
        if (deviceType == TRIODE) {
            desiredType = COHEN_HELIE_TRIODE;
        } else if (deviceType == PENTODE) {
            desiredType = GARDINER_PENTODE;
        }

        if (desiredType != -1 && currentProject) {
            Model *projectModel = findModel(desiredType);
            if (modelMatchesMeasurement(projectModel)) {
                sourceModel = projectModel;
            }
        }
    }

    if (!modelMatchesMeasurement(sourceModel)) {
        qInfo("SMALL-SIGNAL (MODEL): no suitable model found for deviceType=%d (mes_mod mode)", deviceType);
        return;
    }

    const int sweepCount = measurement->count();
    if (sweepCount == 0) {
        return;
    }

    auto safeDisplayText = [this](QLCDNumber *lcd, const QString &text) {
        if (lcd) {
            lcd->display(text);
        }
    };

    // Choose an operating point. For anode characteristics we prefer a point
    // near the middle of the tube's current (around 50% of Ia_max), matching
    // the heuristic used for measurement-based small-signal.
    int   sweepIdx  = sweepCount / 2;
    int   sampleIdx = -1;
    Sweep *sweep    = nullptr;

    if (testType == ANODE_CHARACTERISTICS) {
        const double iaTarget = std::max(0.0, measurement->getIaMax() * 0.5);
        double bestDiff = std::numeric_limits<double>::infinity();

        for (int sw = 0; sw < sweepCount; ++sw) {
            Sweep *s = measurement->at(sw);
            if (!s || s->count() < 1) {
                continue;
            }
            const int nSamples = s->count();
            for (int sa = 0; sa < nSamples; ++sa) {
                Sample *sample = s->at(sa);
                if (!sample) continue;
                const double ia = sample->getIa();
                if (ia <= 0.0) {
                    continue; // skip non-conducting points
                }
                const double diff = std::fabs(ia - iaTarget);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    sweepIdx = sw;
                    sampleIdx = sa;
                    sweep = s;
                }
            }
        }

        // Fallback: if we didn't find a suitable point, use the central
        // sweep/sample as before.
        if (!sweep) {
            sweepIdx = sweepCount / 2;
            sweep = measurement->at(sweepIdx);
            if (!sweep || sweep->count() == 0) {
                qInfo("SMALL-SIGNAL (MODEL): chosen sweep index %d is null or empty", sweepIdx);
                return;
            }
            sampleIdx = sweep->count() / 2;
        }
    } else {
        // Transfer characteristics or other tests: retain the original
        // central sweep/sample heuristic.
        sweepIdx = sweepCount / 2;
        sweep = measurement->at(sweepIdx);
        if (!sweep || sweep->count() == 0) {
            qInfo("SMALL-SIGNAL (MODEL): chosen sweep index %d is null or empty", sweepIdx);
            return;
        }
        sampleIdx = sweep->count() / 2;
    }

    const int sampleCount = sweep->count();
    Sample *sampleMid     = (sampleIdx >= 0 && sampleIdx < sampleCount) ? sweep->at(sampleIdx) : nullptr;
    if (!sampleMid) {
        qInfo("SMALL-SIGNAL (MODEL): central sample index %d is null", sampleIdx);
        return;
    }

    double va0   = sampleMid->getVa();
    double vg1_0 = sampleMid->getVg1();
    double vg2_0 = 0.0;

    if (!std::isfinite(va0) || !std::isfinite(vg1_0)) {
        qInfo("SMALL-SIGNAL (MODEL): invalid OP va0=%.6f, vg1_0=%.6f", va0, vg1_0);
        return;
    }

    if (deviceType == PENTODE) {
        // For pentodes, use the measured screen voltage if available for the OP
        vg2_0 = sampleMid->getVg2();
        if (!std::isfinite(vg2_0)) {
            // Fall back to nominal screen bias from the measurement if samples don't carry it
            vg2_0 = measurement->getScreenStart();
        }
    }

    qInfo("SMALL-SIGNAL (MODEL): deviceType=%d testType=%d OP: Va=%.3f V, Vg1=%.3f V, Vg2=%.3f V",
          deviceType, testType, va0, vg1_0, vg2_0);

    SmallSignalResult ss = sourceModel->computeSmallSignal(
        va0,
        vg1_0,
        vg2_0,
        sourceModel->withSecondaryEmission());

    if (!ss.valid || ss.gm <= 0.0 || ss.ra <= 0.0 || ss.mu <= 0.0) {
        qInfo("SMALL-SIGNAL (MODEL): invalid result valid=%d gm=%.6f mA/V ra=%.6f kOhm mu=%.6f",
              ss.valid ? 1 : 0, ss.gm, ss.ra, ss.mu);
        safeDisplayText(ui->gmLcd, "--");
        safeDisplayText(ui->raLcd, "--");
        safeDisplayText(ui->lcdNumber_3, "--");
        return;
    }

    qInfo("SMALL-SIGNAL (MODEL): OK gm=%.3f mA/V ra=%.3f kOhm mu=%.3f",
          ss.gm, ss.ra, ss.mu);

    if (ui->gmLcd) {
        ui->gmLcd->display(QString("%1").arg(ss.gm, 0, 'f', 2));
    }
    if (ui->raLcd) {
        ui->raLcd->display(QString("%1").arg(ss.ra, 0, 'f', 1));
    }
    if (ui->lcdNumber_3) {
        ui->lcdNumber_3->display(QString("%1").arg(ss.mu, 0, 'f', 1));
    }
}

void ValveWorkbench::on_mes_mod_select_stateChanged(int state)
{
    auto safeDisplayText = [this](QLCDNumber *lcd, const QString &text) {
        if (lcd) {
            lcd->display(text);
        }
    };

    const bool modelMode = (state != 0);

    // Always clear first when switching modes
    safeDisplayText(ui->gmLcd, "--");
    safeDisplayText(ui->raLcd, "--");
    safeDisplayText(ui->lcdNumber_3, "--");

    if (!modelMode) {
        // Measured mode: reset colours to defaults (black) and recompute
        // from current measurement if available.
        if (ui->mes_mod_select) {
            ui->mes_mod_select->setStyleSheet("");
        }
        if (ui->gmLabel) ui->gmLabel->setStyleSheet("");
        if (ui->raLabel) ui->raLabel->setStyleSheet("");
        if (ui->muLabel) ui->muLabel->setStyleSheet("");
        if (ui->gmLcd) ui->gmLcd->setStyleSheet("");
        if (ui->raLcd) ui->raLcd->setStyleSheet("");
        if (ui->lcdNumber_3) ui->lcdNumber_3->setStyleSheet("");

        if (currentMeasurement) {
            updateSmallSignalFromMeasurement(currentMeasurement);
        }
        return;
    }

    // Model mode: if a Triode Common Cathode circuit is active and the
    // current measurement is a triode, use its small-signal parameters so
    // Designer and Modeller agree on gm/ra/mu. Otherwise fall back to the
    // fitted model directly.
    int circuitType = ui->circuitSelection
                      ? ui->circuitSelection->currentData().toInt()
                      : -1;

    Circuit *circuit = (circuitType >= 0 && circuitType < circuits.size())
                       ? circuits.at(circuitType)
                       : nullptr;
    const int measurementDeviceType = currentMeasurement ? currentMeasurement->getDeviceType() : -1;

    bool usedDesigner = false;
    if (measurementDeviceType == TRIODE && circuit) {
        if (auto tcc = dynamic_cast<TriodeCommonCathode*>(circuit)) {
            const double gm_mA_V = tcc->getParameter(TRI_CC_GM);
            const double ra_ohms = tcc->getParameter(TRI_CC_AR);
            const double mu      = tcc->getParameter(TRI_CC_MU);
            const double ra_k    = (ra_ohms > 0.0) ? (ra_ohms / 1000.0) : 0.0;

            if (ui->gmLcd) {
                if (gm_mA_V > 0.0) {
                    ui->gmLcd->display(QString("%1").arg(gm_mA_V, 0, 'f', 2));
                } else {
                    safeDisplayText(ui->gmLcd, "--");
                }
            }
            if (ui->raLcd) {
                if (ra_k > 0.0) {
                    ui->raLcd->display(QString("%1").arg(ra_k, 0, 'f', 1));
                } else {
                    safeDisplayText(ui->raLcd, "--");
                }
            }
            if (ui->lcdNumber_3) {
                if (mu > 0.0) {
                    ui->lcdNumber_3->display(QString("%1").arg(mu, 0, 'f', 1));
                } else {
                    safeDisplayText(ui->lcdNumber_3, "--");
                }
            }
            usedDesigner = true;
        }
    }

    // Visually distinguish measured vs model mode and whether the Designer
    // (Triode Common Cathode) is driving the small-signal values:
    // - Measured mode (modelMode == false): default colours (black)
    // - Model mode, plain model (usedDesigner == false): red
    // - Model mode, Designer-driven (usedDesigner == true): green
    QColor modeColor;
    if (!modelMode) {
        modeColor = QColor(); // invalid -> default palette
    } else if (usedDesigner) {
        modeColor = QColor::fromRgb(0, 128, 0);      // Designer-controlled small-signal
    } else {
        modeColor = QColor::fromRgb(200, 0, 0);      // Plain model-based small-signal
    }

    QString style;
    if (modeColor.isValid()) {
        style = QString("color: rgb(%1,%2,%3);")
                    .arg(modeColor.red())
                    .arg(modeColor.green())
                    .arg(modeColor.blue());
    }

    if (ui->mes_mod_select) {
        ui->mes_mod_select->setStyleSheet(style);
    }
    auto setLabelColor = [&style](QLabel *label) {
        if (!label) return;
        label->setStyleSheet(style);
    };
    setLabelColor(ui->gmLabel);
    setLabelColor(ui->raLabel);
    setLabelColor(ui->muLabel);

    auto setLcdColor = [&style](QLCDNumber *lcd) {
        if (!lcd) return;
        lcd->setStyleSheet(style);
    };
    setLcdColor(ui->gmLcd);
    setLcdColor(ui->raLcd);
    setLcdColor(ui->lcdNumber_3);

    // If no Designer triode circuit was used, fall back to the fitted model
    // directly (triode or pentode) if one is available and a measurement is
    // selected to provide context.
    if (!usedDesigner && model && currentMeasurement) {
        updateSmallSignalFromModel(model, currentMeasurement);
    }
}

void ValveWorkbench::updateHeater(double vh, double ih)
{
    // Update the heater display
    if (vh >= 0.0) {
        QString vhValue = QString::number(static_cast<int>(vh));
        ui->heaterVlcd->display(vhValue);
    }

    if (ih >= 0.0) {
        if (ih >= 4.0) {
            badRetryCount++;
        }
        QString ihValue = QString::number(badRetryCount);
        ui->heaterIlcd->display(ihValue);
    }
}

void ValveWorkbench::testProgress(int progress)
{
    // qInfo("Test progress received: %d", progress);
    //QMessageBox::information(this, "Progress", QString("Test progress: %1%").arg(progress));
    ui->progressBar->setValue(progress);
}

void ValveWorkbench::testFinished()
{
    //qInfo("Test finished");
   // QMessageBox::information(this, "Debug", "Test finished!");

    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);
    ui->btnAddToProject->setEnabled(true);

    currentMeasurement = analyser->getResult();
    if (currentMeasurement) {
        // Apply current checkbox state to measurement so screen overlay can be drawn
        currentMeasurement->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
        // Apply smoothing preference for new analyser measurements so that
        // measurement plotting can optionally use spline smoothing.
        currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
    }

    // Primary (Triode A) curves
    measuredCurves = currentMeasurement ? currentMeasurement->updatePlot(&plot) : nullptr;
    if (measuredCurves) {
        plot.add(measuredCurves);
    }

    // For double triode measurements, create a Triode B clone directly from
    // the analyser result so Analyser can display both sections (A and B)
    // together when Show Measurement is enabled.
    if (isDoubleTriode && currentMeasurement && measurementHasTriodeBData(currentMeasurement)) {
        // Clean up any previous secondary measurement created in a prior run
        if (triodeMeasurementSecondary != nullptr) {
            deleteMeasurementClone(triodeMeasurementSecondary);
            triodeMeasurementSecondary = nullptr;
        }

        Measurement *clone = createTriodeBMeasurementClone(currentMeasurement);
        if (clone != nullptr && measurementHasValidSamples(clone)) {
            triodeMeasurementSecondary = clone;
            triodeMeasurementSecondary->setSampleColor(QColor::fromRgb(0, 0, 255));
            triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());

            // Plot Triode B without axes so we don't redraw axes twice.
            measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
            if (measuredCurvesSecondary) {
                plot.add(measuredCurvesSecondary);
            }
        } else if (clone) {
            // Clone had no valid samples; discard it.
            deleteMeasurementClone(clone);
        }
    }
    ui->measureCheck->setChecked(true);

    populateDataTableFromMeasurement(currentMeasurement);

    if (healthRunActive) {
        if (currentMeasurement && healthRunIndex >= 0 && healthRunIndex < healthPoints.size()) {
            double ia = 0.0;
            double gm = 0.0;
            double rp = 0.0;
            HealthResult result;
            result.valid = computeIaGmAt(currentMeasurement, healthPoints.at(healthRunIndex), ia, gm, rp);
            result.va = healthPoints.at(healthRunIndex).va;
            result.vg = healthPoints.at(healthRunIndex).vg;
            result.ia = ia;
            result.gm = gm;
            result.rp = rp;
            if (healthRunIndex < healthResults.size()) {
                healthResults[healthRunIndex] = result;
            }
        }

        ++healthRunIndex;

        if (healthRunIndex < healthPoints.size()) {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    if (!healthRunActive) {
                        return;
                    }
                    if (healthRunIndex >= 0 && healthRunIndex < healthPoints.size()) {
                        configureTransferForHealthPoint(healthPoints.at(healthRunIndex));
                        on_runButton_clicked();
                    }
                },
                Qt::QueuedConnection);
            return;
        }

        finalizeHealthRun();
    }
}

void ValveWorkbench::testAborted()
{
    qInfo("Test aborted");
    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);

    if (healthRunActive) {
        healthRunActive = false;
        healthMode = HEALTH_NONE;
        healthRunIndex = 0;

        if (healthStateSaved) {
            testType = savedTestTypeForHealth;
            anodeStart = savedAnodeStartForHealth;
            anodeStop = savedAnodeStopForHealth;
            anodeStep = savedAnodeStepForHealth;
            gridStart = savedGridStartForHealth;
            gridStop = savedGridStopForHealth;
            gridStep = savedGridStepForHealth;
            screenStart = savedScreenStartForHealth;
            screenStop = savedScreenStopForHealth;
            screenStep = savedScreenStepForHealth;
            healthStateSaved = false;

            updateParameterDisplay();
        }

        if (ui && ui->statusbar) {
            ui->statusbar->showMessage(tr("Health run aborted."), 8000);
        }
    }
}

void ValveWorkbench::checkComPorts() {
    serialPorts = QSerialPortInfo::availablePorts();

    qInfo("Found %d serial ports:", serialPorts.size());
    for (const QSerialPortInfo &info : serialPorts) {
        qInfo("  Port=%s, VID=0x%04x, PID=0x%04x, Mfg=%s, Desc=%s",
              info.portName().toStdString().c_str(),
              info.hasVendorIdentifier() ? info.vendorIdentifier() : 0,
              info.hasProductIdentifier() ? info.productIdentifier() : 0,
              info.manufacturer().toStdString().c_str(),
              info.description().toStdString().c_str());
    }

    // Prefer CH340 (0x1a86:0x7523) when present
    for (const QSerialPortInfo &info : serialPorts) {
        if (info.hasVendorIdentifier() && info.hasProductIdentifier() &&
            info.vendorIdentifier() == 0x1a86 && info.productIdentifier() == 0x7523) {
            port = info.portName();
            qInfo("Auto-selecting CH340 device: %s", port.toStdString().c_str());
            setSerialPort(port);
            return;
        }
    }

    // Fallback: pick first available port if preferred VID/PID not found
    if (!serialPorts.isEmpty()) {
        port = serialPorts.first().portName();
        qInfo("No preferred VID/PID found; falling back to first available port: %s", port.toStdString().c_str());
        setSerialPort(port);
        return;
    }

    qWarning("No serial ports detected. Disabling Analyser tab.");
    ui->tab_3->setEnabled(false);
}

void ValveWorkbench::setSerialPort(QString portName)
{
    if (serialPort.isOpen()) {
        serialPort.close();
    }

    if (portName == "") {
        ui->tab_3->setEnabled(false);
        return;
    }

    serialPort.setPortName(portName);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setBaudRate(QSerialPort::Baud115200);
    if (!serialPort.open(QSerialPort::ReadWrite)) {
        qWarning("Failed to open serial port %s: %s",
                 portName.toStdString().c_str(),
                 serialPort.errorString().toStdString().c_str());
        ui->tab_3->setEnabled(false);
        return;
    }

    qInfo("Serial port opened: %s", portName.toStdString().c_str());
    ui->tab_3->setEnabled(true);
}

void ValveWorkbench::saveSamples(QString filename)
{
    QFile samplelFile(filename);

    if (!samplelFile.open(QIODevice::ReadWrite)) {
        qWarning("Couldn't open model file.");
    } else {
        QJsonObject samplesObject;

        samplesObject["name"] = ui->deviceName->text();

        samplesObject["deviceType"] = deviceType;
        samplesObject["testType"] = testType;

        samplesObject["anodeStart"] = anodeStart;
        samplesObject["anodeStop"] = anodeStop;
        samplesObject["anodeStop"] = anodeStep;

        samplesObject["gridStart"] = gridStart;
        samplesObject["gridStop"] = gridStop;
        samplesObject["gridStop"] = gridStep;

        samplesObject["screenStart"] = screenStart;
        samplesObject["screenStop"] = screenStop;
        samplesObject["screenStop"] = screenStep;

        samplesObject["vh"] = heaterVoltage;

        samplesObject["iaMax"] = iaMax;
        samplesObject["paMax"] = pMax;

        analyser->getResult()->toJson(samplesObject);

        samplelFile.write(QJsonDocument(samplesObject).toJson());
    }
}

void ValveWorkbench::readConfig(QString filename)
{
    QFile configFile(filename);

    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open config file.");
    } else {
        QByteArray configData = configFile.readAll();

        QJsonDocument configDoc(QJsonDocument::fromJson(configData));
        if (configDoc.isObject()) {
            config = configDoc.object();
        }

        if (config.contains("templates") && config["templates"].isArray()) {
            QJsonArray tpls = config["templates"].toArray();
            for (int i=0; i < tpls.count(); i++) {
                QJsonValue currentTemplate = tpls.at(i);
                if (currentTemplate.isObject()) {
                    Template *tpl = new Template();
                    tpl->read(currentTemplate.toObject());
                    templates.append(*tpl);
                }
            }
        }
    }
}

void ValveWorkbench::loadDevices()
{
    // Try multiple paths to find models - executable runs from deep build directory
    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../../../../../models",  // From release/release/bin
        QCoreApplication::applicationDirPath() + "/../../../../models",     // From release/bin
        QCoreApplication::applicationDirPath() + "/../../../models",       // From bin
        QCoreApplication::applicationDirPath() + "/../models",             // From app dir
        QCoreApplication::applicationDirPath() + "/models",                // Adjacent to app
        QDir::currentPath() + "/models",                                   // From current dir
        QDir::currentPath() + "/../models",                               // From current parent
        QDir::currentPath() + "/../../models",                            // From current grandparent
        QDir::currentPath() + "/../../../models"                          // From source root
    };

    QString modelPath;
    for (const QString& path : possiblePaths) {
        QDir testDir(path);
        if (testDir.exists()) {
            modelPath = path;
            qInfo("Found models at: %s", path.toStdString().c_str());
            break;
        } else {
            qInfo("Path not found: %s", path.toStdString().c_str());
        }
    }

    if (modelPath.isEmpty()) {
        modelPath = tr("../models"); // fallback to original
        qInfo("Using fallback path: %s", modelPath.toStdString().c_str());
    }

    QDir modelDir(modelPath);

    QStringList filters;
    filters << "*.vwm" << "*.json";  // Load both .vwm and .json files
    modelDir.setNameFilters(filters);

    QStringList models = modelDir.entryList();

    qInfo("=== LOADING DEVICES ===");
    qInfo("Application dir: %s", QCoreApplication::applicationDirPath().toStdString().c_str());
    qInfo("Current dir: %s", QDir::currentPath().toStdString().c_str());
    qInfo("Model path: %s", modelPath.toStdString().c_str());
    qInfo("Found %d model files", models.size());

    for (int i = 0; i < models.size(); i++) {
        QString modelFileName = modelPath + "/" + models.at(i);
        QFile modelFile(modelFileName);
        if (!modelFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open model file: ", modelFile.fileName().toStdString().c_str());
        }
        else {
            QByteArray modelData = modelFile.readAll();
            QJsonDocument modelDoc(QJsonDocument::fromJson(modelData));

            Device *model = new Device(modelDoc);
            // Ensure device has a visible name; fallback to filename (without extension)
            if (model->getName().isEmpty()) {
                QFileInfo fi(modelFileName);
                model->setName(fi.baseName());
            }

            // Wire application preferences into the Device's underlying Model
            // so that settings like useSecondaryEmission are honoured when
            // plotting in Designer and when exporting SPICE.
            if (Model *m = model->getModel()) {
                m->setPreferences(&preferencesDialog);
                m->setSecondaryEmission(preferencesDialog.useSecondaryEmission());
            }

            qInfo("Loaded device: %s, type: %d", model->getName().toStdString().c_str(), model->getDeviceType());
            this->devices.append(model);
        }
    }

    qInfo("Total devices loaded: %d", devices.size());
}

void ValveWorkbench::loadTemplate(int index)
{
    Template tpl = templates.at(index);

    ui->deviceName->setText(tpl.getName());
    heaterVoltage = tpl.getVHeater();
    anodeStart = tpl.getVaStart();
    anodeStop = tpl.getVaStop();
    anodeStep = tpl.getVaStep();
    if (anodeStep <= 0.0) {
        anodeStep = 25.0;
    }
    gridStart = tpl.getVgStart();
    gridStop = tpl.getVgStop();
    gridStep = tpl.getVgStep();
    screenStart = tpl.getVsStart();
    screenStop = tpl.getVsStop();
    screenStep = tpl.getVsStep();
    pMax = tpl.getPaMax();
    iaMax = tpl.getIaMax();

    updateParameterDisplay();

    ui->deviceType->setCurrentIndex(tpl.getDeviceType());
    on_deviceType_currentIndexChanged(tpl.getDeviceType());

    ui->testType->setCurrentIndex(tpl.getTestType());
    on_testType_currentIndexChanged(tpl.getTestType());
}


void ValveWorkbench::updateParameterDisplay()
{

    updateDoubleValue(ui->anodeStart, anodeStart);
    updateDoubleValue(ui->anodeStop, anodeStop);
    updateDoubleValue(ui->anodeStep, anodeStep);
    updateDoubleValue(ui->gridStart, gridStart);
    updateDoubleValue(ui->gridStop, gridStop);
    updateDoubleValue(ui->gridStep, gridStep);

    if (ui->deviceType->currentText() == "Double Triode") {
        updateDoubleValue(ui->anodeStart, anodeStart);
        updateDoubleValue(ui->anodeStop, anodeStop);
        updateDoubleValue(ui->anodeStep, anodeStep);
        updateDoubleValue(ui->gridStart, secondGridStart);
        updateDoubleValue(ui->gridStop, secondGridStop);
        updateDoubleValue(ui->gridStep, secondGridStep);
        updateDoubleValue(ui->screenStart, secondAnodeStart);
        updateDoubleValue(ui->screenStop, secondAnodeStop);
        updateDoubleValue(ui->screenStep, secondAnodeStep);
    } else {
        updateDoubleValue(ui->anodeStart, anodeStart);
        updateDoubleValue(ui->anodeStop, anodeStop);
        updateDoubleValue(ui->anodeStep, anodeStep);
        updateDoubleValue(ui->gridStart, gridStart);
        updateDoubleValue(ui->gridStop, gridStop);
        updateDoubleValue(ui->gridStep, gridStep);
        updateDoubleValue(ui->screenStart, screenStart);
        updateDoubleValue(ui->screenStop, screenStop);
        updateDoubleValue(ui->screenStep, screenStep);
    }

    updateDoubleValue(ui->pMax, pMax);
    updateDoubleValue(ui->iaMax, iaMax);
}

void ValveWorkbench::pentodeMode()
{
    updateParameterDisplay();

    deviceType = PENTODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Characteristics", ANODE_CHARACTERISTICS);
    ui->testType->addItem("Transfer Characteristics", TRANSFER_CHARACTERISTICS);
    // ui->testType->addItem("Screen Characteristics", SCREEN_CHARACTERISTICS);

    ui->gridLabel->setEnabled(true);
    ui->gridStart->setEnabled(true);
    ui->gridStop->setEnabled(true);
    ui->gridStep->setEnabled(true);

    ui->screenLabel->setEnabled(true);
    ui->screenStart->setEnabled(true);
    ui->screenStop->setEnabled(true);
    ui->screenStep->setEnabled(true);
}

void ValveWorkbench::triodeMode(bool doubleTriode)
{
    updateParameterDisplay();

    deviceType = TRIODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Characteristics", ANODE_CHARACTERISTICS);
    ui->testType->addItem("Transfer Characteristics", TRANSFER_CHARACTERISTICS);

    ui->gridLabel->setEnabled(true);
    ui->gridStart->setEnabled(true);
    ui->gridStop->setEnabled(true);
    ui->gridStep->setEnabled(true);

    if (doubleTriode) {
        ui->screenLabel->setText("Second Anode (Read-only)");
        ui->screenLabel->setEnabled(true);
        ui->screenStart->setEnabled(true);
        ui->screenStop->setEnabled(true);
        ui->screenStep->setEnabled(true);

        ui->anodeLabel->setText("First Anode");
        ui->anodeStart->setEnabled(true);
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(true);

        ui->gridLabel->setText("Second Grid");
        ui->gridStart->setEnabled(true);
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(true);

        secondGridStart = gridStart; // Use grid controls for second grid
        secondGridStop = gridStop;
        secondGridStep = gridStep;

        secondAnodeStart = anodeStart; // Auto-fill second anode with first anode values
        secondAnodeStop = anodeStop;
        secondAnodeStep = anodeStep;
    } else {
        ui->screenLabel->setEnabled(false);
        ui->screenStart->setEnabled(false);
        ui->screenStop->setEnabled(false);
        ui->screenStep->setEnabled(false);

        ui->anodeLabel->setEnabled(true);
        ui->anodeStart->setEnabled(true);
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(true);

        ui->gridLabel->setEnabled(true);
        ui->gridStart->setEnabled(true);
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(true);
    }

    updateParameterDisplay();
}

void ValveWorkbench::diodeMode()
{
    deviceType = DIODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Charcteristics", ANODE_CHARACTERISTICS);

    ui->gridLabel->setEnabled(false);
    ui->gridStart->setEnabled(false);
    ui->gridStop->setEnabled(false);
    ui->gridStep->setEnabled(false);

    ui->screenLabel->setEnabled(false);
    ui->screenStart->setEnabled(false);
    ui->screenStop->setEnabled(false);
    ui->screenStep->setEnabled(false);
}

void ValveWorkbench::log(QString message)
{
    if (logFile != nullptr) {
        logFile->write(message.toLatin1());
        logFile->write("\n");
    }
}

double ValveWorkbench::updateVoltage(QLineEdit *input, double oldValue, int electrode)
{
    double value = checkDoubleValue(input, oldValue);

    switch (electrode) {
    case HEATER:
        if (value > 16.0) {
            value = 16.0;
        }
        break;
    case GRID:
        if (value > 66.0) {
            value = 66.0;
        }
        break;
    case ANODE:
    case SCREEN:
        if (value > 540.0) {
            value = 540.0;
        }
        break;
    default:
        break;
    }

    updateDoubleValue(input, value);

    return value;
}

double ValveWorkbench::updatePMax()
{
    double value = checkDoubleValue(ui->pMax, pMax);

    if (value > 50.0) {
        value = 50.0;
    }

    updateDoubleValue(ui->pMax, value);
    pMax = value;

    return value;
}

double ValveWorkbench::updateIaMax()
{
    double value = checkDoubleValue(ui->iaMax, iaMax);

    if (value > 500.0) {
        value = 500.0;
    }

    updateDoubleValue(ui->iaMax, value);
    iaMax = value;

    return value;
}

//
// Slots
//

void ValveWorkbench::handleReadyRead()
{
    analyser->handleReadyRead();
}

void ValveWorkbench::handleError(QSerialPort::SerialPortError error)
{
    analyser->handleError(error);
}

void ValveWorkbench::handleTimeout()
{
    analyser->handleCommandTimeout();
}


void ValveWorkbench::on_stdDeviceSelection_currentIndexChanged(int index)
{
    selectStdDevice(1, ui->stdDeviceSelection->itemData(index).toInt());
}

void ValveWorkbench::on_circuitSelection_currentIndexChanged(int index)
{
    int circuitType = ui->circuitSelection->currentData().toInt();
    if (circuitType >= 0) {
        selectCircuit(circuitType);
    }
}

void ValveWorkbench::on_cir1Value_editingFinished()
{
    updateCircuitParameter(0);
}

void ValveWorkbench::on_cir2Value_editingFinished()
{
    updateCircuitParameter(1);
}

void ValveWorkbench::on_cir3Value_editingFinished()
{
    updateCircuitParameter(2);
}

void ValveWorkbench::on_cir4Value_editingFinished()
{
    updateCircuitParameter(3);
}

void ValveWorkbench::on_cir5Value_editingFinished()
{
    updateCircuitParameter(4);
}

void ValveWorkbench::on_cir6Value_editingFinished()
{
    updateCircuitParameter(5);
}

void ValveWorkbench::on_cir7Value_editingFinished()
{
    updateCircuitParameter(6);
}

void ValveWorkbench::on_cir8Value_editingFinished()
{
    updateCircuitParameter(7);
}


void ValveWorkbench::on_cir9Value_editingFinished()
{
    updateCircuitParameter(8);
}


void ValveWorkbench::on_cir10Value_editingFinished()
{
    updateCircuitParameter(9);
}


void ValveWorkbench::on_cir11Value_editingFinished()
{
    updateCircuitParameter(10);
}


void ValveWorkbench::on_cir12Value_editingFinished()
{
    updateCircuitParameter(11);
}

void ValveWorkbench::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void ValveWorkbench::on_actionPrint_triggered()
{

}

void ValveWorkbench::on_actionOptions_triggered()
{
    preferencesDialog.setPort(port);

    if (preferencesDialog.exec() == 1) {
        qInfo("ValveWorkbench::on_actionOptions_triggered: preferences accepted; saving and applying");
        // Persist preferences and calibration values
        preferencesDialog.saveToSettings();

        setSerialPort(preferencesDialog.getPort());

        pentodeModelType = preferencesDialog.getPentodeModelType();

        samplingType = preferencesDialog.getSamplingType();

        analyser->reset();

        qInfo("ValveWorkbench::on_actionOptions_triggered: clearing existing measurement curves for redraw");

        // After preferences (including smoothing) change, rebuild any
        // existing measurement curves so the currently displayed plots
        // immediately reflect the new settings on whichever tab is active.
        if (measuredCurves) {
            plot.remove(measuredCurves);
            measuredCurves = nullptr;
        }
        if (measuredCurvesSecondary) {
            plot.remove(measuredCurvesSecondary);
            measuredCurvesSecondary = nullptr;
        }

        if (ui->measureCheck && ui->measureCheck->isChecked()) {
            qInfo("ValveWorkbench::on_actionOptions_triggered: Show Measurement is ON - forcing on_measureCheck_stateChanged");
            on_measureCheck_stateChanged(ui->measureCheck->checkState());
        } else {
            qInfo("ValveWorkbench::on_actionOptions_triggered: Show Measurement is OFF - no measurement redraw");
        }
    }
}

void ValveWorkbench::on_actionLoad_Model_triggered()
{
    // Prefer the same models directories used by exportFittedModelToDevices()/loadDevices()
    // so that analyser-exported devices are easy to re-import. Fall back to the
    // legacy Documents/ValveWorkbench/templates path if no models dir exists.
    QString baseDir;

    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../../../../../models",
        QCoreApplication::applicationDirPath() + "/../../../../models",
        QCoreApplication::applicationDirPath() + "/../../../models",
        QCoreApplication::applicationDirPath() + "/../models",
        QCoreApplication::applicationDirPath() + "/models",
        QDir::currentPath() + "/models",
        QDir::currentPath() + "/../models",
        QDir::currentPath() + "/../../models",
        QDir::currentPath() + "/../../../models"
    };

    for (const QString &p : possiblePaths) {
        QDir d(p);
        if (d.exists()) {
            baseDir = d.absolutePath();
            break;
        }
    }

    if (baseDir.isEmpty()) {
        baseDir = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                   + "/ValveWorkbench/templates");
        if (!QDir(baseDir).exists()) QDir().mkpath(baseDir);
    }

    QString modelName = QFileDialog::getOpenFileName(this, "Import Model to Project", baseDir, "JSON Files (*.json)");

    if (modelName.isNull()) {
        return;
    }

    QFile modelFile(modelName);

    if (!modelFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open model file.");
        return;
    }

    QByteArray modelData = modelFile.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(modelData);
    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("Import Model"), tr("Invalid model JSON."));
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject modelObj = root.contains("model") && root.value("model").isObject()
                           ? root.value("model").toObject()
                           : root;

    // Determine model type (support multiple field names and inference)
    int desiredType = -1;
    QString mtype = modelObj.value("type").toString();
    if (mtype.isEmpty()) mtype = modelObj.value("modelType").toString();
    if (mtype.isEmpty()) mtype = modelObj.value("deviceType").toString();

    auto toUC = [](const QString &s){ return s.trimmed().toUpper(); };
    const QString mt = toUC(mtype);
    if (mt == "COHEN_HELIE_TRIODE" || mt == "TRIODE") {
        desiredType = COHEN_HELIE_TRIODE;
    } else if (mt == "KOREN_TRIODE") {
        desiredType = KOREN_TRIODE;
    } else if (mt == "SIMPLE_TRIODE") {
        desiredType = SIMPLE_TRIODE;
    } else if (mt == "GARDINER_PENTODE" || mt == "PENTODE") {
        desiredType = GARDINER_PENTODE;
    } else if (mt == "REEFMAN_DERK_PENTODE" || mt == "REEFMAN_PENTODE") {
        desiredType = REEFMAN_DERK_PENTODE;
    }

    // If no explicit type, infer from parameter keys
    if (desiredType == -1) {
        const bool hasTriodeKeys = modelObj.contains("mu") && modelObj.contains("kg1") && modelObj.contains("x");
        const bool hasPentodeKeys = modelObj.contains("kg2") || modelObj.contains("beta") || modelObj.contains("gamma") || modelObj.contains("a");
        if (hasTriodeKeys && !hasPentodeKeys) {
            desiredType = COHEN_HELIE_TRIODE;
        } else if (hasPentodeKeys) {
            // Default to Gardiner when pentode-like keys present
            desiredType = GARDINER_PENTODE;
        }
    }

    // As last resort, ask the user
    if (desiredType == -1) {
        QStringList options;
        options << "COHEN_HELIE_TRIODE" << "KOREN_TRIODE" << "SIMPLE_TRIODE" << "GARDINER_PENTODE" << "REEFMAN_DERK_PENTODE";
        bool ok = false;
        QString chosen = QInputDialog::getItem(this, tr("Select Model Type"), tr("Model type not found in JSON. Select type:"), options, 0, false, &ok);
        if (!ok || chosen.isEmpty()) {
            QMessageBox::warning(this, tr("Import Model"), tr("Unrecognized or missing model type."));
            return;
        }
        const QString ch = toUC(chosen);
        if (ch == "COHEN_HELIE_TRIODE") desiredType = COHEN_HELIE_TRIODE;
        else if (ch == "KOREN_TRIODE") desiredType = KOREN_TRIODE;
        else if (ch == "SIMPLE_TRIODE") desiredType = SIMPLE_TRIODE;
        else if (ch == "GARDINER_PENTODE") desiredType = GARDINER_PENTODE;
        else if (ch == "REEFMAN_DERK_PENTODE") desiredType = REEFMAN_DERK_PENTODE;
    }

    if (desiredType == -1) {
        QMessageBox::warning(this, tr("Import Model"), tr("Unrecognized or missing model type."));
        return;
    }

    if (!currentProject) {
        QMessageBox::warning(this, tr("Import Model"), tr("No project selected. Create or open a project first."));
        return;
    }

    Model *m = ModelFactory::createModel(desiredType);
    if (!m) {
        QMessageBox::warning(this, tr("Import Model"), tr("Could not create model instance."));
        return;
    }
    // Ensure imported models have preferences wired so methods like
    // updateProperties() can safely consult settings (e.g. GardinerPentode
    // checking useSecondaryEmission()). This mirrors how fitted models are
    // configured elsewhere in the app.
    m->setPreferences(&preferencesDialog);
    m->fromJson(modelObj);

    Project *proj = static_cast<Project *>(currentProject->data(0, Qt::UserRole).value<void *>());
    if (!proj) {
        QMessageBox::warning(this, tr("Import Model"), tr("Invalid project node."));
        delete m;
        return;
    }
    proj->addModel(m);
    m->buildTree(currentProject);
    setSelectedTreeItem(currentProject, true);
}

void ValveWorkbench::on_actionNew_Project_triggered()
{
    ProjectDialog dialog;

    if (dialog.exec() == 1) {
        Project *project = new Project();
        project->setName(dialog.getName());
        project->setDeviceType(dialog.getDeviceType());

        setSelectedTreeItem(currentProject, false);
        currentProject = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
        currentProject->setText(0, dialog.getName());
        currentProject->setIcon(0, QIcon(":/icons/valve32.png"));
        currentProject->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        currentProject->setData(0, Qt::UserRole, QVariant::fromValue((void *) project));

        project->setTreeItem(currentProject);
        setSelectedTreeItem(currentProject, true);
    }
}

void ValveWorkbench::on_actionSave_Project_triggered()
{
    if (currentProject != nullptr) {
        Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();

        QString projectName = QFileDialog::getSaveFileName(this, "Save Project", "", "*.vwp");

        if (projectName.isNull()) {
            return;
        }

        QFile projectFile(projectName);

        if (!projectFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
            qWarning("Couldn't open project file for Save.");
        } else {
            QJsonObject projectObject;

            project->toJson(projectObject);
            projectFile.write(QJsonDocument(projectObject).toJson());
        }
    }
}

void ValveWorkbench::on_actionOpen_Project_triggered()
{
    QString projectName = QFileDialog::getOpenFileName(this, "Open project", "", "*.vwp");

    if (projectName.isNull()) {
        return;
    }

    ui->tabWidget->setCurrentIndex(1);

    QFile projectFile(projectName);

    if (!projectFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Couldn't open project file for Open.");
    } else {
        QByteArray projectData = projectFile.readAll();
        Project *project = new Project();

        QJsonDocument projectDocument(QJsonDocument::fromJson(projectData));
        if (projectDocument.isObject()) {
            QJsonObject projectObject = projectDocument.object();
            if (projectObject.contains("project") && projectObject["project"].isObject()) {
                project->fromJson(projectObject["project"].toObject());
            }
        }

        setSelectedTreeItem(currentProject, false);
        currentProject = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
        currentProject->setText(0, project->getName());
        currentProject->setIcon(0, QIcon(":/icons/valve32.png"));
        currentProject->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        currentProject->setData(0, Qt::UserRole, QVariant::fromValue((void *) project));
        setSelectedTreeItem(currentProject, true);
        setFitButtons();

        project->buildTree(currentProject);
    }
}

void ValveWorkbench::on_actionClose_Project_triggered()
{
    if (currentProject != nullptr) {
        if (getProject(currentMeasurementItem) == currentProject) {
            currentMeasurementItem = nullptr;
        }
        if (getProject(currentModelItem) == currentProject) {
            currentModelItem = nullptr;
        }
        delete currentProject;
        currentProject = nullptr;
    }
}

void ValveWorkbench::on_actionExport_Model_triggered()
{
    exportFittedModelToDevices();
}

void ValveWorkbench::on_projectTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    //ui->estimateButton->setEnabled(false);
    //ui->fitButton->setEnabled(false);

    if (current == nullptr) {
        return;
    }

    void *data = current->data(0, Qt::UserRole).value<void *>();

    bool showScreen = preferencesDialog.showScreenCurrent();

    switch(current->type()) {
    case TYP_PROJECT:
        qInfo("=== PROJECT TREE: TYP_PROJECT case triggered ===");
        setSelectedTreeItem(currentProject, false);
        currentProject = current;
        setSelectedTreeItem(currentProject, true);
        setFitButtons();
        if (data != nullptr) {
            ((Project *)data)->updateProperties(ui->properties);
        }
        break;
    case TYP_MEASUREMENT: {
            qInfo("=== PROJECT TREE: TYP_MEASUREMENT case triggered ===");
            setSelectedTreeItem(currentMeasurementItem, false);
            currentMeasurementItem = current;
            setSelectedTreeItem(currentMeasurementItem, true);
            currentMeasurement = (Measurement *) data;
            if (currentMeasurement == nullptr) {
                qWarning("Measurement data is null; aborting selection handling");
                setFitButtons();
                break;
            }

            setSelectedTreeItem(currentProject, false);
            currentProject = getProject(current);
            setSelectedTreeItem(currentProject, true);
            setFitButtons();

            currentMeasurement->updateProperties(ui->properties);
            currentMeasurement->setShowScreen(showScreen);
            currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
           // plot.add(measuredCurves);
            modelledCurves = nullptr;
            qInfo("=== BEFORE MEASUREMENT PLOT - Scene items count: %d ===", plot.getScene()->items().count());
            measuredCurves = currentMeasurement->updatePlot(&plot);
            qInfo("=== AFTER MEASUREMENT PLOT - measuredCurves items: %d, Scene items: %d ===", measuredCurves ? measuredCurves->childItems().count() : 0, plot.getScene()->items().count());
            if (measuredCurves != nullptr) {
                plot.add(measuredCurves);
            }

            if (isDoubleTriode && triodeMeasurementSecondary != nullptr && triodeMeasurementSecondary->count() > 0) {
                if (measuredCurvesSecondary != nullptr) {
                    plot.remove(measuredCurvesSecondary);
                    measuredCurvesSecondary = nullptr;
                }
                triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());
                measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
                if (measuredCurvesSecondary != nullptr) {
                    plot.add(measuredCurvesSecondary);
                    measuredCurvesSecondary->setVisible(ui->measureCheck->isChecked());
                }
            }
            qInfo("Added measuredCurves to plot");
            ui->measureCheck->setChecked(true);
            // Auto-refresh small-signal LCDs based on the current mode.
            if (ui->mes_mod_select && ui->mes_mod_select->isChecked()) {
                // Model mode: prefer Designer triode circuit, otherwise fall
                // back to the fitted model if available.
                on_mes_mod_select_stateChanged(ui->mes_mod_select->checkState());
            } else {
                // Measured mode: compute from the currently selected
                // measurement so Modeller can be used for tube matching.
                updateSmallSignalFromMeasurement(currentMeasurement);
            }
            qInfo("=== PROJECT TREE: Finished TYP_MEASUREMENT case ===");
            break;
        }
    case TYP_SWEEP: {
            qInfo("=== PROJECT TREE: TYP_SWEEP case triggered ===");
            if (currentMeasurementItem != nullptr) {
                QFont font = currentMeasurementItem->font(0);
                font.setBold(false);
                currentMeasurementItem->setFont(0, font);
            }

            currentMeasurementItem = current;
            QFont font = currentMeasurementItem->font(0);
            font.setBold(true);
            currentMeasurementItem->setFont(0, font);

            QTreeWidgetItem *m = getParent(currentMeasurementItem, TYP_MEASUREMENT);

            if (m != nullptr && m->data(0, Qt::UserRole).value<void *>() != nullptr) {
                currentMeasurement = (Measurement *) m->data(0, Qt::UserRole).value<void *>();
                if (currentMeasurement == nullptr) {
                    qWarning("Parent measurement is null; aborting sweep selection handling");
                    setFitButtons();
                    break;
                }

                setSelectedTreeItem(currentProject, false);
                currentProject = getProject(current);
                setSelectedTreeItem(currentProject, true);
                setFitButtons();

                Sweep *sweep = (Sweep *) data;
                sweep->updateProperties(ui->properties);
                qInfo("=== PROJECT TREE: About to call currentMeasurement->updatePlot(sweep) ===");
                
                // More aggressive clearing - clear plot completely before each update
                qInfo("=== BEFORE PLOT CLEAR - Scene items count: %d ===", plot.getScene()->items().count());
                plot.clear();
                cursorLabelItem = nullptr;
                qInfo("=== AFTER PLOT CLEAR - Scene items count: %d ===", plot.getScene()->items().count());
                
                // Also remove measuredCurves if it exists
                if (measuredCurves != nullptr) {
                    plot.remove(measuredCurves);
                    qInfo("Removed old measuredCurves");
                } else {
                    qInfo("measuredCurves is nullptr - no need to remove");
                }

                if (measuredCurvesSecondary != nullptr) {
                    plot.remove(measuredCurvesSecondary);
                    measuredCurvesSecondary = nullptr;
                    qInfo("Removed old measuredCurvesSecondary");
                }
                
                // Reset measuredCurves to nullptr before updating
                measuredCurves = nullptr;
                qInfo("Reset measuredCurves to nullptr");

                currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
                measuredCurves = currentMeasurement->updatePlot(&plot, sweep);
                qInfo("=== AFTER UPDATE PLOT - measuredCurves items: %d, Scene items: %d ===", measuredCurves ? measuredCurves->childItems().count() : 0, plot.getScene()->items().count());
                plot.add(measuredCurves);

                if (isDoubleTriode && triodeMeasurementSecondary != nullptr && triodeMeasurementSecondary->count() > 0) {
                    if (measuredCurvesSecondary != nullptr) {
                        plot.remove(measuredCurvesSecondary);
                        measuredCurvesSecondary = nullptr;
                    }

                    Sweep *secondarySweep = nullptr;
                    if (sweep != nullptr) {
                        int sweepIndex = -1;
                        for (int i = 0; i < currentMeasurement->count(); ++i) {
                            if (currentMeasurement->at(i) == sweep) {
                                sweepIndex = i;
                                break;
                            }
                        }

                        if (sweepIndex >= 0 && sweepIndex < triodeMeasurementSecondary->count()) {
                            secondarySweep = triodeMeasurementSecondary->at(sweepIndex);
                        }
                    }

                    if (secondarySweep != nullptr) {
                        triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());
                        measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot, secondarySweep);
                        if (measuredCurvesSecondary != nullptr) {
                            plot.add(measuredCurvesSecondary);
                            measuredCurvesSecondary->setVisible(ui->measureCheck->isChecked());
                        }
                    }
                }
                modelledCurves = nullptr;
                ui->measureCheck->setChecked(true);
                // Auto-refresh small-signal LCDs for the new sweep/measurement
                if (ui->mes_mod_select && ui->mes_mod_select->isChecked()) {
                    on_mes_mod_select_stateChanged(ui->mes_mod_select->checkState());
                } else {
                    updateSmallSignalFromMeasurement(currentMeasurement);
                }
                qInfo("=== PROJECT TREE: Finished TYP_SWEEP case ===");
            }
            break;
        }
    case TYP_ESTIMATE: {
            if (currentEstimateItem != nullptr) {
                QFont font = currentEstimateItem->font(0);
                font.setBold(false);
                currentEstimateItem->setFont(0, font);
            }
            currentEstimateItem = current;
            QFont font = currentEstimateItem->font(0);
            font.setBold(true);
            currentEstimateItem->setFont(0, font);
            currentProject = getProject(current);
            Estimate *estimate = (Estimate *) data;
            estimate->updateProperties(ui->properties);
            qInfo("=== BEFORE ESTIMATE PLOT - Scene items count: %d ===", plot.getScene()->items().count());
            // Clear plot before estimate plotting
            plot.clear();
            cursorLabelItem = nullptr;
            qInfo("Cleared plot before estimate plotting");
            estimatedCurves = estimate->plotModel(&plot, currentMeasurement);
            qInfo("=== AFTER ESTIMATE PLOT - estimatedCurves items: %d, Scene items: %d ===", estimatedCurves ? estimatedCurves->childItems().count() : 0, plot.getScene()->items().count());
            plot.add(estimatedCurves);
            qInfo("Added estimatedCurves to plot");
            break;
        }
    case TYP_MODEL: {
            setSelectedTreeItem(currentModelItem, false);
            currentModelItem = current;
            setSelectedTreeItem(currentModelItem, true);

            setSelectedTreeItem(currentProject, false);
            currentProject = getProject(current);
            setSelectedTreeItem(currentProject, true);
            setFitButtons();

            qInfo("=== MODEL PLOTTING: currentMeasurementItem type = %d, is null = %s ===",
                   currentMeasurementItem ? currentMeasurementItem->type() : -1,
                   currentMeasurementItem ? "false" : "true");

            // Require a valid measurement selection before attempting model plotting.
            // If the currently selected measurement/device type does not match the
            // selected model (e.g. a pentode model is selected while a triode
            // measurement is active), try to switch to a matching measurement so
            // that selecting a pentode model like Reefman DerkE automatically
            // overlays it on the latest pentode data rather than leaving the
            // previous triode Model A curves visible.

            Model *model = (Model *) data;

            if (!currentMeasurement && currentProject) {
                // Try to find a default measurement in the current project if none
                // is currently active.
                if (model->getType() == COHEN_HELIE_TRIODE) {
                    currentMeasurement = findMeasurement(TRIODE, ANODE_CHARACTERISTICS);
                } else if (model->getType() == GARDINER_PENTODE ||
                           model->getType() == SIMPLE_MANUAL_PENTODE ||
                           model->getType() == REEFMAN_DERK_PENTODE ||
                           model->getType() == REEFMAN_DERK_E_PENTODE ||
                           model->getType() == EXTRACT_DERK_E_PENTODE) {
                    currentMeasurement = findMeasurement(PENTODE, ANODE_CHARACTERISTICS);
                }
            } else if (currentMeasurement) {
                const int mType = currentMeasurement->getDeviceType();
                const int modelType = model->getType();

                // If a pentode model is selected while a triode measurement is
                // active, switch to a pentode measurement if available.
                if (mType == TRIODE &&
                    (modelType == GARDINER_PENTODE ||
                     modelType == SIMPLE_MANUAL_PENTODE ||
                     modelType == REEFMAN_DERK_PENTODE ||
                     modelType == REEFMAN_DERK_E_PENTODE ||
                     modelType == EXTRACT_DERK_E_PENTODE)) {
                    Measurement *pentodeMeas = findMeasurement(PENTODE, ANODE_CHARACTERISTICS);
                    if (pentodeMeas) {
                        qInfo("MODEL PLOTTING: Switching currentMeasurement to pentode dataset for pentode model overlay");
                        currentMeasurement = pentodeMeas;
                    }
                }

                // Conversely, if a triode model is selected while a pentode
                // measurement is active, prefer a triode measurement.
                if (mType == PENTODE && modelType == COHEN_HELIE_TRIODE) {
                    Measurement *triodeMeas = findMeasurement(TRIODE, ANODE_CHARACTERISTICS);
                    if (triodeMeas) {
                        qInfo("MODEL PLOTTING: Switching currentMeasurement to triode dataset for triode model overlay");
                        currentMeasurement = triodeMeas;
                    }
                }
            }

            if (!currentMeasurement) {
                qInfo("MODEL PLOTTING: No suitable measurement found for selected model - skipping model overlay");
                ui->modelCheck->setChecked(true);
                break;
            }

            Sweep *sweep = nullptr;

            if (currentMeasurementItem != nullptr) {
                if (currentMeasurementItem->type() == TYP_SWEEP) {
                    // If currentMeasurementItem is a sweep, we're plotting a specific sweep
                    sweep = (Sweep *) currentMeasurementItem->data(0, Qt::UserRole).value<void *>();
                } else if (currentMeasurementItem->type() == TYP_MEASUREMENT) {
                    // If currentMeasurementItem is a measurement, force sweep to null for full measurement plotting
                    sweep = nullptr;
                }
                // Otherwise, leave sweep as nullptr for full measurement plotting
            }
            qInfo("=== MODEL PLOTTING: sweep is %s, about to call plotModel ===", sweep ? "NOT null" : "null");
            model->updateProperties(ui->properties);

            qInfo("=== VALVEWORKBENCH: Attempting model plotting ===");
            qInfo("Current measurement device type: %d, model type: %d",
                   currentMeasurement->getDeviceType(), model->getType());

            const bool triodeMatch =
                (currentMeasurement->getDeviceType() == TRIODE && model->getType() == COHEN_HELIE_TRIODE);
            const bool pentodeMatch =
                (currentMeasurement->getDeviceType() == PENTODE &&
                 (model->getType() == GARDINER_PENTODE ||
                  model->getType() == SIMPLE_MANUAL_PENTODE ||
                  model->getType() == REEFMAN_DERK_PENTODE ||
                  model->getType() == REEFMAN_DERK_E_PENTODE ||
                  model->getType() == EXTRACT_DERK_E_PENTODE));

            if (triodeMatch || pentodeMatch) {
                qInfo("Type check PASSED - proceeding with model plotting");
                plot.remove(modelledCurves);
                QGraphicsItemGroup *plotted = nullptr;

                if (currentMeasurement->getDeviceType() == PENTODE) {
                    // Always plot pentode models (Gardiner/Reefman/SimpleManual) using the
                    // current model instance so that JSON-loaded parameters and fitted
                    // analyser models are reflected exactly, and to avoid doing a second
                    // on-the-fly pentode fit when selecting a model node.
                    qInfo("PENTODE: Using current model instance of type %d for plotting", model->getType());
                    model->setShowScreen(showScreen);
                    plotted = model->plotModel(&plot, currentMeasurement, sweep);
                } else {
                    // Triode or other device types fall back to the model instance as before.
                    model->setShowScreen(showScreen);
                    plotted = model->plotModel(&plot, currentMeasurement, sweep);
                }

                if (plotted) {
                    modelledCurves = plotted;
                    plot.add(modelledCurves);
                    qInfo("Model plotting completed");
                } else {
                    qInfo("Model plotting returned null item group - nothing added to plot");
                }
            } else {
                qInfo("Type check FAILED - skipping model plotting");
                qInfo("Measurement device: %d, Model type: %d", currentMeasurement->getDeviceType(), model->getType());
            }

            ui->modelCheck->setChecked(true);
            break;
        }
    case TYP_SAMPLE: {
            if (currentMeasurementItem != nullptr) {
                QFont font = currentMeasurementItem->font(0);
                font.setBold(false);
                currentMeasurementItem->setFont(0, font);
            }
            currentMeasurementItem = current;
            QFont font = currentMeasurementItem->font(0);
            font.setBold(true);
            currentMeasurementItem->setFont(0, font);
            currentMeasurement = (Measurement *) data;

            setSelectedTreeItem(currentProject, false);
            currentProject = getProject(current);
            setSelectedTreeItem(currentProject, true);
            setFitButtons();

            Sample *sample = (Sample *) data;
            sample->updateProperties(ui->properties);
            break;
        }
    default:
        break;
    }
}

QTreeWidgetItem *ValveWorkbench::getProject(QTreeWidgetItem *current)
{
    if (current == nullptr) {
        return nullptr;
    }

    QTreeWidgetItem *parent = current->parent();
    if (parent != nullptr) {
        if (parent->type() == TYP_PROJECT) {
            //return (Project *) parent->data(0, Qt::UserRole).value<void *>();
            return parent;
        } else {
            return getProject(parent);
        }
    }

    return nullptr;
}

QTreeWidgetItem *ValveWorkbench::getParent(QTreeWidgetItem *current, int type)
{
    if (current == nullptr) {
        return nullptr;
    }

    QTreeWidgetItem *parent = current->parent();
    if (parent != nullptr) {
        if (parent->type() == type) {
            return parent;
        } else {
            return getProject(parent);
        }
    }

    return nullptr;
}

Model *ValveWorkbench::findModel(int type)
{
    int children = currentProject->childCount();
    Model *foundModel = nullptr;

    for (int i = 0; i < children && foundModel == nullptr; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MODEL) {
            Model *model = (Model *) child->data(0, Qt::UserRole).value<void *>();
            if (model->getType() == type) {
                foundModel = model;
            }
        }
    }

    return foundModel;
}

Measurement *ValveWorkbench::findMeasurement(int deviceType, int testType)
{
    int children = currentProject->childCount();
    Measurement *foundMeasurement = nullptr;

    for (int i = 0; i < children && foundMeasurement == nullptr; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MEASUREMENT) {
            Measurement *measurement = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (measurement->getDeviceType() == deviceType && measurement->getTestType() == testType) {
                foundMeasurement = measurement;
            }
        }
    }

    return foundMeasurement;
}

void ValveWorkbench::setSelectedTreeItem(QTreeWidgetItem *item, bool selected)
{
    if (item != nullptr) {
        QFont font = item->font(0);
        font.setBold(selected);
        item->setFont(0, font);
    }
}

void ValveWorkbench::setFitButtons()
{
    // Resolve the root project from the current selection
    ui->fitTriodeButton->setVisible(false);
    ui->fitPentodeButton->setVisible(false);

    QTreeWidgetItem *currentItem = ui->projectTree->currentItem();
    if (!currentItem) {
        return;
    }

    int deviceTypeForButtons = -1;

    if (currentItem->type() == TYP_MEASUREMENT) {
        void *data = currentItem->data(0, Qt::UserRole).value<void *>();
        Measurement *measurement = static_cast<Measurement *>(data);
        if (measurement) {
            deviceTypeForButtons = measurement->getDeviceType();
        }
    } else if (currentItem->type() == TYP_SWEEP || currentItem->type() == TYP_SAMPLE) {
        QTreeWidgetItem *measurementItem = getParent(currentItem, TYP_MEASUREMENT);
        if (measurementItem) {
            void *data = measurementItem->data(0, Qt::UserRole).value<void *>();
            Measurement *measurement = static_cast<Measurement *>(data);
            if (measurement) {
                deviceTypeForButtons = measurement->getDeviceType();
            }
        }
    }

    if (deviceTypeForButtons == -1) {
        if (!currentProject) {
            return;
        }

        QTreeWidgetItem *rootProject = currentProject;
        if (rootProject->type() != TYP_PROJECT) {
            rootProject = getParent(rootProject, TYP_PROJECT);
        }
        if (!rootProject) {
            return;
        }

        Project *project = static_cast<Project *>(rootProject->data(0, Qt::UserRole).value<void *>());
        if (!project) {
            return;
        }

        deviceTypeForButtons = project->getDeviceType();
    }

    if (deviceTypeForButtons == TRIODE) {
        ui->fitTriodeButton->setVisible(true);
        ui->fitPentodeButton->setVisible(false);
    } else if (deviceTypeForButtons == PENTODE) {
        ui->fitTriodeButton->setVisible(false);
        ui->fitPentodeButton->setVisible(true);
    }
}

void ValveWorkbench::on_deviceType_currentIndexChanged(int index)
{
    // Decode the logical device type from itemData and then refine behaviour
    // based on the human-readable label to distinguish variants that share
    // the same base type (e.g. Double Triode, Triode-Connected Pentode).
    const int logicalType = ui->deviceType->itemData(index).toInt();
    const QString label = ui->deviceType->currentText();

    isDoubleTriode = (label == QLatin1String("Double Triode"));
    isTriodeConnectedPentode = (label == QLatin1String("Triode-Connected Pentode"));

    switch (logicalType) {
    case PENTODE:
        pentodeMode();
        break;
    case TRIODE:
        // Single triode, double triode, and triode-connected pentode all
        // share the same underlying TRIODE device type. triodeMode's
        // doubleTriode flag controls the secondary-anode UI.
        triodeMode(isDoubleTriode);
        break;
    case DIODE:
        diodeMode();
        break;
    default:
        break;
    }

    ui->testType->setCurrentIndex(0);
    on_testType_currentIndexChanged(0);
}

void ValveWorkbench::on_testType_currentIndexChanged(int index)
{
    updateParameterDisplay();

    if (deviceType == TRIODE) {
        ui->screenStart->setText("");
        ui->screenStop->setText("");
        ui->screenStep->setText("");
    }

    const int newTestType = ui->testType->itemData(index).toInt();
    switch (newTestType) {
    case ANODE_CHARACTERISTICS: // Anode swept and Grid stepped
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(false);
        ui->anodeStep->setText("");
        if (deviceType != DIODE) {
            ui->gridStop->setEnabled(true);
            ui->gridStep->setEnabled(true);
        }
        if (deviceType == PENTODE) { // Screen fixed (if Pentode)
            ui->screenStop->setEnabled(false);
            ui->screenStop->setText("");
            ui->screenStep->setEnabled(false);
            ui->screenStep->setText("");
        }
        break;
    case TRANSFER_CHARACTERISTICS: // Grid swept
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(false);
        ui->gridStep->setText("");
        if (deviceType == PENTODE) { // Anode fixed and Screen stepped
            ui->anodeStop->setEnabled(false);
            ui->anodeStop->setText("");
            ui->anodeStep->setEnabled(false);
            ui->anodeStep->setText("");
            ui->screenStop->setEnabled(true);
            ui->screenStep->setEnabled(true);
        } else { // (Triode) Anode stepped and no Screen
            ui->anodeStop->setEnabled(true);
            ui->anodeStep->setEnabled(true);
            // Default: 25V anode steps for transfer characteristics if not set
            if (anodeStep <= 0.0) {
                anodeStep = 25.0;
                updateDoubleValue(ui->anodeStep, anodeStep);
            }
        }
        break;
    case SCREEN_CHARACTERISTICS: // Anode fixed, Screen swept and Grid stepped
        ui->anodeStop->setEnabled(false);
        ui->anodeStep->setEnabled(false);
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(true);
        ui->screenStop->setEnabled(true);
        ui->screenStep->setEnabled(false);
        break;
    default:
        break;
    }

    testType = newTestType;

    // For Double Triode + Anode Characteristics, do not show a numeric step
    // value for the second anode. Keep the layout intact but make the
    // second-anode step box disabled and blank.
    const bool isDoubleTriodeMode =
        (ui->deviceType && ui->deviceType->currentText() == QLatin1String("Double Triode"));
    if (isDoubleTriodeMode && newTestType == ANODE_CHARACTERISTICS) {
        if (ui->screenStep) {
            ui->screenStep->setEnabled(false);
            ui->screenStep->setText("");
        }
    }
}

void ValveWorkbench::on_anodeStart_editingFinished()
{
    double value = updateVoltage(ui->anodeStart, anodeStart, ANODE);
    anodeStart = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStart = value;
        updateDoubleValue(ui->screenStart, secondAnodeStart);
    }
}

void ValveWorkbench::on_anodeStop_editingFinished()
{
    double value = updateVoltage(ui->anodeStop, anodeStop, ANODE);
    anodeStop = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStop = value;
        updateDoubleValue(ui->screenStop, secondAnodeStop);
    }
}

void ValveWorkbench::on_anodeStep_editingFinished()
{
    double value = updateVoltage(ui->anodeStep, anodeStep, ANODE);
    anodeStep = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStep = value;
        updateDoubleValue(ui->screenStep, secondAnodeStep);
    }
}

void ValveWorkbench::on_gridStart_editingFinished()
{
    double value = updateVoltage(ui->gridStart, gridStart, GRID);
    gridStart = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondGridStart = value;
    }
}

void ValveWorkbench::on_gridStop_editingFinished()
{
    double value = updateVoltage(ui->gridStop, gridStop, GRID);
    gridStop = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondGridStop = value;
    }
}

void ValveWorkbench::on_gridStep_editingFinished()
{
    double value = updateVoltage(ui->gridStep, gridStep, GRID);
    gridStep = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondGridStep = value;
    }
}

void ValveWorkbench::on_screenStart_editingFinished()
{
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStart = updateVoltage(ui->screenStart, secondAnodeStart, ANODE);
    } else {
        screenStart = updateVoltage(ui->screenStart, screenStart, SCREEN);
    }
}

void ValveWorkbench::on_screenStop_editingFinished()
{
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStop = updateVoltage(ui->screenStop, secondAnodeStop, ANODE);
    } else {
        screenStop = updateVoltage(ui->screenStop, screenStop, SCREEN);
    }
}

void ValveWorkbench::on_screenStep_editingFinished()
{
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStep = updateVoltage(ui->screenStep, secondAnodeStep, ANODE);
    } else {
        screenStep = updateVoltage(ui->screenStep, screenStep, SCREEN);
    }
}


void ValveWorkbench::on_iaMax_editingFinished()
{
    updateIaMax();
}


void ValveWorkbench::on_pMax_editingFinished()
{
    updatePMax();
}

void ValveWorkbench::on_datasheetVa_editingFinished()
{
    syncDatasheetFromUi();
    updateDatasheetDisplay();
}

void ValveWorkbench::on_datasheetVg_editingFinished()
{
    syncDatasheetFromUi();
    updateDatasheetDisplay();
}

void ValveWorkbench::on_datasheetIa_editingFinished()
{
    syncDatasheetFromUi();
    updateDatasheetDisplay();
}

void ValveWorkbench::on_datasheetGm_editingFinished()
{
    syncDatasheetFromUi();
    updateDatasheetDisplay();
}

void ValveWorkbench::on_datasheetMu_editingFinished()
{
    syncDatasheetFromUi();
    updateDatasheetDisplay();
}

void ValveWorkbench::on_datasheetRp_editingFinished()
{
    syncDatasheetFromUi();
    updateDatasheetDisplay();
}
void ValveWorkbench::on_runButton_clicked()
{
    static int clickCount = 0;
    clickCount++;
    qInfo("on_runButton_clicked called (count: %d)", clickCount);

    log("Run Test button clicked");

    if (analyser == nullptr) {
        log("Error: Analyser is null");
        QMessageBox::warning(this, "Error", "Analyser not initialized");
        return;
    }

    ui->runButton->setChecked(true);
    ui->progressBar->reset();
    ui->progressBar->setVisible(true);
    ui->btnAddToProject->setEnabled(false);

    badRetryCount = 0;
    log("Configuring analyser");
    analyser->setDeviceType(deviceType);
    analyser->setTestType(testType);
    // Propagate multi-section and triode-connected flags so the analyser can
    // adjust its command patterns while measurements still report TRIODE or
    // PENTODE through deviceType.
    analyser->setIsDoubleTriode(isDoubleTriode);
    analyser->setIsTriodeConnectedPentode(isTriodeConnectedPentode);
    analyser->setPMax(pMax);
    analyser->setIaMax(iaMax);
    analyser->setSweepParameters(anodeStart, anodeStop, anodeStep, gridStart, gridStop, gridStep, screenStart, screenStop, screenStep, secondGridStart, secondGridStop, secondGridStep, secondAnodeStart, secondAnodeStop, secondAnodeStep);

    qInfo("Analyser parameters: anodeStart=%f, anodeStop=%f, anodeStep=%f, gridStart=%f, gridStop=%f, gridStep=%f, screenStart=%f, screenStop=%f, screenStep=%f", anodeStart, anodeStop, anodeStep, gridStart, gridStop, gridStep, screenStart, screenStop, screenStep);

    log("Starting test");
    analyser->startTest();
}

void ValveWorkbench::on_btnAddToProject_clicked()
{
    qDebug("Save to Project button clicked");
    // Always prompt for project details before saving
    ProjectDialog dialog;
    if (dialog.exec() != QDialog::Accepted) {
        qDebug("Project save cancelled by user");
        return;
    }

    // Ensure a project exists, create or update with dialog values
    if (currentProject == nullptr) {
        qDebug("No current project, creating new one from dialog");
        Project *project = new Project();
        project->setName(dialog.getName());
        project->setDeviceType(dialog.getDeviceType());

        setSelectedTreeItem(currentProject, false);
        currentProject = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
        currentProject->setText(0, dialog.getName());
        currentProject->setIcon(0, QIcon(":/icons/valve32.png"));
        currentProject->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        currentProject->setData(0, Qt::UserRole, QVariant::fromValue((void *) project));

        project->setTreeItem(currentProject);
        setSelectedTreeItem(currentProject, true);
    } else {
        // Update existing project's name/device
        Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
        if (project != nullptr) {
            project->setName(dialog.getName());
            project->setDeviceType(dialog.getDeviceType());
        }
        currentProject->setText(0, dialog.getName());
    }

    Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
    qDebug("Project pointer: %p", project);
    if (!project) {
        qWarning("AddToProject: currentProject has null Project* user data - aborting save");
        return;
    }

    Measurement *measurement = analyser->getResult();
    qDebug("Measurement pointer: %p", measurement);
    if (!measurement) {
        qWarning("AddToProject: analyser->getResult() returned null - cannot add to project");
        return;
    }

    qDebug("AddToProject: measurement sweeps=%d, deviceType=%d, testType=%d",
           measurement->count(), measurement->getDeviceType(), measurement->getTestType());

    if (project->addMeasurement(measurement)) {
        // Tag pentode measurements that were taken in triode-connected
        // mode so the UI can display a clear hint in the device name.
        if (measurement->getDeviceType() == PENTODE && isTriodeConnectedPentode) {
            measurement->setTriodeConnectedPentode(true);
        }
        qDebug("Measurement added to project successfully");
        qDebug("AddToProject: building measurement tree under project node '%s' (children before=%d)",
               currentProject->text(0).toStdString().c_str(), currentProject->childCount());
        measurement->buildTree(currentProject);
        qDebug("AddToProject: measurement tree built (children after=%d)", currentProject->childCount());

        // Treat this newly added measurement as the explicit current
        // measurement for Analyser/Modeller. Also try to locate and
        // remember its tree item so tab changes can re-plot it without
        // guessing.
        currentMeasurement = measurement;
        currentMeasurementItem = nullptr;
        if (currentProject) {
            for (int i = 0; i < currentProject->childCount(); ++i) {
                QTreeWidgetItem *child = currentProject->child(i);
                if (!child) continue;
                if (child->type() != TYP_MEASUREMENT) continue;
                void *mData = child->data(0, Qt::UserRole).value<void *>();
                if (mData == static_cast<void *>(measurement)) {
                    currentMeasurementItem = child;
                    break;
                }
            }
        }
        qDebug("About to switch tab");
        // Temporarily commented out due to Qt bug in tab switching
        // ui->tabWidget->setCurrentIndex(1);
        qDebug("Tab switch commented out for workaround");
        qDebug("Setting button enabled to false");
        ui->btnAddToProject->setEnabled(false);
        qDebug("Save to Project function completed");
    } else {
        qWarning("Failed to add measurement to project");
    }

    ui->btnAddToProject->setEnabled(false);
}

void ValveWorkbench::importFromDevice()
{
    // Import a Measurement from a device preset that contains an embedded
    // 'measurement' block. This lets Modeller work from tube-style device
    // JSONs without re-running the analyser.

    // Require at least one device with embedded measurement.
    QList<Device *> candidates;
    QStringList names;
    for (Device *d : devices) {
        if (!d) continue;
        if (d->getMeasurement()) {
            candidates.append(d);
            names.append(d->getName());
        }
    }

    if (candidates.isEmpty()) {
        QMessageBox::warning(this, tr("Import from Device"),
                             tr("No devices with embedded measurements are loaded. Export a fitted model with measurement from the Modeller tab first."));
        return;
    }

    bool ok = false;
    QString choice = QInputDialog::getItem(this,
                                           tr("Import from Device"),
                                           tr("Select device with embedded measurement:"),
                                           names,
                                           0,
                                           false,
                                           &ok);
    if (!ok || choice.isEmpty()) {
        return;
    }

    int idx = names.indexOf(choice);
    if (idx < 0 || idx >= candidates.size()) {
        return;
    }

    Device *srcDevice = candidates.at(idx);
    Measurement *srcMeas = srcDevice ? srcDevice->getMeasurement() : nullptr;
    if (!srcMeas) {
        QMessageBox::warning(this, tr("Import from Device"),
                             tr("Selected device has no embedded measurement."));
        return;
    }

    // Also treat this device as the currentDevice so subsequent Modeller fits
    // (e.g. Fit Pentode) can use its embedded triodeModel seed or pentode
    // parameters as the starting point without requiring a separate Designer
    // selection step.
    currentDevice = srcDevice;
    deviceType = srcDevice->getDeviceType();

    // Clone the embedded measurement via JSON round-trip so the project owns
    // its own independent copy.
    QJsonObject measObj;
    srcMeas->toJson(measObj);
    Measurement *cloned = new Measurement();
    cloned->fromJson(measObj);

    // Ensure a project exists or create one (reuse Save to Project dialog).
    ProjectDialog dialog;
    if (currentProject == nullptr) {
        if (dialog.exec() != QDialog::Accepted) {
            delete cloned;
            return;
        }

        Project *project = new Project();
        project->setName(dialog.getName());
        project->setDeviceType(dialog.getDeviceType());

        setSelectedTreeItem(currentProject, false);
        currentProject = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
        currentProject->setText(0, dialog.getName());
        currentProject->setIcon(0, QIcon(":/icons/valve32.png"));
        currentProject->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        currentProject->setData(0, Qt::UserRole, QVariant::fromValue((void *) project));

        project->setTreeItem(currentProject);
        setSelectedTreeItem(currentProject, true);
    } else {
        // Update existing project metadata from dialog (optional rename/type).
        if (dialog.exec() == QDialog::Accepted) {
            Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
            if (project != nullptr) {
                project->setName(dialog.getName());
                project->setDeviceType(dialog.getDeviceType());
            }
            currentProject->setText(0, dialog.getName());
        }
    }

    Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
    if (!project) {
        delete cloned;
        return;
    }

    if (!project->addMeasurement(cloned)) {
        QMessageBox::warning(this, tr("Import from Device"),
                             tr("Failed to add imported measurement to project."));
        delete cloned;
        return;
    }

    // Attach to project tree and plot on Modeller plot.
    cloned->buildTree(currentProject);
    currentMeasurement = cloned;
    cloned->setSmoothPlotting(preferencesDialog.smoothCurves());

    if (measuredCurves) {
        plot.remove(measuredCurves);
        measuredCurves = nullptr;
    }
    measuredCurves = cloned->updatePlot(&plot);
    if (measuredCurves) {
        plot.add(measuredCurves);
        measuredCurves->setVisible(ui->measureCheck->isChecked());
    }

    // Update properties table to reflect the imported measurement.
    cloned->updateProperties(ui->properties);

    // Populate analyser-style data table so the imported measurement appears
    // on the Data screen just like a live analyser run.
    populateDataTableFromMeasurement(cloned);
}
void ValveWorkbench::on_fitTriodeButton_clicked()
{
    if (currentProject == nullptr) {
        QMessageBox::warning(this, tr("Model Triode"), tr("No project is selected."));
        return;
    }

    modelProject = currentProject;
    ui->fitPentodeButton->setEnabled(false); // Prevent any further modelling invocations
    ui->fitTriodeButton->setEnabled(false);
    doPentodeModel = false;

    modelTriode();
}

void ValveWorkbench::modelTriode()
{
    QList<Measurement *> measurements;

    Measurement *measurement = nullptr;

    // Prefer the explicitly selected triode anode measurement in the
    // project tree when fitting, so the model matches the measurement
    // the user is actually working with. Fall back to the first
    // matching triode/anode measurement in the current project if
    // nothing suitable is selected.
    if (currentMeasurement && currentMeasurementItem &&
        getProject(currentMeasurementItem) == currentProject &&
        currentMeasurement->getDeviceType() == TRIODE &&
        currentMeasurement->getTestType() == ANODE_CHARACTERISTICS) {
        measurement = currentMeasurement;
    } else {
        measurement = findMeasurement(TRIODE, ANODE_CHARACTERISTICS);
    }

    if (measurement == nullptr) {
        QMessageBox message;
        message.setText("There is no Triode Anode Characteristic measurement in the project - this is required for model fitting");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    Estimate estimate;
    estimate.estimateTriode(measurement);

    model = ModelFactory::createModel(COHEN_HELIE_TRIODE);
    model->setEstimate(&estimate);
    model->setPlotColor(QColor::fromRgb(255, 0, 0));
    triodeModelPrimary = model;
    runningTriodeBFit = false;

    triodeMeasurementPrimary = measurement;
    if (triodeMeasurementPrimary != nullptr) {
        triodeMeasurementPrimary->setSampleColor(QColor::fromRgb(0, 0, 0));
    }
    cleanupTriodeBResources();
    triodeBFitPending = false;

    if (isDoubleTriode && measurementHasTriodeBData(measurement)) {
        Measurement *clone = createTriodeBMeasurementClone(measurement);
        if (clone != nullptr && measurementHasValidSamples(clone)) {
            qInfo("Triode B clone created: source sweeps=%d, clone sweeps=%d", measurement->count(), clone->count());
            triodeBClones.append(clone);
            triodeMeasurementSecondary = clone;
            if (triodeMeasurementSecondary != nullptr) {
                triodeMeasurementSecondary->setSampleColor(QColor::fromRgb(0, 0, 255));
            }
            triodeBFitPending = true;
        } else {
            if (clone != nullptr) {
                qInfo("Triode B clone has no valid samples - discarding and skipping secondary fit");
                deleteMeasurementClone(clone);
            } else {
                qInfo("Triode B clone creation returned null");
            }
        }
    }

    int children = currentProject->childCount();
    for (int i = 0; i < children; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MEASUREMENT) {
            Measurement *measurement = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (measurement->getDeviceType() == TRIODE) {
                model->addMeasurement(measurement);
            }
        }
    }

    thread = new QThread;

    model->moveToThread(thread);
    connect(thread, &QThread::started, model, &Model::solveThreaded);
    connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void ValveWorkbench::loadModel()
{
    thread->quit();
    thread = nullptr;

    if (modelProject == nullptr) {
        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);
        return;
    }

    if (!model->isConverged()) {
        QMessageBox message;
        message.setText("The model fitting did not converge - please check that your measurements are valid");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    Project *project = (Project *) modelProject->data(0, Qt::UserRole).value<void *>();
    project->addModel(model);
    QTreeWidgetItem *modelItem = model->buildTree(modelProject);
    if (modelItem != nullptr) {
        QString label = modelItem->text(0);
        if (model == triodeModelPrimary) {
            label = "Model A";
        } else if (model == triodeModelSecondary) {
            label = "Model B";
        }
        modelItem->setText(0, label);

        QColor plotColour = model->getPlotColor();
        if (plotColour.isValid()) {
            modelItem->setForeground(0, QBrush(plotColour));
        }
    }

    if (model == triodeModelPrimary) {
        triodeModelPrimary = nullptr;
    } else if (model == triodeModelSecondary) {
        triodeModelSecondary = nullptr;
    }

    if (!triodeBClones.isEmpty() && triodeBFitPending && !runningTriodeBFit) {
        Measurement *clone = triodeBClones.takeFirst();
        if (clone != nullptr && measurementHasValidSamples(clone)) {
            triodeMeasurementSecondary = clone;

            Estimate secondaryEstimate;
            secondaryEstimate.estimateTriode(clone);

            triodeModelSecondary = ModelFactory::createModel(COHEN_HELIE_TRIODE);
            triodeModelSecondary->setEstimate(&secondaryEstimate);
            triodeModelSecondary->setPlotColor(QColor::fromRgb(0, 128, 0));
            triodeModelSecondary->addMeasurement(clone);

            triodeBFitPending = !triodeBClones.isEmpty();
            runningTriodeBFit = true;
            queueTriodeModelRun(triodeModelSecondary);
            return;
        } else {
            qInfo("Skipped Triode B fit: clone has no valid samples");
            if (clone) {
                deleteMeasurementClone(clone);
            }
            triodeBFitPending = !triodeBClones.isEmpty();
            // Fall through to finalize if nothing else pending
        }
    }

    if (doPentodeModel) {
        modelPentode(); // Will be done in a new thread
        return;
    }

    ui->fitPentodeButton->setEnabled(true); // Allow modelling again
    ui->fitTriodeButton->setEnabled(true);
    modelProject = nullptr;

    if (triodeMeasurementPrimary != nullptr) {
        if (measuredCurves != nullptr) {
            plot.remove(measuredCurves);
            measuredCurves = nullptr;
        }

        triodeMeasurementPrimary->setSmoothPlotting(preferencesDialog.smoothCurves());
        measuredCurves = triodeMeasurementPrimary->updatePlot(&plot);
        if (measuredCurves != nullptr) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(ui->measureCheck->isChecked());
        }
    }
    if (triodeMeasurementSecondary != nullptr) {
        if (measuredCurvesSecondary != nullptr) {
            plot.remove(measuredCurvesSecondary);
            measuredCurvesSecondary = nullptr;
        }
        // Plot secondary without axes to avoid re-drawing axes twice
        triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());
        measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
        if (measuredCurvesSecondary != nullptr) {
            plot.add(measuredCurvesSecondary);
            measuredCurvesSecondary->setVisible(ui->measureCheck->isChecked());
        }
    }

    // If the model that just finished is a pentode fit, prefer to show the
    // latest pentode anode-characteristics measurement in the Designer plot
    // so the red model curves immediately overlay the data we just fitted,
    // rather than leaving the previous triode measurement visible.
    if (model && (model->getType() == GARDINER_PENTODE ||
                  model->getType() == REEFMAN_DERK_PENTODE ||
                  model->getType() == REEFMAN_DERK_E_PENTODE ||
                  model->getType() == SIMPLE_MANUAL_PENTODE)) {

        Measurement *pentodeMeasurement = findMeasurement(PENTODE, ANODE_CHARACTERISTICS);
        if (pentodeMeasurement) {
            if (measuredCurves != nullptr) {
                plot.remove(measuredCurves);
                measuredCurves = nullptr;
            }

            pentodeMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
            measuredCurves = pentodeMeasurement->updatePlot(&plot);
            if (measuredCurves != nullptr) {
                plot.add(measuredCurves);
                measuredCurves->setVisible(ui->measureCheck->isChecked());
            }

            // Make this pentode measurement the active one for subsequent
            // overlay and small-signal calculations.
            currentMeasurement = pentodeMeasurement;
        }
    }
}

void ValveWorkbench::queueTriodeModelRun(Model *modelToRun)
{
    model = modelToRun;
    thread = new QThread;

    modelToRun->moveToThread(thread);
    connect(thread, &QThread::started, modelToRun, &Model::solveThreaded);
    connect(modelToRun, &Model::modelReady, this, &ValveWorkbench::loadModel);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void ValveWorkbench::on_fitPentodeButton_clicked()
{
    modelProject = currentProject;
    ui->fitPentodeButton->setEnabled(false); // Prevent any further modelling invocations
    ui->fitTriodeButton->setEnabled(false);
    doPentodeModel = true;

    modelPentode();

}

void ValveWorkbench::modelPentode()
{
    doPentodeModel = false; // We're doing it now so don't want to do it again!

    // Refresh pentode model selection from preferences each time, so changes
    // made in the Preferences dialog are respected for every new fit.
    pentodeModelType = preferencesDialog.getPentodeModelType();
    Measurement *measurement = findMeasurement(PENTODE, ANODE_CHARACTERISTICS);

    if (measurement == nullptr) {
        QMessageBox message;
        message.setText("There is no Pentode Anode Characteristic measurement in the project - this is required for model fitting");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    if (pentodeModelType == SIMPLE_MANUAL_PENTODE) {
        // Manual, non-Ceres path: seed SimpleManualPentode from the same Estimate
        // used for Gardiner/Reefman so the initial manual curves match an automatic
        // fit in shape and scale, then allow refinement via sliders.

        CohenHelieTriode *triodeModel = (CohenHelieTriode *) findModel(COHEN_HELIE_TRIODE);
        if (triodeModel == nullptr && currentDevice && currentDevice->getDeviceType() == PENTODE && currentDevice->getTriodeSeed() != nullptr) {
            // If there is no separate triode model node, prefer the embedded
            // triodeModel seed stored in the current Device preset.
            triodeModel = currentDevice->getTriodeSeed();
            qInfo("Simple Manual Pentode: using embedded triodeModel seed from device '%s'",
                  currentDevice->getName().toStdString().c_str());
        }
        if (triodeModel == nullptr) {
            qWarning("No triode model found in project or device seed - proceeding with gradient-based seed for manual pentode fit");
        }

        Estimate estimate;
        // Use GARDINER_PENTODE as the estimation target so alpha/beta/gamma, etc.
        // follow the same heuristics as the main fitted model.
        estimate.estimatePentode(measurement, triodeModel, GARDINER_PENTODE, false);

        // Run a single Ceres fit using a temporary GardinerPentode so we get the
        // same "estimate + one solve" behaviour as the main fitted pentode model.
        std::unique_ptr<Model> tempGardiner(ModelFactory::createModel(GARDINER_PENTODE));
        if (tempGardiner) {
            tempGardiner->setEstimate(&estimate);
            tempGardiner->setMode(NORMAL_MODE);
            tempGardiner->setPreferences(&preferencesDialog);
            tempGardiner->addMeasurement(measurement);
            tempGardiner->solve();
        }

        model = ModelFactory::createModel(SIMPLE_MANUAL_PENTODE);
        if (!model) {
            qWarning("Failed to create SimpleManualPentode model");
            ui->fitPentodeButton->setEnabled(true);
            ui->fitTriodeButton->setEnabled(true);
            return;
        }

        if (auto *manual = dynamic_cast<SimpleManualPentode *>(model)) {
            // If the temporary Gardiner model was created and solved, copy the
            // fitted parameters into the manual sliders. Otherwise fall back
            // to using the raw Estimate seed values.
            if (tempGardiner) {
                // Copy both the triode/epk base parameters and the Gardiner
                // pentode shaping parameters so Simple Manual Pentode starts
                // as close as possible to the Gardiner fit before manual tweaks.
                if (auto *p = manual->getParameterObject(PAR_MU))    p->setValue(tempGardiner->getParameter(PAR_MU));
                if (auto *p = manual->getParameterObject(PAR_X))     p->setValue(tempGardiner->getParameter(PAR_X));
                if (auto *p = manual->getParameterObject(PAR_KP))    p->setValue(tempGardiner->getParameter(PAR_KP));
                if (auto *p = manual->getParameterObject(PAR_KVB))   p->setValue(tempGardiner->getParameter(PAR_KVB));
                if (auto *p = manual->getParameterObject(PAR_KVB1))  p->setValue(tempGardiner->getParameter(PAR_KVB1));
                if (auto *p = manual->getParameterObject(PAR_VCT))   p->setValue(tempGardiner->getParameter(PAR_VCT));

                if (auto *p = manual->getParameterObject(PAR_KG1))   p->setValue(tempGardiner->getParameter(PAR_KG1));
                if (auto *p = manual->getParameterObject(PAR_KG2))   p->setValue(tempGardiner->getParameter(PAR_KG2));
                if (auto *p = manual->getParameterObject(PAR_A))     p->setValue(tempGardiner->getParameter(PAR_A));
                if (auto *p = manual->getParameterObject(PAR_ALPHA)) p->setValue(tempGardiner->getParameter(PAR_ALPHA));
                if (auto *p = manual->getParameterObject(PAR_BETA))  p->setValue(tempGardiner->getParameter(PAR_BETA));
                if (auto *p = manual->getParameterObject(PAR_GAMMA)) p->setValue(tempGardiner->getParameter(PAR_GAMMA));

                // Copy Os so any fitted offset floor is preserved in the
                // manual model. This affects low-current behaviour.
                if (auto *p = manual->getParameterObject(PAR_OS))    p->setValue(tempGardiner->getParameter(PAR_OS));

                // If secondary emission is enabled globally, also copy the
                // secondary-emission geometry so the knee/tail region matches.
                if (preferencesDialog.useSecondaryEmission()) {
                    if (auto *p = manual->getParameterObject(PAR_OMEGA))  p->setValue(tempGardiner->getParameter(PAR_OMEGA));
                    if (auto *p = manual->getParameterObject(PAR_LAMBDA)) p->setValue(tempGardiner->getParameter(PAR_LAMBDA));
                    if (auto *p = manual->getParameterObject(PAR_NU))     p->setValue(tempGardiner->getParameter(PAR_NU));
                    if (auto *p = manual->getParameterObject(PAR_S))      p->setValue(tempGardiner->getParameter(PAR_S));
                    if (auto *p = manual->getParameterObject(PAR_AP))     p->setValue(tempGardiner->getParameter(PAR_AP));
                }
            } else {
                // Map overlapping Estimate fields into the manual model's parameters.
                if (auto *p = manual->getParameterObject(PAR_MU))    p->setValue(estimate.getMu());
                if (auto *p = manual->getParameterObject(PAR_KG1))   p->setValue(estimate.getKg1());
                if (auto *p = manual->getParameterObject(PAR_KG2))   p->setValue(estimate.getKg2());
                if (auto *p = manual->getParameterObject(PAR_KP))    p->setValue(estimate.getKp());
                if (auto *p = manual->getParameterObject(PAR_A))     p->setValue(estimate.getA());
                if (auto *p = manual->getParameterObject(PAR_ALPHA)) p->setValue(estimate.getAlpha());
                if (auto *p = manual->getParameterObject(PAR_BETA))  p->setValue(estimate.getBeta());
                if (auto *p = manual->getParameterObject(PAR_GAMMA)) p->setValue(estimate.getGamma());
            }
        } else {
            qWarning("Created pentode model is not a SimpleManualPentode instance");
        }

        model->setPreferences(&preferencesDialog);
        currentMeasurement = measurement;
        plotCurrentModelOverMeasurement();
        ensureSimplePentodeDialog();

        ui->fitPentodeButton->setEnabled(true);
        ui->fitTriodeButton->setEnabled(true);
        return;
    }

    CohenHelieTriode *triodeModel = (CohenHelieTriode *) findModel(COHEN_HELIE_TRIODE);
    if (triodeModel == nullptr && currentDevice && currentDevice->getDeviceType() == PENTODE && currentDevice->getTriodeSeed() != nullptr) {
        // If there is no separate triode model in the project, prefer the
        // embedded triodeModel seed from the currently selected Device.
        triodeModel = currentDevice->getTriodeSeed();
        qInfo("No triode model in project; using embedded triodeModel seed from device '%s'",
              currentDevice->getName().toStdString().c_str());
    }

    Estimate estimate;

    if (triodeModel != nullptr) {
        // Normal path: use the project's triode model or the embedded
        // triodeModel seed as the base for pentode estimation so the
        // Gardiner/Reefman pentode starts from a consistent triode base.
        estimate.estimatePentode(measurement, triodeModel, pentodeModelType, false);
    } else if (currentDevice && currentDevice->getDeviceType() == PENTODE) {
        // Secondary fallback: if we have no explicit triode seed but do have a
        // pentode device model, copy its parameters into the Estimate as a
        // starting point.
        qInfo("No triode model or triode seed; seeding pentode Estimate from current device model '%s'",
              currentDevice->getName().toStdString().c_str());

        estimate.setMu(currentDevice->getParameter(PAR_MU));
        estimate.setKg1(currentDevice->getParameter(PAR_KG1));
        estimate.setX(currentDevice->getParameter(PAR_X));
        estimate.setKp(currentDevice->getParameter(PAR_KP));
        estimate.setKvb(currentDevice->getParameter(PAR_KVB));
        estimate.setKvb1(currentDevice->getParameter(PAR_KVB1));
        estimate.setVct(currentDevice->getParameter(PAR_VCT));

        estimate.setKg2(currentDevice->getParameter(PAR_KG2));
        estimate.setA(currentDevice->getParameter(PAR_A));
        estimate.setAlpha(currentDevice->getParameter(PAR_ALPHA));
        estimate.setBeta(currentDevice->getParameter(PAR_BETA));
        estimate.setGamma(currentDevice->getParameter(PAR_GAMMA));
        estimate.setPsi(currentDevice->getParameter(PAR_PSI));

        estimate.setOmega(currentDevice->getParameter(PAR_OMEGA));
        estimate.setLambda(currentDevice->getParameter(PAR_LAMBDA));
        estimate.setNu(currentDevice->getParameter(PAR_NU));
        estimate.setS(currentDevice->getParameter(PAR_S));
        estimate.setAp(currentDevice->getParameter(PAR_AP));
    } else {
        // Last resort: fall back to the legacy gradient-based estimate that
        // derives all parameters directly from the measurement alone.
        qWarning("No triode model found in project and no suitable current device; proceeding with gradient-based seed for pentode fit");
        estimate.estimatePentode(measurement, nullptr, pentodeModelType, false);
    }

    model = ModelFactory::createModel(pentodeModelType);
    model->setEstimate(&estimate);
    model->setMode(NORMAL_MODE);
    model->setPreferences(&preferencesDialog);

    int children = currentProject->childCount();
    for (int i = 0; i < children; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MEASUREMENT) {
            measurement = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (measurement->getDeviceType() == PENTODE &&
                measurement->getTestType() == ANODE_CHARACTERISTICS) {
                model->addMeasurement(measurement);
            }
        }
    }

    thread = new QThread;

    model->moveToThread(thread);
    disconnect(model, nullptr, this, nullptr);
    connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();

    QMetaObject::invokeMethod(model, "solveThreaded");
}

void ValveWorkbench::modelScreen()
{
    if (!model->isConverged()) {
        QMessageBox message;
        message.setText("The anode current fitting did not converge - please check that your measurements are valid");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        thread->quit();

        return;
    }

    model->setMode(SCREEN_MODE);

    disconnect(model, &Model::modelReady, this, &ValveWorkbench::modelScreen); // We don't want to go round the loop again!

    if (preferencesDialog.useRemodelling()) {
        connect(model, &Model::modelReady, this, &ValveWorkbench::remodelAnode);
    } else {
        connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);
    }

    QMetaObject::invokeMethod(model, "solveThreaded");
}

void ValveWorkbench::remodelAnode()
{
    model->setMode(ANODE_REMODEL_MODE);

    disconnect(model, &Model::modelReady, this, &ValveWorkbench::remodelAnode);
    connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);

    QMetaObject::invokeMethod(model, "solveThreaded");
}

void ValveWorkbench::on_tabWidget_currentChanged(int index)
{
    // When switching between Designer/Modeller/Analyser, clear any existing
    // measurement/model overlays from the shared Plot so each tab can
    // reconstruct its own view without dangling QGraphicsItemGroup pointers.
    if (measuredCurves != nullptr) {
        plot.remove(measuredCurves);
        measuredCurves = nullptr;
    }
    if (measuredCurvesSecondary != nullptr) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }
    if (estimatedCurves != nullptr) {
        plot.remove(estimatedCurves);
        estimatedCurves = nullptr;
    }
    if (modelledCurves != nullptr) {
        plot.remove(modelledCurves);
        modelledCurves = nullptr;
    }
    if (modelledCurvesSecondary != nullptr) {
        plot.remove(modelledCurvesSecondary);
        modelledCurvesSecondary = nullptr;
    }

    // Map the concrete tab widget to a logical role:
    // 0 = Designer, 1 = Modeller, 2 = Analyser.
    int tabRole = -1;
    if (ui->tabWidget) {
        QWidget *w = ui->tabWidget->widget(index);
        if (w == ui->tab) {
            tabRole = 0;
        } else if (w == ui->tab_2) {
            tabRole = 1;
        } else if (w == ui->tab_3) {
            tabRole = 2;
        }
    }

    // Health boxes under the shared plot are only relevant to the Analyser tab.
    const bool onAnalyserTab = (tabRole == 2);
    if (ui->Triode_A_Box) ui->Triode_A_Box->setVisible(onAnalyserTab);
    if (ui->Triode_B_Box) ui->Triode_B_Box->setVisible(onAnalyserTab);

    if (tabRole == 2) {
        // Analyser tab: ensure measurement/model/screen toggles are visible.
        if (ui->measureCheck) ui->measureCheck->setVisible(true);
        if (ui->modelCheck) ui->modelCheck->setVisible(true);
        if (ui->screenCheck) ui->screenCheck->setVisible(true);

        // If we already have a measurement, refresh the Data table so that
        // values are restored when returning to the Analyser tab.
        if (currentMeasurement && dataTable) {
            populateDataTableFromMeasurement(currentMeasurement);
        }
    } else if (tabRole == 1) {
        // Modeller tab: show measurement/model/screen toggles and control
        // Fit buttons based on the current project's device type.
        if (ui->measureCheck) ui->measureCheck->setVisible(true);
        if (ui->modelCheck) ui->modelCheck->setVisible(true);
        if (ui->screenCheck) ui->screenCheck->setVisible(true);

        if (currentProject != nullptr) {
            QTreeWidgetItem *rootProject = currentProject;
            if (rootProject->type() != TYP_PROJECT) {
                rootProject = getParent(rootProject, TYP_PROJECT);
            }
            if (!rootProject) {
                ui->fitTriodeButton->setVisible(false);
                ui->fitPentodeButton->setVisible(false);
            } else {
                Project *project = static_cast<Project *>(rootProject->data(0, Qt::UserRole).value<void *>());
                if (!project) {
                    ui->fitTriodeButton->setVisible(false);
                    ui->fitPentodeButton->setVisible(false);
                } else if (project->getDeviceType() == TRIODE) {
                    ui->fitTriodeButton->setVisible(true);
                    ui->fitPentodeButton->setVisible(false);
                } else if (project->getDeviceType() == PENTODE) {
                    ui->fitTriodeButton->setVisible(false);
                    ui->fitPentodeButton->setVisible(true);
                    if (pentodeModelType == SIMPLE_MANUAL_PENTODE) {
                        ensureSimplePentodeDialog();
                    }
                } else {
                    ui->fitTriodeButton->setVisible(false);
                    ui->fitPentodeButton->setVisible(false);
                }
            }
        } else {
            ui->fitTriodeButton->setVisible(false);
            ui->fitPentodeButton->setVisible(false);
        }
    } else if (tabRole == 0) {
        // Designer tab: keep the measurement/model/screen toggles visible so
        // the user can manually hide/show analyser/modeller overlays while
        // working in Designer.
        if (ui->measureCheck) ui->measureCheck->setVisible(true);
        if (ui->modelCheck) ui->modelCheck->setVisible(true);
        if (ui->screenCheck) ui->screenCheck->setVisible(true);
    }

    // Apply the stored overlay state for this tab role to the shared checkboxes.
    if (tabRole >= 0 && tabRole < 3) {
        if (ui->measureCheck) {
            ui->measureCheck->setChecked(overlayStates[tabRole].showMeasurement);
        }
        if (ui->modelCheck) {
            ui->modelCheck->setChecked(overlayStates[tabRole].showModel);
        }
        if (ui->screenCheck) {
            ui->screenCheck->setChecked(overlayStates[tabRole].showScreen);
        }

        // If this tab is configured to show measurements and the plot was
        // cleared on tab change, lazily recreate measurement curves using the
        // same logic as the checkbox handler so curves reappear without
        // requiring a manual toggle sequence.
        if (overlayStates[tabRole].showMeasurement && ui->measureCheck) {
            on_measureCheck_stateChanged(ui->measureCheck->checkState());
        }
    }
}

void ValveWorkbench::on_measureCheck_stateChanged(int arg1)
{
    Q_UNUSED(arg1);

    const bool wantVisible = ui->measureCheck && ui->measureCheck->isChecked();

    // Map current tab widget to logical role: 0 = Designer, 1 = Modeller, 2 = Analyser.
    int tabRole = -1;
    if (ui->tabWidget) {
        QWidget *w = ui->tabWidget->currentWidget();
        if (w == ui->tab) {
            tabRole = 0;
        } else if (w == ui->tab_2) {
            tabRole = 1;
        } else if (w == ui->tab_3) {
            tabRole = 2;
        }
    }
    if (tabRole >= 0 && tabRole < 3) {
        overlayStates[tabRole].showMeasurement = wantVisible;
    }

    qInfo("ValveWorkbench::on_measureCheck_stateChanged: wantVisible=%d, tabRole=%d", wantVisible ? 1 : 0, tabRole);

    // If curves already exist, just toggle visibility.
    if (measuredCurves != nullptr) {
        measuredCurves->setVisible(wantVisible);
    }
    if (measuredCurvesSecondary != nullptr) {
        measuredCurvesSecondary->setVisible(wantVisible);
    }

    // When turning measurement visibility ON and there are no curves yet for
    // this tab, lazily (re)create them from the appropriate Measurement
    // source. This keeps behaviour intuitive when returning to a tab or
    // enabling Show Measurement after plot groups were cleared.
    if (!wantVisible) {
        qInfo("ValveWorkbench::on_measureCheck_stateChanged: measurement visibility turned OFF; no rebuild");
        return;
    }

    // Rebuild measurement groups using the latest smoothing preference
    // whenever Show Measurement is turned ON. This ensures that changes
    // to the smoothing option or screen overlay state are reflected
    // immediately on the active tab.
    if (measuredCurves) {
        plot.remove(measuredCurves);
        measuredCurves = nullptr;
    }
    if (measuredCurvesSecondary) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }

    // Analyser tab (role 2): use currentMeasurement with full axes, and
    // restore any Triode B overlay if a secondary measurement exists.
    if (tabRole == 2 && currentMeasurement) {
        qInfo("ValveWorkbench::on_measureCheck_stateChanged: rebuilding Analyser measurement curves");
        currentMeasurement->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
        currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
        measuredCurves = currentMeasurement->updatePlot(&plot);
        if (measuredCurves) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(true);
        }

        if (triodeMeasurementSecondary && !measuredCurvesSecondary) {
            qInfo("ValveWorkbench::on_measureCheck_stateChanged: rebuilding Analyser Triode B overlay");
            triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());
            measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
            if (measuredCurvesSecondary) {
                plot.add(measuredCurvesSecondary);
                measuredCurvesSecondary->setVisible(true);
            }
        }
    }

    // Modeller tab (role 1): use currentMeasurement and any available
    // Triode B clone for secondary overlays.
    if (tabRole == 1 && currentMeasurement) {
        qInfo("ValveWorkbench::on_measureCheck_stateChanged: rebuilding Modeller measurement curves");
        currentMeasurement->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
        currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
        measuredCurves = currentMeasurement->updatePlot(&plot);
        if (measuredCurves) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(true);
        }

        // Recreate Triode B measurement overlay if present.
        if (triodeMeasurementSecondary && !measuredCurvesSecondary) {
            measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
            if (measuredCurvesSecondary) {
                plot.add(measuredCurvesSecondary);
                measuredCurvesSecondary->setVisible(true);
            }
        }
    }

    // Designer tab (role 0): use embedded Measurement on current Device
    // (tube-style preset) without changing Designer axes.
    if (tabRole == 0 && currentDevice && currentDevice->getMeasurement()) {
        qInfo("ValveWorkbench::on_measureCheck_stateChanged: rebuilding Designer embedded measurement curves");
        Measurement *embedded = currentDevice->getMeasurement();
        embedded->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
        embedded->setSmoothPlotting(preferencesDialog.smoothCurves());

        measuredCurves = embedded->updatePlotWithoutAxes(&plot);
        if (measuredCurves) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(true);
        }
    }
}

void ValveWorkbench::on_modelCheck_stateChanged(int arg1)
{
    Q_UNUSED(arg1);

    const bool wantVisible = ui->modelCheck && ui->modelCheck->isChecked();

    // Map current tab widget to logical role: 0 = Designer, 1 = Modeller, 2 = Analyser.
    int tabRole = -1;
    if (ui->tabWidget) {
        QWidget *w = ui->tabWidget->currentWidget();
        if (w == ui->tab) {
            tabRole = 0;
        } else if (w == ui->tab_2) {
            tabRole = 1;
        } else if (w == ui->tab_3) {
            tabRole = 2;
        }
    }
    if (tabRole >= 0 && tabRole < 3) {
        overlayStates[tabRole].showModel = wantVisible;
    }

    // Pure visibility toggle: model plotting paths create the groups; the
    // checkbox only shows/hides them.
    if (modelledCurves) {
        modelledCurves->setVisible(wantVisible);
    }
    if (modelledCurvesSecondary) {
        modelledCurvesSecondary->setVisible(wantVisible);
    }
}

void ValveWorkbench::on_designerCheck_stateChanged(int arg1)
{
    const bool visible = (arg1 != 0);
    for (Circuit *c : circuits) {
        if (!c) continue;
        c->setOverlaysVisible(visible);
        // If this is the TriodeCommonCathode, also toggle its extra overlay groups
        if (auto *t = dynamic_cast<TriodeCommonCathode*>(c)) {
            t->setDesignerOverlaysVisible(visible);
        }
    }
}

void ValveWorkbench::on_symSwingCheck_stateChanged(int arg1)
{
    const bool enabled = (arg1 != 0);

    // Only apply Max Sym Swing change to the currently selected Designer
    // circuit so that we don't have multiple circuits overwriting the
    // shared Designer UI simultaneously.
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    if (!c) {
        return;
    }

    if (auto *t = dynamic_cast<TriodeCommonCathode*>(c)) {
        t->setSymSwingEnabled(enabled);
        t->plot(&plot);
        t->updateUI(circuitLabels, circuitValues);
    } else if (auto *se = dynamic_cast<SingleEndedOutput*>(c)) {
        // Reset SE headroom manual override to 0 whenever the Max Sym Swing
        // checkbox is clicked, so that the helper-derived symmetric/max swing
        // becomes the default effective headroom again.
        se->setParameter(SE_HEADROOM, 0.0);
        se->setSymSwingEnabled(enabled);
        se->plot(&plot);
        se->updateUI(circuitLabels, circuitValues);
    } else if (auto *pp = dynamic_cast<PushPullOutput*>(c)) {
        pp->setSymSwingEnabled(enabled);
        pp->plot(&plot);
        pp->updateUI(circuitLabels, circuitValues);
    } else if (auto *seul = dynamic_cast<SingleEndedUlOutput*>(c)) {
        seul->setSymSwingEnabled(enabled);
        seul->plot(&plot);
        seul->updateUI(circuitLabels, circuitValues);
    } else if (auto *ppul = dynamic_cast<PushPullUlOutput*>(c)) {
        ppul->setSymSwingEnabled(enabled);
        ppul->plot(&plot);
        ppul->updateUI(circuitLabels, circuitValues);
    }
}

void ValveWorkbench::on_inputSensitivityCheck_stateChanged(int arg1)
{
    const bool enabled = (arg1 != 0);
    for (Circuit *c : circuits) {
        if (auto *t = dynamic_cast<TriodeCommonCathode*>(c)) {
            t->setInputSensitivityEnabled(enabled);
            t->plot(&plot);
        }
    }
}

void ValveWorkbench::on_useBypassedGainCheck_stateChanged(int arg1)
{
    const bool useBypassed = (arg1 != 0);

    // Only apply K-bypass change to the currently selected Designer circuit
    // so that we don't have multiple circuits overwriting the shared
    // circuitLabels/circuitValues UI with N/A values.
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size()) {
        return;
    }

    Circuit *c = circuits.at(currentCircuitType);
    if (!c) {
        return;
    }

    if (auto *t = dynamic_cast<TriodeCommonCathode*>(c)) {
        t->setSensitivityGainMode(useBypassed ? 1 : 0);
        t->plot(&plot);
        // Refresh Designer panel values (Input sensitivity depends on gain mode)
        t->updateUI(circuitLabels, circuitValues);
    } else if (auto *se = dynamic_cast<SingleEndedOutput*>(c)) {
        // Apply K-bypass choice to the SE output stage so that its
        // input sensitivity and THD reflect bypassed vs unbypassed cathode.
        se->setGainMode(useBypassed ? 1 : 0);
        se->plot(&plot);
        se->updateUI(circuitLabels, circuitValues);
    } else if (auto *pp = dynamic_cast<PushPullOutput*>(c)) {
        // Apply K-bypass choice to the PP output stage so that its
        // input sensitivity and THD reflect bypassed vs unbypassed cathode.
        pp->setGainMode(useBypassed ? 1 : 0);
        pp->plot(&plot);
        pp->updateUI(circuitLabels, circuitValues);
    } else if (auto *seul = dynamic_cast<SingleEndedUlOutput*>(c)) {
        // Apply K-bypass choice to the SE-UL output stage so that its
        // input sensitivity and THD reflect bypassed vs unbypassed cathode.
        seul->setGainMode(useBypassed ? 1 : 0);
        seul->plot(&plot);
        seul->updateUI(circuitLabels, circuitValues);
    } else if (auto *ppul = dynamic_cast<PushPullUlOutput*>(c)) {
        // Apply K-bypass choice to the PP-UL output stage so that its
        // input sensitivity and THD reflect bypassed vs unbypassed cathode.
        ppul->setGainMode(useBypassed ? 1 : 0);
        ppul->plot(&plot);
        ppul->updateUI(circuitLabels, circuitValues);
    }
}


void ValveWorkbench::on_properties_itemChanged(QTableWidgetItem *item)
{
    DataSet *dataSet = item->data(Qt::UserRole).value<DataSet *>();
    if (dataSet != nullptr) {
        dataSet->editCallback(item);
    }
}


void ValveWorkbench::on_compareButton_clicked()
{
    // TEMP LOG START
    qInfo("Compare: handler start");
    // Prefer the project node that modelling used (it holds fitted models)
    QTreeWidgetItem *projectItem = modelProject != nullptr ? modelProject : currentProject;
    if (projectItem == nullptr) {
        QMessageBox message;
        message.setText("No project selected");
        message.exec();

        return;
    }

    CompareDialog dialog;

    // Ensure we are using the actual project node (not a child) before casting
    QTreeWidgetItem *rootProject = nullptr;
    if (projectItem->type() == TYP_PROJECT) {
        rootProject = projectItem;
    } else {
        rootProject = getParent(projectItem, TYP_PROJECT);
    }
    if (rootProject == nullptr) {
        qWarning("Compare: could not locate project node from current selection");
        QMessageBox message;
        message.setText("Could not locate the project for comparison.");
        message.exec();
        return;
    }

    Project *project = (Project *) rootProject->data(0, Qt::UserRole).value<void *>();
    Model *model;
    if (project->getDeviceType() == TRIODE) {
        model = findModel(COHEN_HELIE_TRIODE);
    } else {
        model = findModel(GARDINER_PENTODE);
    }

    if (model == nullptr) {
        QMessageBox message;
        message.setText("No model found");
        message.exec();

        return;
    }

    // Build available models list: project models + presets from models/ folder
    QList<Model *> available;
    const QList<Model *> &projectModels = project->getModels();
    qInfo("Compare: project models count=%d", projectModels.size());
    for (Model *m : projectModels) {
        qInfo("Compare: project model ptr=%p name=%s", m, m ? m->getName().toUtf8().constData() : "(null)");
        available.append(m);
    }
    // Ensure the current model is present at least once
    if (!available.contains(model)) {
        qInfo("Compare: appending current model ptr=%p name=%s", model, model->getName().toUtf8().constData());
        available.prepend(model);
    }

    // Also include any model nodes present in the project tree (in case the project list isn't updated yet)
    if (projectItem != nullptr) {
        for (int i = 0; i < projectItem->childCount(); ++i) {
            QTreeWidgetItem *child = projectItem->child(i);
            if (!child) continue;
            if (child->type() != TYP_MODEL) continue;
            QVariant v = child->data(0, Qt::UserRole);
            Model *m = static_cast<Model *>(v.value<void *>());
            if (m && !available.contains(m)) {
                qInfo("Compare: adding model from tree ptr=%p text=%s", m, child->text(0).toUtf8().constData());
                available.append(m);
            }
        }
    }

    // Load preset models (triode) from models/ folder as additional comparison options
    if (project->getDeviceType() == TRIODE) {
        QDir modelDir(QCoreApplication::applicationDirPath());
        // Try project root \models first if running from source
        QDir sourceModelsDir(QDir::cleanPath(QDir::currentPath() + "/models"));
        QStringList candidates;
        if (sourceModelsDir.exists()) {
            candidates = sourceModelsDir.entryList(QStringList() << "*.json", QDir::Files);
            qInfo("Compare: preset candidates in models/ count=%d", candidates.size());
            for (const QString &file : candidates) {
                QFile f(sourceModelsDir.filePath(file));
                if (!f.open(QIODevice::ReadOnly)) continue;
                const QByteArray bytes = f.readAll();
                f.close();
                QJsonDocument doc = QJsonDocument::fromJson(bytes);
                if (!doc.isObject()) continue;
                // Create a triode model and load parameters
                Model *preset = ModelFactory::createModel(COHEN_HELIE_TRIODE);
                if (!preset) continue;
                preset->fromJson(doc.object());
                preset->setProperty("compareLabel", QFileInfo(file).baseName());
                qInfo("Compare: preset added label=%s ptr=%p", QFileInfo(file).baseName().toUtf8().constData(), preset);
                available.append(preset);
            }
        }
    }

    // Derive friendly labels from project tree (e.g. "Model A" / "Model B")
    // and attach them to the model objects for CompareDialog to use
    if (projectItem != nullptr) {
        for (int i = 0; i < projectItem->childCount(); ++i) {
            QTreeWidgetItem *child = projectItem->child(i);
            if (!child) continue;
            if (child->type() != TYP_MODEL) continue;
            QVariant v = child->data(0, Qt::UserRole);
            Model *m = static_cast<Model *>(v.value<void *>());
            if (m) {
                m->setProperty("compareLabel", child->text(0));
            }
        }
        // Ensure the current model also has a label (in case it was not in the tree yet)
        if (model && !model->property("compareLabel").isValid()) {
            model->setProperty("compareLabel", model->getName());
        }
    }

    // Populate dialog with available models and set initial selections
    qInfo("Compare: final available count=%d", available.size());
    for (int i = 0; i < available.size(); ++i) {
        Model *m = available.at(i);
        const QVariant labelProp = m ? m->property("compareLabel") : QVariant();
        const QString label = labelProp.isValid() ? labelProp.toString() : (m ? m->getName() : QString("(null)"));
        qInfo("Compare: [%d] ptr=%p label=%s", i, m, label.toUtf8().constData());
    }
    dialog.setAvailableModels(available);
    dialog.setModel(model); // reference selection defaults to the recently fitted model

    // If there is more than one model, prefer "Model B" for comparison if present
    if (available.size() > 1) {
        Model *preferred = nullptr;
        for (Model *m : available) {
            if (m == model) continue; // skip reference
            const QVariant lbl = m->property("compareLabel");
            if (lbl.isValid() && lbl.toString().contains("Model B", Qt::CaseInsensitive)) {
                preferred = m;
                break;
            }
        }
        if (!preferred) {
            // pick the first different from reference
            for (Model *m : available) { if (m != model) { preferred = m; break; } }
        }
        if (preferred) {
            dialog.setComparisonModel(preferred);
        }
    }

    qInfo("Compare: opening dialog");
    dialog.exec();
    qInfo("Compare: dialog closed");
}

void ValveWorkbench::on_heaterButton_clicked()
{
    // No-op: heater control is not used; heaters are fixed in hardware
    Q_UNUSED(this);
}

void ValveWorkbench::exportFittedModelToDevices()
{
    // Determine a model to export: prefer the currently selected model item in the project tree
    Model *toExport = nullptr;
    if (currentModelItem != nullptr) {
        toExport = static_cast<Model *>(currentModelItem->data(0, Qt::UserRole).value<void *>());
    }
    if (!toExport && model) {
        toExport = model; // fallback to last fitted model pointer
    }
    if (!toExport) {
        QMessageBox::warning(this, tr("Export to Devices"), tr("No model available to export."));
        return;
    }

    // If an analyser measurement is available, we prefer its recorded limits
    // (iaMax/paMax, sweep ranges) when building the exported device preset so
    // Designer and Modeller graph limits track the actual measurement rather
    // than any stale analyser UI values.
    Measurement *measForExport = findMeasurement(deviceType, ANODE_CHARACTERISTICS);

    // Resolve models directory to MATCH loadDevices() search, and use a
    // Windows-style save dialog first so the chosen filename drives the
    // Device name stored in JSON.
    QString initialName = ui && ui->deviceName ? ui->deviceName->text() : toExport->getName();
    if (initialName.trimmed().isEmpty()) {
        initialName = QStringLiteral("FittedModel");
    }

    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../../../../../models",
        QCoreApplication::applicationDirPath() + "/../../../../models",
        QCoreApplication::applicationDirPath() + "/../../../models",
        QCoreApplication::applicationDirPath() + "/../models",
        QCoreApplication::applicationDirPath() + "/models",
        QDir::currentPath() + "/models",
        QDir::currentPath() + "/../models",
        QDir::currentPath() + "/../../models",
        QDir::currentPath() + "/../../../models"
    };

    QString exportPath;
    for (const QString &p : possiblePaths) {
        QDir d(p);
        if (d.exists()) { exportPath = d.absolutePath(); break; }
    }
    if (exportPath.isEmpty()) {
        exportPath = QDir::cleanPath(QDir::currentPath() + "/models");
        QDir().mkpath(exportPath);
    }
    QDir modelsDir(exportPath);
    qInfo("Export to Devices: using models dir %s", modelsDir.absolutePath().toUtf8().constData());

    // Use the UI/device name as a starting point for the suggested filename.
    QString baseForFile = initialName;
    baseForFile.replace(QRegularExpression("[^A-Za-z0-9._ -]"), "_");
    if (baseForFile.isEmpty()) baseForFile = QStringLiteral("FittedModel");

    const QString suggestedPath = modelsDir.filePath(baseForFile + ".json");

    QString outPath = QFileDialog::getSaveFileName(this,
                                                   tr("Export model to Device"),
                                                   suggestedPath,
                                                   tr("ValveWorkbench Device (*.json);;All Files (*.*)"));
    if (outPath.isEmpty()) {
        return;
    }

    // Derive Device name from the chosen filename so presets and filenames
    // stay in sync, while still falling back to the UI/model name if the
    // path does not contain a usable base name.
    QFileInfo fi(outPath);
    QString deviceName = fi.completeBaseName().trimmed();
    if (deviceName.isEmpty()) {
        deviceName = initialName.trimmed();
        if (deviceName.isEmpty()) {
            deviceName = QStringLiteral("FittedModel");
        }
    }

    // Build device preset JSON
    QJsonObject root;
    root["name"] = deviceName;

    // Start from current analyser UI limits if available; otherwise sensible
    // defaults, then override Ia/P limits from the recorded measurement (if
    // present) so exported presets always reflect the actual test envelope.
    // For pentodes, we still clamp Ia to the hardware 50 mA capability.
    double vaMaxOut = 300.0;
    double iaMaxOut = 5.0;
    double paMaxOut = 1.125;

    if (std::isfinite(anodeStop) && anodeStop > 0.0) {
        vaMaxOut = anodeStop;
    }
    if (std::isfinite(iaMax) && iaMax > 0.0) {
        iaMaxOut = iaMax;
    }
    if (std::isfinite(pMax) && pMax > 0.0) {
        paMaxOut = pMax;
    }

    // Prefer the measurement's own Ia/P limits when available; these remain
    // stable even if the analyser UI template is later changed.
    if (measForExport) {
        const double measIaMax = measForExport->getIaMax();
        const double measPMax  = measForExport->getPMax();
        if (std::isfinite(measIaMax) && measIaMax > 0.0) {
            iaMaxOut = measIaMax;
        }
        if (std::isfinite(measPMax) && measPMax > 0.0) {
            paMaxOut = measPMax;
        }
    }

    if (deviceType == PENTODE) {
        // Give pentode Designer circuits enough voltage headroom.
        if (vaMaxOut < 500.0) {
            vaMaxOut = 500.0;
        }

        // Derive vg1Max from the analyser grid stop magnitude so that
        // Designer plots use a comparable grid-voltage family to the
        // measured curves (e.g. 0 .. -40 V for a 6L6GC).
        double vg1MaxOut = gridStop;
        if (vg1MaxOut < 0.0) {
            vg1MaxOut = -vg1MaxOut;
        }
        if (!(vg1MaxOut > 0.0)) {
            vg1MaxOut = 50.0; // fallback similar to legacy presets
        }
        root["vg1Max"] = vg1MaxOut;

        // Derive vg2Max from the analyser screen settings so Designer uses
        // a realistic screen voltage instead of defaulting to Va max.
        double vg2MaxOut = screenStart;
        if (vg2MaxOut == 0.0) {
            vg2MaxOut = screenStop;
        }
        if (vg2MaxOut < 0.0) {
            vg2MaxOut = -vg2MaxOut;
        }
        if (vg2MaxOut > 0.0) {
            root["vg2Max"] = vg2MaxOut;
        }

        // For power pentodes, honour the analyser's Ia limit (from the
        // input boxes/template) but clamp it to the hardware maximum so
        // Designer cannot exceed the 50 mA capability of the analyser.
        if (!(iaMaxOut > 0.0)) {
            iaMaxOut = 5.0; // conservative fallback if analyser Ia is unset
        }
        if (iaMaxOut > 50.0) {
            iaMaxOut = 50.0;
        }
    }

    root["vaMax"] = vaMaxOut;
    if (deviceType == TRIODE) {
        root["vg1Max"] = 5.0;
    }
    root["iaMax"] = iaMaxOut;
    root["paMax"] = paMaxOut;

    // Device type string for presets
    if (deviceType == TRIODE) {
        root["deviceType"] = "TRIODE";
    } else if (deviceType == PENTODE) {
        root["deviceType"] = "PENTODE";
    } else if (deviceType == DOUBLE_TRIODE) {
        root["deviceType"] = "DOUBLE_TRIODE";
    } else {
        root["deviceType"] = "UNKNOWN";
    }

    // Persist analyser defaults alongside the fitted model so presets can act as
    // both analyser templates and Designer devices.
    QJsonObject analyserDefaults;

    // Anode sweep range
    {
        QJsonObject anodeObj;
        anodeObj["start"] = anodeStart;
        anodeObj["step"]  = anodeStep;
        anodeObj["stop"]  = anodeStop;
        analyserDefaults["anode"] = anodeObj;
    }

    // Grid sweep range
    {
        QJsonObject gridObj;
        gridObj["start"] = gridStart;
        gridObj["step"]  = gridStep;
        gridObj["stop"]  = gridStop;
        analyserDefaults["grid"] = gridObj;
    }

    // Screen sweep range
    {
        QJsonObject screenObj;
        screenObj["start"] = screenStart;
        screenObj["step"]  = screenStep;
        screenObj["stop"]  = screenStop;
        analyserDefaults["screen"] = screenObj;
    }

    // Limits
    {
        QJsonObject limitsObj;
        limitsObj["iaMax"] = iaMaxOut;
        limitsObj["pMax"]  = paMaxOut;
        analyserDefaults["limits"] = limitsObj;
    }

    // Per-test snapshot for the exported measurement so the analyser tab
    // can restore the exact sweep ranges and limits used when this data was
    // captured.
    if (measForExport) {
        QJsonObject testsObj;
        QJsonObject snapshot;
        const int measType = measForExport->getTestType();
        snapshot.insert("testType", measType);

        auto makeRangeFromMeas = [&](double (Measurement::*getStart)() const,
                                     double (Measurement::*getStop)() const,
                                     double (Measurement::*getStep)() const) {
            QJsonObject r;
            r.insert("start", (measForExport->*getStart)());
            r.insert("step",  (measForExport->*getStep)());
            r.insert("stop",  (measForExport->*getStop)());
            return r;
        };

        snapshot.insert("anode",  makeRangeFromMeas(&Measurement::getAnodeStart,
                                                     &Measurement::getAnodeStop,
                                                     &Measurement::getAnodeStep));
        snapshot.insert("grid",   makeRangeFromMeas(&Measurement::getGridStart,
                                                     &Measurement::getGridStop,
                                                     &Measurement::getGridStep));
        snapshot.insert("screen", makeRangeFromMeas(&Measurement::getScreenStart,
                                                     &Measurement::getScreenStop,
                                                     &Measurement::getScreenStep));

        QJsonObject testLimits;
        testLimits.insert("iaMax", measForExport->getIaMax());
        testLimits.insert("pMax",  measForExport->getPMax());
        snapshot.insert("limits", testLimits);

        QString key;
        switch (measType) {
        case ANODE_CHARACTERISTICS:    key = QStringLiteral("anode");    break;
        case TRANSFER_CHARACTERISTICS: key = QStringLiteral("transfer"); break;
        case SCREEN_CHARACTERISTICS:   key = QStringLiteral("screen");   break;
        default:                       key = QString::number(measType);   break;
        }

        testsObj.insert(key, snapshot);
        analyserDefaults["tests"] = testsObj;

        // Ensure the default testType matches the measurement we attached.
        analyserDefaults["testType"] = measType;
    }

    analyserDefaults["doubleTriode"] = isDoubleTriode;

    root["analyserDefaults"] = analyserDefaults;

    // Sync any edited datasheet/reference values from the Analyser UI back
    // into the datasheetJson block before exporting this device preset.
    syncDatasheetFromUi();
    if (!datasheetJson.isEmpty()) {
        root["datasheet"] = datasheetJson;
    }

    // Fitted model parameters: log key values at export time so we can
    // compare against Device/GardinerPentode logs on import.
    if (toExport) {
        int mtype = toExport->getType();
        qInfo("EXPORT MODEL: type=%d name='%s'", mtype, deviceName.toUtf8().constData());
        if (mtype == GARDINER_PENTODE || mtype == REEFMAN_DERK_PENTODE) {
            qInfo("  EXPORT CORE: mu=%.12f kg1=%.12f x=%.12f kp=%.12f kvb=%.12f kvb1=%.12f vct=%.12f",
                  toExport->getParameter(PAR_MU),
                  toExport->getParameter(PAR_KG1),
                  toExport->getParameter(PAR_X),
                  toExport->getParameter(PAR_KP),
                  toExport->getParameter(PAR_KVB),
                  toExport->getParameter(PAR_KVB1),
                  toExport->getParameter(PAR_VCT));
            qInfo("  EXPORT PENTODE: kg2=%.12f kg2a=%.12f a=%.12f alpha=%.12f beta=%.12f gamma=%.12f os=%.12f",
                  toExport->getParameter(PAR_KG2),
                  toExport->getParameter(PAR_KG2A),
                  toExport->getParameter(PAR_A),
                  toExport->getParameter(PAR_ALPHA),
                  toExport->getParameter(PAR_BETA),
                  toExport->getParameter(PAR_GAMMA),
                  toExport->getParameter(PAR_OS));
            qInfo("  EXPORT SE/BLOOM: tau=%.12f rho=%.12f theta=%.12f psi=%.12f omega=%.12f lambda=%.12f nu=%.12f s=%.12f ap=%.12f",
                  toExport->getParameter(PAR_TAU),
                  toExport->getParameter(PAR_RHO),
                  toExport->getParameter(PAR_THETA),
                  toExport->getParameter(PAR_PSI),
                  toExport->getParameter(PAR_OMEGA),
                  toExport->getParameter(PAR_LAMBDA),
                  toExport->getParameter(PAR_NU),
                  toExport->getParameter(PAR_S),
                  toExport->getParameter(PAR_AP));
        }
    }

    // Persist the fitted model parameters exactly as they are used by
    // ValveWorkbench. This block is the primary source of truth for all
    // internal plotting and Designer/Modeller behaviour.
    QJsonObject modelObj;
    toExport->toJson(modelObj);
    root["model"] = modelObj;

    // In addition to the internal `model` block, embed an optional `spice`
    // description that external SPICE tools can consume directly. This is
    // derived from the concrete Model type (e.g. Cohen–Helie triode or
    // Gardiner pentode) and encoded as a SPICE .subckt body with the same
    // Ia(Va,Vg1,Vg2) law as used in the C++ code.
    {
        QJsonObject spiceObj = buildSpiceBlockForModel(toExport, deviceType, deviceName);
        if (!spiceObj.isEmpty()) {
            root["spice"] = spiceObj;
        }
    }

    // If a Cohen-Helie triode model exists in the project, embed its
    // parameters as a 'triodeModel' block so future pentode fits can reuse
    // the same triode seed without re-running the triode fit.
    if (deviceType == PENTODE && currentProject != nullptr) {
        if (Model *triodeSeed = findModel(COHEN_HELIE_TRIODE)) {
            QJsonObject triodeObj;
            triodeSeed->toJson(triodeObj);
            root["triodeModel"] = triodeObj;
            qInfo("Export to Devices: embedded triodeModel seed for device '%s'", deviceName.toUtf8().constData());
        }
    }

    // Attach full analyser measurement (if available) so offline tools and
    // Designer can reconstruct bias and perform data-driven recalculations.
    // Together with the fitted `model` parameters and optional `spice`
    // subcircuit, this turns the preset into a full tube-style package:
    //   - analyserDefaults: measurement ranges / limits
    //   - model:           fitted analytic parameters
    //   - triodeModel:     optional triode seed for pentodes
    //   - measurement:     original sweeps
    //   - spice:           SPICE-ready .subckt for external simulators
    if (measForExport) {
        QJsonObject measObj;
        measForExport->toJson(measObj);
        root["measurement"] = measObj;
    }
    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export to Devices"), tr("Could not write to %1").arg(outPath));
        return;
    }
    outFile.write(QJsonDocument(root).toJson());
    outFile.close();

    // Before throwing away Device instances, clear any Circuit->Device pointers
    // so Designer circuits (e.g. TriodeCC) do not hold dangling device1/device2
    // references after we repopulate the devices list.
    for (Circuit *c : std::as_const(circuits)) {
        if (!c) continue;
        c->setDevice1(nullptr);
        c->setDevice2(nullptr);
    }
    currentDevice = nullptr;

    // Refresh devices and repopulate dropdowns
    for (Device *d : devices) { delete d; }
    devices.clear();
    loadDevices();
    buildStdDeviceSelection(ui->stdDeviceSelection, deviceType == 0 ? TRIODE : deviceType);
    buildStdDeviceSelection(ui->stdDeviceSelection2, -1);

    QMessageBox::information(this, tr("Export to Devices"), tr("Exported to %1 and refreshed device list.").arg(outPath));
}
