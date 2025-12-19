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
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QStatusBar>
#include <QGraphicsTextItem>
#include <QGraphicsPolygonItem>
#include <QPolygonF>
#include <QLabel>
#include <QLineF>
#include <QGraphicsEllipseItem>
#include <functional>
#include <QLineEdit>
#include <QPen>
#include <QSizePolicy>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

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
#include "valvemodel/circuit/triodecc_dccf_twostage.h"
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

    auto clearHealthRefs = [&]() {
        // Clear Triode A health Ref column
        setField(ui->triodeA_Ia_ref, QString());
        setField(ui->triodeA_rp_ref, QString());
        setField(ui->triodeA_gm_ref, QString());
        setField(ui->triodeA_mu_ref, QString());

        // Clear Triode B health Ref column
        setField(ui->triodeB_Ia_ref, QString());
        setField(ui->triodeB_rp_ref, QString());
        setField(ui->triodeB_gm_ref, QString());
        setField(ui->triodeB_mu_ref, QString());

        // Clear sample-count display in Datasheet / Reference panel
        if (ui->datasheetRefCountValue) {
            ui->datasheetRefCountValue->clear();
            ui->datasheetRefCountValue->setToolTip(QString());
        }
    };

    auto clearAll = [&]() {
        // Datasheet panel
        setField(ui->datasheetVa, QString());
        setField(ui->datasheetVg, QString());
        setField(ui->datasheetVg2, QString());
        setField(ui->datasheetVaRef, QString());
        setField(ui->datasheetVgRef, QString());
        setField(ui->datasheetVg2Ref, QString());
        setField(ui->datasheetIa, QString());
        setField(ui->datasheetGm, QString());
        setField(ui->datasheetMu, QString());
        setField(ui->datasheetRp, QString());

        // Health Ref columns
        clearHealthRefs();
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

    // Datasheet panel values
    const QString vaStr = numToString(rp.value("va"), 1);
    const QString vgStr = numToString(rp.value("vg"), 1);
    const QString vg2Str = numToString(rp.value("vg2"), 1);
    const QString iaStr = numToString(rp.value("ia"), 3);
    const QString gmStr = numToString(rp.value("gm"), 1);
    const QString muStr = numToString(rp.value("mu"), 1);
    const QString rpStr = numToString(rp.value("rp"), 0);

    setField(ui->datasheetVa, vaStr);
    setField(ui->datasheetVg, vgStr);
    setField(ui->datasheetVg2, vg2Str);
    setField(ui->datasheetIa, iaStr);
    setField(ui->datasheetGm, gmStr);
    setField(ui->datasheetMu, muStr);
    setField(ui->datasheetRp, rpStr);

    // Optional reference-tube sample count surfaced as "Ref xN" in the
    // Datasheet / Reference box on the Va row. This is derived from the
    // datasheet.healthReference.sampleCount accumulated by
    // captureHealthReferenceFromLastRun().
    QString refCountText;
    QString refCountTooltip;

    const QJsonObject healthRefObj = datasheetJson.value(QStringLiteral("healthReference")).toObject();
    const QJsonObject centerObj = healthRefObj.value(QStringLiteral("center")).toObject();
    const int sampleCount = healthRefObj.value(QStringLiteral("sampleCount")).toInt(0);
    if (sampleCount > 0) {
        refCountText = tr("x%1").arg(sampleCount);
        refCountTooltip = tr("Reference tube averages are based on %1 saved run(s) for this template.")
                               .arg(sampleCount);
    }

    if (ui->datasheetRefCountValue) {
        ui->datasheetRefCountValue->setText(refCountText);
        ui->datasheetRefCountValue->setToolTip(refCountTooltip);
    }

    // Reference operating point (may differ from datasheet refPoints[0]).
    setField(ui->datasheetVaRef, numToString(centerObj.value(QStringLiteral("va")), 1));
    setField(ui->datasheetVgRef, numToString(centerObj.value(QStringLiteral("vg")), 1));
    setField(ui->datasheetVg2Ref, numToString(centerObj.value(QStringLiteral("vg2")), 1));

    // Mirror reference metrics into the Triode A/B Health Ref columns.
    // Ia is in mA, gm is shown in µS for UI consistency.
    setField(ui->triodeA_Ia_ref, iaStr);
    setField(ui->triodeA_gm_ref, gmStr);
    setField(ui->triodeA_mu_ref, muStr);
    setField(ui->triodeA_rp_ref, rpStr);

    if (deviceType == PENTODE) {
        double ig2Ref = 0.0;
        if (!datasheetJson.isEmpty()) {
            ig2Ref = centerObj.value(QStringLiteral("ig2Ref_mA")).toDouble(0.0);
        }
        const double vg2Ref = rp.value(QStringLiteral("vg2")).toDouble(0.0);
        const double pg2Ref_W = (vg2Ref > 0.0 && ig2Ref > 0.0) ? (vg2Ref * ig2Ref / 1000.0) : 0.0;

        setField(ui->triodeB_Ia_ref, (ig2Ref > 0.0) ? QString::number(ig2Ref, 'f', 3) : QString());
        setField(ui->triodeB_rp_ref, (pg2Ref_W > 0.0) ? QString::number(pg2Ref_W, 'f', 3) : QString());
        setField(ui->triodeB_gm_ref, QString());
        setField(ui->triodeB_mu_ref, QString());
    } else {
        setField(ui->triodeB_Ia_ref, iaStr);
        setField(ui->triodeB_gm_ref, gmStr);
        setField(ui->triodeB_mu_ref, muStr);
        setField(ui->triodeB_rp_ref, rpStr);
    }
}

static double axisIntervalFor(double maxValue)
{
    double interval = 0.5;
    if (maxValue > 5.0)   interval = 1.0;
    if (maxValue > 10.0)  interval = 2.0;
    if (maxValue > 20.0)  interval = 5.0;
    if (maxValue > 50.0)  interval = 10.0;
    if (maxValue > 100.0) interval = 20.0;
    if (maxValue > 200.0) interval = 50.0;
    if (maxValue > 500.0) interval = 100.0;
    return interval;
}

void ValveWorkbench::on_autoscaleModellerPlotButton_clicked()
{
    if (!ui || !currentMeasurement) {
        return;
    }

    const int testType = currentMeasurement->getTestType();

    auto measureBounds = [this](double &vaMaxOut, double &iaMaxOut, double &vg1MinOut, double &vg1MaxOut) {
        vaMaxOut = 0.0;
        iaMaxOut = 0.0;
        vg1MinOut = 0.0;
        vg1MaxOut = 0.0;
        bool haveVg = false;

        const int sweepCount = currentMeasurement->count();
        for (int si = 0; si < sweepCount; ++si) {
            Sweep *sw = currentMeasurement->at(si);
            if (!sw) continue;
            const int sampleCount = sw->count();
            for (int sj = 0; sj < sampleCount; ++sj) {
                Sample *s = sw->at(sj);
                if (!s) continue;

                const double va = s->getVa();
                const double ia = s->getIa();
                const double vg1 = s->getVg1();

                if (std::isfinite(va) && va > vaMaxOut) vaMaxOut = va;
                if (std::isfinite(ia) && ia > iaMaxOut) iaMaxOut = ia;
                if (std::isfinite(vg1)) {
                    if (!haveVg) {
                        vg1MinOut = vg1;
                        vg1MaxOut = vg1;
                        haveVg = true;
                    } else {
                        if (vg1 < vg1MinOut) vg1MinOut = vg1;
                        if (vg1 > vg1MaxOut) vg1MaxOut = vg1;
                    }
                }
            }
        }

        // If there were no samples, fall back to config-derived hints.
        if (!(vaMaxOut > 0.0)) {
            vaMaxOut = currentMeasurement->getAnodeStop();
        }
        if (!(iaMaxOut > 0.0)) {
            iaMaxOut = currentMeasurement->getIaMax();
        }
        if (!haveVg) {
            const double gridStop = currentMeasurement->getGridStop();
            vg1MinOut = (gridStop > 0.0) ? -gridStop : -5.0;
            vg1MaxOut = 0.0;
        }
    };

    double vaMaxObs = 0.0;
    double iaMaxObs = 0.0;
    double vg1MinObs = 0.0;
    double vg1MaxObs = 0.0;
    measureBounds(vaMaxObs, iaMaxObs, vg1MinObs, vg1MaxObs);

    double xStart = 0.0;
    double xStop = 0.0;
    if (testType == ANODE_CHARACTERISTICS) {
        xStart = 0.0;
        // Autorange X to observed Va (but never exceed the configured stop).
        xStop = vaMaxObs;
        const double configured = currentMeasurement->getAnodeStop();
        if (configured > 0.0 && configured < xStop) {
            xStop = configured;
        }
    } else {
        // Transfer / screen plots: autorange grid voltage span (clamped to <= 0V on the right).
        xStart = vg1MinObs;
        xStop = vg1MaxObs;
        if (xStop > 0.0) xStop = 0.0;
        if (xStart > xStop) std::swap(xStart, xStop);
    }
    if (!(xStop > xStart)) {
        xStart = 0.0;
        xStop = 300.0;
    }

    double yStop = iaMaxObs;
    if (std::isfinite(yStop) && yStop > 0.0) {
        yStop *= 1.05;
    }
    if (!(yStop > 0.0)) yStop = 50.0;

    plot.clear();
    plot.setAxes(xStart, xStop, axisIntervalFor(std::fabs(xStop - xStart)),
                 0.0, yStop, axisIntervalFor(yStop), 2, 1);

    if (measuredCurves) {
        plot.remove(measuredCurves);
        measuredCurves = nullptr;
    }
    if (measuredCurvesSecondary) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }
    if (modelledCurves) {
        plot.remove(modelledCurves);
        modelledCurves = nullptr;
    }
    if (modelledCurvesSecondary) {
        plot.remove(modelledCurvesSecondary);
        modelledCurvesSecondary = nullptr;
    }

    currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
    if (currentMeasurement->getDeviceType() == PENTODE) {
        currentMeasurement->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
    }

    if (ui->measureCheck && ui->measureCheck->isChecked()) {
        measuredCurves = currentMeasurement->updatePlotWithoutAxes(&plot);
        if (measuredCurves) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(true);
        }
    }

    if (ui->modelCheck && ui->modelCheck->isChecked()) {
        plotCurrentModelOverMeasurement();
    }
}

void ValveWorkbench::on_fullScale50mAButton_clicked()
{
    if (!ui || !currentMeasurement) {
        return;
    }

    const double xScale = plot.getXScale();
    const double yScale = plot.getYScale();
    const double xStart = plot.getXStart();
    double xStop = xStart;
    if (xScale > 0.0 && yScale > 0.0) {
        xStop = xStart + static_cast<double>(PLOT_WIDTH) / xScale;
    } else {
        // If the plot hasn't been initialized yet, fall back to autoscale.
        on_autoscaleModellerPlotButton_clicked();
        return;
    }

    plot.clear();
    plot.setAxes(xStart, xStop, axisIntervalFor(std::fabs(xStop - xStart)),
                 0.0, 50.0, axisIntervalFor(50.0), 2, 1);

    if (measuredCurves) {
        plot.remove(measuredCurves);
        measuredCurves = nullptr;
    }
    if (measuredCurvesSecondary) {
        plot.remove(measuredCurvesSecondary);
        measuredCurvesSecondary = nullptr;
    }
    if (modelledCurves) {
        plot.remove(modelledCurves);
        modelledCurves = nullptr;
    }
    if (modelledCurvesSecondary) {
        plot.remove(modelledCurvesSecondary);
        modelledCurvesSecondary = nullptr;
    }

    currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
    if (currentMeasurement->getDeviceType() == PENTODE) {
        currentMeasurement->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
    }

    if (ui->measureCheck && ui->measureCheck->isChecked()) {
        measuredCurves = currentMeasurement->updatePlotWithoutAxes(&plot);
        if (measuredCurves) {
            plot.add(measuredCurves);
            measuredCurves->setVisible(true);
        }
    }

    if (ui->modelCheck && ui->modelCheck->isChecked()) {
        plotCurrentModelOverMeasurement();
    }
}

QList<Measurement *> ValveWorkbench::collectModellingTestMeasurements(QTreeWidgetItem *projectItem) const
{
    QList<Measurement *> result;
    if (!projectItem) {
        return result;
    }

    auto isModellingTestsLabel = [&](const QString &label) -> bool {
        const QString t = label.trimmed();
        if (t.isEmpty()) {
            return false;
        }
        if (t.contains(QStringLiteral("Triode-connected"), Qt::CaseInsensitive)) {
            return true;
        }
        if (t.contains(QStringLiteral("Pentode anode"), Qt::CaseInsensitive)) {
            return true;
        }
        if (t.contains(QStringLiteral("Pentode transfer"), Qt::CaseInsensitive)) {
            return true;
        }
        return false;
    };

    const int children = projectItem->childCount();
    for (int i = 0; i < children; ++i) {
        QTreeWidgetItem *child = projectItem->child(i);
        if (!child || child->type() != TYP_MEASUREMENT) {
            continue;
        }
        Measurement *m = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
        if (!m) {
            continue;
        }

        if (m->isTriodeConnectedPentode() || isModellingTestsLabel(m->getCustomLabel())) {
            result.append(m);
        }
    }

    return result;
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
    const QString vg2Text = ui->datasheetVg2 ? ui->datasheetVg2->text().trimmed() : QString();
    const QString iaText = ui->datasheetIa ? ui->datasheetIa->text().trimmed() : QString();
    const QString gmText = ui->datasheetGm ? ui->datasheetGm->text().trimmed() : QString();
    const QString muText = ui->datasheetMu ? ui->datasheetMu->text().trimmed() : QString();
    const QString rpText = ui->datasheetRp ? ui->datasheetRp->text().trimmed() : QString();

    const bool allEmpty = vaText.isEmpty() && vgText.isEmpty() && vg2Text.isEmpty() && iaText.isEmpty() &&
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
    setFromField("vg2", ui->datasheetVg2);
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

    // Datasheet gm is stored in micro-siemens (µS / "micromhos") for UI
    // consistency. Convert to mA/V for internal use so it matches the units
    // returned by computeIaGmAt(), which works in mA/V.
    double gm_uS = 0.0;
    if (!read("gm", gm_uS)) return false;
    gm0 = gm_uS / 1000.0; // 1000 µS = 1 mA/V
    // mu and rp are optional for now; treat missing as 0
    read("mu", mu0);
    read("rp", rp0);

    return true;
}

 bool ValveWorkbench::ensureDatasheetRefPointPentode(double &va0, double &vg0, double &vg20, double &ia0, double &gm0)
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
     if (!read("vg2", vg20)) return false;
     if (!read("ia", ia0)) return false;

     // Datasheet gm is stored in micro-siemens (µS / "micromhos") for UI
     // consistency. Convert to mA/V for internal use so it matches the units
     // returned by computeIaGmAt(), which works in mA/V.
     //
     // For pentodes, allow gm to be omitted from the template so we can still
     // compute Ia-based Health percentages.
     double gm_uS = 0.0;
     if (read("gm", gm_uS)) {
         gm0 = gm_uS / 1000.0; // 1000 µS = 1 mA/V
     } else {
         gm0 = 0.0;
     }

     return true;
 }

 bool ValveWorkbench::findPentodeHealthOperatingPoint(Measurement *measurement, double targetIa_mA, HealthPoint &outPt, double &outIa_mA) const
 {
     outIa_mA = 0.0;
     if (!measurement) {
         return false;
     }
     if (!(targetIa_mA > 0.0)) {
         return false;
     }
     if (measurement->count() <= 0) {
         return false;
     }

     Sweep *sweep = measurement->at(0);
     if (!sweep || sweep->count() < 2) {
         return false;
     }

     double bestVg1 = 0.0;
     double bestIa = 0.0;
     double bestScore = std::numeric_limits<double>::infinity();
     bool have = false;

     for (int i = 1; i < sweep->count(); ++i) {
         Sample *s0 = sweep->at(i - 1);
         Sample *s1 = sweep->at(i);
         if (!s0 || !s1) continue;

         const double ia0 = s0->getIa();
         const double ia1 = s1->getIa();
         const double vg0 = s0->getVg1();
         const double vg1 = s1->getVg1();

         if (!std::isfinite(ia0) || !std::isfinite(ia1) || !std::isfinite(vg0) || !std::isfinite(vg1)) {
             continue;
         }
         if (ia0 < 0.0 || ia1 < 0.0) {
             continue;
         }
         if (std::fabs(vg1 - vg0) < 1e-9) {
             continue;
         }

         const double lo = std::min(ia0, ia1);
         const double hi = std::max(ia0, ia1);
         if (targetIa_mA < lo || targetIa_mA > hi) {
             continue;
         }

         const double slope = (ia1 - ia0) / (vg1 - vg0);
         if (!(slope > 0.0)) {
             continue;
         }

         const double t = (targetIa_mA - ia0) / (ia1 - ia0);
         const double vgInterp = vg0 + t * (vg1 - vg0);
         const double iaInterp = ia0 + t * (ia1 - ia0);
         const double score = std::fabs(iaInterp - targetIa_mA);

         if (score < bestScore) {
             bestScore = score;
             bestVg1 = vgInterp;
             bestIa = iaInterp;
             have = true;
         }
     }

     if (!have) {
         return false;
     }

     Sample *sAny = sweep->at(0);
     if (!sAny) {
         return false;
     }

     outPt.va = sAny->getVa();
     outPt.vg2 = sAny->getVg2();
     outPt.vg = bestVg1;
     outIa_mA = bestIa;
     return true;
 }

 void ValveWorkbench::startHealthRun(HealthMode mode)
{
    double va0 = 0.0;
    double vg0 = 0.0;
    double vg20 = 0.0;
    double ia0 = 0.0;
    double gm0 = 0.0;
    double mu0 = 0.0;
    double rp0 = 0.0;

    // Reset OP-finder state for each new Health run.
    healthOpFinderActive = false;
    healthOpTargetIa_mA = 0.0;

    // Save current analyser UI state so it can be restored after Health.
    // (Must happen before any Health-specific parameter overrides.)
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

    if (!healthRunActive && !healthPrereqAnodeSweepActive) {
        healthPrereqAnodeMeasurement = nullptr;
    }

    Measurement *anodeCandidate = nullptr;
    if (currentProject) {
        anodeCandidate = findMeasurement(deviceType, ANODE_CHARACTERISTICS);
    }
    if (!anodeCandidate) {
        anodeCandidate = healthPrereqAnodeMeasurement;
    }

    const bool haveAnodeSweep =
        (anodeCandidate != nullptr &&
         anodeCandidate->getDeviceType() == deviceType &&
         anodeCandidate->getTestType() == ANODE_CHARACTERISTICS &&
         measurementHasValidSamples(anodeCandidate));

    if (!haveAnodeSweep) {
        if (!analyserTestsDefaults.isEmpty()) {
            QJsonObject snapshot;
            for (auto it = analyserTestsDefaults.begin(); it != analyserTestsDefaults.end(); ++it) {
                if (!it.value().isObject()) continue;
                const QJsonObject tObj = it.value().toObject();
                const int tType = tObj.value(QStringLiteral("testType")).toInt(-1);
                if (tType == ANODE_CHARACTERISTICS) {
                    snapshot = tObj;
                    break;
                }
            }

            if (!snapshot.isEmpty()) {
                auto setRangeFrom = [&](const char *key, double &start, double &stop, double &step) {
                    const QJsonObject r = snapshot.value(QLatin1String(key)).toObject();
                    if (!r.isEmpty()) {
                        start = r.value(QStringLiteral("start")).toDouble(start);
                        stop  = r.value(QStringLiteral("stop")).toDouble(stop);
                        step  = r.value(QStringLiteral("step")).toDouble(step);
                    }
                };
                setRangeFrom("anode",  anodeStart,  anodeStop,  anodeStep);
                setRangeFrom("grid",   gridStart,   gridStop,   gridStep);
                setRangeFrom("screen", screenStart, screenStop, screenStep);

                // The analyser's sweep generator expects grid values to be
                // positive magnitudes ("-ve grid"), and will internally apply
                // sign when storing/plotting samples. Some saved defaults store
                // true negative grid volts; normalise to magnitudes here.
                gridStart = std::fabs(gridStart);
                gridStop  = std::fabs(gridStop);
                gridStep  = std::fabs(gridStep);
                if (!(gridStep > 0.0)) {
                    gridStep = 1.0;
                }

                const QJsonObject lim2 = snapshot.value(QStringLiteral("limits")).toObject();
                if (!lim2.isEmpty()) {
                    iaMax = lim2.value(QStringLiteral("iaMax")).toDouble(iaMax);
                    pMax  = lim2.value(QStringLiteral("pMax")).toDouble(pMax);
                }
            }
        }

        if (deviceType == PENTODE) {
            // Avoid running pentode anode characteristics starting at Va=0 when Vg2 is high.
            // That condition can immediately spike Ig2 / trip clamps and abort the run.
            if (anodeStart < 20.0) {
                anodeStart = 20.0;
            }
            if (anodeStop < anodeStart) {
                anodeStop = anodeStart;
            }
            if (!(anodeStep > 0.0)) {
                anodeStep = 5.0;
            }
        }

        testType = ANODE_CHARACTERISTICS;
        healthMode = mode;
        healthRunActive = true;
        healthRunIndex = 0;
        healthPrereqAnodeSweepActive = true;
        on_runButton_clicked();
        return;
    }

    if (deviceType == TRIODE) {
        if (!ensureDatasheetRefPoint(va0, vg0, ia0, gm0, mu0, rp0)) {
            QMessageBox::warning(this, tr("Health Test"), tr("No valid datasheet reference point is available. Load a template with datasheet.refPoints[0] first."));
            return;
        }

        // Full Health uses a middle-swing operating point instead of the datasheet point.
        // Find a Vg1 that yields a target Ia at the datasheet Va.
        if (mode == HEALTH_FULL) {
            healthOpFinderActive = true;
            const double hwIaLimit_mA = 50.0;
            const double targetIa = 0.5 * std::min(iaMax, hwIaLimit_mA);
            healthOpTargetIa_mA = std::max(1.0, targetIa);

            testType = TRANSFER_CHARACTERISTICS;
            anodeStart = va0;
            anodeStop  = va0;
            anodeStep  = std::max(1.0, std::fabs(va0));

            screenStart = 0.0;
            screenStop  = 0.0;
            screenStep  = 0.0;

            // Sweep Vg1 from very negative (large magnitude) towards less negative.
            // Use the datasheet Vg0 to size the sweep, but always ensure a wide enough range.
            const double vgStopMag = std::max(20.0, std::fabs(vg0) + 10.0);
            gridStart = 1.0;        // finish near -1V
            gridStop  = vgStopMag;  // start near -(vgStopMag)V
            gridStep  = 0.5;

            healthMode = mode;
            healthRunActive = true;
            healthRunIndex = 0;
            on_runButton_clicked();
            return;
        }
    } else if (deviceType == PENTODE) {
        if (mode == HEALTH_QUICK) {
            if (!ensureDatasheetRefPointPentode(va0, vg0, vg20, ia0, gm0)) {
                QMessageBox::warning(this, tr("Health Test"), tr("No valid datasheet reference point is available. Load a template with datasheet.refPoints[0] first."));
                return;
            }
        } else {
            // For pentodes, avoid datasheet operating points (often exceed analyser limits).
            // Instead, find a safe operating point at a conservative Va/Vg2 by running a short
            // transfer sweep and selecting the Vg1 that yields a target Ia.
            healthOpFinderActive = true;
            // Target is half of the smaller of the configured Ia limit and the hardware limit.
            // The analyser hardware clamps measured Ia to 50 mA.
            const double hwIaLimit_mA = 50.0;
            const double targetIa = 0.5 * std::min(iaMax, hwIaLimit_mA);
            healthOpTargetIa_mA = std::max(1.0, targetIa);

            // Conservative rails for OP finder.
            testType = TRANSFER_CHARACTERISTICS;
            anodeStart = 120.0;
            anodeStop  = 120.0;
            anodeStep  = 120.0;

            screenStart = 120.0;
            screenStop  = 120.0;
            screenStep  = 120.0;

            // Sweep Vg1 from very negative (large magnitude) towards less negative.
            gridStart = 2.0;   // finish near -2V
            gridStop  = 35.0;  // start near -35V
            gridStep  = 1.0;

            healthMode = mode;
            healthRunActive = true;
            healthRunIndex = 0;
            on_runButton_clicked();
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Health Test"), tr("Quick/Full Health currently support triode and pentode devices only."));
        return;
    }

    // Build health test points around the datasheet operating point.
    healthPoints.clear();
    healthResults.clear();

    if (isDoubleTriode && triodeMeasurementSecondary != nullptr) {
        deleteMeasurementClone(triodeMeasurementSecondary);
        triodeMeasurementSecondary = nullptr;
    }

    HealthPoint center;
    center.va = va0;
    center.vg = vg0;
    center.vg2 = vg20;
    healthPoints.append(center);

    if (mode == HEALTH_QUICK) {
        const double dVaFracQuick = 0.10;
        double dVa = std::fabs(va0) * dVaFracQuick;
        if (dVa < 20.0) dVa = 20.0;
        if (dVa > 60.0) dVa = 60.0;

        double dVg = 0.3;
        if (std::fabs(vg0) > 2.0) {
            dVg = 0.5;
        }

        const double vaLow  = std::max(0.0, va0 - dVa);
        const double vaHigh = va0 + dVa;
        const double vgLo   = vg0 - dVg;
        const double vgHi   = vg0 + dVg;

        HealthPoint p;
        p.va = va0;    p.vg = vgLo;   p.vg2 = vg20;  healthPoints.append(p);
        p.va = va0;    p.vg = vgHi;   p.vg2 = vg20;  healthPoints.append(p);

        p.va = vaLow;  p.vg = vg0;    p.vg2 = vg20;  healthPoints.append(p);
        p.va = vaLow;  p.vg = vgLo;   p.vg2 = vg20;  healthPoints.append(p);
        p.va = vaLow;  p.vg = vgHi;   p.vg2 = vg20;  healthPoints.append(p);

        p.va = vaHigh; p.vg = vg0;    p.vg2 = vg20;  healthPoints.append(p);
        p.va = vaHigh; p.vg = vgLo;   p.vg2 = vg20;  healthPoints.append(p);
        p.va = vaHigh; p.vg = vgHi;   p.vg2 = vg20;  healthPoints.append(p);
    } else if (mode == HEALTH_FULL) {
        const int clusterSize = 3;
        bool usedStoredCorners = false;

        if (!datasheetJson.isEmpty()) {
            const QJsonObject healthRefObj = datasheetJson.value("healthReference").toObject();
            const QJsonArray cornersArr = healthRefObj.value("corners").toArray();

            if (cornersArr.size() >= 4) {
                HealthPoint p;
                for (int c = 0; c < 4; ++c) {
                    const QJsonObject cObj = cornersArr.at(c).toObject();
                    const double vaCorner = cObj.value("va").toDouble();
                    const double vgCorner = cObj.value("vg").toDouble();

                    double dVgCorner = 0.3;
                    if (std::fabs(vgCorner) > 2.0) {
                        dVgCorner = 0.5;
                    }

                    p.va = vaCorner; p.vg = vgCorner;              p.vg2 = vg20;  healthPoints.append(p);
                    p.va = vaCorner; p.vg = vgCorner - dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                    p.va = vaCorner; p.vg = vgCorner + dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                }

                if (healthPoints.size() >= 1 + 4 * clusterSize) {
                    usedStoredCorners = true;
                } else {
                    healthPoints.resize(1);
                }
            }
        }

        if (!usedStoredCorners) {
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

            const double vaCorners[4] = { vaLow, vaLow, vaHigh, vaHigh };
            const double vgCorners[4] = { vgLo,  vgHi,  vgLo,   vgHi  };

            HealthPoint p;
            for (int c = 0; c < 4; ++c) {
                const double vaCorner = vaCorners[c];
                const double vgCorner = vgCorners[c];

                double dVgCorner = 0.3;
                if (std::fabs(vgCorner) > 2.0) {
                    dVgCorner = 0.5;
                }

                p.va = vaCorner; p.vg = vgCorner;              p.vg2 = vg20;  healthPoints.append(p);
                p.va = vaCorner; p.vg = vgCorner - dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                p.va = vaCorner; p.vg = vgCorner + dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
            }
        }
    }

    healthResults.resize(healthPoints.size());
    for (int i = 0; i < healthResults.size(); ++i) {
        healthResults[i].valid = false;
        healthResults[i].va = 0.0;
        healthResults[i].vg = 0.0;
        healthResults[i].vg2 = 0.0;
        healthResults[i].ia = 0.0;
        healthResults[i].gm = 0.0;
        healthResults[i].rp = 0.0;
        healthResults[i].ig2 = 0.0;
    }

    healthMode = mode;
    healthRunActive = true;
    healthRunIndex = 0;

    configureTransferForHealthPoint(healthPoints[0]);
    on_runButton_clicked();
}

void ValveWorkbench::configureTransferForHealthPoint(const HealthPoint &pt)
{
    testType = TRANSFER_CHARACTERISTICS;

    anodeStart = pt.va;
    anodeStop  = pt.va;
    anodeStep  = std::max(1.0, std::fabs(pt.va));

    const double vgMag = std::fabs(pt.vg);

    const double triodeHalfSpan = 0.3;
    const double pentodeLoSpan  = 1.0;
    const double pentodeHiSpan  = 8.0;

    double startMag = 0.0;
    double stopMag  = 0.0;
    if (deviceType == PENTODE) {
        startMag = (vgMag > pentodeLoSpan) ? (vgMag - pentodeLoSpan) : 0.0;
        stopMag  = vgMag + pentodeHiSpan;
    } else {
        startMag = (vgMag > triodeHalfSpan) ? (vgMag - triodeHalfSpan) : 0.0;
        stopMag  = vgMag + triodeHalfSpan;
    }

    if (stopMag <= startMag) {
        stopMag = startMag + 0.5;
    }

    gridStart = startMag;
    gridStop  = stopMag;

    if (healthRunActive) {
        gridStep = 0.2;
    }

    if (deviceType == PENTODE) {
        screenStart = pt.vg2;
        screenStop  = pt.vg2;
        screenStep  = std::max(1.0, std::fabs(pt.vg2));
    } else {
        screenStart = 0.0;
        screenStop  = 0.0;
        screenStep  = 0.0;
    }
}

bool ValveWorkbench::computeIaGmAt(Measurement *measurement,
                                  const HealthPoint &pt,
                                  double &ia_mA,
                                  double &gm_mA_V,
                                  double &rp_ohms,
                                  double *ig2_mA)
{
    ia_mA = 0.0;
    gm_mA_V = 0.0;
    rp_ohms = 0.0;
    if (ig2_mA) {
        *ig2_mA = 0.0;
    }

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

    const bool isPentodeHealth = (deviceType == PENTODE);
    for (int sw = 0; sw < sweepCount; ++sw) {
        Sweep *sweep = measurement->at(sw);
        if (!sweep) {
            continue;
        }
        const int nSamples = sweep->count();
        for (int sa = 0; sa < nSamples; ++sa) {
            Sample *sample = sweep->at(sa);
            if (!sample) {
                continue;
            }

            const double va = sample->getVa();
            const double vg = sample->getVg1();
            const double vg2 = sample->getVg2();

            const double dVa = va - pt.va;
            const double dVg = vg - pt.vg;
            const double dVg2 = isPentodeHealth ? (vg2 - pt.vg2) : 0.0;

            const double score = dVg * dVg + 0.25 * dVa * dVa + 0.25 * dVg2 * dVg2;
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
    Sample *bestSample = bestSweep->at(bestSampleIdx);
    if (!bestSample) {
        return false;
    }

    ia_mA = bestSample->getIa();
    if (!std::isfinite(ia_mA)) {
        ia_mA = 0.0;
        return false;
    }

    if (ig2_mA) {
        const double ig2 = bestSample->getIg2();
        if (std::isfinite(ig2) && ig2 >= 0.0) {
            *ig2_mA = ig2;
        }
    }

    auto slopeBetween = [](Sample *a, Sample *b, double &gmOut) -> bool {
        if (!a || !b) return false;
        const double ia0 = a->getIa();
        const double ia1 = b->getIa();
        const double vg0 = a->getVg1();
        const double vg1 = b->getVg1();
        const double dVg = vg1 - vg0;
        if (!std::isfinite(ia0) || !std::isfinite(ia1) || !std::isfinite(dVg)) return false;
        if (std::fabs(dVg) < 1e-9) return false;
        gmOut = (ia1 - ia0) / dVg;
        return std::isfinite(gmOut);
    };

    const int n = bestSweep->count();
    double gmTmp = 0.0;
    bool haveGm = false;
    if (bestSampleIdx > 0 && bestSampleIdx + 1 < n) {
        haveGm = slopeBetween(bestSweep->at(bestSampleIdx - 1), bestSweep->at(bestSampleIdx + 1), gmTmp);
    }
    if (!haveGm && bestSampleIdx > 0) {
        haveGm = slopeBetween(bestSweep->at(bestSampleIdx - 1), bestSweep->at(bestSampleIdx), gmTmp);
    }
    if (!haveGm && bestSampleIdx + 1 < n) {
        haveGm = slopeBetween(bestSweep->at(bestSampleIdx), bestSweep->at(bestSampleIdx + 1), gmTmp);
    }

    if (haveGm) {
        gm_mA_V = gmTmp;
        if (!std::isfinite(gm_mA_V) || gm_mA_V < 0.0) {
            gm_mA_V = 0.0;
        }
    }

    auto estimateRpFromAnode = [&](Measurement *anodeMeasurement, double &rpOut) -> bool {
        if (!anodeMeasurement || anodeMeasurement->getTestType() != ANODE_CHARACTERISTICS) {
            qInfo("Health rp: no valid anode measurement for rp (ptr=%p, testType=%d)",
                  static_cast<void *>(anodeMeasurement),
                  anodeMeasurement ? anodeMeasurement->getTestType() : -1);
            return false;
        }

        const int sweepCount = anodeMeasurement->count();
        if (sweepCount <= 0) {
            qInfo("Health rp: anode measurement has no sweeps (ptr=%p)", static_cast<void *>(anodeMeasurement));
            return false;
        }

        const bool isPentode = (deviceType == PENTODE);
        double bestScore = std::numeric_limits<double>::infinity();
        int bestSweep = -1;
        int skippedTooSmall = 0;
        for (int sw = 0; sw < sweepCount; ++sw) {
            Sweep *sweep = anodeMeasurement->at(sw);
            if (!sweep || sweep->count() < 3) {
                if (sweep) {
                    ++skippedTooSmall;
                }
                continue;
            }

            double vgNom = sweep->getVg1Nominal();
            if (!std::isfinite(vgNom)) {
                Sample *s0 = sweep->at(0);
                if (s0) {
                    vgNom = s0->getVg1();
                }
            }
            if (!std::isfinite(vgNom)) {
                continue;
            }

            // Historical data can store Vg1Nominal as a positive magnitude (e.g. 10 means -10V
            // physically), while HealthPoint.vg uses the physical sign (typically negative).
            // Choose the sign (+/-) for the nominal that best matches the target point.
            double vgNomPhysical = vgNom;
            if (vgNom > 0.0 && std::fabs(pt.vg) > 1e-9) {
                const double candPos = vgNom;
                const double candNeg = -vgNom;
                vgNomPhysical = (std::fabs(candNeg - pt.vg) < std::fabs(candPos - pt.vg)) ? candNeg : candPos;
            }

            double vg2Nom = 0.0;
            if (isPentode) {
                Sample *s0 = sweep->at(0);
                if (s0) {
                    vg2Nom = s0->getVg2();
                }
            }

            const double dVg = vgNomPhysical - pt.vg;
            const double dVg2 = isPentode ? (vg2Nom - pt.vg2) : 0.0;
            const double score = dVg * dVg + 0.25 * dVg2 * dVg2;
            if (score < bestScore) {
                bestScore = score;
                bestSweep = sw;
            }
        }

        if (bestSweep < 0) {
            qInfo("Health rp: could not select sweep family (sweeps=%d, skippedTooSmall=%d, target Va=%.3f Vg=%.3f Vg2=%.3f)",
                  sweepCount,
                  skippedTooSmall,
                  pt.va,
                  pt.vg,
                  pt.vg2);
            return false;
        }

        Sweep *sweep = anodeMeasurement->at(bestSweep);
        if (!sweep || sweep->count() < 3) {
            qInfo("Health rp: selected sweep invalid/too small (idx=%d, count=%d)",
                  bestSweep,
                  sweep ? sweep->count() : 0);
            return false;
        }

        qInfo("Health rp: using sweep %d (samples=%d) for target Va=%.3f Vg=%.3f Vg2=%.3f", 
              bestSweep,
              sweep->count(),
              pt.va,
              pt.vg,
              pt.vg2);

        int bestIdx = -1;
        double bestVaDiff = std::numeric_limits<double>::infinity();
        const int n = sweep->count();
        for (int i = 0; i < n; ++i) {
            Sample *s = sweep->at(i);
            if (!s) {
                continue;
            }
            const double va = s->getVa();
            if (!std::isfinite(va)) {
                continue;
            }
            const double diff = std::fabs(va - pt.va);
            if (diff < bestVaDiff) {
                bestVaDiff = diff;
                bestIdx = i;
            }
        }

        if (bestIdx < 0) {
            qInfo("Health rp: could not find Va match in sweep (target Va=%.3f)", pt.va);
            return false;
        }

        const int iStart = std::max(0, bestIdx - 2);
        const int iEnd = std::min(n - 1, bestIdx + 2);

        double Sx = 0.0;
        double Sy = 0.0;
        double Sxx = 0.0;
        double Sxy = 0.0;
        int N = 0;

        for (int i = iStart; i <= iEnd; ++i) {
            Sample *s = sweep->at(i);
            if (!s) {
                continue;
            }
            const double va = s->getVa();
            const double ia = s->getIa();
            if (!std::isfinite(va) || !std::isfinite(ia) || !(ia > 0.0)) {
                continue;
            }
            Sx += va;
            Sy += ia;
            Sxx += va * va;
            Sxy += va * ia;
            ++N;
        }

        double slope_dIa_dVa = 0.0;
        const double den = static_cast<double>(N) * Sxx - Sx * Sx;
        if (N >= 3 && std::fabs(den) > 1e-12) {
            slope_dIa_dVa = (static_cast<double>(N) * Sxy - Sx * Sy) / den; // mA/V
        }

        if (!(slope_dIa_dVa > 0.0) || !std::isfinite(slope_dIa_dVa)) {
            Sample *sPrev = sweep->at(std::max(0, bestIdx - 1));
            Sample *sNext = sweep->at(std::min(n - 1, bestIdx + 1));
            if (sPrev && sNext) {
                const double vaPrev = sPrev->getVa();
                const double iaPrev = sPrev->getIa();
                const double vaNext = sNext->getVa();
                const double iaNext = sNext->getIa();
                const double dVa = vaNext - vaPrev;
                const double dIa = iaNext - iaPrev;
                if (std::fabs(dVa) > 1e-9 && std::isfinite(dIa)) {
                    slope_dIa_dVa = dIa / dVa;
                }
            }
        }

        if (!(slope_dIa_dVa > 0.0) || !std::isfinite(slope_dIa_dVa)) {
            qInfo("Health rp: slope invalid (N=%d den=%.6g slope=%.6g) around idx=%d VaTarget=%.3f", 
                  N,
                  den,
                  slope_dIa_dVa,
                  bestIdx,
                  pt.va);
            return false;
        }

        rpOut = 1000.0 / slope_dIa_dVa; // (V/mA)*1000 = ohms

        qInfo("Health rp: slope dIa/dVa=%.6f mA/V => rp=%.3f ohms", slope_dIa_dVa, rpOut);
        return (std::isfinite(rpOut) && rpOut > 0.0);
    };

    if (rp_ohms <= 0.0) {
        Measurement *anodeMeasurement = measurement;
        if (measurement->getTestType() != ANODE_CHARACTERISTICS) {
            Measurement *candidate = findMeasurement(deviceType, ANODE_CHARACTERISTICS);
            if (candidate && measurementHasValidSamples(candidate)) {
                anodeMeasurement = candidate;
            }
            if (anodeMeasurement == measurement) {
                Measurement *cached = healthPrereqAnodeMeasurement;
                if (cached &&
                    cached->getDeviceType() == deviceType &&
                    cached->getTestType() == ANODE_CHARACTERISTICS &&
                    measurementHasValidSamples(cached)) {
                    anodeMeasurement = cached;
                }
            }
        }

        qInfo("Health rp: resolve anodeMeasurement for rp (current testType=%d, chosen ptr=%p, chosen testType=%d, sweeps=%d)",
              measurement ? measurement->getTestType() : -1,
              static_cast<void *>(anodeMeasurement),
              anodeMeasurement ? anodeMeasurement->getTestType() : -1,
              anodeMeasurement ? anodeMeasurement->count() : -1);

        double rpTmp = 0.0;
        if (estimateRpFromAnode(anodeMeasurement, rpTmp)) {
            rp_ohms = rpTmp;
        } else {
            qInfo("Health rp: estimateRpFromAnode FAILED; leaving rp=0");
        }
    }

    return true;
}

void ValveWorkbench::finalizeHealthRun()
{
    double va0 = 0.0;
    double vg0 = 0.0;
    double vg20 = 0.0;
    double ia0 = 0.0;
    double gm0 = 0.0;
    double mu0 = 0.0;
    double rp0 = 0.0;

    bool haveRef = false;
    if (deviceType == PENTODE) {
        haveRef = ensureDatasheetRefPointPentode(va0, vg0, vg20, ia0, gm0);

        // For pentodes, allow optional mu/rp reference values to be provided
        // in datasheet.refPoints[0] so the Health percent columns can be
        // populated when templates include them.
        if (haveRef && !datasheetJson.isEmpty()) {
            const QJsonArray refPoints = datasheetJson.value("refPoints").toArray();
            if (!refPoints.isEmpty() && refPoints.at(0).isObject()) {
                const QJsonObject rpObj = refPoints.at(0).toObject();
                const QJsonValue muVal = rpObj.value("mu");
                const QJsonValue rpVal = rpObj.value("rp");
                if (muVal.isDouble()) {
                    mu0 = muVal.toDouble(0.0);
                }
                if (rpVal.isDouble()) {
                    rp0 = rpVal.toDouble(0.0);
                }
            }
        }
    } else {
        haveRef = ensureDatasheetRefPoint(va0, vg0, ia0, gm0, mu0, rp0);
    }

    // Health metric: treat the score as the complement of fractional
    // deviation from the reference. A tube at 120% of spec (ratio 1.2)
    // has |1.2-1| = 0.2 deviation, so health = 1 - 0.2 = 0.8 (80%).
    // Likewise a tube at 80% of spec also scores 80%. Values at or
    // beyond 0% or 200% of spec collapse to 0% health.
    auto metricScore = [](double meas, double ref) -> double {
        if (!(ref > 0.0) || !(meas > 0.0)) {
            return 0.0;
        }
        const double ratio = meas / ref;
        const double dev   = std::fabs(ratio - 1.0);
        double s = 1.0 - dev;
        if (s < 0.0) s = 0.0;
        if (s > 1.0) s = 1.0;
        return s;
    };

    double quickPercent = 0.0;
    double fullPercent  = 0.0;

    // Optional reference-tube-based health scores (centre and 4-corner Ia)
    // derived from datasheet.healthReference.center/corners. These are
    // surfaced on the Ref: labels under the Quick/Full Health buttons.
    double quickPercentRef = 0.0;
    double fullPercentRef  = 0.0;

    // Optional reference surface for Full Health is currently triode-only.
    // For pentodes we only score against the datasheet operating point.
    Measurement *refMeasurementForHealth = nullptr;
    if (currentDevice) {
        Measurement *m = currentDevice->getMeasurement();
        if (m && measurementHasValidSamples(m)) {
            refMeasurementForHealth = m;
        }
    }

    QVector<double> iaRefSurface;
    QVector<double> gmRefSurface;
    QVector<double> ig2RefSurface;
    QVector<double> iaRefSurfaceB; // per-triode reference for Triode B (B-specific only)
    bool haveHealthReference = false;

    if (!healthPoints.isEmpty()) {
        const int n = healthPoints.size();
        iaRefSurface.resize(n);
        gmRefSurface.resize(n);
        ig2RefSurface.resize(n);
        iaRefSurfaceB.resize(n);
        for (int i = 0; i < n; ++i) {
            iaRefSurface[i]  = 0.0;
            gmRefSurface[i]  = 0.0;
            ig2RefSurface[i] = 0.0;
            iaRefSurfaceB[i] = 0.0;
        }
    }

    if (refMeasurementForHealth && !healthPoints.isEmpty()) {
        const int n = healthPoints.size();
        for (int i = 0; i < n; ++i) {
            double iaRef = 0.0;
            double gmRef = 0.0;
            double rpRef = 0.0;
            double ig2Ref = 0.0;
            if (computeIaGmAt(refMeasurementForHealth,
                              healthPoints.at(i),
                              iaRef,
                              gmRef,
                              rpRef,
                              &ig2Ref)) {
                iaRefSurface[i] = iaRef;
                gmRefSurface[i] = gmRef;
                ig2RefSurface[i] = ig2Ref;
            }
        }
    }

    if (!healthPoints.isEmpty() && !datasheetJson.isEmpty()) {
        const QJsonObject healthRefObj = datasheetJson.value("healthReference").toObject();
        const QJsonArray cornersArr = healthRefObj.value("corners").toArray();
        const int clusterSize = 3;

        if (cornersArr.size() >= 4 && iaRefSurface.size() >= 1 + clusterSize * 4) {
            for (int corner = 0; corner < 4; ++corner) {
                const QJsonObject cObj = cornersArr.at(corner).toObject();

                // Legacy single reference current (pre per-triode support).
                const double iaRefLegacy = cObj.value("iaRef_mA").toDouble(0.0);
                // Optional per-triode reference currents.
                const double iaRefAStored = cObj.value("iaRefA_mA").toDouble(0.0);
                const double iaRefBStored = cObj.value("iaRefB_mA").toDouble(0.0);

                const double ig2RefStored = cObj.value("ig2Ref_mA").toDouble(0.0);

                // Triode A reference: prefer iaRefA_mA, fall back to legacy.
                const double iaRefA = (iaRefAStored > 0.0) ? iaRefAStored : iaRefLegacy;
                // Triode B reference: only use an explicit iaRefB_mA value. Do
                // NOT silently substitute Triode A's reference here so that B
                // scoring can distinguish between true B-specific references
                // and corners that have never been calibrated for Triode B.
                const double iaRefB = iaRefBStored;

                if (!(iaRefA > 0.0) && !(iaRefB > 0.0)) {
                    continue;
                }

                const int baseIdx = 1 + corner * clusterSize;
                for (int k = 0; k < clusterSize; ++k) {
                    const int idx = baseIdx + k;
                    if (idx >= iaRefSurface.size()) {
                        break;
                    }
                    if (iaRefA > 0.0) {
                        iaRefSurface[idx] = iaRefA;
                    }
                    if (ig2RefStored > 0.0 && idx < ig2RefSurface.size()) {
                        ig2RefSurface[idx] = ig2RefStored;
                    }
                    if (iaRefB > 0.0 && idx < iaRefSurfaceB.size()) {
                        iaRefSurfaceB[idx] = iaRefB;
                    }
                }
            }
            haveHealthReference = true;
        }
    }

    const bool haveHealthSurface = (!healthPoints.isEmpty() &&
                                    (refMeasurementForHealth != nullptr || haveHealthReference));

    // Centre reference Ia used for per-triode corner scaling. Prefer the
    // embedded reference surface at the central HealthPoint when available,
    // otherwise fall back to the datasheet Ia0.
    double iaRefCentreForOffsets = 0.0;
    if (!healthPoints.isEmpty()) {
        if (haveHealthSurface && iaRefSurface.size() > 0 && iaRefSurface.at(0) > 0.0) {
            iaRefCentreForOffsets = iaRefSurface.at(0);
        } else if (haveRef && ia0 > 0.0) {
            iaRefCentreForOffsets = ia0;
        }
    }

    // Robust cluster estimator for up to three nearby values (Quick Health
    // centre row). Uses a median-based outlier gate and averages the
    // survivors. Returns true and writes 'effective' on success.
    auto robust3xCluster = [](const QVector<double> &values, double outlierTol, double &effective) -> bool {
        if (values.isEmpty()) {
            return false;
        }

        QVector<double> sorted = values;
        std::sort(sorted.begin(), sorted.end());

        double median = 0.0;
        if (sorted.size() == 1) {
            median = sorted[0];
        } else if (sorted.size() == 2) {
            median = 0.5 * (sorted[0] + sorted[1]);
        } else {
            median = sorted[1];
        }

        if (!(median > 0.0)) {
            return false;
        }

        double sum = 0.0;
        int count = 0;
        for (double v : values) {
            if (!(v > 0.0)) {
                continue;
            }
            const double ratio = v / median;
            const double dev = std::fabs(ratio - 1.0);
            if (dev > outlierTol) {
                continue;
            }
            sum += v;
            ++count;
        }

        if (count > 0) {
            effective = sum / static_cast<double>(count);
        } else {
            effective = median;
        }

        return (effective > 0.0);
    };

    const double outlierTol = 0.40; // 40% deviation from median

    // Per-triode centre-based scaling factors applied to the reference Ia
    // at each corner during Full Health 4-corner scoring. This makes the
    // corner test primarily a shape check (pattern of Ia across corners)
    // while Quick Health remains responsible for catching large centre
    // Ia/gm differences.
    double iaScaleAForCorners = 1.0;
    double iaScaleBForCorners = 1.0;

    // Robust Ia/gm for Triode A Quick Health: use the three centre-row
    // points (same Va, Vg offsets) to stabilise both Ia and gm.
    double iaQuickEffective = 0.0;
    double gmQuickEffective = 0.0;
    bool haveIaQuickEffective = false;
    bool haveGmQuickEffective = false;

    if (healthMode == HEALTH_QUICK && healthResults.size() >= 3) {
        QVector<double> iaCandidates;
        QVector<double> gmCandidates;
        iaCandidates.reserve(3);
        gmCandidates.reserve(3);

        for (int i = 0; i < 3; ++i) {
            const HealthResult &hr = healthResults.at(i);
            if (!hr.valid) {
                continue;
            }
            if (hr.ia > 0.0) {
                iaCandidates.append(hr.ia);
            }
            if (hr.gm > 0.0) {
                gmCandidates.append(hr.gm);
            }
        }

        if (!iaCandidates.isEmpty()) {
            haveIaQuickEffective = robust3xCluster(iaCandidates, outlierTol, iaQuickEffective);
        }
        if (gmCandidates.size() >= 2) {
            haveGmQuickEffective = robust3xCluster(gmCandidates, outlierTol, gmQuickEffective);
        }
    }

    if (healthResults.size() > 0 && haveRef) {
        const HealthResult &r0 = healthResults.at(0);
        if (r0.valid) {
            const double iaForQuick = haveIaQuickEffective ? iaQuickEffective : r0.ia;
            const double gmForQuick = haveGmQuickEffective ? gmQuickEffective : r0.gm;
            const double sIa = metricScore(iaForQuick, ia0);
            if (gm0 > 0.0) {
                const double sGm = metricScore(gmForQuick, gm0);
                quickPercent = 100.0 * 0.5 * (sIa + sGm);
                qInfo("Health Quick: Ia_meas=%.6f mA, Ia_ref=%.6f mA, sIa=%.3f, gm_meas=%.6f mA/V, gm_ref=%.6f mA/V, sGm=%.3f, quickPercent=%.1f%%",
                      iaForQuick,
                      ia0,
                      sIa,
                      gmForQuick,
                      gm0,
                      sGm,
                      quickPercent);
            } else {
                // gm reference missing: score Quick Health on Ia only
                quickPercent = 100.0 * sIa;
                qInfo("Health Quick (Ia-only): Ia_meas=%.6f mA, Ia_ref=%.6f mA, sIa=%.3f, quickPercent=%.1f%%",
                      iaForQuick,
                      ia0,
                      sIa,
                      quickPercent);
            }
        }
    }

    if (healthMode == HEALTH_FULL && !healthResults.isEmpty()) {
        // Full Health combines the centre Quick Health score (Ia+gm at the
        // datasheet operating point) with four Ia-only corner scores. Each
        // corner now uses a 3-sweep cluster (same Va, three nearby Vg
        // values) and both DUT and reference Ia are aggregated with the
        // same robust3xCluster logic.
        double sumScores = 0.0;
        int scoreCount = 0;

        if (healthResults.size() > 0) {
            const HealthResult &r0 = healthResults.at(0);
            if (r0.valid) {
                const double iaForQuick = haveIaQuickEffective ? iaQuickEffective : r0.ia;
                const double gmForQuick = haveGmQuickEffective ? gmQuickEffective : r0.gm;

                if (haveHealthSurface &&
                    iaRefSurface.size() > 0 && iaRefSurface.at(0) > 0.0 &&
                    gmRefSurface.size() > 0 && gmRefSurface.at(0) > 0.0) {
                    const double sIa = metricScore(iaForQuick, iaRefSurface.at(0));
                    const double sGm = metricScore(gmForQuick, gmRefSurface.at(0));
                    sumScores += 0.5 * (sIa + sGm);
                    ++scoreCount;
                } else if (quickPercent > 0.0) {
                    sumScores += quickPercent / 100.0; // normalised 0..1
                    ++scoreCount;
                }
            }
        }

        // Corner layout for HEALTH_FULL: index 0 is centre; each of the four
        // corners contributes three HealthPoints (cluster of Vg offsets) in
        // order, so indices [1..3], [4..6], [7..9], [10..12].
        const int clusterSize = 3;
        const int cornerCount = 4;

        if (healthResults.size() >= 1 + clusterSize) {
            for (int corner = 0; corner < cornerCount; ++corner) {
                const int baseIdx = 1 + corner * clusterSize;
                if (baseIdx >= healthResults.size()) {
                    break;
                }

                QVector<double> iaCorner;
                QVector<double> iaRefCorner;
                iaCorner.reserve(clusterSize);
                iaRefCorner.reserve(clusterSize);

                for (int k = 0; k < clusterSize; ++k) {
                    const int idx = baseIdx + k;
                    if (idx >= healthResults.size()) {
                        break;
                    }
                    const HealthResult &r = healthResults.at(idx);
                    if (r.valid && r.ia > 0.0) {
                        iaCorner.append(r.ia);
                    }

                    double iaRef = ia0;
                    if (haveHealthSurface && idx < iaRefSurface.size() && iaRefSurface.at(idx) > 0.0) {
                        iaRef = iaRefSurface.at(idx);
                    }
                    iaRefCorner.append(iaRef);
                }

                double iaCornerEff = 0.0;
                double iaRefEff    = 0.0;
                bool haveIaCorner  = !iaCorner.isEmpty() && robust3xCluster(iaCorner, outlierTol, iaCornerEff);
                bool haveIaRef     = !iaRefCorner.isEmpty() && robust3xCluster(iaRefCorner, outlierTol, iaRefEff);

                if (haveIaCorner && haveIaRef && iaRefEff > 0.0) {
                    const double sIa = metricScore(iaCornerEff, iaRefEff);
                    qInfo("Health Full A corner %d: Ia_meas_eff=%.6f mA, Ia_ref_eff=%.6f mA, sIa=%.3f",
                          corner,
                          iaCornerEff,
                          iaRefEff,
                          sIa);
                    sumScores += sIa;
                    ++scoreCount;
                }
            }
        }

        if (scoreCount > 0) {
            fullPercent = 100.0 * (sumScores / static_cast<double>(scoreCount));
        }
    }

    // --- Optional reference-tube-based scores (Quick/Full) ---
    if (!datasheetJson.isEmpty()) {
        const QJsonObject healthRefObj = datasheetJson.value("healthReference").toObject();
        const QJsonObject centerObj    = healthRefObj.value("center").toObject();

        // Centre Ia/gm for Triode A reference tube
        const double iaRefA_centre = centerObj.value("iaRefA_mA").toDouble(0.0);
        const double gmRefA_centre = centerObj.value("gmRefA_mA_V").toDouble(0.0);

        if (healthResults.size() > 0) {
            const HealthResult &r0 = healthResults.at(0);
            if (r0.valid && iaRefA_centre > 0.0 && gmRefA_centre > 0.0) {
                const double iaForQuick = haveIaQuickEffective ? iaQuickEffective : r0.ia;
                const double gmForQuick = haveGmQuickEffective ? gmQuickEffective : r0.gm;
                const double sIaRef = metricScore(iaForQuick, iaRefA_centre);
                const double sGmRef = metricScore(gmForQuick, gmRefA_centre);
                quickPercentRef = 100.0 * 0.5 * (sIaRef + sGmRef);
            }
        }

        // For Full Health, also compute per-corner Ia-only reference scores
        if (healthMode == HEALTH_FULL) {
            const QJsonArray cornersArr = healthRefObj.value("corners").toArray();
            const int clusterSize = 3;

            if (cornersArr.size() >= 4 && healthPoints.size() >= 1 + 4 * clusterSize) {
                double sumRefScores = 0.0;
                int    refCount     = 0;

                for (int corner = 0; corner < 4; ++corner) {
                    const QJsonObject cObj = cornersArr.at(corner).toObject();
                    const double iaRefCornerA = cObj.value("iaRefA_mA").toDouble(0.0);
                    if (!(iaRefCornerA > 0.0)) {
                        continue;
                    }

                    const int baseIdx = 1 + corner * clusterSize;
                    QVector<double> iaCorner;
                    iaCorner.reserve(clusterSize);

                    for (int k = 0; k < clusterSize; ++k) {
                        const int idx = baseIdx + k;
                        if (idx >= healthResults.size()) {
                            break;
                        }
                        const HealthResult &hr = healthResults.at(idx);
                        if (hr.valid && hr.ia > 0.0) {
                            iaCorner.append(hr.ia);
                        }
                    }

                    if (iaCorner.isEmpty()) {
                        continue;
                    }

                    double iaCornerEff = 0.0;
                    if (!robust3xCluster(iaCorner, outlierTol, iaCornerEff) || !(iaCornerEff > 0.0)) {
                        continue;
                    }

                    const double sIaRefCorner = metricScore(iaCornerEff, iaRefCornerA);
                    sumRefScores += sIaRefCorner;
                    ++refCount;
                }

                if (refCount > 0) {
                    const double fullRefFromCorners = 100.0 * (sumRefScores / static_cast<double>(refCount));
                    // Combine centre (Quick) and corners equally for the
                    // reference-tube Full score, mirroring the datasheet path.
                    if (quickPercentRef > 0.0) {
                        fullPercentRef = 0.5 * quickPercentRef + 0.5 * fullRefFromCorners;
                    } else {
                        fullPercentRef = fullRefFromCorners;
                    }
                }
            }
        }
    }

    // Update the per-button health labels under Quick/Full Health using a
    // compact format so they fit alongside the existing controls.
    //
    //  - Quick label (Q): shows the datasheet-based Quick score only.
    //  - Full label  (F): shows the reference-tube-based Full score when
    //    available, otherwise falls back to the datasheet-based Full score.
    if (ui) {
        if (healthMode == HEALTH_QUICK || healthMode == HEALTH_FULL) {
            if (ui->quickHealthDsLabel) {
                ui->quickHealthDsLabel->setText(
                    tr("Q %1").arg(QString::number(quickPercent, 'f', 0)));
            }
        }
        if (healthMode == HEALTH_FULL) {
            if (ui->fullHealthDsLabel) {
                double labelValue = (fullPercentRef > 0.0) ? fullPercentRef : fullPercent;
                ui->fullHealthDsLabel->setText(
                    tr("F %1").arg(QString::number(labelValue, 'f', 0)));
            }
        }

        // Hide the separate Ref labels to avoid consuming horizontal space;
        // we now encode both DS and Ref scores in the single compact label
        // per test.
        if (ui->quickHealthRefLabel) {
            ui->quickHealthRefLabel->setVisible(false);
        }
        if (ui->fullHealthRefLabel) {
            ui->fullHealthRefLabel->setVisible(false);
        }
    }

    // Populate the Triode A/B Health "Measured" and "%" columns using the
    // central health result and the datasheet reference point. Also populate
    // the per-corner 4 Cor Pct column for Full Health runs.
    if (ui) {
        auto setEdit = [](QLineEdit *edit, const QString &text) {
            if (edit) {
                edit->setText(text);
            }
        };

        auto formatValue = [](double v, int decimals) -> QString {
            if (!std::isfinite(v)) {
                return QString();
            }
            return QString::number(v, 'f', decimals);
        };

        // Robust aggregate for N values: median-based outlier gate then average.
        auto robustAggregate = [](const QVector<double> &values, double outlierTol, double &effective) -> bool {
            if (values.isEmpty()) {
                return false;
            }

            QVector<double> sorted = values;
            std::sort(sorted.begin(), sorted.end());

            double median = 0.0;
            if (sorted.size() % 2 == 1) {
                median = sorted.at(sorted.size() / 2);
            } else {
                const int hi = sorted.size() / 2;
                median = 0.5 * (sorted.at(hi - 1) + sorted.at(hi));
            }

            if (!(median > 0.0) || !std::isfinite(median)) {
                return false;
            }

            double sum = 0.0;
            int count = 0;
            for (double v : values) {
                if (!(v > 0.0) || !std::isfinite(v)) {
                    continue;
                }
                const double ratio = v / median;
                const double dev = std::fabs(ratio - 1.0);
                if (dev > outlierTol) {
                    continue;
                }
                sum += v;
                ++count;
            }

            if (count > 0) {
                effective = sum / static_cast<double>(count);
            } else {
                effective = median;
            }

            return (effective > 0.0) && std::isfinite(effective);
        };

        auto formatPercent = [](double meas, double ref) -> QString {
            if (!(ref > 0.0) || !(meas > 0.0)) {
                return QString();
            }
            const double percent = 100.0 * meas / ref;
            if (!std::isfinite(percent)) {
                return QString();
            }
            return QString::number(percent, 'f', 0);
        };

        if (!healthResults.isEmpty()) {
            const HealthResult &r0 = healthResults.first();

            // Aggregate across all valid points (Quick: 9 points, Full: 13 points).
            // This makes the green boxes represent the overall health sample rather
            // than a single centre point.
            const double outlierTolAgg = 0.40; // 40% deviation from median
            QVector<double> iaValues;
            QVector<double> gmValues;
            QVector<double> rpValues;
            QVector<double> muValues;
            QVector<double> ig2Values;
            QVector<double> pg2Values;
            iaValues.reserve(healthResults.size());
            gmValues.reserve(healthResults.size());
            rpValues.reserve(healthResults.size());
            muValues.reserve(healthResults.size());
            ig2Values.reserve(healthResults.size());
            pg2Values.reserve(healthResults.size());

            for (const HealthResult &hr : healthResults) {
                if (!hr.valid) {
                    continue;
                }
                if (hr.ia > 0.0 && std::isfinite(hr.ia)) {
                    iaValues.append(hr.ia);
                }
                if (hr.gm > 0.0 && std::isfinite(hr.gm)) {
                    gmValues.append(hr.gm);
                }
                if (hr.rp > 0.0 && std::isfinite(hr.rp)) {
                    rpValues.append(hr.rp);
                }
                if (hr.gm > 0.0 && hr.rp > 0.0 && std::isfinite(hr.gm) && std::isfinite(hr.rp)) {
                    const double mu = (hr.gm * hr.rp / 1000.0);
                    if (mu > 0.0 && std::isfinite(mu)) {
                        muValues.append(mu);
                    }
                }
                if (hr.ig2 > 0.0 && std::isfinite(hr.ig2)) {
                    ig2Values.append(hr.ig2);
                }
                if (hr.vg2 > 0.0 && hr.ig2 > 0.0 && std::isfinite(hr.vg2) && std::isfinite(hr.ig2)) {
                    const double pg2_W = (hr.vg2 * hr.ig2 / 1000.0);
                    if (pg2_W > 0.0 && std::isfinite(pg2_W)) {
                        pg2Values.append(pg2_W);
                    }
                }
            }

            double iaAgg = 0.0;
            double gmAgg = 0.0;
            double rpAgg = 0.0;
            double muAgg = 0.0;
            double ig2Agg = 0.0;
            double pg2Agg = 0.0;
            const bool haveIaAgg = robustAggregate(iaValues, outlierTolAgg, iaAgg);
            const bool haveGmAgg = robustAggregate(gmValues, outlierTolAgg, gmAgg);
            const bool haveRpAgg = robustAggregate(rpValues, outlierTolAgg, rpAgg);
            const bool haveMuAgg = robustAggregate(muValues, outlierTolAgg, muAgg);
            const bool haveIg2Agg = robustAggregate(ig2Values, outlierTolAgg, ig2Agg);
            const bool havePg2Agg = robustAggregate(pg2Values, outlierTolAgg, pg2Agg);

            qInfo("Health UI: centre result valid=%d Va=%.3f Vg=%.3f Vg2=%.3f Ia=%.6f mA gm=%.6f mA/V rp=%.3f ohms ig2=%.6f mA",
                  r0.valid ? 1 : 0,
                  r0.va,
                  r0.vg,
                  r0.vg2,
                  r0.ia,
                  r0.gm,
                  r0.rp,
                  r0.ig2);

            qInfo("Health UI: aggregate (n=%d/%d) Ia=%.6f mA gm=%.6f mA/V rp=%.3f ohms mu=%.3f ig2=%.6f mA pg2=%.6f W",
                  iaValues.size(),
                  healthResults.size(),
                  haveIaAgg ? iaAgg : 0.0,
                  haveGmAgg ? gmAgg : 0.0,
                  haveRpAgg ? rpAgg : 0.0,
                  haveMuAgg ? muAgg : 0.0,
                  haveIg2Agg ? ig2Agg : 0.0,
                  havePg2Agg ? pg2Agg : 0.0);

            if (r0.valid) {
                const double iaForDisplayA = haveIaAgg ? iaAgg : (haveIaQuickEffective ? iaQuickEffective : r0.ia);
                const double gmForDisplayA = haveGmAgg ? gmAgg : (haveGmQuickEffective ? gmQuickEffective : r0.gm);
                const double rpForDisplayA = haveRpAgg ? rpAgg : r0.rp;
                const double muMeasuredA = haveMuAgg
                                           ? muAgg
                                           : ((rpForDisplayA > 0.0 && gmForDisplayA > 0.0)
                                                  ? (gmForDisplayA * rpForDisplayA / 1000.0)
                                                  : 0.0);

                qInfo("Health UI: triodeA rpText='%s' muText='%s' (rp>0=%d mu>0=%d)",
                      (rpForDisplayA > 0.0) ? QString::number(rpForDisplayA, 'f', 0).toStdString().c_str() : "",
                      (muMeasuredA > 0.0) ? QString::number(muMeasuredA, 'f', 1).toStdString().c_str() : "",
                      (rpForDisplayA > 0.0) ? 1 : 0,
                      (muMeasuredA > 0.0) ? 1 : 0);

                // Triode A measured values
                // Ia is in mA; gm is displayed in µS for consistency with
                // datasheet/reference values (gm field is labeled gm (µS)).
                setEdit(ui->triodeA_Ia_measured, formatValue(iaForDisplayA, 3));
                setEdit(ui->triodeA_gm_measured, formatValue(gmForDisplayA * 1000.0, 0));
                setEdit(ui->triodeA_mu_measured,
                        (muMeasuredA > 0.0) ? formatValue(muMeasuredA, 1) : QString());
                setEdit(ui->triodeA_rp_measured,
                        (rpForDisplayA > 0.0) ? formatValue(rpForDisplayA, 0) : QString());

                if (haveRef) {
                    setEdit(ui->triodeA_Ia_pct, formatPercent(iaForDisplayA, ia0));
                    setEdit(ui->triodeA_gm_pct, formatPercent(gmForDisplayA, gm0));
                    setEdit(ui->triodeA_mu_pct,
                            (mu0 > 0.0 && muMeasuredA > 0.0)
                                ? formatPercent(muMeasuredA, mu0)
                                : QString());
                    setEdit(ui->triodeA_rp_pct,
                            (rp0 > 0.0 && rpForDisplayA > 0.0)
                                ? formatPercent(rpForDisplayA, rp0)
                                : QString());
                } else {
                    setEdit(ui->triodeA_Ia_pct, QString());
                    setEdit(ui->triodeA_gm_pct, QString());
                    setEdit(ui->triodeA_mu_pct, QString());
                    setEdit(ui->triodeA_rp_pct, QString());
                }

                // Capture Triode A's centre Ia offset versus the chosen
                // reference centre so that Full Health 4-corner scores for A
                // can use a scaled Ia reference at each corner.
                // (Centre-based scaling for corners is currently disabled;
                //  keep iaScaleAForCorners at its default of 1.0.)

                if (deviceType == PENTODE) {
                    if (ui->Triode_B_Box) {
                        ui->Triode_B_Box->setTitle(tr("Screen Health"));
                    }
                    if (ui->triodeB_Ia_label) {
                        ui->triodeB_Ia_label->setText(tr("Ig2(mA)"));
                    }
                    if (ui->triodeB_rp_label) {
                        ui->triodeB_rp_label->setText(tr("Pg2(W)"));
                    }
                    if (ui->triodeBHeaderFourCornerPct) ui->triodeBHeaderFourCornerPct->setVisible(false);
                    if (ui->triodeB_corner1_pct) ui->triodeB_corner1_pct->setVisible(false);
                    if (ui->triodeB_corner2_pct) ui->triodeB_corner2_pct->setVisible(false);
                    if (ui->triodeB_corner3_pct) ui->triodeB_corner3_pct->setVisible(false);
                    if (ui->triodeB_corner4_pct) ui->triodeB_corner4_pct->setVisible(false);
                    if (ui->triodeB_gm_label) ui->triodeB_gm_label->setVisible(false);
                    if (ui->triodeB_mu_label) ui->triodeB_mu_label->setVisible(false);
                    if (ui->triodeB_gm_measured) ui->triodeB_gm_measured->setVisible(false);
                    if (ui->triodeB_gm_ref) ui->triodeB_gm_ref->setVisible(false);
                    if (ui->triodeB_gm_pct) ui->triodeB_gm_pct->setVisible(false);
                    if (ui->triodeB_mu_measured) ui->triodeB_mu_measured->setVisible(false);
                    if (ui->triodeB_mu_ref) ui->triodeB_mu_ref->setVisible(false);
                    if (ui->triodeB_mu_pct) ui->triodeB_mu_pct->setVisible(false);

                    const double ig2Meas = haveIg2Agg ? ig2Agg : r0.ig2;
                    const double pg2Meas_W = havePg2Agg ? pg2Agg
                                                       : ((r0.vg2 > 0.0 && ig2Meas > 0.0) ? (r0.vg2 * ig2Meas / 1000.0) : 0.0);

                    double ig2RefCentre = 0.0;
                    if (haveHealthSurface && ig2RefSurface.size() > 0 && ig2RefSurface.at(0) > 0.0) {
                        ig2RefCentre = ig2RefSurface.at(0);
                    } else if (!datasheetJson.isEmpty()) {
                        const QJsonObject centerObj = datasheetJson.value("healthReference").toObject().value("center").toObject();
                        ig2RefCentre = centerObj.value("ig2Ref_mA").toDouble(0.0);
                    }
                    const double pg2Ref_W = (r0.vg2 > 0.0 && ig2RefCentre > 0.0) ? (r0.vg2 * ig2RefCentre / 1000.0) : 0.0;

                    setEdit(ui->triodeB_Ia_measured, formatValue(ig2Meas, 3));
                    setEdit(ui->triodeB_Ia_ref, (ig2RefCentre > 0.0) ? formatValue(ig2RefCentre, 3) : QString());
                    setEdit(ui->triodeB_Ia_pct,
                            (ig2RefCentre > 0.0)
                                ? QString::number(100.0 * metricScore(ig2Meas, ig2RefCentre), 'f', 0)
                                : QString());

                    setEdit(ui->triodeB_rp_measured, (pg2Meas_W > 0.0) ? formatValue(pg2Meas_W, 3) : QString());
                    setEdit(ui->triodeB_rp_ref, (pg2Ref_W > 0.0) ? formatValue(pg2Ref_W, 3) : QString());
                    setEdit(ui->triodeB_rp_pct,
                            (pg2Ref_W > 0.0)
                                ? QString::number(100.0 * metricScore(pg2Meas_W, pg2Ref_W), 'f', 0)
                                : QString());

                    // Clear unused fields
                    setEdit(ui->triodeB_gm_measured, QString());
                    setEdit(ui->triodeB_gm_ref, QString());
                    setEdit(ui->triodeB_gm_pct, QString());
                    setEdit(ui->triodeB_mu_measured, QString());
                    setEdit(ui->triodeB_mu_ref, QString());
                    setEdit(ui->triodeB_mu_pct, QString());
                }
                // Triode B measured values for double triodes with valid B data
                else if (isDoubleTriode && triodeMeasurementSecondary &&
                    measurementHasValidSamples(triodeMeasurementSecondary)) {
                    qInfo("Health TriodeB: secondary present at finalizeHealthRun - sweeps=%d, testType=%d",
                          triodeMeasurementSecondary->count(),
                          triodeMeasurementSecondary->getTestType());
                    double iaB = 0.0;
                    double gmB = 0.0;
                    double rpB = 0.0;

                    HealthPoint centerPoint;
                    centerPoint.va = va0;
                    centerPoint.vg = vg0;
                    centerPoint.vg2 = vg20;
                    if (!healthPoints.isEmpty()) {
                        centerPoint = healthPoints.first();
                    }

                    const bool okB = computeIaGmAt(triodeMeasurementSecondary,
                                                   centerPoint,
                                                   iaB,
                                                   gmB,
                                                   rpB);
                    if (okB) {
                        qInfo("Health TriodeB: centre OK (Va=%.3f, Vg=%.3f) Ia=%.6f mA, gm=%.6f mA/V, rp=%.1f ohms",
                              centerPoint.va,
                              centerPoint.vg,
                              iaB,
                              gmB,
                              rpB);
                        double iaBDisplay = iaB;
                        double gmBDisplay = gmB;

                        // Apply the same three-point robustness to Triode B
                        // Ia/gm during Quick Health, using the B clone and
                        // the three centre-row HealthPoints.
                        if (healthMode == HEALTH_QUICK && healthPoints.size() >= 3) {
                            QVector<double> iaBCandidates;
                            QVector<double> gmBCandidates;
                            iaBCandidates.reserve(3);
                            gmBCandidates.reserve(3);

                            for (int i = 0; i < 3; ++i) {
                                const HealthPoint &hp = healthPoints.at(i);
                                double iaTmp = 0.0;
                                double gmTmp = 0.0;
                                double rpTmp = 0.0;
                                if (computeIaGmAt(triodeMeasurementSecondary,
                                                   hp,
                                                   iaTmp,
                                                   gmTmp,
                                                   rpTmp)) {
                                    if (iaTmp > 0.0) {
                                        iaBCandidates.append(iaTmp);
                                    }
                                    if (gmTmp > 0.0) {
                                        gmBCandidates.append(gmTmp);
                                    }
                                }
                            }

                            double iaBQuickEffective = 0.0;
                            double gmBQuickEffective = 0.0;

                            if (!iaBCandidates.isEmpty() &&
                                robust3xCluster(iaBCandidates, outlierTol, iaBQuickEffective)) {
                                iaBDisplay = iaBQuickEffective;
                            }
                            if (!gmBCandidates.isEmpty() &&
                                robust3xCluster(gmBCandidates, outlierTol, gmBQuickEffective)) {
                                gmBDisplay = gmBQuickEffective;
                            }
                        }

                        const double muMeasuredB = (rpB > 0.0 && gmBDisplay > 0.0)
                                                   ? (gmBDisplay * rpB / 1000.0)
                                                   : 0.0;

                        setEdit(ui->triodeB_Ia_measured, formatValue(iaBDisplay, 3));
                        // gm displayed in µS
                        setEdit(ui->triodeB_gm_measured, formatValue(gmBDisplay * 1000.0, 0));
                        setEdit(ui->triodeB_mu_measured,
                                (muMeasuredB > 0.0) ? formatValue(muMeasuredB, 1) : QString());
                        setEdit(ui->triodeB_rp_measured,
                                (rpB > 0.0) ? formatValue(rpB, 0) : QString());

                        if (haveRef) {
                            setEdit(ui->triodeB_Ia_pct, formatPercent(iaBDisplay, ia0));
                            setEdit(ui->triodeB_gm_pct, formatPercent(gmBDisplay, gm0));
                            setEdit(ui->triodeB_mu_pct,
                                    (mu0 > 0.0 && muMeasuredB > 0.0)
                                        ? formatPercent(muMeasuredB, mu0)
                                        : QString());
                            setEdit(ui->triodeB_rp_pct,
                                    (rp0 > 0.0 && rpB > 0.0)
                                        ? formatPercent(rpB, rp0)
                                        : QString());
                        } else {
                            setEdit(ui->triodeB_Ia_pct, QString());
                            setEdit(ui->triodeB_gm_pct, QString());
                            setEdit(ui->triodeB_mu_pct, QString());
                            setEdit(ui->triodeB_rp_pct, QString());
                        }

                        // Capture Triode B's centre Ia offset versus the same
                        // reference centre so that its Full Health 4-corner
                        // scores are judged against a scaled Ia reference at
                        // each corner.
                        // (Centre-based scaling for corners is currently disabled;
                        //  keep iaScaleBForCorners at its default of 1.0.)
                    } else {
                        qInfo("Health TriodeB: computeIaGmAt FAILED at centre point (Va=%.3f, Vg=%.3f)",
                              centerPoint.va,
                              centerPoint.vg);
                        setEdit(ui->triodeB_Ia_measured, QString());
                        setEdit(ui->triodeB_gm_measured, QString());
                        setEdit(ui->triodeB_mu_measured, QString());
                        setEdit(ui->triodeB_rp_measured, QString());
                        setEdit(ui->triodeB_Ia_pct, QString());
                        setEdit(ui->triodeB_gm_pct, QString());
                        setEdit(ui->triodeB_mu_pct, QString());
                        setEdit(ui->triodeB_rp_pct, QString());
                    }
                } else {
                    // Not a double triode or no Triode B data: clear B health display
                    qInfo("Health TriodeB: no valid secondary at finalizeHealthRun (isDoubleTriode=%d, secondaryPtr=%p)",
                          isDoubleTriode ? 1 : 0,
                          static_cast<void *>(triodeMeasurementSecondary));
                    setEdit(ui->triodeB_Ia_measured, QString());
                    setEdit(ui->triodeB_gm_measured, QString());
                    setEdit(ui->triodeB_mu_measured, QString());
                    setEdit(ui->triodeB_rp_measured, QString());
                    setEdit(ui->triodeB_Ia_pct, QString());
                    setEdit(ui->triodeB_gm_pct, QString());
                    setEdit(ui->triodeB_mu_pct, QString());
                    setEdit(ui->triodeB_rp_pct, QString());
                }
            }
        }

        // Populate the 4-corner percentage (orange) fields for Full Health runs.
        // Always clear them first so Quick Health (single point) leaves them empty.
        // Corners are Ia-only checks using a 3-sweep cluster at each corner for
        // both DUT and reference Ia.
        auto formatCornerScore = [&](int cornerIdx) -> QString {
            const int clusterSize = 3;
            const int baseIdx = 1 + cornerIdx * clusterSize;
            if (!haveRef || baseIdx < 1 || baseIdx >= healthResults.size()) {
                return QString();
            }

            QVector<double> iaCorner;
            QVector<double> iaRefCorner;
            iaCorner.reserve(clusterSize);
            iaRefCorner.reserve(clusterSize);

            for (int k = 0; k < clusterSize; ++k) {
                const int idx = baseIdx + k;
                if (idx >= healthResults.size()) {
                    break;
                }

                const HealthResult &hr = healthResults.at(idx);
                if (hr.valid && hr.ia > 0.0) {
                    iaCorner.append(hr.ia);
                }

                double iaRef = ia0;
                if (haveHealthSurface && idx < iaRefSurface.size()) {
                    if (iaRefSurface.at(idx) > 0.0) iaRef = iaRefSurface.at(idx);
                }
                iaRefCorner.append(iaRef);
            }

            double iaCornerEff = 0.0;
            double iaRefEff    = 0.0;
            bool haveIaCorner  = !iaCorner.isEmpty() && robust3xCluster(iaCorner, outlierTol, iaCornerEff);
            bool haveIaRef     = !iaRefCorner.isEmpty() && robust3xCluster(iaRefCorner, outlierTol, iaRefEff);

            if (!haveIaCorner || !haveIaRef || iaRefEff <= 0.0) {
                return QString();
            }

            const double sIa = metricScore(iaCornerEff, iaRefEff);
            const double pct = 100.0 * sIa;
            if (!std::isfinite(pct)) {
                return QString();
            }
            return QString::number(pct, 'f', 0);
        };

        // Clear Triode A/B corner columns by default.
        setEdit(ui->triodeA_corner1_pct, QString());
        setEdit(ui->triodeA_corner2_pct, QString());
        setEdit(ui->triodeA_corner3_pct, QString());
        setEdit(ui->triodeA_corner4_pct, QString());
        setEdit(ui->triodeB_corner1_pct, QString());
        setEdit(ui->triodeB_corner2_pct, QString());
        setEdit(ui->triodeB_corner3_pct, QString());
        setEdit(ui->triodeB_corner4_pct, QString());

        if (healthMode == HEALTH_FULL && haveRef && healthResults.size() >= 13) {
            // Corner clusters for Triode A: indices [1..3], [4..6], [7..9], [10..12].
            setEdit(ui->triodeA_corner1_pct, formatCornerScore(0));
            setEdit(ui->triodeA_corner2_pct, formatCornerScore(1));
            setEdit(ui->triodeA_corner3_pct, formatCornerScore(2));
            setEdit(ui->triodeA_corner4_pct, formatCornerScore(3));

            // For double triodes, derive matching corner scores for Triode B
            // from the secondary measurement using the same 3-sweep corner
            // clusters.
            if (deviceType == PENTODE) {
                // Screen Health corner column is hidden for pentodes (informational view).
                setEdit(ui->triodeB_corner1_pct, QString());
                setEdit(ui->triodeB_corner2_pct, QString());
                setEdit(ui->triodeB_corner3_pct, QString());
                setEdit(ui->triodeB_corner4_pct, QString());
            } else if (isDoubleTriode && triodeMeasurementSecondary &&
                measurementHasValidSamples(triodeMeasurementSecondary)) {

                auto formatCornerScoreB = [&](int cornerIdx) -> QString {
                    const int clusterSize = 3;
                    const int baseIdx = 1 + cornerIdx * clusterSize;
                    if (baseIdx < 1 || baseIdx >= healthPoints.size()) {
                        return QString();
                    }

                    QVector<double> iaBCorner;
                    QVector<double> iaRefCorner;
                    iaBCorner.reserve(clusterSize);
                    iaRefCorner.reserve(clusterSize);

                    bool hasBSpecificRef = false;

                    for (int k = 0; k < clusterSize; ++k) {
                        const int idx = baseIdx + k;
                        if (idx >= healthPoints.size()) {
                            break;
                        }

                        const HealthPoint &hp = healthPoints.at(idx);
                        double iaB = 0.0;
                        double gmB = 0.0;
                        double rpB = 0.0;
                        bool okB = computeIaGmAt(triodeMeasurementSecondary,
                                                 hp,
                                                 iaB,
                                                 gmB,
                                                 rpB);
                        // For corner scoring we only require a positive Ia.
                        // Treat the point as unusable only when IaB is non-positive;
                        // keep IaB even if the gm fit failed so a healthy tube does
                        // not lose its Triode B corner due to a local gm issue.
                        if (!okB && iaB <= 0.0) {
                            qInfo("Health TriodeB: corner %d idx=%d computeIaGmAt FAILED (Va=%.3f, Vg=%.3f)",
                                  cornerIdx,
                                  idx,
                                  hp.va,
                                  hp.vg);
                            continue;
                        }
                        if (iaB > 0.0) {
                            iaBCorner.append(iaB);
                        }

                        double iaRef = ia0;
                        if (haveHealthSurface) {
                            // Prefer Triode B's own reference surface when available.
                            if (idx < iaRefSurfaceB.size() && iaRefSurfaceB.at(idx) > 0.0) {
                                iaRef = iaRefSurfaceB.at(idx);
                                hasBSpecificRef = true;
                            } else if (idx < iaRefSurface.size() && iaRefSurface.at(idx) > 0.0) {
                                // Fall back to the shared/triode-A reference surface.
                                iaRef = iaRefSurface.at(idx);
                            }
                        }
                        iaRefCorner.append(iaRef);
                    }

                    // If this corner never had a genuine B-specific reference
                    // current, do not compare Triode B against Triode A's
                    // reference. Leave the corner blank instead of showing
                    // an artificially harsh 0% score.
                    if (!hasBSpecificRef) {
                        return QString();
                    }

                    double iaBEff   = 0.0;
                    double iaRefEff = 0.0;
                    bool haveIaB    = !iaBCorner.isEmpty() && robust3xCluster(iaBCorner, outlierTol, iaBEff);
                    bool haveIaRef  = !iaRefCorner.isEmpty() && robust3xCluster(iaRefCorner, outlierTol, iaRefEff);

                    if (!haveRef || !haveIaB || !haveIaRef || iaRefEff <= 0.0) {
                        return QString();
                    }

                    const double sIa = metricScore(iaBEff, iaRefEff);
                    qInfo("Health Full B corner %d: Ia_meas_eff=%.6f mA, Ia_ref_eff=%.6f mA, sIa=%.3f",
                          cornerIdx,
                          iaBEff,
                          iaRefEff,
                          sIa);
                    const double pct = 100.0 * sIa;
                    if (!std::isfinite(pct)) {
                        return QString();
                    }
                    return QString::number(pct, 'f', 0);
                };

                setEdit(ui->triodeB_corner1_pct, formatCornerScoreB(0));
                setEdit(ui->triodeB_corner2_pct, formatCornerScoreB(1));
                setEdit(ui->triodeB_corner3_pct, formatCornerScoreB(2));
                setEdit(ui->triodeB_corner4_pct, formatCornerScoreB(3));
            }
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

    const double fullDisplayValue = (fullPercentRef > 0.0) ? fullPercentRef : fullPercent;

    QString msg;
    if (healthResults.size() > 0) {
        if (quickPercent > 0.0) {
            msg += tr("Quick Health: %1% ").arg(QString::number(quickPercent, 'f', 0));
        }
        if (healthResults.size() > 1 && fullDisplayValue > 0.0) {
            msg += tr("Full Health: %1% ").arg(QString::number(fullDisplayValue, 'f', 0));
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

bool ValveWorkbench::captureHealthReferenceFromLastRun()
{
    const int clusterSize = 3;
    const int requiredPoints = 1 + 4 * clusterSize;

    if (deviceType != TRIODE && deviceType != PENTODE) {
        QMessageBox::warning(this,
                             tr("Save Reference Tube"),
                             tr("Reference tubes are currently supported for triode and pentode devices only."));
        return false;
    }
    if (datasheetJson.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Save Reference Tube"),
                             tr("No datasheet block is loaded. Load a template first."));
        return false;
    }
    if (healthResults.size() < requiredPoints || healthPoints.size() < requiredPoints) {
        QMessageBox::warning(this,
                             tr("Save Reference Tube"),
                             tr("Full Health results are not available. Run a Full Health test first."));
        return false;
    }

    auto robust3xCluster = [](const QVector<double> &values, double outlierTol, double &effective) -> bool {
        if (values.isEmpty()) {
            return false;
        }

        QVector<double> sorted = values;
        std::sort(sorted.begin(), sorted.end());

        double median = 0.0;
        if (sorted.size() == 1) {
            median = sorted[0];
        } else if (sorted.size() == 2) {
            median = 0.5 * (sorted[0] + sorted[1]);
        } else {
            median = sorted[1];
        }

        if (!(median > 0.0)) {
            return false;
        }

        double sum = 0.0;
        int count = 0;
        for (double v : values) {
            if (!(v > 0.0)) {
                continue;
            }
            const double ratio = v / median;
            const double dev = std::fabs(ratio - 1.0);
            if (dev > outlierTol) {
                continue;
            }
            sum += v;
            ++count;
        }

        if (count > 0) {
            effective = sum / static_cast<double>(count);
        } else {
            effective = median;
        }

        return (effective > 0.0);
    };

    const double outlierTol = 0.40;

    // Capture centre reference metrics for Triode A/B so Quick/Full Health
    // can compute scores versus this specific reference tube in addition to
    // the datasheet operating point.
    HealthPoint centerPoint;
    bool haveCenterPoint = false;
    if (!healthPoints.isEmpty()) {
        centerPoint = healthPoints.first();
        haveCenterPoint = true;
    }

    double iaCentreA = 0.0;
    double gmCentreA = 0.0;
    double ig2Centre = 0.0;
    bool haveCentreA = false;
    if (!healthResults.isEmpty()) {
        const HealthResult &hr0 = healthResults.first();
        if (hr0.valid && hr0.ia > 0.0 && hr0.gm > 0.0) {
            iaCentreA = hr0.ia;
            gmCentreA = hr0.gm;
            haveCentreA = true;
        }
        if (deviceType == PENTODE && hr0.valid && hr0.ig2 > 0.0) {
            ig2Centre = hr0.ig2;
        }
    }

    double iaCentreB = 0.0;
    double gmCentreB = 0.0;
    bool haveCentreB = false;
    if (isDoubleTriode && triodeMeasurementSecondary &&
        measurementHasValidSamples(triodeMeasurementSecondary) &&
        haveCenterPoint) {
        double rpB = 0.0;
        if (computeIaGmAt(triodeMeasurementSecondary,
                          centerPoint,
                          iaCentreB,
                          gmCentreB,
                          rpB) &&
            iaCentreB > 0.0 && gmCentreB > 0.0) {
            haveCentreB = true;
        }
    }

    QJsonArray cornersArr;
    for (int corner = 0; corner < 4; ++corner) {
        const int baseIdx = 1 + corner * clusterSize;
        if (baseIdx < 0 || baseIdx >= healthResults.size()) {
            continue;
        }

        // Triode A reference Ia cluster from HealthResult data.
        QVector<double> iaValuesA;
        iaValuesA.reserve(clusterSize);
        QVector<double> ig2Values;
        ig2Values.reserve(clusterSize);
        for (int k = 0; k < clusterSize; ++k) {
            const int idx = baseIdx + k;
            if (idx >= healthResults.size()) {
                break;
            }
            const HealthResult &hr = healthResults.at(idx);
            if (hr.valid && hr.ia > 0.0) {
                iaValuesA.append(hr.ia);
            }
            if (deviceType == PENTODE && hr.valid && hr.ig2 > 0.0) {
                ig2Values.append(hr.ig2);
            }
        }
        if (iaValuesA.isEmpty()) {
            continue;
        }

        double iaEffA = 0.0;
        if (!robust3xCluster(iaValuesA, outlierTol, iaEffA)) {
            continue;
        }

        double ig2Eff = 0.0;
        bool haveIg2 = false;
        if (deviceType == PENTODE && !ig2Values.isEmpty()) {
            haveIg2 = robust3xCluster(ig2Values, outlierTol, ig2Eff);
        }

        // Optional Triode B reference Ia cluster using the secondary
        // measurement at the same HealthPoints when available.
        double iaEffB = 0.0;
        bool haveIaB = false;
        if (isDoubleTriode && triodeMeasurementSecondary &&
            measurementHasValidSamples(triodeMeasurementSecondary)) {

            QVector<double> iaValuesB;
            iaValuesB.reserve(clusterSize);
            for (int k = 0; k < clusterSize; ++k) {
                const int idx = baseIdx + k;
                if (idx >= healthPoints.size()) {
                    break;
                }
                const HealthPoint &hp = healthPoints.at(idx);

                double iaB = 0.0;
                double gmB = 0.0;
                double rpB = 0.0;
                bool okB = computeIaGmAt(triodeMeasurementSecondary,
                                         hp,
                                         iaB,
                                         gmB,
                                         rpB);
                if (!okB && iaB <= 0.0) {
                    qInfo("SaveRef B corner %d idx=%d computeIaGmAt FAILED (Va=%.3f, Vg=%.3f)",
                          corner,
                          idx,
                          hp.va,
                          hp.vg);
                }
                // For reference Ia we only require a positive current; use Ia
                // even if the gm fit failed so that a healthy reference tube
                // does not lose its B corner.
                if (iaB > 0.0) {
                    iaValuesB.append(iaB);
                }
            }

            if (!iaValuesB.isEmpty()) {
                bool okClusterB = robust3xCluster(iaValuesB, outlierTol, iaEffB);
                qInfo("SaveRef B corner %d: n=%d ok=%d iaEffB=%.6f mA", corner, iaValuesB.size(), okClusterB ? 1 : 0, iaEffB);

                // If the robust 3-point cluster rejects the B values or
                // yields a non-positive result, fall back to a simple mean
                // of the positive IaB samples so that a healthy reference
                // tube does not silently lose its Triode B corner data.
                if (!okClusterB || !(iaEffB > 0.0)) {
                    double sumB = 0.0;
                    int countB = 0;
                    for (double v : iaValuesB) {
                        if (v > 0.0) {
                            sumB += v;
                            ++countB;
                        }
                    }
                    if (countB > 0) {
                        iaEffB = sumB / static_cast<double>(countB);
                        okClusterB = (iaEffB > 0.0);
                        qInfo("SaveRef B corner %d: cluster fallback used, mean IaB=%.6f mA over %d samples", corner, iaEffB, countB);
                    }
                }

                if (okClusterB && iaEffB > 0.0) {
                    haveIaB = true;
                }
            }
        }

        const HealthResult &hr0 = healthResults.at(baseIdx);

        QJsonObject cornerObj;
        cornerObj.insert(QStringLiteral("va"), hr0.va);
        cornerObj.insert(QStringLiteral("vg"), hr0.vg);
        if (deviceType == PENTODE) {
            cornerObj.insert(QStringLiteral("vg2"), hr0.vg2);
        }
        if (deviceType == PENTODE && haveIg2 && ig2Eff > 0.0) {
            cornerObj.insert(QStringLiteral("ig2Ref_mA"), ig2Eff);
        }
        // Legacy single reference current (Triode A).
        cornerObj.insert(QStringLiteral("iaRef_mA"), iaEffA);
        // Per-triode reference for Triode A.
        cornerObj.insert(QStringLiteral("iaRefA_mA"), iaEffA);
        // Per-triode reference for Triode B when a valid B cluster exists.
        if (haveIaB) {
            cornerObj.insert(QStringLiteral("iaRefB_mA"), iaEffB);
        }
        cornersArr.append(cornerObj);
    }

    if (cornersArr.size() != 4) {
        QMessageBox::warning(this,
                             tr("Save Reference Tube"),
                             tr("Could not derive four valid corner clusters from the last Full Health run."));
        return false;
    }

    // Merge the newly derived reference values with any existing
    // datasheet.healthReference using a cumulative average so repeated
    // "Save as Reference Tube" operations refine the stored reference
    // instead of overwriting it.
    QJsonObject healthRefObj = datasheetJson.value(QStringLiteral("healthReference")).toObject();

    const QJsonArray existingCorners = healthRefObj.value(QStringLiteral("corners")).toArray();
    const QJsonObject existingCenter = healthRefObj.value(QStringLiteral("center")).toObject();

    int prevCount = healthRefObj.value(QStringLiteral("sampleCount")).toInt(0);
    if (prevCount <= 0) {
        // If we already had some reference data but no explicit sampleCount,
        // treat it as a single prior sample for backwards compatibility.
        if (existingCorners.size() == 4 || !existingCenter.isEmpty()) {
            prevCount = 1;
        }
    }

    auto accumulateScalar = [prevCount](double oldVal, double newVal) -> double {
        if (!(newVal > 0.0)) {
            return oldVal;
        }
        if (!(oldVal > 0.0) || prevCount <= 0) {
            return newVal;
        }
        return (oldVal * static_cast<double>(prevCount) + newVal) /
               static_cast<double>(prevCount + 1);
    };

    // --- Centre accumulation ---
    if (haveCenterPoint && (haveCentreA || haveCentreB)) {
        QJsonObject centerMerged;
        centerMerged.insert(QStringLiteral("va"), centerPoint.va);
        centerMerged.insert(QStringLiteral("vg"), centerPoint.vg);
        if (deviceType == PENTODE) {
            centerMerged.insert(QStringLiteral("vg2"), centerPoint.vg2);
        }

        if (haveCentreA || !existingCenter.isEmpty()) {
            const double oldIaA = existingCenter.value(QStringLiteral("iaRefA_mA")).toDouble(0.0);
            const double oldGmA = existingCenter.value(QStringLiteral("gmRefA_mA_V")).toDouble(0.0);
            const double iaA = accumulateScalar(oldIaA, iaCentreA);
            const double gmA = accumulateScalar(oldGmA, gmCentreA);
            if (iaA > 0.0) centerMerged.insert(QStringLiteral("iaRefA_mA"), iaA);
            if (gmA > 0.0) centerMerged.insert(QStringLiteral("gmRefA_mA_V"), gmA);
        }

        if (deviceType == PENTODE) {
            const double oldIg2 = existingCenter.value(QStringLiteral("ig2Ref_mA")).toDouble(0.0);
            const double ig2 = accumulateScalar(oldIg2, ig2Centre);
            if (ig2 > 0.0) {
                centerMerged.insert(QStringLiteral("ig2Ref_mA"), ig2);
            }
        }

        if (haveCentreB || !existingCenter.isEmpty()) {
            const double oldIaB = existingCenter.value(QStringLiteral("iaRefB_mA")).toDouble(0.0);
            const double oldGmB = existingCenter.value(QStringLiteral("gmRefB_mA_V")).toDouble(0.0);
            const double iaB = accumulateScalar(oldIaB, iaCentreB);
            const double gmB = accumulateScalar(oldGmB, gmCentreB);
            if (iaB > 0.0) centerMerged.insert(QStringLiteral("iaRefB_mA"), iaB);
            if (gmB > 0.0) centerMerged.insert(QStringLiteral("gmRefB_mA_V"), gmB);
        }

        healthRefObj.insert(QStringLiteral("center"), centerMerged);
    }

    // --- Corner accumulation ---
    QJsonArray mergedCorners;

    for (int corner = 0; corner < 4; ++corner) {
        const QJsonObject newCorner = cornersArr.at(corner).toObject();
        const QJsonObject oldCorner =
            (existingCorners.size() == 4) ? existingCorners.at(corner).toObject() : QJsonObject();

        QJsonObject merged;
        merged.insert(QStringLiteral("va"), newCorner.value(QStringLiteral("va"))); // latest Va
        merged.insert(QStringLiteral("vg"), newCorner.value(QStringLiteral("vg"))); // latest Vg

        const double newIaALegacy = newCorner.value(QStringLiteral("iaRef_mA")).toDouble(0.0);
        const double newIaA       = newCorner.value(QStringLiteral("iaRefA_mA")).toDouble(newIaALegacy);
        const double oldIaALegacy = oldCorner.value(QStringLiteral("iaRef_mA")).toDouble(0.0);
        const double oldIaA       = oldCorner.value(QStringLiteral("iaRefA_mA")).toDouble(oldIaALegacy);

        const double iaANewAccum = accumulateScalar(oldIaA, newIaA);
        if (iaANewAccum > 0.0) {
            merged.insert(QStringLiteral("iaRef_mA"), iaANewAccum);
            merged.insert(QStringLiteral("iaRefA_mA"), iaANewAccum);
        }

        const double newIaB = newCorner.value(QStringLiteral("iaRefB_mA")).toDouble(0.0);
        const double oldIaB = oldCorner.value(QStringLiteral("iaRefB_mA")).toDouble(0.0);
        const double iaBAccum = accumulateScalar(oldIaB, newIaB);
        if (iaBAccum > 0.0) {
            merged.insert(QStringLiteral("iaRefB_mA"), iaBAccum);
        }

        mergedCorners.append(merged);
    }

    healthRefObj.insert(QStringLiteral("version"), 1);
    healthRefObj.insert(QStringLiteral("sampleCount"), prevCount + 1);
    healthRefObj.insert(QStringLiteral("corners"), mergedCorners);
    datasheetJson.insert(QStringLiteral("healthReference"), healthRefObj);

    return true;
}

void ValveWorkbench::on_actionSave_as_Reference_Tube_triggered()
{
    if (!captureHealthReferenceFromLastRun()) {
        return;
    }

    updateDatasheetDisplay();

    on_pushButton_4_clicked();
}

void ValveWorkbench::on_actionReset_Reference_Tube_triggered()
{
    if (datasheetJson.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Reset Reference Tube"),
                             tr("No datasheet block is loaded. Load a template first."));
        return;
    }

    // Remove the stored healthReference block (including any sampleCount) so
    // future Health runs and saves start from a clean slate.
    datasheetJson.remove(QStringLiteral("healthReference"));

    // Refresh the datasheet and Health Reference columns so the UI no longer
    // shows any stale reference-tube metrics.
    updateDatasheetDisplay();

    if (ui && ui->statusbar) {
        ui->statusbar->showMessage(tr("Reference tube calibration has been reset for this template."), 8000);
    }

    // Persist the cleared reference into the template using the existing
    // "Save Template" handler.
    on_pushButton_4_clicked();
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

void ValveWorkbench::on_modellingTestsButton_clicked()
{
    if (!analyser) {
        QMessageBox::warning(this, tr("Modelling Tests"), tr("Analyser is not initialised."));
        return;
    }

    if (ui && ui->runButton && ui->runButton->isChecked()) {
        QMessageBox::warning(this, tr("Modelling Tests"), tr("A test is already running. Please wait for it to finish."));
        return;
    }

    // Prompt once for project details (create or update current project)
    ProjectDialog dialog;
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (currentProject == nullptr) {
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
        Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
        if (project != nullptr) {
            project->setName(dialog.getName());
            project->setDeviceType(dialog.getDeviceType());
        }
        currentProject->setText(0, dialog.getName());
    }

    Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
    if (!project) {
        QMessageBox::warning(this, tr("Modelling Tests"), tr("Invalid project node."));
        return;
    }

    modellingStateSaved = true;
    savedDeviceTypeForModelling = deviceType;
    savedIsTriodeConnectedForModelling = isTriodeConnectedPentode;
    savedTestTypeForModelling = testType;
    savedAnodeStartForModelling = anodeStart;
    savedAnodeStopForModelling = anodeStop;
    savedAnodeStepForModelling = anodeStep;
    savedGridStartForModelling = gridStart;
    savedGridStopForModelling = gridStop;
    savedGridStepForModelling = gridStep;
    savedScreenStartForModelling = screenStart;
    savedScreenStopForModelling = screenStop;
    savedScreenStepForModelling = screenStep;
    savedIaMaxForModelling = iaMax;
    savedPMaxForModelling = pMax;

    modellingSteps.clear();
    modellingRunIndex = 0;
    modellingRunActive = true;

    {
        ModellingTestStep s;
        s.label = tr("Triode-connected anode");
        s.deviceType = PENTODE;
        s.testType = ANODE_CHARACTERISTICS;
        s.triodeConnectedPentode = true;
        s.anodeStart = 0.0;
        s.anodeStop = 250.0;
        s.anodeStep = 5.0;
        s.gridStart = 35.0;
        s.gridStop = 2.0;
        s.gridStep = 1.0;
        s.screenStart = 0.0;
        s.screenStop = 0.0;
        s.screenStep = 0.0;
        modellingSteps.append(s);
    }

    {
        const double transferVg2 = (std::isfinite(savedScreenStartForModelling) && savedScreenStartForModelling > 0.0)
                                      ? savedScreenStartForModelling
                                      : 150.0;
        ModellingTestStep s;
        s.label = tr("Pentode transfer (Va=200V, Vg2=%1V)").arg(QString::number(transferVg2, 'f', 0));
        s.deviceType = PENTODE;
        s.testType = TRANSFER_CHARACTERISTICS;
        s.triodeConnectedPentode = false;
        s.anodeStart = 200.0;
        s.anodeStop = 200.0;
        s.anodeStep = 200.0;
        s.gridStart = 2.0;
        s.gridStop = 35.0;
        s.gridStep = 1.0;
        s.screenStart = transferVg2;
        s.screenStop = transferVg2;
        s.screenStep = std::max(1.0, std::fabs(transferVg2));
        modellingSteps.append(s);
    }

    {
        const double transferVg2 = (std::isfinite(savedScreenStartForModelling) && savedScreenStartForModelling > 0.0)
                                      ? savedScreenStartForModelling
                                      : 150.0;
        ModellingTestStep s;
        s.label = tr("Pentode transfer 2 (Va=200V, Vg2=%1V)").arg(QString::number(transferVg2, 'f', 0));
        s.deviceType = PENTODE;
        s.testType = TRANSFER_CHARACTERISTICS;
        s.triodeConnectedPentode = false;
        s.anodeStart = 200.0;
        s.anodeStop = 200.0;
        s.anodeStep = 200.0;
        s.gridStart = 2.0;
        s.gridStop = 35.0;
        s.gridStep = 1.0;
        s.screenStart = transferVg2;
        s.screenStop = transferVg2;
        s.screenStep = std::max(1.0, std::fabs(transferVg2));
        modellingSteps.append(s);
    }

    {
        const double transferVg2 = (std::isfinite(savedScreenStartForModelling) && savedScreenStartForModelling > 0.0)
                                      ? savedScreenStartForModelling
                                      : 150.0;
        ModellingTestStep s;
        s.label = tr("Pentode transfer 3 (Va=200V, Vg2=%1V)").arg(QString::number(transferVg2, 'f', 0));
        s.deviceType = PENTODE;
        s.testType = TRANSFER_CHARACTERISTICS;
        s.triodeConnectedPentode = false;
        s.anodeStart = 200.0;
        s.anodeStop = 200.0;
        s.anodeStep = 200.0;
        s.gridStart = 2.0;
        s.gridStop = 35.0;
        s.gridStep = 1.0;
        s.screenStart = transferVg2;
        s.screenStop = transferVg2;
        s.screenStep = std::max(1.0, std::fabs(transferVg2));
        modellingSteps.append(s);
    }

    QList<double> vg2List;
    {
        auto addUnique = [&](double vg2) {
            if (!(std::isfinite(vg2) && vg2 > 0.0)) {
                return;
            }
            for (double existing : vg2List) {
                if (std::fabs(existing - vg2) < 1e-6) {
                    return;
                }
            }
            vg2List.append(vg2);
        };

        // Ensure we always run at the user's current/default screen voltage
        // (i.e. the value that was in the UI before starting Modelling Tests).
        addUnique(savedScreenStartForModelling);

        // Keep a couple of standard Vg2 points for coverage.
        addUnique(150.0);
        addUnique(200.0);
    }

    for (double vg2 : vg2List) {
        ModellingTestStep s;
        s.label = tr("Pentode anode (Vg2=%1V)").arg(QString::number(vg2, 'f', 0));
        s.deviceType = PENTODE;
        s.testType = ANODE_CHARACTERISTICS;
        s.triodeConnectedPentode = false;
        s.anodeStart = 0.0;
        s.anodeStop = 400.0;
        s.anodeStep = 5.0;
        s.gridStart = 35.0;
        s.gridStop = 2.0;
        s.gridStep = 1.0;
        s.screenStart = vg2;
        s.screenStop = vg2;
        s.screenStep = vg2;
        modellingSteps.append(s);
    }

    applyModellingStep(modellingSteps.at(0));
    on_runButton_clicked();
}

void ValveWorkbench::applyModellingStep(const ModellingTestStep &s)
{
    int deviceIndex = -1;
    for (int i = 0; i < ui->deviceType->count(); ++i) {
        if (ui->deviceType->itemData(i).toInt() != s.deviceType) {
            continue;
        }
        if (s.triodeConnectedPentode) {
            if (ui->deviceType->itemText(i) == QLatin1String("Triode-Connected Pentode")) {
                deviceIndex = i;
                break;
            }
        } else {
            if (s.deviceType == PENTODE && ui->deviceType->itemText(i) == QLatin1String("Pentode")) {
                deviceIndex = i;
                break;
            }
            if (s.deviceType == TRIODE && ui->deviceType->itemText(i) == QLatin1String("Triode")) {
                deviceIndex = i;
                break;
            }
            if (s.deviceType == DIODE && ui->deviceType->itemText(i) == QLatin1String("Diode")) {
                deviceIndex = i;
                break;
            }
        }
    }
    if (deviceIndex < 0) {
        deviceIndex = 0;
    }

    ui->deviceType->setCurrentIndex(deviceIndex);
    on_deviceType_currentIndexChanged(deviceIndex);

    int testIndex = -1;
    for (int i = 0; i < ui->testType->count(); ++i) {
        if (ui->testType->itemData(i).toInt() == s.testType) {
            testIndex = i;
            break;
        }
    }
    if (testIndex >= 0) {
        ui->testType->setCurrentIndex(testIndex);
        on_testType_currentIndexChanged(testIndex);
    }

    testType = s.testType;
    anodeStart = s.anodeStart;
    anodeStop  = s.anodeStop;
    anodeStep  = s.anodeStep;
    gridStart  = s.gridStart;
    gridStop   = s.gridStop;
    gridStep   = s.gridStep;
    screenStart = s.screenStart;
    screenStop  = s.screenStop;
    screenStep  = s.screenStep;

    iaMax = std::max(iaMax, 80.0);
    pMax  = std::max(pMax, 20.0);

    updateParameterDisplay();
}

void ValveWorkbench::restoreModellingState()
{
    if (!modellingStateSaved) {
        return;
    }

    deviceType = savedDeviceTypeForModelling;
    isTriodeConnectedPentode = savedIsTriodeConnectedForModelling;

    int deviceIndex = -1;
    for (int i = 0; i < ui->deviceType->count(); ++i) {
        if (ui->deviceType->itemData(i).toInt() != deviceType) {
            continue;
        }
        if (isTriodeConnectedPentode) {
            if (ui->deviceType->itemText(i) == QLatin1String("Triode-Connected Pentode")) {
                deviceIndex = i;
                break;
            }
        } else {
            deviceIndex = i;
            if (deviceType == PENTODE && ui->deviceType->itemText(i) == QLatin1String("Pentode")) break;
            if (deviceType == TRIODE && ui->deviceType->itemText(i) == QLatin1String("Triode")) break;
            if (deviceType == DIODE && ui->deviceType->itemText(i) == QLatin1String("Diode")) break;
        }
    }
    if (deviceIndex < 0) {
        deviceIndex = 0;
    }
    ui->deviceType->setCurrentIndex(deviceIndex);
    on_deviceType_currentIndexChanged(deviceIndex);

    int testIndex = -1;
    for (int i = 0; i < ui->testType->count(); ++i) {
        if (ui->testType->itemData(i).toInt() == savedTestTypeForModelling) {
            testIndex = i;
            break;
        }
    }
    if (testIndex >= 0) {
        ui->testType->setCurrentIndex(testIndex);
        on_testType_currentIndexChanged(testIndex);
    }

    testType = savedTestTypeForModelling;
    anodeStart = savedAnodeStartForModelling;
    anodeStop  = savedAnodeStopForModelling;
    anodeStep  = savedAnodeStepForModelling;
    gridStart  = savedGridStartForModelling;
    gridStop   = savedGridStopForModelling;
    gridStep   = savedGridStepForModelling;
    screenStart = savedScreenStartForModelling;
    screenStop  = savedScreenStopForModelling;
    screenStep  = savedScreenStepForModelling;
    iaMax = savedIaMaxForModelling;
    pMax  = savedPMaxForModelling;

    modellingStateSaved = false;
    updateParameterDisplay();
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

    // Name: prefer the filename base (what the user actually picked) and
    // fall back to the JSON 'name' if no usable base name is available.
    if (ui && ui->deviceName) {
        const QString baseName = QFileInfo(fileName).baseName().trimmed();
        const QString jsonName = obj.value("name").toString().trimmed();
        const QString chosenName = !baseName.isEmpty() ? baseName
                                                       : (!jsonName.isEmpty() ? jsonName
                                                                              : QStringLiteral("Device"));
        ui->deviceName->setText(chosenName);
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

        // Cache all per-test snapshots so switching testType in the UI can
        // restore the appropriate ranges/limits without clobbering others.
        const bool hasMeasurement = obj.contains("measurement") && obj.value("measurement").isObject();
        const QJsonObject testsObj = defs.value("tests").toObject();
        analyserTestsDefaults = testsObj;

        // For pure templates (no embedded measurement), also apply the
        // snapshot corresponding to the saved test type immediately so the
        // initial UI state matches what was last saved for that test.
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
        QJsonObject snapshot;
        snapshot.insert("testType", testType);

        auto sanitiseRange = [](double &start, double &stop, double &step) {
            if (!std::isfinite(start)) start = 0.0;
            if (!std::isfinite(stop))  stop  = start;
            if (!std::isfinite(step))  step  = 0.0;
            // Treat near-zero/denormal steps as "unspecified".
            if (std::fabs(step) < 1e-12) {
                step = 0.0;
            }
            // If range is effectively fixed, store as start==stop with sentinel step=0.
            if (std::fabs(stop - start) < 1e-9) {
                stop = start;
                step = 0.0;
            }
        };

        // For triode transfer characteristics, conceptually treat the anode
        // as fixed at a single Va. Persist that intent in the template by
        // recording a range where start == stop and a sentinel step of 0.0;
        // runtime code will normalise step to a sensible default while still
        // generating only one anode family.
        double snapAnodeStart = anodeStart;
        double snapAnodeStop  = anodeStop;
        double snapAnodeStep  = anodeStep;
        if (testType == TRANSFER_CHARACTERISTICS && deviceType != PENTODE) {
            snapAnodeStop = snapAnodeStart;
            snapAnodeStep = 0.0;
        }

        double snapGridStart = gridStart;
        double snapGridStop  = gridStop;
        double snapGridStep  = gridStep;

        double snapScreenStart = screenStart;
        double snapScreenStop  = screenStop;
        double snapScreenStep  = screenStep;

        // Pentode anode/transfer tests use a fixed screen voltage. The UI disables
        // some screen fields depending on test type, and stale/denormal values can
        // otherwise leak into the saved template.
        if (deviceType == PENTODE && testType != SCREEN_CHARACTERISTICS) {
            snapScreenStop = snapScreenStart;
            snapScreenStep = 0.0;
        }

        sanitiseRange(snapAnodeStart, snapAnodeStop, snapAnodeStep);
        sanitiseRange(snapGridStart, snapGridStop, snapGridStep);
        sanitiseRange(snapScreenStart, snapScreenStop, snapScreenStep);

        snapshot.insert("anode",  makeRange(snapAnodeStart, snapAnodeStop, snapAnodeStep));
        snapshot.insert("grid",   makeRange(snapGridStart, snapGridStop, snapGridStep));
        snapshot.insert("screen", makeRange(snapScreenStart, snapScreenStop, snapScreenStep));
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

        // Merge with any existing per-test defaults we may have loaded from
        // the template/device so previously saved test settings are
        // preserved when updating just one test type.
        QJsonObject testsObj = analyserTestsDefaults;
        testsObj.insert(key, snapshot);
        analyserTestsDefaults = testsObj;
        defs.insert("tests", testsObj);
    }

    obj.insert("analyserDefaults", defs);

    // Sync any edited datasheet/reference values from the Analyser UI back
    // into the datasheetJson block before saving.
    syncDatasheetFromUi();
    if (!datasheetJson.isEmpty()) {
        obj.insert("datasheet", datasheetJson);
    }

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

    {
        QFile existing(fileName);
        if (existing.exists() && existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QByteArray bytes = existing.readAll();
            existing.close();
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                const QJsonObject prev = doc.object();
                const QString prevDeviceType = prev.value(QStringLiteral("deviceType")).toString();
                const bool looksLikeDevice = prev.contains(QStringLiteral("model")) ||
                                             prev.contains(QStringLiteral("measurement")) ||
                                             prev.contains(QStringLiteral("triodeModel")) ||
                                             prev.contains(QStringLiteral("spice"));

                if (looksLikeDevice && !prevDeviceType.isEmpty()) {
                    static const QStringList preserveKeys = {
                        QStringLiteral("vaMax"),
                        QStringLiteral("vg1Max"),
                        QStringLiteral("vg2Max"),
                        QStringLiteral("iaMax"),
                        QStringLiteral("paMax"),
                        QStringLiteral("model"),
                        QStringLiteral("measurement"),
                        QStringLiteral("triodeModel"),
                        QStringLiteral("spice")
                    };

                    for (const QString &k : preserveKeys) {
                        if (prev.contains(k) && !obj.contains(k)) {
                            obj.insert(k, prev.value(k));
                        }
                    }
                }
            }
        }
    }
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Save Template"), tr("Could not write file."));
        return;
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
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

        // Preserve the nominal anode voltage for this sweep so transfer plots
        // for the Triode B clone are labelled with the same Va as the source
        // measurement instead of defaulting to 0.0 V.
        cloneSweep->setVaNominal(sourceSweep->getVaNominal());

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

    if (tabRole == 1 || tabRole == 2) {
        // Modeller tab: apply to the active project measurement and redraw the
        // measured plot (axes managed by Measurement itself).
        if (isDoubleTriode) {
            const bool measurementVisible = ui->measureCheck && ui->measureCheck->isChecked();
            if (triodeMeasurementSecondary) {
                if (!measuredCurvesSecondary && show) {
                    triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());
                    measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
                    if (measuredCurvesSecondary) {
                        plot.add(measuredCurvesSecondary);
                    }
                }
                if (measuredCurvesSecondary) {
                    measuredCurvesSecondary->setVisible(show && measurementVisible);
                }
            }
        } else if (currentMeasurement != nullptr) {
            currentMeasurement->setShowScreen(show);
            currentMeasurement->setSmoothPlotting(preferencesDialog.smoothCurves());
            if (measuredCurves != nullptr) {
                plot.remove(measuredCurves);
                measuredCurves = nullptr;
            }
            if (measuredCurvesSecondary != nullptr) {
                plot.remove(measuredCurvesSecondary);
                measuredCurvesSecondary = nullptr;
            }

            measuredCurves = currentMeasurement->updatePlot(&plot);
            if (measuredCurves) {
                plot.add(measuredCurves);
                measuredCurves->setVisible(ui->measureCheck && ui->measureCheck->isChecked());
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

    // Narrow the Analyser Triode A/B health value columns (Measured / Ref / Pct)
    // so they visually behave like ~4-digit fields.
    auto narrowHealthField = [&](QLineEdit *edit) {
        if (!edit) return;
        edit->setMaxLength(4);
        edit->setMaximumWidth(40);
        edit->setAlignment(Qt::AlignCenter);
    };

    auto wideHealthField = [&](QLineEdit *edit) {
        if (!edit) return;
        edit->setMaxLength(7);
        edit->setMaximumWidth(60);
        edit->setAlignment(Qt::AlignCenter);
    };

    // Triode A health rows
    narrowHealthField(ui->triodeA_Ia_measured);
    narrowHealthField(ui->triodeA_Ia_ref);
    narrowHealthField(ui->triodeA_Ia_pct);
    wideHealthField(ui->triodeA_rp_measured);
    wideHealthField(ui->triodeA_rp_ref);
    narrowHealthField(ui->triodeA_rp_pct);
    narrowHealthField(ui->triodeA_gm_measured);
    narrowHealthField(ui->triodeA_gm_ref);
    narrowHealthField(ui->triodeA_gm_pct);
    narrowHealthField(ui->triodeA_mu_measured);
    narrowHealthField(ui->triodeA_mu_ref);
    narrowHealthField(ui->triodeA_mu_pct);
    narrowHealthField(ui->triodeA_corner1_pct);
    narrowHealthField(ui->triodeA_corner2_pct);
    narrowHealthField(ui->triodeA_corner3_pct);
    narrowHealthField(ui->triodeA_corner4_pct);

    // Triode B health rows
    narrowHealthField(ui->triodeB_Ia_measured);
    narrowHealthField(ui->triodeB_Ia_ref);
    narrowHealthField(ui->triodeB_Ia_pct);
    wideHealthField(ui->triodeB_rp_measured);
    wideHealthField(ui->triodeB_rp_ref);
    narrowHealthField(ui->triodeB_rp_pct);
    narrowHealthField(ui->triodeB_gm_measured);
    narrowHealthField(ui->triodeB_gm_ref);
    narrowHealthField(ui->triodeB_gm_pct);
    narrowHealthField(ui->triodeB_mu_measured);
    narrowHealthField(ui->triodeB_mu_ref);
    narrowHealthField(ui->triodeB_mu_pct);
    narrowHealthField(ui->triodeB_corner1_pct);
    narrowHealthField(ui->triodeB_corner2_pct);
    narrowHealthField(ui->triodeB_corner3_pct);
    narrowHealthField(ui->triodeB_corner4_pct);

    // Color-code health fields by dependency so users can see which tests
    // drive which values:
    // - Quick Health only: Ia / gm measured and percent (blue).
    // - Anode-curve dependent: rp / mu measured and percent (green).
    // 4-corner Full Health column is styled in the .ui (orange border/text).
    auto setColor = [](QWidget *w, const char *rgb){
        if (!w) return;
        w->setStyleSheet(QStringLiteral("color: %1;").arg(QLatin1String(rgb)));
    };

    // Quick-health metrics: Ia and gm (Triode A & B)
    const char *quickColor = "rgb(0,0,192)";
    setColor(ui->triodeA_Ia_measured, quickColor);
    setColor(ui->triodeA_Ia_pct, quickColor);
    setColor(ui->triodeA_gm_measured, quickColor);
    setColor(ui->triodeA_gm_pct, quickColor);
    setColor(ui->triodeB_Ia_measured, quickColor);
    setColor(ui->triodeB_Ia_pct, quickColor);
    setColor(ui->triodeB_gm_measured, quickColor);
    setColor(ui->triodeB_gm_pct, quickColor);

    // Anode-dependent metrics: rp and mu (Triode A & B)
    const char *anodeColor = "rgb(0,128,0)";
    setColor(ui->triodeA_rp_measured, anodeColor);
    setColor(ui->triodeA_rp_pct, anodeColor);
    setColor(ui->triodeA_mu_measured, anodeColor);
    setColor(ui->triodeA_mu_pct, anodeColor);
    setColor(ui->triodeB_rp_measured, anodeColor);
    setColor(ui->triodeB_rp_pct, anodeColor);
    setColor(ui->triodeB_mu_measured, anodeColor);
    setColor(ui->triodeB_mu_pct, anodeColor);

    // Style the analyser health buttons to match their roles.
    if (ui->runButton) {
        ui->runButton->setStyleSheet("color: rgb(0,128,0);");
    }
    if (ui->quickHealthButton) {
        ui->quickHealthButton->setStyleSheet("color: rgb(0,0,192);");
    }
    if (ui->fullHealthButton) {
        ui->fullHealthButton->setStyleSheet("color: rgb(255,140,0);");
    }
    // Save to Project is also a required step in the workflow, so give it
    // the same green accent as other anode/required actions.
    if (ui->btnAddToProject) {
        ui->btnAddToProject->setStyleSheet("color: rgb(0,128,0);");
    }

    // Ensure all 5 columns in the Triode A/B health grids share width equally.
    if (ui->gridLayout_TriodeAHealth) {
        for (int c = 0; c < 5; ++c) {
            ui->gridLayout_TriodeAHealth->setColumnStretch(c, 1);
        }
    }
    if (ui->gridLayout_TriodeBHealth) {
        for (int c = 0; c < 5; ++c) {
            ui->gridLayout_TriodeBHealth->setColumnStretch(c, 1);
        }
    }

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
    // a dedicated row directly beneath the headroom helper display in the
    // left-hand Designer parameter column. Concretely, we insert a small
    // horizontal layout immediately after the last circuit row
    // (`horizontalLayout_23`, cir16), which Triode CC uses for the
    // HD3/HD5-at-headroom line, so the toggles visually sit "under" the
    // HD2/HD4 + HD3/HD5 metrics.
    if (ui->verticalLayout) {
        // Remove from the bottom toggle row if present so we don't keep them
        // in two places.
        auto removeIfIn = [&](QCheckBox *w){
            if (!w) return;
            if (ui->horizontalLayout_9 && ui->horizontalLayout_9->indexOf(w) >= 0) {
                ui->horizontalLayout_9->removeWidget(w);
            }
        };
        removeIfIn(symSwingCheck);
        removeIfIn(useBypassedGainCheck);

        // Determine insertion index: default to end, but prefer just after the
        // cir16 row (horizontalLayout_23) when available.
        int insertAt = ui->verticalLayout->count();
        if (ui->horizontalLayout_23) {
            int idx = ui->verticalLayout->indexOf(ui->horizontalLayout_23);
            if (idx >= 0) {
                insertAt = idx + 1;
            }
        }

        QHBoxLayout *designerTogglesRow = new QHBoxLayout();
        designerTogglesRow->addStretch();
        if (symSwingCheck) {
            designerTogglesRow->addWidget(symSwingCheck);
        }
        if (useBypassedGainCheck) {
            designerTogglesRow->addWidget(useBypassedGainCheck);
        }
        designerTogglesRow->addStretch();

        ui->verticalLayout->insertLayout(insertAt, designerTogglesRow);

        // Start with these toggles hidden; selectCircuit will decide when to
        // show them based on the active circuit type.
        if (symSwingCheck) {
            symSwingCheck->setVisible(false);
        }
        if (useBypassedGainCheck) {
            useBypassedGainCheck->setVisible(false);
        }
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

    const bool enableHarmonicsTab = false;
    if (enableHarmonicsTab) {
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
    }

    // Apply Data tab visibility based on Preferences (default off for release)
    {
        int dataIndex = -1;
        for (int i = 0; i < ui->tabWidget->count(); ++i) {
            if (ui->tabWidget->tabText(i) == QLatin1String("Data")) {
                dataIndex = i;
                break;
            }
        }
        if (dataIndex >= 0) {
            const bool showData = preferencesDialog.showDataTab();
            if (!showData && ui->tabWidget->currentIndex() == dataIndex) {
                ui->tabWidget->setCurrentIndex(0);
            }
            ui->tabWidget->setTabVisible(dataIndex, showData);
        }
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

        // Update basic UI selections to match the base model. The analyser
        // deviceName field represents the currently loaded template/preset,
        // not the default Device chosen at startup, so avoid stamping a
        // specific device label (e.g. "6N2P-EV") into it here. If no
        // template has populated the field yet, leave the text empty and
        // show an explicit placeholder instead.
        if (ui->deviceName) {
            if (ui->deviceName->text().trimmed().isEmpty()) {
                ui->deviceName->clear();
                ui->deviceName->setPlaceholderText(tr("(no template loaded)"));
            }
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

    if (ui->headroomWaveformView) {
        if (!headroomWaveformScene) {
            headroomWaveformScene = new QGraphicsScene(this);
        }
        ui->headroomWaveformView->setScene(headroomWaveformScene);
        headroomWaveformScene->clear();

        // Encourage the Headroom Waveshape viewer to use more of the
        // available vertical space in its group box so the Va(t) waveform
        // is easier to read at a glance, without forcing the group box to
        // extend beyond the available column height.
        ui->headroomWaveformView->setMinimumHeight(135);
        ui->headroomWaveformView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // Nudge the grey QGraphicsView area up slightly inside the Headroom
        // Waveshape group box so it is not visually clipped by the bottom
        // frame. A small bottom layout margin (~4 px) keeps a clear gap
        // without pushing the group box beyond the column.
        if (ui->headroomWaveformGroupBox && ui->headroomWaveformGroupBox->layout()) {
            ui->headroomWaveformGroupBox->layout()->setContentsMargins(0, 0, 0, 4);
        }
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
    circuits[TEST_CALCULATOR]         = new TriodeCcDccfTwoStage();
}

void ValveWorkbench::runHarmonicsScan()
{
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    auto fitHarmonicsViewToContents = [this]() {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }
        QRectF r = harmonicsView->scene()->itemsBoundingRect();
        if (!r.isValid() || r.isEmpty()) {
            return;
        }
        const double padX = std::max(10.0, r.width() * 0.05);
        const double padY = std::max(10.0, r.height() * 0.05);
        r.adjust(-padX, -padY, padX, padY);
        harmonicsView->setSceneRect(r);
        harmonicsView->fitInView(r, Qt::KeepAspectRatio);
    };

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
    } else if (auto *pp = dynamic_cast<PushPullOutput*>(c)) {
        harmonicsText->append(tr("Running Push-Pull time-domain harmonic scan..."));
        pp->computeTimeDomainHarmonicScan(headroomVals, hd2Vals, hd3Vals, hd4Vals, thdVals);
    } else {
        harmonicsText->append(tr("Harmonic scan is currently implemented for the Single Ended Output, Triode Common Cathode, and Push-Pull Output circuits only.\nPlease select one of these circuits in the Designer tab and choose a device."));
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

    // If a few clipping points explode THD, the plot becomes unusable.
    // Cap the plotted Y range to keep the low-distortion region readable,
    // but still report the true max in the text output.
    double yMaxPlot = yMax;
    bool yCapped = false;
    if (yMaxPlot > 20.0) {
        yMaxPlot = 20.0;
        yCapped = true;
    }

    auto niceStep = [](double rawStep) -> double {
        if (!(rawStep > 0.0) || !std::isfinite(rawStep)) {
            return 1.0;
        }
        const double exp10 = std::pow(10.0, std::floor(std::log10(rawStep)));
        const double f = rawStep / exp10;
        double nf = 1.0;
        if (f <= 1.0) nf = 1.0;
        else if (f <= 2.0) nf = 2.0;
        else if (f <= 5.0) nf = 5.0;
        else nf = 10.0;
        return nf * exp10;
    };

    // Ensure monotonic axis mapping even if the simulation returns points in descending order.
    //
    // IMPORTANT: We must ignore NaN/inf samples when computing axis bounds.
    // std::minmax_element does not handle NaNs in a useful way (comparisons return false),
    // which can leave xStart/xStop as NaN and cause xScale to blow up. The primary symptom
    // is markers/labels appearing far off to the left of the plot.
    double xStart = 0.0;
    double xStop = 1.0;
    bool haveX = false;
    for (double x : headroomVals) {
        if (!std::isfinite(x)) {
            continue;
        }
        if (!haveX) {
            xStart = x;
            xStop = x;
            haveX = true;
        } else {
            xStart = std::min(xStart, x);
            xStop = std::max(xStop, x);
        }
    }
    if (!haveX || !(xStop > xStart)) {
        xStart = 0.0;
        xStop = xStart + 1.0;
    }
    const double xMajor = niceStep((xStop - xStart) / 6.0);
    const double yStart = 0.0;
    const double yStop  = yMaxPlot * 1.05;
    const double yMajor = niceStep((yStop - yStart) / 6.0);

    harmonicsPlot.clear();
    harmonicsPlot.setAxes(xStart, xStop, xMajor, yStart, yStop, yMajor);

    // Axis titles are not part of Plot::setAxes(); add them here so the plot can be understood
    // without having to infer what the axes represent.
    //
    // These titles ignore view scaling so they remain readable after fitInView().
    auto addAxisTitles = [&](const QString &xTitle, const QString &yTitle) {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }

        QGraphicsScene *scene = harmonicsView->scene();

        auto *xText = scene->addText(xTitle);
        xText->setDefaultTextColor(Qt::black);
        xText->setZValue(900.0);
        xText->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        xText->setPos(PLOT_WIDTH * 0.5 - xText->boundingRect().width() * 0.5, PLOT_HEIGHT + 36.0);

        auto *yText = scene->addText(yTitle);
        yText->setDefaultTextColor(Qt::black);
        yText->setZValue(900.0);
        yText->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        yText->setRotation(-90.0);
        yText->setPos(-70.0, PLOT_HEIGHT * 0.5 + yText->boundingRect().width() * 0.5);
    };

    addAxisTitles(QStringLiteral("Headroom (Vpk)"), QStringLiteral("Distortion (%)"));

    // Helper: if the X array contains NaN/inf at a particular index (can happen when the simulator
    // discards a sample), pick the nearest finite X so markers stay on-plot.
    auto nearestFiniteX = [](const QVector<double> &xs, int idx) -> double {
        if (idx < 0 || idx >= xs.size()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (std::isfinite(xs[idx])) {
            return xs[idx];
        }
        for (int d = 1; d < xs.size(); ++d) {
            const int lo = idx - d;
            const int hi = idx + d;
            if (lo >= 0 && std::isfinite(xs[lo])) {
                return xs[lo];
            }
            if (hi < xs.size() && std::isfinite(xs[hi])) {
                return xs[hi];
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    };

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

    // Harmonics headroom scan visualization
    //
    // We plot a *family* of distortion curves vs headroom (Vpk):
    // - HD2 (even) in blue
    // - HD3 (odd)  in green
    // - HD4 (even) in brown
    // - THD        in red
    //
    // The goal is not only to show distortion rising with headroom, but to make it easier to pick
    // an operating point where:
    // - even harmonics are strong (for "warmth" / richness)
    // - odd harmonics are suppressed (to avoid "harshness")
    //
    // Markers used on this plot:
    // - Min THD (circle ○): lowest total distortion.
    // - Max Even (square □): maximizes (HD2+HD4).
    // - Min Odd  (triangle ▽): minimizes HD3.
    // - Max Even/Odd (diamond ◇): maximizes (HD2+HD4)/HD3.
    //
    // Notes on Y-axis capping:
    // We may cap the displayed Y range for readability when a few extreme clipping points dominate.
    // When capped, we still report the *true* values in the text output, but we clamp marker Y
    // positions into the visible range so the markers remain visible.

    drawCurve(hd2Vals, QColor::fromRgb(0, 0, 255));      // HD2 blue
    drawCurve(hd3Vals, QColor::fromRgb(0, 128, 0));      // HD3 green
    drawCurve(hd4Vals, QColor::fromRgb(165, 42, 42));    // HD4 brown
    drawCurve(thdVals, QColor::fromRgb(255, 0, 0));      // THD red

    auto addLegend = [this]() {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }

        QGraphicsScene *scene = harmonicsView->scene();
        const double legendW = 270.0;
        const double legendRowH = 26.0;
        const double legendPad = 8.0;
        const int rows = 8;
        const double legendH = legendPad * 2.0 + rows * legendRowH;

        const double x0 = PLOT_WIDTH - legendW - 10.0;
        const double y0 = 10.0;

        auto *box = scene->addRect(x0, y0, legendW, legendH, QPen(Qt::black), QBrush(QColor(255, 255, 255, 220)));
        box->setZValue(1000.0);
        box->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        struct Entry { QString label; QColor color; };
        const Entry entries[] = {
            { QStringLiteral("HD2"), QColor::fromRgb(0, 0, 255) },
            { QStringLiteral("HD3"), QColor::fromRgb(0, 128, 0) },
            { QStringLiteral("HD4"), QColor::fromRgb(165, 42, 42) },
            { QStringLiteral("THD"), QColor::fromRgb(255, 0, 0) },
            { QStringLiteral("Min THD (\u25CB)"), QColor::fromRgb(255, 140, 0) },
            { QStringLiteral("Max Even (\u25A1)"), QColor::fromRgb(0, 0, 255) },
            { QStringLiteral("Min Odd (\u25BD)"), QColor::fromRgb(0, 128, 0) },
            { QStringLiteral("Max Even/Odd (\u25C7)"), QColor::fromRgb(128, 0, 128) },
        };

        for (int i = 0; i < rows; ++i) {
            const double y = y0 + legendPad + i * legendRowH + 4.0;
            QPen pen(entries[i].color);
            pen.setWidth(2);
            auto *swatch = scene->addLine(x0 + legendPad, y, x0 + legendPad + 26.0, y, pen);
            swatch->setZValue(1001.0);
            swatch->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

            auto *text = scene->addText(entries[i].label);
            text->setDefaultTextColor(entries[i].color);
            text->setPos(x0 + legendPad + 32.0, y - 10.0);
            text->setZValue(1001.0);
            text->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            text->setFlag(QGraphicsItem::ItemIsSelectable, false);
            text->setFlag(QGraphicsItem::ItemIsMovable, false);
        }
    };

    auto findMin = [](const QVector<double> &vals, int &idx, double &vmin) {
        idx = -1;
        vmin = std::numeric_limits<double>::infinity();
        for (int i = 0; i < vals.size(); ++i) {
            const double v = vals[i];
            if (std::isfinite(v) && v >= 0.0 && v < vmin) {
                vmin = v;
                idx = i;
            }
        }
    };

    auto findMaxRatio = [&](const QVector<double> &num, const QVector<double> &den, int &idx, double &best) {
        idx = -1;
        best = 0.0;
        const int n = std::min(num.size(), den.size());
        for (int i = 0; i < n; ++i) {
            const double a = num[i];
            const double b = den[i];
            if (!std::isfinite(a) || !std::isfinite(b) || a < 0.0 || b <= 0.0) {
                continue;
            }
            const double r = a / b;
            if (std::isfinite(r) && r > best) {
                best = r;
                idx = i;
            }
        }
    };

    int idxMinThd = -1;
    double minThd = 0.0;
    findMin(thdVals, idxMinThd, minThd);

    if (idxMinThd >= 0) {
        const double x = nearestFiniteX(headroomVals, idxMinThd);
        const double y = thdVals[idxMinThd];
        if (std::isfinite(x) && std::isfinite(y)) {
            auto *marker = harmonicsPlot.createLabel(x, y, 0.0, QColor::fromRgb(255, 140, 0));
            marker->setPlainText(QStringLiteral("\u25CB"));
            marker->setDefaultTextColor(QColor::fromRgb(255, 140, 0));
        }
        harmonicsText->append(tr("Min THD: %1% at Headroom=%2 Vpk")
                                  .arg(minThd, 0, 'f', 3)
                                  .arg(x, 0, 'f', 1));
    }

    int idxMaxRatio = -1;
    double bestRatio = 0.0;
    QVector<double> evenVals;
    QVector<double> oddVals;
    evenVals.reserve(hd2Vals.size());
    oddVals.reserve(hd2Vals.size());
    for (int i = 0; i < hd2Vals.size(); ++i) {
        const double hd2 = hd2Vals[i];
        const double hd3 = (i < hd3Vals.size()) ? hd3Vals[i] : 0.0;
        const double hd4 = (i < hd4Vals.size()) ? hd4Vals[i] : 0.0;
        // "Even" = sum of selected even harmonics present in this plot.
        // For headroom scan we choose HD2 and HD4.
        const double even = (std::isfinite(hd2) ? hd2 : 0.0) + (std::isfinite(hd4) ? hd4 : 0.0);
        // "Odd" = selected odd harmonic(s). For headroom scan we use HD3.
        const double odd = (std::isfinite(hd3) ? hd3 : 0.0);
        evenVals.append(even);
        oddVals.append(odd);
    }
    findMaxRatio(evenVals, oddVals, idxMaxRatio, bestRatio);

    int idxMaxEven = -1;
    double bestEven = 0.0;
    for (int i = 0; i < evenVals.size(); ++i) {
        const double v = evenVals[i];
        if (std::isfinite(v) && v >= 0.0 && (idxMaxEven < 0 || v > bestEven)) {
            bestEven = v;
            idxMaxEven = i;
        }
    }

    int idxMinOdd = -1;
    double bestOdd = std::numeric_limits<double>::infinity();
    for (int i = 0; i < oddVals.size(); ++i) {
        const double v = oddVals[i];
        if (std::isfinite(v) && v >= 0.0 && v < bestOdd) {
            bestOdd = v;
            idxMinOdd = i;
        }
    }

    if (idxMaxEven >= 0) {
        const double x = nearestFiniteX(headroomVals, idxMaxEven);
        const double yTrue = evenVals[idxMaxEven];
        // Clamp marker position into view if the plot Y axis is capped.
        // We still report the true value in the text output.
        const double yPlot = std::min(yTrue, yMaxPlot * 0.98);
        if (std::isfinite(x) && std::isfinite(yPlot)) {
            auto *marker = harmonicsPlot.createLabel(x, yPlot, 0.0, QColor::fromRgb(0, 0, 255));
            marker->setPlainText(QStringLiteral("\u25A1"));
            marker->setDefaultTextColor(QColor::fromRgb(0, 0, 255));
        }
        harmonicsText->append(tr("Max Even (HD2+HD4): %1% at Headroom=%2 Vpk")
                                  .arg(bestEven, 0, 'f', 3)
                                  .arg(x, 0, 'f', 1));
    }

    if (idxMinOdd >= 0) {
        const double x = nearestFiniteX(headroomVals, idxMinOdd);
        const double y = oddVals[idxMinOdd];
        if (std::isfinite(x) && std::isfinite(y)) {
            auto *marker = harmonicsPlot.createLabel(x, y, 0.0, QColor::fromRgb(0, 128, 0));
            marker->setPlainText(QStringLiteral("\u25BD"));
            marker->setDefaultTextColor(QColor::fromRgb(0, 128, 0));
        }
        harmonicsText->append(tr("Min Odd (HD3): %1% at Headroom=%2 Vpk")
                                  .arg(bestOdd, 0, 'f', 3)
                                  .arg(x, 0, 'f', 1));
    }
    if (idxMaxRatio >= 0) {
        const double x = nearestFiniteX(headroomVals, idxMaxRatio);
        const double y = hd2Vals[idxMaxRatio];
        if (std::isfinite(x) && std::isfinite(y)) {
            auto *marker = harmonicsPlot.createLabel(x, y, 0.0, QColor::fromRgb(128, 0, 128));
            marker->setPlainText(QStringLiteral("\u25C7"));
            marker->setDefaultTextColor(QColor::fromRgb(128, 0, 128));
        }
        const double even = (idxMaxRatio < evenVals.size()) ? evenVals[idxMaxRatio] : 0.0;
        const double odd = (idxMaxRatio < oddVals.size()) ? oddVals[idxMaxRatio] : 0.0;
        harmonicsText->append(tr("Max Even/Odd ratio: %1 at Headroom=%2 Vpk (Even=%3%, Odd=%4%)")
                                  .arg(bestRatio, 0, 'f', 3)
                                  .arg(x, 0, 'f', 1)
                                  .arg(even, 0, 'f', 3)
                                  .arg(odd, 0, 'f', 3));
    }

    harmonicsText->append(tr("Plotted HD2 (blue), HD3 (green), HD4 (brown), and THD (red) vs headroom (Vpk)."));
    if (yCapped) {
        harmonicsText->append(tr("Note: Y-axis display capped at %1% for readability (true max was %2%).")
                                  .arg(yMaxPlot, 0, 'f', 1)
                                  .arg(yMax, 0, 'f', 1));
    }

    addLegend();

    fitHarmonicsViewToContents();
}

void ValveWorkbench::runHarmonicsBiasSweep()
{
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    auto fitHarmonicsViewToContents = [this]() {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }
        QRectF r = harmonicsView->scene()->itemsBoundingRect();
        if (!r.isValid() || r.isEmpty()) {
            return;
        }
        const double padX = std::max(10.0, r.width() * 0.05);
        const double padY = std::max(10.0, r.height() * 0.05);
        r.adjust(-padX, -padY, padX, padY);
        harmonicsView->setSceneRect(r);
        harmonicsView->fitInView(r, Qt::KeepAspectRatio);
    };

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
    updateYMax(hd5Vals);
    updateYMax(thdVals);
    if (yMax <= 0.0) yMax = 1.0;

    // If a few clipping points explode THD, the plot becomes unusable.
    // Cap the plotted Y range to keep the low-distortion region readable,
    // but still report the true max in the text output.
    double yMaxPlot = yMax;
    bool yCapped = false;
    if (yMaxPlot > 20.0) {
        yMaxPlot = 20.0;
        yCapped = true;
    }

    auto niceStep = [](double rawStep) -> double {
        if (!(rawStep > 0.0) || !std::isfinite(rawStep)) {
            return 1.0;
        }
        const double exp10 = std::pow(10.0, std::floor(std::log10(rawStep)));
        const double f = rawStep / exp10;
        double nf = 1.0;
        if (f <= 1.0) nf = 1.0;
        else if (f <= 2.0) nf = 2.0;
        else if (f <= 5.0) nf = 5.0;
        else nf = 10.0;
        return nf * exp10;
    };

    // Ensure monotonic axis mapping even if the sweep produces points in descending order.
    // See runHarmonicsScan() for rationale (ignore NaN/inf samples to avoid a broken xScale).
    double xStart = 0.0;
    double xStop = 1.0;
    bool haveX = false;
    for (double x : iaVals) {
        if (!std::isfinite(x)) {
            continue;
        }
        if (!haveX) {
            xStart = x;
            xStop = x;
            haveX = true;
        } else {
            xStart = std::min(xStart, x);
            xStop = std::max(xStop, x);
        }
    }
    if (!haveX || !(xStop > xStart)) {
        xStart = 0.0;
        xStop = xStart + 1.0;
    }
    const double xMajor = niceStep((xStop - xStart) / 6.0);
    const double yStart = 0.0;
    const double yStop  = yMaxPlot * 1.05;
    const double yMajor = niceStep((yStop - yStart) / 6.0);

    harmonicsPlot.clear();
    harmonicsPlot.setAxes(xStart, xStop, xMajor, yStart, yStop, yMajor);

    // Axis titles are not part of Plot::setAxes(); add them here so the plot can be understood
    // without having to infer what the axes represent.
    // These titles ignore view scaling so they remain readable after fitInView().
    auto addAxisTitles = [&](const QString &xTitle, const QString &yTitle) {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }

        QGraphicsScene *scene = harmonicsView->scene();

        auto *xText = scene->addText(xTitle);
        xText->setDefaultTextColor(Qt::black);
        xText->setZValue(900.0);
        xText->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        xText->setPos(PLOT_WIDTH * 0.5 - xText->boundingRect().width() * 0.5, PLOT_HEIGHT + 36.0);

        auto *yText = scene->addText(yTitle);
        yText->setDefaultTextColor(Qt::black);
        yText->setZValue(900.0);
        yText->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        yText->setRotation(-90.0);
        yText->setPos(-70.0, PLOT_HEIGHT * 0.5 + yText->boundingRect().width() * 0.5);
    };

    addAxisTitles(QStringLiteral("Bias current IA (mA)"), QStringLiteral("Distortion (%)"));

    // Helper: if the X array contains NaN/inf at a particular index (can happen when the sweep
    // discards a sample), pick the nearest finite X so markers stay on-plot.
    auto nearestFiniteX = [](const QVector<double> &xs, int idx) -> double {
        if (idx < 0 || idx >= xs.size()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (std::isfinite(xs[idx])) {
            return xs[idx];
        }
        for (int d = 1; d < xs.size(); ++d) {
            const int lo = idx - d;
            const int hi = idx + d;
            if (lo >= 0 && std::isfinite(xs[lo])) {
                return xs[lo];
            }
            if (hi < xs.size() && std::isfinite(xs[hi])) {
                return xs[hi];
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    };

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

    // Harmonics bias sweep visualization
    //
    // X axis: bias current IA (mA)
    // Y axis: harmonic distortion magnitude (%)
    //
    // This view is intended to help pick a bias point with:
    // - higher even content (HD2+HD4)
    // - lower odd content (HD3+HD5)
    // plus basic sanity targets like low THD.
    //
    // Markers used on this plot:
    // - Peaks (●): per-curve maxima (for quick reference)
    // - Min THD (○): lowest THD across the sweep
    // - Max Even (□): max (HD2+HD4)
    // - Min Odd  (▽): min (HD3+HD5)
    // - Max Even/Odd (◇): max (HD2+HD4)/(HD3+HD5)
    harmonicsText->append(tr("Harmonic vs Operating Point Analysis:"));
    harmonicsText->append(tr("Individual harmonic curves vs bias current"));
    harmonicsText->append(tr("Blue=HD2, Green=HD3, Brown=HD4, Red=THD"));
    
    // Plot individual harmonics with distinct colors and labels
    drawCurve(hd2Vals, QColor::fromRgb(0, 0, 255));      // HD2 blue
    drawCurve(hd3Vals, QColor::fromRgb(0, 128, 0));      // HD3 green  
    drawCurve(hd4Vals, QColor::fromRgb(165, 42, 42));    // HD4 brown
    drawCurve(hd5Vals, QColor::fromRgb(128, 0, 128));    // HD5 purple
    drawCurve(thdVals, QColor::fromRgb(255, 0, 0));      // THD red

    auto addLegend = [this]() {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }

        QGraphicsScene *scene = harmonicsView->scene();
        const double legendW = 270.0;
        const double legendRowH = 26.0;
        const double legendPad = 8.0;
        const int rows = 10;
        const double legendH = legendPad * 2.0 + rows * legendRowH;

        const double x0 = PLOT_WIDTH - legendW - 10.0;
        const double y0 = 10.0;

        auto *box = scene->addRect(x0, y0, legendW, legendH, QPen(Qt::black), QBrush(QColor(255, 255, 255, 220)));
        box->setZValue(1000.0);
        box->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        struct Entry { QString label; QColor color; };
        const Entry entries[] = {
            { QStringLiteral("HD2"), QColor::fromRgb(0, 0, 255) },
            { QStringLiteral("HD3"), QColor::fromRgb(0, 128, 0) },
            { QStringLiteral("HD4"), QColor::fromRgb(165, 42, 42) },
            { QStringLiteral("HD5"), QColor::fromRgb(128, 0, 128) },
            { QStringLiteral("THD"), QColor::fromRgb(255, 0, 0) },
            { QStringLiteral("Peaks (\u25CF)"), QColor::fromRgb(255, 165, 0) },
            { QStringLiteral("Min THD (\u25CB)"), QColor::fromRgb(255, 140, 0) },
            { QStringLiteral("Max Even (\u25A1)"), QColor::fromRgb(0, 0, 255) },
            { QStringLiteral("Min Odd (\u25BD)"), QColor::fromRgb(0, 128, 0) },
            { QStringLiteral("Max Even/Odd (\u25C7)"), QColor::fromRgb(128, 0, 128) },
        };

        for (int i = 0; i < rows; ++i) {
            const double y = y0 + legendPad + i * legendRowH + 4.0;
            QPen pen(entries[i].color);
            pen.setWidth(2);
            auto *swatch = scene->addLine(x0 + legendPad, y, x0 + legendPad + 26.0, y, pen);
            swatch->setZValue(1001.0);
            swatch->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

            auto *text = scene->addText(entries[i].label);
            text->setDefaultTextColor(entries[i].color);
            text->setPos(x0 + legendPad + 32.0, y - 10.0);
            text->setZValue(1001.0);
            text->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            text->setFlag(QGraphicsItem::ItemIsSelectable, false);
            text->setFlag(QGraphicsItem::ItemIsMovable, false);
        }
    };
    
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
        const double x = nearestFiniteX(iaVals, peakIdx);
        harmonicsText->append(tr("%1 peak: %2% at IA=%3mA").arg(name).arg(peakVal, 0, 'f', 2).arg(x, 0, 'f', 1));
        
        // Mark peak on plot with circle
        harmonicsPlot.createLabel(x, peakVal, peakVal, QColor::fromRgb(255, 165, 0))->setPlainText("●");
    };
    
    findPeak(hd2Vals, "HD2");
    findPeak(hd3Vals, "HD3");
    findPeak(hd4Vals, "HD4");
    findPeak(hd5Vals, "HD5");
    findPeak(thdVals, "THD");

    // Also report/mark the minimum THD operating point as a recommended bias.
    int idxMinThd = -1;
    double minThd = std::numeric_limits<double>::infinity();
    for (int i = 0; i < thdVals.size(); ++i) {
        const double v = thdVals[i];
        if (std::isfinite(v) && v >= 0.0 && v < minThd) {
            minThd = v;
            idxMinThd = i;
        }
    }
    if (idxMinThd >= 0) {
        const double x = nearestFiniteX(iaVals, idxMinThd);
        const double y = thdVals[idxMinThd];
        if (std::isfinite(x) && std::isfinite(y)) {
            auto *marker = harmonicsPlot.createLabel(x, y, 0.0, QColor::fromRgb(255, 140, 0));
            marker->setPlainText(QStringLiteral("\u25CB"));
            marker->setDefaultTextColor(QColor::fromRgb(255, 140, 0));
        }
        harmonicsText->append(tr("Recommended bias (min THD): IA=%1 mA, THD=%2%")
                                  .arg(x, 0, 'f', 2)
                                  .arg(minThd, 0, 'f', 3));
    }

    // Also report a harmonic-balance bias point: maximize even harmonics while minimizing odd harmonics.
    int idxMaxEvenOdd = -1;
    double bestEvenOdd = 0.0;
    for (int i = 0; i < iaVals.size(); ++i) {
        const double hd2 = (i < hd2Vals.size()) ? hd2Vals[i] : 0.0;
        const double hd3 = (i < hd3Vals.size()) ? hd3Vals[i] : 0.0;
        const double hd4 = (i < hd4Vals.size()) ? hd4Vals[i] : 0.0;
        const double hd5 = (i < hd5Vals.size()) ? hd5Vals[i] : 0.0;
        const double even = (std::isfinite(hd2) ? hd2 : 0.0) + (std::isfinite(hd4) ? hd4 : 0.0);
        const double odd = (std::isfinite(hd3) ? hd3 : 0.0) + (std::isfinite(hd5) ? hd5 : 0.0);
        if (!std::isfinite(even) || !std::isfinite(odd) || even < 0.0 || odd < 0.0) {
            continue;
        }
        const double ratio = even / (odd + 1e-12);
        if (std::isfinite(ratio) && ratio > bestEvenOdd) {
            bestEvenOdd = ratio;
            idxMaxEvenOdd = i;
        }
    }
    if (idxMaxEvenOdd >= 0) {
        const double x = nearestFiniteX(iaVals, idxMaxEvenOdd);
        const double y = (idxMaxEvenOdd < hd2Vals.size()) ? hd2Vals[idxMaxEvenOdd] : 0.0;
        if (std::isfinite(x) && std::isfinite(y)) {
            auto *marker = harmonicsPlot.createLabel(x, y, 0.0, QColor::fromRgb(128, 0, 128));
            marker->setPlainText(QStringLiteral("\u25C7"));
            marker->setDefaultTextColor(QColor::fromRgb(128, 0, 128));
        }
        const double hd2 = (idxMaxEvenOdd < hd2Vals.size()) ? hd2Vals[idxMaxEvenOdd] : 0.0;
        const double hd3 = (idxMaxEvenOdd < hd3Vals.size()) ? hd3Vals[idxMaxEvenOdd] : 0.0;
        const double hd4 = (idxMaxEvenOdd < hd4Vals.size()) ? hd4Vals[idxMaxEvenOdd] : 0.0;
        const double hd5 = (idxMaxEvenOdd < hd5Vals.size()) ? hd5Vals[idxMaxEvenOdd] : 0.0;
        const double even = (std::isfinite(hd2) ? hd2 : 0.0) + (std::isfinite(hd4) ? hd4 : 0.0);
        const double odd = (std::isfinite(hd3) ? hd3 : 0.0) + (std::isfinite(hd5) ? hd5 : 0.0);
        harmonicsText->append(tr("Recommended bias (harmonic balance): IA=%1 mA, Even=%2%, Odd=%3%, Even/Odd=%4")
                                  .arg(x, 0, 'f', 2)
                                  .arg(even, 0, 'f', 3)
                                  .arg(odd, 0, 'f', 3)
                                  .arg(bestEvenOdd, 0, 'f', 3));
    }

    int idxMaxEven = -1;
    double bestEven = 0.0;
    int idxMinOdd = -1;
    double bestOdd = std::numeric_limits<double>::infinity();
    for (int i = 0; i < iaVals.size(); ++i) {
        const double hd2 = (i < hd2Vals.size()) ? hd2Vals[i] : 0.0;
        const double hd3 = (i < hd3Vals.size()) ? hd3Vals[i] : 0.0;
        const double hd4 = (i < hd4Vals.size()) ? hd4Vals[i] : 0.0;
        const double hd5 = (i < hd5Vals.size()) ? hd5Vals[i] : 0.0;
        const double even = (std::isfinite(hd2) ? hd2 : 0.0) + (std::isfinite(hd4) ? hd4 : 0.0);
        const double odd = (std::isfinite(hd3) ? hd3 : 0.0) + (std::isfinite(hd5) ? hd5 : 0.0);
        if (std::isfinite(even) && even >= 0.0 && (idxMaxEven < 0 || even > bestEven)) {
            bestEven = even;
            idxMaxEven = i;
        }
        if (std::isfinite(odd) && odd >= 0.0 && odd < bestOdd) {
            bestOdd = odd;
            idxMinOdd = i;
        }
    }

    if (idxMaxEven >= 0) {
        const double x = nearestFiniteX(iaVals, idxMaxEven);
        const double yPlot = std::min(bestEven, yMaxPlot * 0.98);
        if (std::isfinite(x) && std::isfinite(yPlot)) {
            auto *marker = harmonicsPlot.createLabel(x, yPlot, 0.0, QColor::fromRgb(0, 0, 255));
            marker->setPlainText(QStringLiteral("\u25A1"));
            marker->setDefaultTextColor(QColor::fromRgb(0, 0, 255));
        }
        harmonicsText->append(tr("Max Even (HD2+HD4): IA=%1 mA, Even=%2%")
                                  .arg(x, 0, 'f', 2)
                                  .arg(bestEven, 0, 'f', 3));
    }

    if (idxMinOdd >= 0) {
        const double x = nearestFiniteX(iaVals, idxMinOdd);
        if (std::isfinite(x) && std::isfinite(bestOdd)) {
            auto *marker = harmonicsPlot.createLabel(x, bestOdd, 0.0, QColor::fromRgb(0, 128, 0));
            marker->setPlainText(QStringLiteral("\u25BD"));
            marker->setDefaultTextColor(QColor::fromRgb(0, 128, 0));
        }
        harmonicsText->append(tr("Min Odd (HD3+HD5): IA=%1 mA, Odd=%2%")
                                  .arg(x, 0, 'f', 2)
                                  .arg(bestOdd, 0, 'f', 3));
    }

    harmonicsText->append(tr("Plotted individual harmonics vs bias current with peak markers."));
    if (yCapped) {
        harmonicsText->append(tr("Note: Y-axis display capped at %1% for readability (true max was %2%).")
                                  .arg(yMaxPlot, 0, 'f', 1)
                                  .arg(yMax, 0, 'f', 1));
    }

    addLegend();

    fitHarmonicsViewToContents();
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

    if (harmonicsView && harmonicsView->scene()) {
        QRectF r = harmonicsView->scene()->itemsBoundingRect();
        if (r.isValid() && !r.isEmpty()) {
            const double padX = std::max(10.0, r.width() * 0.05);
            const double padY = std::max(10.0, r.height() * 0.05);
            r.adjust(-padX, -padY, padX, padY);
            harmonicsView->setSceneRect(r);
            harmonicsView->fitInView(r, Qt::KeepAspectRatio);
        }
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
    // 3D Harmonic Waterfall ("mountain" surface)
    //
    // User intent:
    // - Render a 3D surface over a 2D operating region (bias vs headroom).
    // - Surface height encodes overall THD (bigger THD => higher peak).
    // - Surface color encodes even-vs-odd harmonic balance:
    //     Blue  = even-dominant
    //     Red   = odd-dominant
    //     Purple= balanced
    // - Provide an electrical way to identify "sweet spots" reminiscent of empirical biasing
    //   and push-pull balancing by ear.
    //
    // Implementation overview:
    // - Use existing SingleEndedOutput::computeHarmonicSurfaceData() which returns a bias×headroom grid
    //   of HD2..HD5 magnitudes.
    // - Compute THD = sqrt(HD2^2 + HD3^2 + HD4^2 + HD5^2) per grid point to define height.
    // - Compute even/odd balance ratio per point to drive red/blue color.
    // - Project 3D (bias, headroom, THD) to 2D using a lightweight oblique projection controlled
    //   by the rotation sliders already present on the Harmonics tab.
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    harmonicsText->clear();
    harmonicsText->append(tr("Generating 3D Harmonic Waterfall (dual mountain: Even vs Odd)..."));

    // Determine the currently selected Designer circuit.
    const int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0 || currentCircuitType >= circuits.size() || !circuits.at(currentCircuitType)) {
        harmonicsText->append(tr("No valid Designer circuit selected. Please select 'Single Ended Output' on the Designer tab."));
        return;
    }

    Circuit *circuit = circuits.at(currentCircuitType);
    auto *se = dynamic_cast<SingleEndedOutput*>(circuit);
    if (!se) {
        harmonicsText->append(tr("3D Waterfall is currently implemented for the Single Ended Output circuit only."));
        harmonicsText->append(tr("Please select 'Single Ended Output' in the Designer tab and choose a device."));
        return;
    }

    // Show rotation controls for the 3D plot.
    if (harmonicsRotationXSlider) {
        harmonicsRotationXSlider->show();
    }
    if (harmonicsRotationYSlider) {
        harmonicsRotationYSlider->show();
    }
    if (harmonicsTab) {
        if (auto rotationLabel = harmonicsTab->findChild<QLabel*>("rotationLabel")) {
            rotationLabel->show();
        }
        if (auto rotationXLabel = harmonicsTab->findChild<QLabel*>("rotationXLabel")) {
            rotationXLabel->show();
        }
        if (auto rotationYLabel = harmonicsTab->findChild<QLabel*>("rotationYLabel")) {
            rotationYLabel->show();
        }
    }

    // Gather harmonic surface data over a bias×headroom grid.
    QVector<double> biasPoints;
    QVector<double> headroomPoints;
    QVector<QVector<QVector<double>>> harmonicSurface;
    se->computeHarmonicSurfaceData(biasPoints, headroomPoints, harmonicSurface);

    // Expect 4 layers: HD2, HD3, HD4, HD5.
    if (biasPoints.isEmpty() || headroomPoints.isEmpty() || harmonicSurface.size() < 4) {
        harmonicsText->append(tr("No valid harmonic surface data generated (ensure device selected + reasonable parameters)."));
        return;
    }

    // Clear any previous plot.
    harmonicsPlot.clear();
    if (harmonicsView && harmonicsView->scene()) {
        harmonicsView->scene()->clear();
    }
    harmonicsView->setScene(harmonicsPlot.getScene());

    const int biasSteps = biasPoints.size();
    const int headSteps = headroomPoints.size();

    // Compute THD and even/odd components at each grid point.
    // even = HD2 + HD4
    // odd  = HD3 + HD5
    // balance = (even - odd) / (even + odd + eps) in [-1..1]
    QVector<QVector<double>> thdGrid(headSteps);
    QVector<QVector<double>> evenGrid(headSteps);
    QVector<QVector<double>> oddGrid(headSteps);
    QVector<QVector<double>> balanceGrid(headSteps);
    double thdMax = 0.0;
    double evenMax = 0.0;
    double oddMax = 0.0;
    for (int h = 0; h < headSteps; ++h) {
        thdGrid[h].resize(biasSteps);
        evenGrid[h].resize(biasSteps);
        oddGrid[h].resize(biasSteps);
        balanceGrid[h].resize(biasSteps);
        for (int b = 0; b < biasSteps; ++b) {
            const double hd2 = harmonicSurface[0][h][b];
            const double hd3 = harmonicSurface[1][h][b];
            const double hd4 = harmonicSurface[2][h][b];
            const double hd5 = harmonicSurface[3][h][b];
            const double thd = std::sqrt(hd2 * hd2 + hd3 * hd3 + hd4 * hd4 + hd5 * hd5);
            thdGrid[h][b] = thd;

            const double even = std::max(0.0, hd2) + std::max(0.0, hd4);
            const double odd = std::max(0.0, hd3) + std::max(0.0, hd5);
            evenGrid[h][b] = even;
            oddGrid[h][b] = odd;
            const double denom = even + odd + 1e-9;
            balanceGrid[h][b] = (even - odd) / denom;

            if (std::isfinite(thd)) {
                thdMax = std::max(thdMax, thd);
            }
            if (std::isfinite(even)) {
                evenMax = std::max(evenMax, even);
            }
            if (std::isfinite(odd)) {
                oddMax = std::max(oddMax, odd);
            }
        }
    }
    if (!(thdMax > 0.0) || !std::isfinite(thdMax)) {
        thdMax = 1.0;
    }

    // For the dual-surface rendering, Z is not THD; Z is the harmonic magnitude (Even or Odd).
    // We scale the projection by the maximum of the two so both surfaces share the same height scale.
    double zMax = std::max(evenMax, oddMax);
    if (!(zMax > 0.0) || !std::isfinite(zMax)) {
        zMax = 1.0;
    }

    // Projection parameters.
    // The existing sliders were originally treated as fractional parameters (default 0.6 and 0.3).
    // We keep that model: slider value in [-100..100] maps to [-1..1] for an oblique projection.
    const double depthX = harmonicsRotationXSlider ? (static_cast<double>(harmonicsRotationXSlider->value()) / 100.0) : 0.6;
    const double depthY = harmonicsRotationYSlider ? (static_cast<double>(harmonicsRotationYSlider->value()) / 100.0) : 0.3;

    // World scales in scene coordinates.
    // These are chosen to fit within the existing plot scene size while leaving room for labels.
    const double worldW = PLOT_WIDTH * 0.85;
    const double worldD = PLOT_HEIGHT * 0.60;
    const double worldH = PLOT_HEIGHT * 0.60;

    const double biasMin = biasPoints.first();
    const double biasMax = biasPoints.last();
    const double biasSpan = std::max(1e-9, biasMax - biasMin);

    // The surface is simulated over a bias×headroom grid, but the user wants the *axis* to be
    // anode voltage rather than headroom.
    //
    // We therefore map each sample to an anode-voltage coordinate that captures “how close to
    // clipping we are” as drive increases:
    //   Va(min) = Va_bias - headroomVpk
    //
    // This makes “more drive” move the surface toward lower Va(min), which is intuitive for
    // viewing distortion onset.
    QVector<QVector<double>> vaMinGrid(headSteps);
    double vaMinAxisMin = std::numeric_limits<double>::infinity();
    double vaMinAxisMax = -std::numeric_limits<double>::infinity();
    const double vb = se->getParameter(SE_VB);
    for (int h = 0; h < headSteps; ++h) {
        vaMinGrid[h].resize(biasSteps);
        const double head = headroomPoints[h];
        for (int b = 0; b < biasSteps; ++b) {
            const double bias = biasPoints[b];
            const double vaBias = se->estimateAnodeVoltageAtBias(bias);
            double vaMin = vaBias - head;
            if (!std::isfinite(vaMin)) {
                vaMin = 0.0;
            }
            vaMin = std::max(0.0, vaMin);
            if (vb > 0.0 && std::isfinite(vb)) {
                vaMin = std::min(vaMin, vb);
            }
            vaMinGrid[h][b] = vaMin;
            vaMinAxisMin = std::min(vaMinAxisMin, vaMin);
            vaMinAxisMax = std::max(vaMinAxisMax, vaMin);
        }
    }
    if (!std::isfinite(vaMinAxisMin) || !std::isfinite(vaMinAxisMax) || !(vaMinAxisMax > vaMinAxisMin)) {
        vaMinAxisMin = 0.0;
        vaMinAxisMax = std::max(1.0, vb);
    }
    const double vaSpan = std::max(1e-9, vaMinAxisMax - vaMinAxisMin);

    // Project 3D point (bias, Va(min), Z) into scene coordinates.
    // In this "dual mountain" mode:
    // - Z is the harmonic magnitude (%) for the surface being drawn (Even or Odd)
    //
    // Coordinate meaning:
    // - X = bias current (mA)
    // - Y = anode voltage Va(min) during the swing (V)
    // - Z = harmonic magnitude (%)
    //
    // Visual meaning:
    // - Lower Va(min) (more drive / deeper into clipping region) should move "into" the scene (depth).
    // - Higher THD should go "up" (smaller scene Y).
    const double marginL = 60.0;
    const double marginT = 40.0;
    const double baseY = marginT + worldH + std::abs(depthY) * worldD;

    auto project = [&](double bias_mA, double vaMin_V, double z_pct) -> QPointF {
        const double xn = (bias_mA - biasMin) / biasSpan;
        const double yn = (vaMin_V - vaMinAxisMin) / vaSpan;
        const double zn = std::clamp(z_pct / zMax, 0.0, 1.0);

        const double X = xn * worldW;
        const double Y = yn * worldD;
        const double Z = zn * worldH;

        const double sx = marginL + X + Y * depthX;
        const double sy = baseY - Z - Y * depthY;
        return QPointF(sx, sy);
    };

    // Map a surface height to a saturated color (keeps hue, scales intensity toward black).
    auto heightToColor = [&](const QColor &base, double zPct) -> QColor {
        const double zn = std::clamp(zPct / zMax, 0.0, 1.0);
        const double intensity = 0.25 + 0.75 * std::sqrt(zn); // 0.25..1.0
        int r = static_cast<int>(std::clamp(base.red() * intensity, 0.0, 255.0));
        int g = static_cast<int>(std::clamp(base.green() * intensity, 0.0, 255.0));
        int b = static_cast<int>(std::clamp(base.blue() * intensity, 0.0, 255.0));
        QColor c(r, g, b, 150);
        return c;
    };

    // Draw 3D surfaces as two meshes of quads.
    // Draw order matters: we draw from far-to-near in the depth axis so nearer polygons overpaint.
    QGraphicsScene *scene = harmonicsPlot.getScene();
    if (!scene) {
        harmonicsText->append(tr("Internal error: plot scene is null."));
        return;
    }

    QPen wirePen(QColor(0, 0, 0, 40));
    wirePen.setWidthF(0.0);

    const QColor evenBase = QColor::fromRgb(0, 0, 255);
    const QColor oddBase = QColor::fromRgb(255, 0, 0);

    for (int h = headSteps - 2; h >= 0; --h) {
        for (int b = 0; b < biasSteps - 1; ++b) {
            const double bias0 = biasPoints[b];
            const double bias1 = biasPoints[b + 1];

            const double ze00 = evenGrid[h][b];
            const double ze10 = evenGrid[h][b + 1];
            const double ze01 = evenGrid[h + 1][b];
            const double ze11 = evenGrid[h + 1][b + 1];

            const double zo00 = oddGrid[h][b];
            const double zo10 = oddGrid[h][b + 1];
            const double zo01 = oddGrid[h + 1][b];
            const double zo11 = oddGrid[h + 1][b + 1];

            const double va00 = vaMinGrid[h][b];
            const double va10 = vaMinGrid[h][b + 1];
            const double va01 = vaMinGrid[h + 1][b];
            const double va11 = vaMinGrid[h + 1][b + 1];

            // Odd surface (red)
            {
                const double zAvg = 0.25 * (zo00 + zo10 + zo01 + zo11);
                const QColor fill = heightToColor(oddBase, zAvg);
                QPolygonF poly;
                poly << project(bias0, va00, zo00)
                     << project(bias1, va10, zo10)
                     << project(bias1, va11, zo11)
                     << project(bias0, va01, zo01);
                auto *cell = scene->addPolygon(poly, wirePen, QBrush(fill));
                (void)cell;
            }

            // Even surface (blue)
            {
                const double zAvg = 0.25 * (ze00 + ze10 + ze01 + ze11);
                const QColor fill = heightToColor(evenBase, zAvg);
                QPolygonF poly;
                poly << project(bias0, va00, ze00)
                     << project(bias1, va10, ze10)
                     << project(bias1, va11, ze11)
                     << project(bias0, va01, ze01);
                auto *cell = scene->addPolygon(poly, wirePen, QBrush(fill));
                (void)cell;
            }
        }
    }

    // Operating-point bias sweep slice.
    //
    // The 3D surface is a 2D grid (bias × headroom). The user's goal is to bias the stage
    // at the *current* operating point (i.e., at the current headroom setting) to obtain
    // a preferred even/odd harmonic balance.
    //
    // Since the surface is discrete, we choose the nearest simulated headroom row to the
    // circuit's current SE_HEADROOM value and then sweep bias along that row.
    //
    // Selection rule (for this 1D slice): favor a point that is
    // - even-dominant (more blue than red) and
    // - higher THD (taller peak) at that headroom.
    //
    // This is intentionally aligned with the "Dumble sweet spot" idea: audible richness
    // often correlates with a strong even structure at moderate/high distortion.
    int headIdxOp = 0;
    {
        const double headTarget = se->getParameter(SE_HEADROOM);
        double bestDist = std::numeric_limits<double>::infinity();
        for (int h = 0; h < headroomPoints.size(); ++h) {
            const double d = std::abs(headroomPoints[h] - headTarget);
            if (std::isfinite(d) && d < bestDist) {
                bestDist = d;
                headIdxOp = h;
            }
        }
    }

    int bestBiasIdxOp = -1;
    double bestScoreOp = -std::numeric_limits<double>::infinity();
    for (int b = 0; b < biasSteps; ++b) {
        const double thd = thdGrid[headIdxOp][b];
        const double bal = balanceGrid[headIdxOp][b];
        if (!std::isfinite(thd) || thd <= 0.0 || !std::isfinite(bal)) {
            continue;
        }

        // Weight in [0..1] that prefers even dominance. bal in [-1..1].
        const double evenWeight = std::clamp(0.5 + 0.5 * bal, 0.0, 1.0);
        const double score = thd * evenWeight;
        if (score > bestScoreOp) {
            bestScoreOp = score;
            bestBiasIdxOp = b;
        }
    }

    if (bestBiasIdxOp >= 0) {
        const double bias = biasPoints[bestBiasIdxOp];
        const double head = headroomPoints[headIdxOp];
        const double vaBias = se->estimateAnodeVoltageAtBias(bias);
        const double vaMin = std::max(0.0, vaBias - head);

        const double hd2 = harmonicSurface[0][headIdxOp][bestBiasIdxOp];
        const double hd3 = harmonicSurface[1][headIdxOp][bestBiasIdxOp];
        const double hd4 = harmonicSurface[2][headIdxOp][bestBiasIdxOp];
        const double hd5 = harmonicSurface[3][headIdxOp][bestBiasIdxOp];
        const double even = (std::isfinite(hd2) ? std::max(0.0, hd2) : 0.0) + (std::isfinite(hd4) ? std::max(0.0, hd4) : 0.0);
        const double odd = (std::isfinite(hd3) ? std::max(0.0, hd3) : 0.0) + (std::isfinite(hd5) ? std::max(0.0, hd5) : 0.0);
        const double ratio = even / (odd + 1e-12);
        const double thd = thdGrid[headIdxOp][bestBiasIdxOp];
        const double bal = balanceGrid[headIdxOp][bestBiasIdxOp];

        // Place the marker on top of the two-surface mountain at this point.
        const double zMarker = std::max(even, odd);

        const QPointF p = project(bias, vaMin, zMarker);
        auto *marker = scene->addText(QStringLiteral("✚"));
        marker->setDefaultTextColor(Qt::black);
        marker->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        marker->setZValue(3500.0);
        marker->setPos(p.x() - 10.0, p.y() - 12.0);

        harmonicsText->append(tr(""));
        harmonicsText->append(tr("Operating-point bias sweep (fixed Headroom):"));
        harmonicsText->append(tr("  Target headroom=%1 Vpk (nearest simulated=%2 Vpk)")
                                  .arg(se->getParameter(SE_HEADROOM), 0, 'f', 2)
                                  .arg(head, 0, 'f', 2));
        harmonicsText->append(tr("  Recommended bias: IA=%1 mA, Va(bias)=%2 V, Va(min)=%3 V, Headroom=%4 Vpk, THD=%5%, Even=%6%, Odd=%7%, Even/Odd=%8, balance=%9")
                                  .arg(bias, 0, 'f', 2)
                                  .arg(vaBias, 0, 'f', 1)
                                  .arg(vaMin, 0, 'f', 1)
                                  .arg(head, 0, 'f', 2)
                                  .arg(thd, 0, 'f', 2)
                                  .arg(even, 0, 'f', 2)
                                  .arg(odd, 0, 'f', 2)
                                  .arg(ratio, 0, 'f', 2)
                                  .arg(bal, 0, 'f', 2));
    }

    // Also compute additional "bias sweep at operating headroom" landmarks.
    // These are the same categories used in the 2D plots, but shown as markers on the 3D surface.
    //
    // - Min THD (○)
    // - Max Even (□): max (HD2+HD4)
    // - Min Odd (▽): min (HD3+HD5)
    // - Max Even/Odd (◇): max (HD2+HD4)/(HD3+HD5)
    {
        auto addSurfaceMarker = [&](const QString &symbol, const QColor &color, double bias, double vaMin, double thd) {
            // Markers are placed at the max(Even,Odd) height so they sit on the dual mountain.
            const QPointF p = project(bias, vaMin, thd);
            auto *t = scene->addText(symbol);
            t->setDefaultTextColor(color);
            t->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            t->setZValue(3600.0);
            t->setPos(p.x() - 8.0, p.y() - 12.0);
        };

        int idxMinThd = -1;
        double minThd = std::numeric_limits<double>::infinity();

        int idxMaxEven = -1;
        double bestEven = 0.0;

        int idxMinOdd = -1;
        double bestOdd = std::numeric_limits<double>::infinity();

        int idxMaxRatio = -1;
        double bestRatio = -std::numeric_limits<double>::infinity();

        for (int b = 0; b < biasSteps; ++b) {
            const double thd = thdGrid[headIdxOp][b];
            if (!std::isfinite(thd) || thd < 0.0) {
                continue;
            }

            const double hd2 = harmonicSurface[0][headIdxOp][b];
            const double hd3 = harmonicSurface[1][headIdxOp][b];
            const double hd4 = harmonicSurface[2][headIdxOp][b];
            const double hd5 = harmonicSurface[3][headIdxOp][b];

            const double even = (std::isfinite(hd2) ? std::max(0.0, hd2) : 0.0) + (std::isfinite(hd4) ? std::max(0.0, hd4) : 0.0);
            const double odd = (std::isfinite(hd3) ? std::max(0.0, hd3) : 0.0) + (std::isfinite(hd5) ? std::max(0.0, hd5) : 0.0);

            if (thd < minThd) {
                minThd = thd;
                idxMinThd = b;
            }
            if (even > bestEven) {
                bestEven = even;
                idxMaxEven = b;
            }
            if (odd < bestOdd) {
                bestOdd = odd;
                idxMinOdd = b;
            }

            const double ratio = even / (odd + 1e-12);
            if (std::isfinite(ratio) && ratio > bestRatio) {
                bestRatio = ratio;
                idxMaxRatio = b;
            }
        }

        const double head = headroomPoints[headIdxOp];
        harmonicsText->append(tr(""));
        harmonicsText->append(tr("Operating-point slice landmarks (same headroom row):"));

        if (idxMinThd >= 0) {
            const double bias = biasPoints[idxMinThd];
            const double va = se->estimateAnodeVoltageAtBias(bias);
            const double thd = thdGrid[headIdxOp][idxMinThd];
            const double vaMin = std::max(0.0, va - head);
            const double hd2 = harmonicSurface[0][headIdxOp][idxMinThd];
            const double hd3 = harmonicSurface[1][headIdxOp][idxMinThd];
            const double hd4 = harmonicSurface[2][headIdxOp][idxMinThd];
            const double hd5 = harmonicSurface[3][headIdxOp][idxMinThd];
            const double even = std::max(0.0, hd2) + std::max(0.0, hd4);
            const double odd = std::max(0.0, hd3) + std::max(0.0, hd5);
            const double zMarker = std::max(even, odd);
            addSurfaceMarker(QStringLiteral("\u25CB"), QColor::fromRgb(255, 140, 0), bias, vaMin, zMarker);
            harmonicsText->append(tr("  Min THD (\u25CB): IA=%1 mA, Va(bias)=%2 V, Va(min)=%3 V, THD=%4%")
                                      .arg(bias, 0, 'f', 2)
                                      .arg(va, 0, 'f', 1)
                                      .arg(vaMin, 0, 'f', 1)
                                      .arg(thd, 0, 'f', 2));
        }

        if (idxMaxEven >= 0) {
            const double bias = biasPoints[idxMaxEven];
            const double va = se->estimateAnodeVoltageAtBias(bias);
            const double thd = thdGrid[headIdxOp][idxMaxEven];
            const double vaMin = std::max(0.0, va - head);
            const double hd2 = harmonicSurface[0][headIdxOp][idxMaxEven];
            const double hd3 = harmonicSurface[1][headIdxOp][idxMaxEven];
            const double hd4 = harmonicSurface[2][headIdxOp][idxMaxEven];
            const double hd5 = harmonicSurface[3][headIdxOp][idxMaxEven];
            const double even = std::max(0.0, hd2) + std::max(0.0, hd4);
            const double odd = std::max(0.0, hd3) + std::max(0.0, hd5);
            const double zMarker = std::max(even, odd);
            addSurfaceMarker(QStringLiteral("\u25A1"), QColor::fromRgb(0, 0, 255), bias, vaMin, zMarker);
            harmonicsText->append(tr("  Max Even (\u25A1): IA=%1 mA, Va(bias)=%2 V, Va(min)=%3 V, Even=%4%")
                                      .arg(bias, 0, 'f', 2)
                                      .arg(va, 0, 'f', 1)
                                      .arg(vaMin, 0, 'f', 1)
                                      .arg(bestEven, 0, 'f', 2));
        }

        if (idxMinOdd >= 0) {
            const double bias = biasPoints[idxMinOdd];
            const double va = se->estimateAnodeVoltageAtBias(bias);
            const double thd = thdGrid[headIdxOp][idxMinOdd];
            const double vaMin = std::max(0.0, va - head);
            const double hd2 = harmonicSurface[0][headIdxOp][idxMinOdd];
            const double hd3 = harmonicSurface[1][headIdxOp][idxMinOdd];
            const double hd4 = harmonicSurface[2][headIdxOp][idxMinOdd];
            const double hd5 = harmonicSurface[3][headIdxOp][idxMinOdd];
            const double even = std::max(0.0, hd2) + std::max(0.0, hd4);
            const double odd = std::max(0.0, hd3) + std::max(0.0, hd5);
            const double zMarker = std::max(even, odd);
            addSurfaceMarker(QStringLiteral("\u25BD"), QColor::fromRgb(0, 128, 0), bias, vaMin, zMarker);
            harmonicsText->append(tr("  Min Odd (\u25BD): IA=%1 mA, Va(bias)=%2 V, Va(min)=%3 V, Odd=%4%")
                                      .arg(bias, 0, 'f', 2)
                                      .arg(va, 0, 'f', 1)
                                      .arg(vaMin, 0, 'f', 1)
                                      .arg(bestOdd, 0, 'f', 2));
        }

        if (idxMaxRatio >= 0) {
            const double bias = biasPoints[idxMaxRatio];
            const double va = se->estimateAnodeVoltageAtBias(bias);
            const double thd = thdGrid[headIdxOp][idxMaxRatio];
            const double vaMin = std::max(0.0, va - head);
            const double hd2 = harmonicSurface[0][headIdxOp][idxMaxRatio];
            const double hd3 = harmonicSurface[1][headIdxOp][idxMaxRatio];
            const double hd4 = harmonicSurface[2][headIdxOp][idxMaxRatio];
            const double hd5 = harmonicSurface[3][headIdxOp][idxMaxRatio];
            const double even = std::max(0.0, hd2) + std::max(0.0, hd4);
            const double odd = std::max(0.0, hd3) + std::max(0.0, hd5);
            const double zMarker = std::max(even, odd);
            addSurfaceMarker(QStringLiteral("\u25C7"), QColor::fromRgb(128, 0, 128), bias, vaMin, zMarker);
            harmonicsText->append(tr("  Max Even/Odd (\u25C7): IA=%1 mA, Va(bias)=%2 V, Va(min)=%3 V, Even/Odd=%4")
                                      .arg(bias, 0, 'f', 2)
                                      .arg(va, 0, 'f', 1)
                                      .arg(vaMin, 0, 'f', 1)
                                      .arg(bestRatio, 0, 'f', 2));
        }
    }

    // Draw axes (projected) and labels.
    {
        const double x0 = biasMin;
        const double x1 = biasMax;
        const double y0 = vaMinAxisMin;
        const double y1 = vaMinAxisMax;
        const double z0 = 0.0;
        const double z1 = zMax;

        QPen axisPen(Qt::black);
        axisPen.setWidthF(2.0);

        const QPointF o = project(x0, y0, z0);
        const QPointF xb = project(x1, y0, z0);
        const QPointF yh = project(x0, y1, z0);
        const QPointF zt = project(x0, y0, z1);

        scene->addLine(QLineF(o, xb), axisPen);
        scene->addLine(QLineF(o, yh), axisPen);
        scene->addLine(QLineF(o, zt), axisPen);

        auto addLabelAt = [&](const QPointF &p, const QString &text) {
            auto *t = scene->addText(text);
            t->setDefaultTextColor(Qt::black);
            t->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            t->setZValue(2000.0);
            t->setPos(p.x() - t->boundingRect().width() * 0.5, p.y() - 18.0);
        };

        addLabelAt(QPointF((o.x() + xb.x()) * 0.5, (o.y() + xb.y()) * 0.5 + 30.0), QStringLiteral("Bias current IA (mA)"));
        addLabelAt(QPointF((o.x() + yh.x()) * 0.5 - 40.0, (o.y() + yh.y()) * 0.5), QStringLiteral("Anode Va(min) (V)"));
        addLabelAt(QPointF(zt.x() - 40.0, zt.y() - 10.0), QStringLiteral("Harmonic magnitude (%)"));

        // Numeric tick labels so the 3D plot communicates actual values.
        // We keep this intentionally lightweight: just a few major ticks per axis.
        auto addTicks = [&](double a0, double a1,
                            const std::function<QPointF(double)> &pos,
                            int ticks,
                            int decimals) {
            ticks = std::max(2, ticks);
            for (int i = 0; i < ticks; ++i) {
                const double u = static_cast<double>(i) / static_cast<double>(ticks - 1);
                const double v = a0 + (a1 - a0) * u;
                const QPointF p = pos(v);

                auto *dot = scene->addEllipse(p.x() - 2.0, p.y() - 2.0, 4.0, 4.0, Qt::NoPen, QBrush(Qt::black));
                dot->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
                dot->setZValue(2100.0);

                auto *t = scene->addText(QString::number(v, 'f', decimals));
                t->setDefaultTextColor(Qt::black);
                t->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
                t->setZValue(2100.0);
                t->setPos(p.x() - t->boundingRect().width() * 0.5, p.y() + 6.0);
            }
        };

        addTicks(biasMin, biasMax,
                 [&](double bias) { return project(bias, vaMinAxisMin, 0.0); },
                 5,
                 0);

        addTicks(vaMinAxisMin, vaMinAxisMax,
                 [&](double va) { return project(biasMin, va, 0.0); },
                 5,
                 0);

        addTicks(0.0, zMax,
                 [&](double thd) { return project(biasMin, vaMinAxisMin, thd); },
                 5,
                 1);
    }

    // Add a small legend for the surfaces and markers.
    {
        const double x0 = PLOT_WIDTH - 260.0;
        const double y0 = 10.0;
        const double w = 250.0;
        const double h = 150.0;
        auto *box = scene->addRect(x0, y0, w, h, QPen(Qt::black), QBrush(QColor(255, 255, 255, 220)));
        box->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        box->setZValue(2000.0);

        auto *t1 = scene->addText(QStringLiteral("Surfaces:"));
        t1->setDefaultTextColor(Qt::black);
        t1->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        t1->setZValue(2001.0);
        t1->setPos(x0 + 8.0, y0 + 4.0);

        auto *tEven = scene->addText(QStringLiteral("Blue: Even = HD2 + HD4"));
        tEven->setDefaultTextColor(QColor::fromRgb(0, 0, 255));
        tEven->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        tEven->setZValue(2001.0);
        tEven->setPos(x0 + 8.0, y0 + 28.0);

        auto *tOdd = scene->addText(QStringLiteral("Red: Odd = HD3 + HD5"));
        tOdd->setDefaultTextColor(QColor::fromRgb(255, 0, 0));
        tOdd->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        tOdd->setZValue(2001.0);
        tOdd->setPos(x0 + 8.0, y0 + 46.0);

        // Marker key (kept short, but enough to understand what the symbols mean).
        const double my = y0 + 70.0;
        auto addKey = [&](double y, const QString &sym, const QColor &c, const QString &label) {
            auto *tt = scene->addText(sym + QStringLiteral("  ") + label);
            tt->setDefaultTextColor(c);
            tt->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            tt->setZValue(2001.0);
            tt->setPos(x0 + 8.0, y);
        };

        addKey(my + 0.0,  QStringLiteral("✚"), QColor::fromRgb(0, 0, 0), QStringLiteral("Op bias rec (current headroom)"));
        addKey(my + 18.0, QStringLiteral("\u25CB"), QColor::fromRgb(255, 140, 0), QStringLiteral("Min THD (slice)"));
        addKey(my + 36.0, QStringLiteral("\u25A1"), QColor::fromRgb(0, 0, 255), QStringLiteral("Max Even (slice)"));
        addKey(my + 54.0, QStringLiteral("\u25BD"), QColor::fromRgb(0, 128, 0), QStringLiteral("Min Odd (slice)"));
        addKey(my + 72.0, QStringLiteral("\u25C7"), QColor::fromRgb(128, 0, 128), QStringLiteral("Max Even/Odd (slice)"));
        addKey(my + 90.0, QStringLiteral("★"), QColor::fromRgb(0, 0, 0), QStringLiteral("Surface sweet spots"));
    }

    // Identify a few candidate "sweet spots": points that have both a high THD (strong effect)
    // and an even-dominant balance (more even than odd).
    //
    // We compute a simple desirability score:
    //   score = THD * clamp01(0.5 + 0.5*balance)
    // where balance in [-1..1] favors even-dominant points.
    struct Spot { int h; int b; double score; double thd; double bal; };
    QVector<Spot> spots;
    spots.reserve(headSteps * biasSteps);

    auto desirability = [](double thd, double bal) -> double {
        const double w = std::clamp(0.5 + 0.5 * bal, 0.0, 1.0);
        return thd * w;
    };

    for (int h = 1; h < headSteps - 1; ++h) {
        for (int b = 1; b < biasSteps - 1; ++b) {
            const double thd = thdGrid[h][b];
            const double bal = balanceGrid[h][b];
            if (!std::isfinite(thd) || thd <= 0.0) {
                continue;
            }
            // Only consider even-favoring candidates.
            if (bal < 0.10) {
                continue;
            }
            const double sc = desirability(thd, bal);

            // Local-maximum test vs 8-neighborhood in desirability.
            bool isLocalMax = true;
            for (int dh = -1; dh <= 1 && isLocalMax; ++dh) {
                for (int db = -1; db <= 1; ++db) {
                    if (dh == 0 && db == 0) continue;
                    const double thdN = thdGrid[h + dh][b + db];
                    const double balN = balanceGrid[h + dh][b + db];
                    const double scN = desirability(thdN, balN);
                    if (std::isfinite(scN) && scN >= sc) {
                        isLocalMax = false;
                        break;
                    }
                }
            }
            if (!isLocalMax) {
                continue;
            }

            spots.push_back({h, b, sc, thd, bal});
        }
    }

    std::sort(spots.begin(), spots.end(), [](const Spot &a, const Spot &b) { return a.score > b.score; });
    const int maxSpots = std::min(5, static_cast<int>(spots.size()));
    harmonicsText->append(tr("3D Dual Surface Waterfall generated"));
    harmonicsText->append(tr("Surface: X=IA (mA), Y=Va(min) (V)."));
    harmonicsText->append(tr("Heights: Blue=Even (HD2+HD4), Red=Odd (HD3+HD5)."));
    harmonicsText->append(tr("Sweet-spot scoring: high THD + even-dominant local maxima."));
    harmonicsText->append(tr("Candidate sweet spots:"));

    for (int i = 0; i < maxSpots; ++i) {
        const Spot &s = spots[i];
        const double bias = biasPoints[s.b];
        const double head = headroomPoints[s.h];
        const double vaBias = se->estimateAnodeVoltageAtBias(bias);
        const double vaMin = vaMinGrid[s.h][s.b];

        const double hd2 = harmonicSurface[0][s.h][s.b];
        const double hd3 = harmonicSurface[1][s.h][s.b];
        const double hd4 = harmonicSurface[2][s.h][s.b];
        const double hd5 = harmonicSurface[3][s.h][s.b];
        const double even = (std::isfinite(hd2) ? std::max(0.0, hd2) : 0.0) + (std::isfinite(hd4) ? std::max(0.0, hd4) : 0.0);
        const double odd = (std::isfinite(hd3) ? std::max(0.0, hd3) : 0.0) + (std::isfinite(hd5) ? std::max(0.0, hd5) : 0.0);
        const double zMarker = std::max(even, odd);

        const QPointF p = project(bias, vaMin, zMarker);

        auto *marker = scene->addText(QStringLiteral("★"));
        marker->setDefaultTextColor(Qt::black);
        marker->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        marker->setZValue(3000.0);
        marker->setPos(p.x() - 8.0, p.y() - 12.0);

        harmonicsText->append(tr("  #%1: IA=%2 mA, Va(bias)=%3 V, Va(min)=%4 V, Headroom=%5 Vpk, THD=%6%, Even=%7%, Odd=%8%, balance=%9")
                                  .arg(i + 1)
                                  .arg(bias, 0, 'f', 2)
                                  .arg(vaBias, 0, 'f', 1)
                                  .arg(vaMin, 0, 'f', 1)
                                  .arg(head, 0, 'f', 2)
                                  .arg(s.thd, 0, 'f', 2)
                                  .arg(even, 0, 'f', 2)
                                  .arg(odd, 0, 'f', 2)
                                  .arg(s.bal, 0, 'f', 2));
    }

    // Fit the view to the rendered content with padding.
    if (harmonicsView && harmonicsView->scene()) {
        QRectF r = harmonicsView->scene()->itemsBoundingRect();
        if (r.isValid() && !r.isEmpty()) {
            const double padX = std::max(10.0, r.width() * 0.05);
            const double padY = std::max(10.0, r.height() * 0.05);
            r.adjust(-padX, -padY, padX, padY);
            harmonicsView->setSceneRect(r);
            harmonicsView->fitInView(r, Qt::KeepAspectRatio);
        }
    }
}

void ValveWorkbench::onHarmonicsRotationChanged()
{
    // Check if we have valid sliders and a waterfall is currently displayed
    if (!harmonicsRotationXSlider || !harmonicsRotationYSlider || !harmonicsText) {
        return;
    }
    
    // Only regenerate if a waterfall was the last plot generated
    if (harmonicsText->toPlainText().contains("Waterfall generated")) {
        harmonicsText->append(tr("\n--- 3D Rotation Updated ---"));
        runHarmonicsWaterfall();
    }
}

void ValveWorkbench::runHarmonicsClippingAnalysis()
{
    if (!harmonicsText || !harmonicsView) {
        return;
    }

    auto fitHarmonicsViewToContents = [this]() {
        if (!harmonicsView || !harmonicsView->scene()) {
            return;
        }
        QRectF r = harmonicsView->scene()->itemsBoundingRect();
        if (!r.isValid() || r.isEmpty()) {
            return;
        }
        const double padX = std::max(10.0, r.width() * 0.05);
        const double padY = std::max(10.0, r.height() * 0.05);
        r.adjust(-padX, -padY, padX, padY);
        harmonicsView->setSceneRect(r);
        harmonicsView->fitInView(r, Qt::KeepAspectRatio);
    };

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

    fitHarmonicsViewToContents();
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
    ui->circuitSelection->addItem("Single Ended Output", SINGLE_ENDED_OUTPUT);
    ui->circuitSelection->addItem("Ultralinear Single Ended", ULTRALINEAR_SINGLE_ENDED);
    ui->circuitSelection->addItem("Push Pull Output", PUSH_PULL_OUTPUT);
    ui->circuitSelection->addItem("Ultralinear Push Pull", ULTRALINEAR_PUSH_PULL);
    ui->circuitSelection->addItem("Triode CC + DC Follower (2-stage)", TEST_CALCULATOR);
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

    if (auto tcc = dynamic_cast<TriodeCommonCathode*>(circuit)) {
        updateHeadroomWaveformView(tcc);
    } else if (auto se = dynamic_cast<SingleEndedOutput*>(circuit)) {
        updateHeadroomWaveformView(se);
    } else if (auto pp = dynamic_cast<PushPullOutput*>(circuit)) {
        updateHeadroomWaveformView(pp);
    }

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

        // Prefer a measurement-driven model overlay whenever an embedded
        // analyser measurement is present for a pentode anode sweep. This
        // keeps the red model curves aligned with the same Vg1/Vg2 families
        // used by the black measurement curves.
        if (embedded && deviceModel && embedded->getDeviceType() == PENTODE &&
            embedded->getTestType() == ANODE_CHARACTERISTICS) {
            QGraphicsItemGroup *plotted = deviceModel->plotModel(&plot, embedded, nullptr);
            if (plotted) {
                modelledCurves = plotted;
                plot.add(modelledCurves);
                modelledCurves->setVisible(ui->modelCheck->isChecked());
            }
        }

        // Fallback: draw using the device's internal anodePlot, which uses
        // vg1Max/vg2Max from the preset JSON.
        if (!modelledCurves) {
            modelledCurves = device->anodePlot(&plot);
            if (modelledCurves) {
                modelledCurves->setVisible(ui->modelCheck->isChecked());
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

    if (headroomWaveformScene) {
        headroomWaveformScene->clear();
    }
    if (ui->headroomWaveformGroupBox) {
        ui->headroomWaveformGroupBox->setVisible(false);
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

        // No valid circuit: hide stage-level Designer toggles such as
        // Max Sym Swing and K bypass.
        if (symSwingCheck) {
            symSwingCheck->setVisible(false);
        }
        if (useBypassedGainCheck) {
            useBypassedGainCheck->setVisible(false);
        }
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

    // Show the shared stage-level Designer toggles (Max Sym Swing, K bypass)
    // only for circuits that support symmetric/max headroom helpers and
    // cathode-bypass gain mode. This includes Triode CC, Pentode CC, and the
    // main output-stage circuits (SE, SE-UL, PP, UL-PP).
    bool usesStageToggles =
        (dynamic_cast<TriodeCommonCathode*>(circuit)   != nullptr) ||
        (dynamic_cast<PentodeCommonCathode*>(circuit)  != nullptr) ||
        (dynamic_cast<SingleEndedOutput*>(circuit)     != nullptr) ||
        (dynamic_cast<SingleEndedUlOutput*>(circuit)   != nullptr) ||
        (dynamic_cast<PushPullOutput*>(circuit)        != nullptr) ||
        (dynamic_cast<PushPullUlOutput*>(circuit)      != nullptr);

    if (symSwingCheck) {
        symSwingCheck->setVisible(usesStageToggles);
    }
    if (useBypassedGainCheck) {
        useBypassedGainCheck->setVisible(usesStageToggles);
    }

    if (ui->headroomWaveformGroupBox) {
        bool showWave =
            (dynamic_cast<TriodeCommonCathode*>(circuit) != nullptr) ||
            (dynamic_cast<SingleEndedOutput*>(circuit)     != nullptr) ||
            (dynamic_cast<PushPullOutput*>(circuit)        != nullptr);
        ui->headroomWaveformGroupBox->setVisible(showWave);
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
            updateHeadroomWaveformView(tcc);

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

        // When Autoscale Y is enabled, treat any parameter change on the
        // main output-stage circuits as affecting the stage so that axis
        // limits are recomputed and the graph refits to the current data
        // (headroom, bias, etc.). When Autoscale Y is disabled, fall back
        // to the previous behaviour where only VB/RAA edits adjust axes.
        bool affectsOutputStage = false;
        if (se || seul || pp || ppul) {
            if (ui && ui->autoscaleYCheck && ui->autoscaleYCheck->isChecked()) {
                affectsOutputStage = true;
            } else {
                affectsOutputStage =
                    (isSeVB || isSeUlVB || isPpVB || isPpUlVB || isPpRaa || isPpUlRaa);
            }
        }

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
                if (ui->autoscaleYCheck) {
                    if (!ui->autoscaleYCheck->isChecked()) {
                        if (currentYStop > 0.0) {
                            iaMaxNew = currentYStop;
                        }
                    } else if (se) {
                        double iaBias_mA = se->getParameter(SE_IA);
                        if (index == SE_IA) {
                            iaBias_mA = value;
                        }

                        double headroomVpk = se->getParameter(SE_HEADROOM);
                        if (index == SE_HEADROOM) {
                            headroomVpk = value;
                        }

                        double raa_ohms = se->getParameter(SE_RA);
                        if (raa_ohms > 0.0) {
                            double iaSwing_mA = 0.0;
                            if (headroomVpk > 0.0) {
                                iaSwing_mA = (headroomVpk / raa_ohms) * 1000.0;
                            }
                            double approxPeak = iaBias_mA + iaSwing_mA;
                            if (approxPeak > 0.0 && std::isfinite(approxPeak)) {
                                iaMaxNew = approxPeak * 1.1;
                            }
                        } else if (iaBias_mA > 0.0) {
                            iaMaxNew = iaBias_mA * 1.5;
                        }
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

    if (auto tcc = dynamic_cast<TriodeCommonCathode*>(circuit)) {
        updateHeadroomWaveformView(tcc);
    } else if (auto se = dynamic_cast<SingleEndedOutput*>(circuit)) {
        updateHeadroomWaveformView(se);
    } else if (auto pp = dynamic_cast<PushPullOutput*>(circuit)) {
        updateHeadroomWaveformView(pp);
    }

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

void ValveWorkbench::updateHeadroomWaveformView(TriodeCommonCathode *tcc)
{
    if (!ui || !ui->headroomWaveformView || !headroomWaveformScene) {
        return;
    }

    headroomWaveformScene->clear();
    if (!tcc) {
        return;
    }

    const QVector<double> &wave = tcc->getLastHeadroomWaveform();
    if (wave.isEmpty()) {
        return;
    }

    double mean = 0.0;
    for (double v : wave) {
        mean += v;
    }
    mean /= static_cast<double>(wave.size());

    double peak = 0.0;
    for (double v : wave) {
        const double y = v - mean;
        peak = std::max(peak, std::fabs(y));
    }
    if (!(peak > 0.0) || !std::isfinite(peak)) {
        return;
    }

    QPainterPath path;
    const int n = wave.size();
    for (int i = 0; i < n; ++i) {
        const double x = (n > 1) ? (static_cast<double>(i) / static_cast<double>(n - 1)) : 0.0;
        const double y = -(wave.at(i) - mean) / peak;
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    QPen pen(QColor::fromRgb(0, 120, 255));
    pen.setWidthF(0.0);
    pen.setCosmetic(true);
    QGraphicsPathItem *item = headroomWaveformScene->addPath(path, pen);

    QRectF r = item ? item->boundingRect() : QRectF();
    if (r.isValid()) {
        const double padX = r.width() * 0.05;
        const double padY = r.height() * 0.20;
        r.adjust(-padX, -padY, padX, padY);
        headroomWaveformScene->setSceneRect(r);
        ui->headroomWaveformView->fitInView(r, Qt::KeepAspectRatio);
    }
}

void ValveWorkbench::updateHeadroomWaveformView(SingleEndedOutput *se)
{
    if (!ui || !ui->headroomWaveformView || !headroomWaveformScene) {
        return;
    }

    headroomWaveformScene->clear();
    if (!se) {
        return;
    }

    const QVector<double> &wave = se->getLastHeadroomWaveform();
    if (wave.isEmpty()) {
        return;
    }

    double mean = 0.0;
    for (double v : wave) {
        mean += v;
    }
    mean /= static_cast<double>(wave.size());

    double peak = 0.0;
    for (double v : wave) {
        const double y = v - mean;
        peak = std::max(peak, std::fabs(y));
    }
    if (!(peak > 0.0) || !std::isfinite(peak)) {
        return;
    }

    QPainterPath path;
    const int n = wave.size();
    for (int i = 0; i < n; ++i) {
        const double x = (n > 1) ? (static_cast<double>(i) / static_cast<double>(n - 1)) : 0.0;
        const double y = -(wave.at(i) - mean) / peak;
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    QPen pen(QColor::fromRgb(0, 120, 255));
    pen.setWidthF(0.0);
    pen.setCosmetic(true);
    QGraphicsPathItem *item = headroomWaveformScene->addPath(path, pen);

    QRectF r = item ? item->boundingRect() : QRectF();
    if (r.isValid()) {
        const double padX = r.width() * 0.05;
        const double padY = r.height() * 0.20;
        r.adjust(-padX, -padY, padX, padY);
        headroomWaveformScene->setSceneRect(r);
        ui->headroomWaveformView->fitInView(r, Qt::KeepAspectRatio);
    }
}

void ValveWorkbench::updateHeadroomWaveformView(PushPullOutput *pp)
{
    if (!ui || !ui->headroomWaveformView || !headroomWaveformScene) {
        return;
    }

    headroomWaveformScene->clear();
    if (!pp) {
        return;
    }

    const QVector<double> &wave = pp->getLastHeadroomWaveform();
    if (wave.isEmpty()) {
        return;
    }

    double mean = 0.0;
    for (double v : wave) {
        mean += v;
    }
    mean /= static_cast<double>(wave.size());

    double peak = 0.0;
    for (double v : wave) {
        const double y = v - mean;
        peak = std::max(peak, std::fabs(y));
    }
    if (!(peak > 0.0) || !std::isfinite(peak)) {
        return;
    }

    QPainterPath path;
    const int n = wave.size();
    for (int i = 0; i < n; ++i) {
        const double x = (n > 1) ? (static_cast<double>(i) / static_cast<double>(n - 1)) : 0.0;
        const double y = -(wave.at(i) - mean) / peak;
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    QPen pen(QColor::fromRgb(0, 120, 255));
    pen.setWidthF(0.0);
    pen.setCosmetic(true);
    QGraphicsPathItem *item = headroomWaveformScene->addPath(path, pen);

    QRectF r = item ? item->boundingRect() : QRectF();
    if (r.isValid()) {
        const double padX = r.width() * 0.05;
        const double padY = r.height() * 0.20;
        r.adjust(-padX, -padY, padX, padY);
        headroomWaveformScene->setSceneRect(r);
        ui->headroomWaveformView->fitInView(r, Qt::KeepAspectRatio);
    }
}

// Helper: pick an operating point (sweepIdx, sampleIdx) from an ANODE_CHARACTERISTICS
// measurement near 50% of Ia_max. Returns false on failure.
static bool pickOperatingPointFromAnode(Measurement *measurement,
                                        int &sweepIdx,
                                        int &sampleIdx,
                                        double &vaOp,
                                        double &vg1Op,
                                        double &vg2Op,
                                        double vaTarget,
                                        double vg1Target)
{
    if (!measurement) return false;

    const int sweepCount = measurement->count();
    if (sweepCount == 0) {
        return false;
    }

    const bool haveTarget = std::isfinite(vaTarget) && std::isfinite(vg1Target);

    sweepIdx  = sweepCount / 2;
    sampleIdx = -1;
    Sweep *sweep = nullptr;

    if (haveTarget) {
        double bestScore = std::numeric_limits<double>::infinity();

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
                const double va = sample->getVa();
                const double vg = sample->getVg1();
                const double dVa = va - vaTarget;
                const double dVg = vg - vg1Target;
                const double score = dVg * dVg + 0.25 * dVa * dVa;
                if (score < bestScore) {
                    bestScore = score;
                    sweepIdx  = sw;
                    sampleIdx = sa;
                    sweep     = s;
                }
            }
        }
    } else {
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
                    sweepIdx  = sw;
                    sampleIdx = sa;
                    sweep     = s;
                }
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
// Ia vs Vg1 over a Vg-centred window with bin-averaged samples. This mirrors
// the Health gm estimator so that dense, quantised transfer sweeps produce a
// smooth, physically plausible gm for the Modeller small-signal LCDs.
// Returns gm in mA/V, or <= 0.0 on failure.
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

    struct LocalPoint {
        double vg;
        double ia;
    };

    const double targetVg   = vg1Op;
    const double maxWindow  = 0.6;
    const double vgBinEps   = 0.01;
    const int    minBinsReq = 3;

    double window = 0.3;
    QVector<LocalPoint> rawPoints;

    auto collectPointsInWindow = [&](double halfWidth, QVector<LocalPoint> &out) {
        out.clear();
        for (int i = 0; i < sampleCount; ++i) {
            Sample *s = sweep->at(i);
            if (!s) continue;
            const double ia = s->getIa();
            const double vg = s->getVg1();
            if (!std::isfinite(ia) || !std::isfinite(vg) || ia <= 0.0) {
                continue;
            }
            const double dVg = vg - targetVg;
            if (std::fabs(dVg) <= halfWidth) {
                rawPoints.push_back({vg, ia});
            }
        }
    };

    while (true) {
        collectPointsInWindow(window, rawPoints);
        if (rawPoints.size() >= minBinsReq || window >= maxWindow) {
            break;
        }
        window = std::min(maxWindow, window * 1.5 + 0.05);
    }

    if (rawPoints.size() < 2) {
        return 0.0;
    }

    std::sort(rawPoints.begin(), rawPoints.end(), [](const LocalPoint &a, const LocalPoint &b) {
        return a.vg < b.vg;
    });

    struct VgBin {
        double vgSum;
        double iaSum;
        int    count;
    };

    QVector<VgBin> bins;
    bins.reserve(rawPoints.size());

    for (const LocalPoint &p : rawPoints) {
        if (bins.isEmpty()) {
            bins.push_back({p.vg, p.ia, 1});
            continue;
        }

        VgBin &last = bins.last();
        const double vgLastAvg = last.vgSum / static_cast<double>(last.count);
        if (std::fabs(p.vg - vgLastAvg) <= vgBinEps) {
            last.vgSum += p.vg;
            last.iaSum += p.ia;
            ++last.count;
        } else {
            bins.push_back({p.vg, p.ia, 1});
        }
    }

    if (bins.size() < 2) {
        return 0.0;
    }

    const int Nb = bins.size();
    double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;

    for (int i = 0; i < Nb; ++i) {
        const VgBin &b = bins.at(i);
        const double vg = b.vgSum / static_cast<double>(b.count);
        const double ia = b.iaSum / static_cast<double>(b.count);
        Sx  += vg;
        Sy  += ia;
        Sxx += vg * vg;
        Sxy += vg * ia;
    }

    double gm_mA_V = 0.0;
    const double den = static_cast<double>(Nb) * Sxx - Sx * Sx;
    if (std::fabs(den) > 1e-12) {
        gm_mA_V = (static_cast<double>(Nb) * Sxy - Sx * Sy) / den;
    }

    if (gm_mA_V <= 0.0) {
        const VgBin &b0 = bins.first();
        const VgBin &b1 = bins.last();
        const double vg0 = b0.vgSum / static_cast<double>(b0.count);
        const double ia0 = b0.iaSum / static_cast<double>(b0.count);
        const double vg1 = b1.vgSum / static_cast<double>(b1.count);
        const double ia1 = b1.iaSum / static_cast<double>(b1.count);
        const double dVg = vg1 - vg0;
        if (std::fabs(dVg) < 1e-6) {
            return 0.0;
        }
        gm_mA_V = (ia1 - ia0) / dVg;
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

    // Prefer the datasheet reference operating point when available so that
    // small-signal gm/ra/mu are anchored to the same Va/Vg used for
    // datasheet comparisons and Health tests.
    double va0 = 0.0;
    double vg0 = 0.0;
    double ia0 = 0.0;
    double gm0 = 0.0;
    double mu0 = 0.0;
    double rp0 = 0.0;
    const bool haveDatasheetRef = ensureDatasheetRefPoint(va0, vg0, ia0, gm0, mu0, rp0);

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
        !pickOperatingPointFromAnode(anodeMeasurement,
                                     opSweepIdx,
                                     opSampleIdx,
                                     vaOp,
                                     vg1Op,
                                     vg2Op,
                                     haveDatasheetRef ? va0 : std::numeric_limits<double>::quiet_NaN(),
                                     haveDatasheetRef ? vg0 : std::numeric_limits<double>::quiet_NaN())) {
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
    Q_UNUSED(vh);
    Q_UNUSED(ih);

    if (!analyser) {
        return;
    }

    if (ui->heaterVlcd) {
        ui->heaterVlcd->display(QString::number(analyser->getAveragingSamples()));
    }

    if (ui->heaterIlcd) {
        ui->heaterIlcd->display(QString::number(analyser->getRetryLimitExceededCount()));
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

        // During Quick/Full Health, leave all health transfer sweeps visible on
        // the plot. Only the first health point configures axes; subsequent
        // points overlay without clearing.
        const bool overlayHealth = (healthRunActive && healthRunIndex > 0);

        if (overlayHealth) {
            QGraphicsItemGroup *overlayGroup = currentMeasurement->updatePlotWithoutAxes(&plot);
            if (overlayGroup) {
                plot.add(overlayGroup);
            }
        } else {
            // Default behaviour: reconfigure axes from this measurement and
            // draw its sweeps as the primary measured curves.
            measuredCurves = currentMeasurement->updatePlot(&plot);
            if (measuredCurves) {
                plot.add(measuredCurves);
            }
        }
    }

    // For double triode measurements, build a Triode B clone from the latest
    // analyser result. During Health runs we use the clone from the *first*
    // (centre) health point for Triode B statistics, and create additional
    // temporary clones for later points purely for plotting so that blue
    // curves accumulate in the same way as the black Triode A curves.
    if (isDoubleTriode && currentMeasurement && measurementHasTriodeBData(currentMeasurement)) {
        Measurement *clone = createTriodeBMeasurementClone(currentMeasurement);
        if (clone != nullptr && measurementHasValidSamples(clone)) {
            // When a Health run is active, always overlay this clone's curves
            // so that Triode B sweeps from every health point are visible.
            if (healthRunActive) {
                clone->setSampleColor(QColor::fromRgb(0, 0, 255));
                clone->setSmoothPlotting(preferencesDialog.smoothCurves());
                QGraphicsItemGroup *overlayGroupB = clone->updatePlotWithoutAxes(&plot);
                if (overlayGroupB) {
                    plot.add(overlayGroupB);
                }
            }

            // Decide whether this clone should be kept for Triode B health
            // calculations / general use, or discarded after plotting.
            bool adoptForMetrics = false;
            if (!healthRunActive) {
                // Normal (non-Health) double-triode runs always adopt the
                // latest clone for Triode B overlays and analysis.
                adoptForMetrics = true;
            } else if (healthRunIndex == 0) {
                // For Quick/Full Health, only the *first* health point (the
                // central operating point) should define the Triode B Health
                // statistics so that later corner sweeps do not change the
                // measured Ia/gm once the run is complete.
                adoptForMetrics = true;
            }

            if (adoptForMetrics) {
                if (triodeMeasurementSecondary != nullptr) {
                    deleteMeasurementClone(triodeMeasurementSecondary);
                    triodeMeasurementSecondary = nullptr;
                }

                triodeMeasurementSecondary = clone;
                qInfo("Health TriodeB: adopted secondary clone in testFinished - sweeps=%d, testType=%d, healthRunActive=%d, healthRunIndex=%d",
                      triodeMeasurementSecondary->count(),
                      triodeMeasurementSecondary->getTestType(),
                      healthRunActive ? 1 : 0,
                      healthRunIndex);
                triodeMeasurementSecondary->setSampleColor(QColor::fromRgb(0, 0, 255));
                triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());

                // Ownership of clone has been transferred.
                clone = nullptr;
            }
        }

        // Discard any temporary clone that was not adopted for metrics.
        if (clone) {
            deleteMeasurementClone(clone);
        }
    }

    // Outside of Health runs, keep a single Triode B overlay in sync with the
    // current secondary clone. During Quick/Full Health we instead create
    // per-sweep temporary clones above so blue curves accumulate just like the
    // primary black curves.
    if (!healthRunActive && isDoubleTriode && triodeMeasurementSecondary &&
        measurementHasValidSamples(triodeMeasurementSecondary)) {
        if (measuredCurvesSecondary != nullptr) {
            plot.remove(measuredCurvesSecondary);
            measuredCurvesSecondary = nullptr;
        }

        measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
        if (measuredCurvesSecondary) {
            plot.add(measuredCurvesSecondary);
        }
    }
    ui->measureCheck->setChecked(true);

    populateDataTableFromMeasurement(currentMeasurement);

    if (modellingRunActive) {
        Project *project = nullptr;
        if (currentProject) {
            project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
        }

        if (!project) {
            modellingRunActive = false;
            modellingRunIndex = 0;
            modellingSteps.clear();
            restoreModellingState();
            QMessageBox::warning(this, tr("Modelling Tests"), tr("No project is available to save measurements."));
            return;
        }

        if (currentMeasurement && modellingRunIndex >= 0 && modellingRunIndex < modellingSteps.size()) {
            const ModellingTestStep step = modellingSteps.at(modellingRunIndex);
            currentMeasurement->setCustomLabel(step.label);

            if (project->addMeasurement(currentMeasurement)) {
                currentMeasurement->buildTree(currentProject);
            }

            currentMeasurementItem = nullptr;
            if (currentProject) {
                for (int i = 0; i < currentProject->childCount(); ++i) {
                    QTreeWidgetItem *child = currentProject->child(i);
                    if (!child) continue;
                    if (child->type() != TYP_MEASUREMENT) continue;
                    void *mData = child->data(0, Qt::UserRole).value<void *>();
                    if (mData == static_cast<void *>(currentMeasurement)) {
                        currentMeasurementItem = child;
                        break;
                    }
                }
            }

            ui->btnAddToProject->setEnabled(false);
        }

        modellingRunIndex++;
        if (modellingRunIndex >= 0 && modellingRunIndex < modellingSteps.size()) {
            const ModellingTestStep next = modellingSteps.at(modellingRunIndex);
            QMetaObject::invokeMethod(
                this,
                [this, next]() {
                    applyModellingStep(next);
                    on_runButton_clicked();
                },
                Qt::QueuedConnection);
            return;
        }

        modellingRunActive = false;
        modellingRunIndex = 0;
        modellingSteps.clear();
        restoreModellingState();
        if (ui && ui->statusbar) {
            ui->statusbar->showMessage(tr("Modelling Tests complete."), 8000);
        }
        return;
    }

    if (healthRunActive) {
        if (healthPrereqAnodeSweepActive) {
            healthPrereqAnodeSweepActive = false;
            healthPrereqAnodeMeasurement = currentMeasurement;

            Project *project = nullptr;
            if (currentProject) {
                project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
            }
            if (project && currentMeasurement) {
                if (project->addMeasurement(currentMeasurement)) {
                    currentMeasurement->buildTree(currentProject);
                }
            }

            QMetaObject::invokeMethod(
                this,
                [this]() {
                    startHealthRun(healthMode);
                },
                Qt::QueuedConnection);
            return;
        }

        // Health OP-finder: first run is a sweep to pick a "middle swing" operating point.
        if (healthOpFinderActive) {
            HealthPoint op;
            double iaAtOp = 0.0;
            if (!findPentodeHealthOperatingPoint(currentMeasurement, healthOpTargetIa_mA, op, iaAtOp)) {
                healthRunActive = false;
                healthOpFinderActive = false;
                healthMode = HEALTH_NONE;
                QMessageBox::warning(this, tr("Health Test"),
                                     tr("Could not find a safe operating point near %1 mA. Try lowering Va/Vg2 or increasing the grid magnitude range.")
                                         .arg(QString::number(healthOpTargetIa_mA, 'f', 0)));
                return;
            }

            // Now build the normal Health point list around the found Vg1.
            healthOpFinderActive = false;

            // Use the found operating point as the centre.
            const double va0 = op.va;
            const double vg0 = op.vg;
            const double vg20 = op.vg2;

            healthPoints.clear();
            healthResults.clear();

            HealthPoint center;
            center.va = va0;
            center.vg = vg0;
            center.vg2 = vg20;
            healthPoints.append(center);

            if (healthMode == HEALTH_QUICK) {
                const double dVaFracQuick = 0.10;
                double dVa = std::fabs(va0) * dVaFracQuick;
                if (dVa < 10.0) dVa = 10.0;
                if (dVa > 30.0) dVa = 30.0;

                const double dVg = 0.5;

                const double vaLow  = std::max(0.0, va0 - dVa);
                const double vaHigh = va0 + dVa;
                const double vgLo   = vg0 - dVg;
                const double vgHi   = vg0 + dVg;

                HealthPoint p;
                p.va = va0;    p.vg = vgLo;   p.vg2 = vg20;  healthPoints.append(p);
                p.va = va0;    p.vg = vgHi;   p.vg2 = vg20;  healthPoints.append(p);

                p.va = vaLow;  p.vg = vg0;    p.vg2 = vg20;  healthPoints.append(p);
                p.va = vaLow;  p.vg = vgLo;   p.vg2 = vg20;  healthPoints.append(p);
                p.va = vaLow;  p.vg = vgHi;   p.vg2 = vg20;  healthPoints.append(p);

                p.va = vaHigh; p.vg = vg0;    p.vg2 = vg20;  healthPoints.append(p);
                p.va = vaHigh; p.vg = vgLo;   p.vg2 = vg20;  healthPoints.append(p);
                p.va = vaHigh; p.vg = vgHi;   p.vg2 = vg20;  healthPoints.append(p);
            } else if (healthMode == HEALTH_FULL) {
                if (deviceType == TRIODE) {
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

                    const double vaCorners[4] = { vaLow, vaLow, vaHigh, vaHigh };
                    const double vgCorners[4] = { vgLo,  vgHi,  vgLo,   vgHi  };

                    HealthPoint p;
                    for (int c = 0; c < 4; ++c) {
                        const double vaCorner = vaCorners[c];
                        const double vgCorner = vgCorners[c];

                        double dVgCorner = 0.3;
                        if (std::fabs(vgCorner) > 2.0) {
                            dVgCorner = 0.5;
                        }

                        p.va = vaCorner; p.vg = vgCorner;              p.vg2 = vg20;  healthPoints.append(p);
                        p.va = vaCorner; p.vg = vgCorner - dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                        p.va = vaCorner; p.vg = vgCorner + dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                    }
                } else {
                    const double dVaFrac = 0.15;
                    double dVa = std::fabs(va0) * dVaFrac;
                    if (dVa < 20.0) dVa = 20.0;
                    if (dVa > 50.0) dVa = 50.0;

                    const double vaLow  = std::max(0.0, va0 - dVa);
                    const double vaHigh = va0 + dVa;
                    const double vgLo   = vg0 - 0.5;
                    const double vgHi   = vg0 + 0.5;

                    const double vaCorners[4] = { vaLow, vaLow, vaHigh, vaHigh };
                    const double vgCorners[4] = { vgLo,  vgHi,  vgLo,   vgHi  };

                    HealthPoint p;
                    for (int c = 0; c < 4; ++c) {
                        const double vaCorner = vaCorners[c];
                        const double vgCorner = vgCorners[c];

                        double dVgCorner = 0.3;
                        if (std::fabs(vgCorner) > 2.0) {
                            dVgCorner = 0.5;
                        }

                        p.va = vaCorner; p.vg = vgCorner;              p.vg2 = vg20;  healthPoints.append(p);
                        p.va = vaCorner; p.vg = vgCorner - dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                        p.va = vaCorner; p.vg = vgCorner + dVgCorner;  p.vg2 = vg20;  healthPoints.append(p);
                    }
                }
            }

            healthResults.resize(healthPoints.size());
            for (int i = 0; i < healthResults.size(); ++i) {
                healthResults[i].valid = false;
                healthResults[i].va = 0.0;
                healthResults[i].vg = 0.0;
                healthResults[i].vg2 = 0.0;
                healthResults[i].ia = 0.0;
                healthResults[i].gm = 0.0;
                healthResults[i].rp = 0.0;
                healthResults[i].ig2 = 0.0;
            }

            healthRunIndex = 0;

            QMetaObject::invokeMethod(
                this,
                [this]() {
                    if (!healthRunActive) {
                        return;
                    }
                    if (!healthPoints.isEmpty()) {
                        configureTransferForHealthPoint(healthPoints.at(0));
                        on_runButton_clicked();
                    }
                },
                Qt::QueuedConnection);
            return;
        }

        if (currentMeasurement && healthRunIndex >= 0 && healthRunIndex < healthPoints.size()) {
            double ia = 0.0;
            double gm = 0.0;
            double rp = 0.0;
            double ig2 = 0.0;
            HealthResult result;
            result.valid = computeIaGmAt(currentMeasurement, healthPoints.at(healthRunIndex), ia, gm, rp, &ig2);

            qInfo("Health compute[%d/%d]: valid=%d Va=%.3f Vg=%.3f Vg2=%.3f Ia=%.6f mA gm=%.6f mA/V rp=%.3f ohms ig2=%.6f mA",
                  healthRunIndex,
                  healthPoints.size(),
                  result.valid ? 1 : 0,
                  healthPoints.at(healthRunIndex).va,
                  healthPoints.at(healthRunIndex).vg,
                  healthPoints.at(healthRunIndex).vg2,
                  ia,
                  gm,
                  rp,
                  ig2);

            result.va = healthPoints.at(healthRunIndex).va;
            result.vg = healthPoints.at(healthRunIndex).vg;
            result.vg2 = healthPoints.at(healthRunIndex).vg2;
            result.ia = ia;
            result.gm = gm;
            result.rp = rp;
            result.ig2 = ig2;
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

    if (modellingRunActive) {
        modellingRunActive = false;
        modellingRunIndex = 0;
        modellingSteps.clear();
        restoreModellingState();
        if (ui && ui->statusbar) {
            ui->statusbar->showMessage(tr("Modelling Tests aborted."), 8000);
        }
    }

    if (healthRunActive) {
        healthRunActive = false;
        healthMode = HEALTH_NONE;
        healthRunIndex = 0;
        healthPrereqAnodeSweepActive = false;

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
    // Always show the primary anode/grid sweep parameters in the Analyser
    // controls. For Double Triode mode we mirror the second anode values in
    // the Screen row (labelled "Second Anode"), while the grid controls
    // continue to reflect the shared grid sweep that drives both triodes.

    updateDoubleValue(ui->anodeStart, anodeStart);
    updateDoubleValue(ui->anodeStop, anodeStop);
    updateDoubleValue(ui->anodeStep, anodeStep);
    updateDoubleValue(ui->gridStart, gridStart);
    updateDoubleValue(ui->gridStop, gridStop);
    updateDoubleValue(ui->gridStep, gridStep);

    if (ui->deviceType->currentText() == "Double Triode") {
        updateDoubleValue(ui->screenStart, secondAnodeStart);
        updateDoubleValue(ui->screenStop, secondAnodeStop);
        updateDoubleValue(ui->screenStep, secondAnodeStep);
    } else {
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
        // Second anode fields mirror the first anode and are not editable.
        ui->screenStart->setEnabled(false);
        ui->screenStop->setEnabled(false);
        ui->screenStep->setEnabled(false);

        ui->anodeLabel->setText("First Anode");
        ui->anodeStart->setEnabled(true);
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(true);
        // Keep the grid row labelled as the primary grid voltage and treat it
        // as the shared grid sweep for both triodes. The second grid follows
        // the same sweep and does not need its own dedicated row.
        ui->gridLabel->setText(tr("-ve Grid Voltage:"));

        secondGridStart = gridStart; // Second grid follows the same settings
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


void ValveWorkbench::on_cir13Value_editingFinished()
{
    // Row 13 (cir13Value) drives circuit parameter index 12. For
    // Triode Common Cathode this is TRI_CC_HEADROOM (Headroom Vpk).
    updateCircuitParameter(12);
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

        {
            int dataIndex = -1;
            for (int i = 0; i < ui->tabWidget->count(); ++i) {
                if (ui->tabWidget->tabText(i) == QLatin1String("Data")) {
                    dataIndex = i;
                    break;
                }
            }
            if (dataIndex >= 0) {
                const bool showData = preferencesDialog.showDataTab();
                if (!showData && ui->tabWidget->currentIndex() == dataIndex) {
                    ui->tabWidget->setCurrentIndex(0);
                }
                ui->tabWidget->setTabVisible(dataIndex, showData);
            }
        }

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

            // Keep Triode B clone/overlay in sync with the currently selected measurement.
            if (triodeMeasurementSecondary != nullptr) {
                deleteMeasurementClone(triodeMeasurementSecondary);
                triodeMeasurementSecondary = nullptr;
            }
            if (currentMeasurement && measurementHasTriodeBData(currentMeasurement)) {
                Measurement *clone = createTriodeBMeasurementClone(currentMeasurement);
                if (clone != nullptr && measurementHasValidSamples(clone)) {
                    triodeMeasurementSecondary = clone;
                    triodeMeasurementSecondary->setSampleColor(QColor::fromRgb(0, 0, 255));
                } else {
                    if (clone != nullptr) {
                        deleteMeasurementClone(clone);
                    }
                }
            }

            if (triodeMeasurementSecondary != nullptr && triodeMeasurementSecondary->count() > 0) {
                if (measuredCurvesSecondary != nullptr) {
                    plot.remove(measuredCurvesSecondary);
                    measuredCurvesSecondary = nullptr;
                }
                triodeMeasurementSecondary->setSmoothPlotting(preferencesDialog.smoothCurves());
                measuredCurvesSecondary = triodeMeasurementSecondary->updatePlotWithoutAxes(&plot);
                if (measuredCurvesSecondary != nullptr) {
                    plot.add(measuredCurvesSecondary);
                    bool triodeBVisible = ui->measureCheck && ui->measureCheck->isChecked();
                    if (ui->tabWidget &&
                        ui->tabWidget->currentWidget() == ui->tab_2 &&
                        measurementHasTriodeBData(currentMeasurement) &&
                        ui->screenCheck) {
                        triodeBVisible = ui->measureCheck->isChecked() && ui->screenCheck->isChecked();
                    }
                    measuredCurvesSecondary->setVisible(triodeBVisible);
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
                            bool triodeBVisible = ui->measureCheck && ui->measureCheck->isChecked();
                            if (ui->tabWidget &&
                                ui->tabWidget->currentWidget() == ui->tab_2 &&
                                isDoubleTriode &&
                                ui->screenCheck) {
                                triodeBVisible = ui->measureCheck->isChecked() && ui->screenCheck->isChecked();
                            }
                            measuredCurvesSecondary->setVisible(triodeBVisible);
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
    if (!currentProject) {
        return nullptr;
    }

    int children = currentProject->childCount();
    Model *foundModel = nullptr;

    for (int i = 0; i < children && foundModel == nullptr; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (!child) {
            continue;
        }
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
    if (!currentProject) {
        return nullptr;
    }

    int children = currentProject->childCount();
    Measurement *foundMeasurement = nullptr;

    for (int i = 0; i < children && foundMeasurement == nullptr; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (!child) {
            continue;
        }
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

    // Retask the shared screen checkbox label when a Double Triode device
    // is selected so that Modeller can present it as a Triode B overlay
    // toggle while Pentode/other modes retain their screen-current meaning.
    if (ui->screenCheck) {
        if (isDoubleTriode) {
            ui->screenCheck->setText(tr("Show Triode B"));
        } else {
            ui->screenCheck->setText(tr("Show Screen Current"));
        }
    }

    if (ui->Triode_A_Box) {
        if (logicalType == PENTODE) {
            ui->Triode_A_Box->setTitle(tr("Pentode Health"));
        } else {
            ui->Triode_A_Box->setTitle(tr("Triode A Health"));
        }
    }

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
    // Reset to the first test type in the list for the new device and apply
    // the corresponding analyser parameter rules.
    ui->testType->setCurrentIndex(0);
    on_testType_currentIndexChanged(0);
}

void ValveWorkbench::on_testType_currentIndexChanged(int index)
{
    const int newTestType = ui->testType->itemData(index).toInt();

    // If we have per-test defaults loaded from analyserDefaults.tests,
    // restore the ranges/limits corresponding to the newly selected test
    // type before updating the UI fields.
    if (!analyserTestsDefaults.isEmpty()) {
        QJsonObject snapshot;
        for (auto it = analyserTestsDefaults.begin(); it != analyserTestsDefaults.end(); ++it) {
            if (!it.value().isObject()) {
                continue;
            }
            const QJsonObject tObj = it.value().toObject();
            const int tType = tObj.value(QStringLiteral("testType")).toInt(-1);
            if (tType == newTestType) {
                snapshot = tObj;
                break;
            }
        }

        if (!snapshot.isEmpty()) {
            auto setRangeFrom = [&](const char *key, double &start, double &stop, double &step) {
                const QJsonObject r = snapshot.value(QLatin1String(key)).toObject();
                if (!r.isEmpty()) {
                    start = r.value(QStringLiteral("start")).toDouble(start);
                    stop  = r.value(QStringLiteral("stop")).toDouble(stop);
                    step  = r.value(QStringLiteral("step")).toDouble(step);
                }
            };
            setRangeFrom("anode",  anodeStart,  anodeStop,  anodeStep);
            setRangeFrom("grid",   gridStart,   gridStop,   gridStep);
            setRangeFrom("screen", screenStart, screenStop, screenStep);

            const QJsonObject lim2 = snapshot.value(QStringLiteral("limits")).toObject();
            if (!lim2.isEmpty()) {
                iaMax = lim2.value(QStringLiteral("iaMax")).toDouble(iaMax);
                pMax  = lim2.value(QStringLiteral("pMax")).toDouble(pMax);
            }
        }
    }

    updateParameterDisplay();

    // For single-triode devices the Screen row is unused, so keep it blank.
    // In Double Triode mode the Screen row is repurposed as a read-only
    // display of the second anode range and should not be cleared here.
    if (deviceType == TRIODE && !isDoubleTriode) {
        ui->screenStart->setText("");
        ui->screenStop->setText("");
        ui->screenStep->setText("");
    }

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
        // For transfer tests, allow the user to adjust the effective grid
        // resolution via gridStep. The analyser maps this to the sweep point
        // density while Health runs keep their own fixed high resolution.
        ui->gridStep->setEnabled(true);
        if (deviceType == PENTODE) { // Anode fixed and Screen stepped
            ui->anodeStop->setEnabled(false);
            ui->anodeStop->setText("");
            ui->anodeStep->setEnabled(false);
            ui->anodeStep->setText("");
            ui->screenStop->setEnabled(true);
            ui->screenStep->setEnabled(true);
        } else { // (Triode / Double Triode) anode stepped in hardware
            // Keep the anode range as an advanced control and avoid
            // presenting step/stop as primary inputs for a grid transfer test.
            ui->anodeStop->setEnabled(false);
            ui->anodeStop->setText("");
            ui->anodeStep->setEnabled(false);
            ui->anodeStep->setText("");
            // Ensure a sensible default step is available internally even if
            // the field is hidden.
            if (anodeStep <= 0.0) {
                anodeStep = 25.0;
            }
            // For triode transfer tests we conceptually want a single anode
            // voltage (fixed Va) while sweeping grid. Clamp the anode range
            // so the analyser only uses anodeStart.
            anodeStop = anodeStart;
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
    // For triode transfer characteristics we conceptually want a single
    // anode voltage (fixed Va) while sweeping grid. If the user edits Anode
    // Start while TRANSFER_CHARACTERISTICS is selected, keep the stop value
    // locked to the same voltage so the analyser only generates one anode
    // family.
    if (testType == TRANSFER_CHARACTERISTICS && deviceType != PENTODE) {
        anodeStop = anodeStart;
    }
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

void ValveWorkbench::on_datasheetVg2_editingFinished()
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

    if (ui->heaterVlcd) {
        ui->heaterVlcd->display(QString::number(analyser->getAveragingSamples()));
    }

    if (ui->heaterIlcd) {
        ui->heaterIlcd->display(QString::number(analyser->getRetryLimitExceededCount()));
    }
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
    if (currentMeasurementItem && getProject(currentMeasurementItem) == currentProject) {
        QTreeWidgetItem *mItem = currentMeasurementItem;
        if (mItem->type() != TYP_MEASUREMENT) {
            mItem = getParent(currentMeasurementItem, TYP_MEASUREMENT);
        }
        if (mItem && mItem->type() == TYP_MEASUREMENT) {
            Measurement *selected = (Measurement *) mItem->data(0, Qt::UserRole).value<void *>();
            if (selected && selected->getDeviceType() == TRIODE && selected->getTestType() == ANODE_CHARACTERISTICS) {
                measurement = selected;
            }
        }
    }
    if (measurement == nullptr) {
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

    Model *seedModel = nullptr;
    if (currentDevice && currentDevice->getDeviceType() == TRIODE) {
        Model *deviceModel = currentDevice->getModel();
        if (deviceModel && deviceModel->getType() == COHEN_HELIE_TRIODE) {
            seedModel = deviceModel;
        }
    }
    if (seedModel == nullptr) {
        seedModel = findModel(COHEN_HELIE_TRIODE);
    }

    if (seedModel) {
        estimate.setMu(seedModel->getParameter(PAR_MU));
        estimate.setKg1(seedModel->getParameter(PAR_KG1));
        estimate.setX(seedModel->getParameter(PAR_X));
        estimate.setKp(seedModel->getParameter(PAR_KP));
        estimate.setKvb(seedModel->getParameter(PAR_KVB));
        estimate.setKvb1(seedModel->getParameter(PAR_KVB1));
        estimate.setVct(seedModel->getParameter(PAR_VCT));
    } else {
        estimate.estimateTriode(measurement);
    }

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

    if (measurementHasTriodeBData(measurement)) {
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

    if (triodeMeasurementPrimary != nullptr) {
        model->addMeasurement(triodeMeasurementPrimary);
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

    Model *completedModel = model;
    const bool completedWasTriodePrimary = (model == triodeModelPrimary);

    if (modelProject == nullptr) {
        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);
        if (ui && ui->processModellingTestsButton) ui->processModellingTestsButton->setEnabled(true);
        return;
    }

    if (!model->isConverged()) {
        QMessageBox message;
        message.setText("The model fitting did not converge - please check that your measurements are valid");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);
        if (ui && ui->processModellingTestsButton) ui->processModellingTestsButton->setEnabled(true);

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

            Model *seedModel = nullptr;
            if (completedWasTriodePrimary && completedModel && completedModel->getType() == COHEN_HELIE_TRIODE) {
                seedModel = completedModel;
            }
            if (seedModel == nullptr && currentDevice && currentDevice->getDeviceType() == TRIODE) {
                Model *deviceModel = currentDevice->getModel();
                if (deviceModel && deviceModel->getType() == COHEN_HELIE_TRIODE) {
                    seedModel = deviceModel;
                }
            }
            if (seedModel == nullptr) {
                seedModel = findModel(COHEN_HELIE_TRIODE);
            }

            if (seedModel) {
                secondaryEstimate.setMu(seedModel->getParameter(PAR_MU));
                secondaryEstimate.setKg1(seedModel->getParameter(PAR_KG1));
                secondaryEstimate.setX(seedModel->getParameter(PAR_X));
                secondaryEstimate.setKp(seedModel->getParameter(PAR_KP));
                secondaryEstimate.setKvb(seedModel->getParameter(PAR_KVB));
                secondaryEstimate.setKvb1(seedModel->getParameter(PAR_KVB1));
                secondaryEstimate.setVct(seedModel->getParameter(PAR_VCT));
            } else {
                secondaryEstimate.estimateTriode(clone);
            }

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
    if (ui && ui->processModellingTestsButton) ui->processModellingTestsButton->setEnabled(true);
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
            bool triodeBVisible = ui->measureCheck && ui->measureCheck->isChecked();
            if (ui->tabWidget &&
                ui->tabWidget->currentWidget() == ui->tab_2 &&
                isDoubleTriode &&
                ui->screenCheck) {
                triodeBVisible = ui->measureCheck->isChecked() && ui->screenCheck->isChecked();
            }
            measuredCurvesSecondary->setVisible(triodeBVisible);
        }
    }

    // If the model that just finished is a pentode fit, prefer to show the
    // latest pentode anode-characteristics measurement in the Designer plot
    // so the red model curves immediately overlay the data we just fitted,
    // rather than leaving the previous triode measurement visible.
    if (model && (model->getType() == GARDINER_PENTODE ||
                  model->getType() == REEFMAN_DERK_PENTODE ||
                  model->getType() == REEFMAN_DERK_E_PENTODE ||
                  model->getType() == EXTRACT_DERK_E_PENTODE ||
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

// Modeller: entry point for pentode fitting from the UI.
// - Records the current project node as the target for the fitted model.
// - Disables both Fit Pentode/Triode buttons to avoid concurrent runs.
// - Sets a flag so that, if a triode B fit finishes and wants to chain
//   into a pentode fit, only a single pentode modelling pass is started.
// - Delegates to modelPentode(), which selects the active pentode model
//   type and kicks off the appropriate solve or manual path.
void ValveWorkbench::on_fitPentodeButton_clicked()
{
    modelProject = currentProject;
    ui->fitPentodeButton->setEnabled(false); // Prevent any further modelling invocations
    ui->fitTriodeButton->setEnabled(false);
    doPentodeModel = true;

    modelPentode();

}

void ValveWorkbench::on_processModellingTestsButton_clicked()
{
    QTreeWidgetItem *projectItem = currentProject;
    if (projectItem && projectItem->type() != TYP_PROJECT) {
        projectItem = getParent(projectItem, TYP_PROJECT);
    }

    if (!projectItem || projectItem->type() != TYP_PROJECT) {
        QMessageBox::warning(this, tr("Process Modelling Tests"), tr("Please select a project in the project tree."));
        return;
    }

    QList<Measurement *> measurements = collectModellingTestMeasurements(projectItem);
    if (measurements.isEmpty()) {
        QMessageBox::warning(this, tr("Process Modelling Tests"), tr("No Modelling Tests measurements were found in this project."));
        return;
    }

    Measurement *triodeConnected = nullptr;
    QList<Measurement *> pentodeAnodes;
    QList<Measurement *> transfers;
    for (Measurement *m : measurements) {
        if (!m) {
            continue;
        }
        if (m->isTriodeConnectedPentode()) {
            triodeConnected = m;
            continue;
        }
        if (m->getDeviceType() == PENTODE && m->getTestType() == ANODE_CHARACTERISTICS) {
            pentodeAnodes.append(m);
            continue;
        }
        if (m->getDeviceType() == PENTODE && m->getTestType() == TRANSFER_CHARACTERISTICS) {
            transfers.append(m);
            continue;
        }
    }

    if (pentodeAnodes.isEmpty()) {
        QMessageBox::warning(this, tr("Process Modelling Tests"), tr("No pentode anode-characteristics Modelling Tests measurements were found in this project."));
        return;
    }

    auto parseVg2FromLabel = [](const QString &label, bool *okOut) -> double {
        if (okOut) *okOut = false;
        QRegularExpression re(QStringLiteral(R"(Vg2\s*=\s*(\d+(?:\.\d+)?)\s*V)"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(label);
        if (!m.hasMatch()) {
            return 0.0;
        }
        bool ok = false;
        const double vg2 = m.captured(1).toDouble(&ok);
        if (okOut) *okOut = ok;
        return ok ? vg2 : 0.0;
    };

    Measurement *seedMeasurement = nullptr;
    for (Measurement *m : pentodeAnodes) {
        if (!m) continue;
        bool ok = false;
        const double vg2 = parseVg2FromLabel(m->getCustomLabel(), &ok);
        if (ok && std::fabs(vg2 - 150.0) < 0.5) {
            seedMeasurement = m;
            break;
        }
    }
    if (seedMeasurement == nullptr) {
        seedMeasurement = pentodeAnodes.first();
    }

    modelProject = projectItem;
    if (ui && ui->fitPentodeButton) ui->fitPentodeButton->setEnabled(false);
    if (ui && ui->fitTriodeButton) ui->fitTriodeButton->setEnabled(false);
    if (ui && ui->processModellingTestsButton) ui->processModellingTestsButton->setEnabled(false);

    CohenHelieTriode *triodeModel = (CohenHelieTriode *) findModel(COHEN_HELIE_TRIODE);
    if (triodeModel == nullptr && currentDevice && currentDevice->getDeviceType() == PENTODE && currentDevice->getTriodeSeed() != nullptr) {
        triodeModel = currentDevice->getTriodeSeed();
    }

    std::unique_ptr<CohenHelieTriode> triodeSeedFromMeasurement;
    if (triodeModel == nullptr && triodeConnected != nullptr) {
        Estimate triodeEstimate;
        triodeEstimate.estimateTriode(triodeConnected);
        triodeSeedFromMeasurement = std::make_unique<CohenHelieTriode>();
        triodeSeedFromMeasurement->setEstimate(&triodeEstimate);
        triodeModel = triodeSeedFromMeasurement.get();
    }

    Estimate estimate;
    estimate.estimatePentode(seedMeasurement, triodeModel, EXTRACT_DERK_E_PENTODE, preferencesDialog.useSecondaryEmission());

    model = ModelFactory::createModel(EXTRACT_DERK_E_PENTODE);
    if (!model) {
        if (ui && ui->fitPentodeButton) ui->fitPentodeButton->setEnabled(true);
        if (ui && ui->fitTriodeButton) ui->fitTriodeButton->setEnabled(true);
        if (ui && ui->processModellingTestsButton) ui->processModellingTestsButton->setEnabled(true);
        QMessageBox::warning(this, tr("Process Modelling Tests"), tr("Failed to create ExtractModel pentode model."));
        return;
    }

    model->setEstimate(&estimate);
    model->setMode(NORMAL_MODE);
    model->setPreferences(&preferencesDialog);
    model->setPlotColor(QColor::fromRgb(255, 0, 0));

    for (Measurement *m : pentodeAnodes) {
        if (m) {
            model->addMeasurement(m);
        }
    }

    auto makeBinnedTransfer = [&](const QList<Measurement *> &src) -> std::unique_ptr<Measurement> {
        if (src.isEmpty()) {
            return nullptr;
        }

        double vaNominal = 0.0;
        double vg2Nominal = 0.0;
        bool foundNominal = false;
        for (Measurement *m : src) {
            if (!m) continue;
            if (m->count() <= 0) continue;
            Sweep *sw = m->at(0);
            if (!sw) continue;
            vaNominal = sw->getVaNominal();
            vg2Nominal = sw->getVg2Nominal();
            foundNominal = true;
            break;
        }

        std::unique_ptr<Measurement> binned = std::make_unique<Measurement>();
        binned->setDeviceType(PENTODE);
        binned->setTestType(TRANSFER_CHARACTERISTICS);

        // Mirror configuration values so Model::addMeasurement() filtering behaves consistently.
        Measurement *ref = src.first();
        if (ref) {
            binned->setAnodeStart(ref->getAnodeStart());
            binned->setAnodeStop(ref->getAnodeStop());
            binned->setAnodeStep(ref->getAnodeStep());
            binned->setGridStart(ref->getGridStart());
            binned->setGridStop(ref->getGridStop());
            binned->setGridStep(ref->getGridStep());
            binned->setScreenStart(ref->getScreenStart());
            binned->setScreenStop(ref->getScreenStop());
            binned->setScreenStep(ref->getScreenStep());
            binned->setIaMax(ref->getIaMax());
            binned->setPMax(ref->getPMax());
            binned->setHeaterVoltage(ref->getHeaterVoltage());
        }

        QString label = src.first() ? src.first()->measurementName() : QString();
        if (!label.isEmpty()) {
            binned->setCustomLabel(label + tr(" (binned)"));
        }

        if (!foundNominal) {
            // Fall back to configured test values if sweep nominal metadata is absent.
            if (ref) {
                vaNominal = ref->getAnodeStart();
                vg2Nominal = ref->getScreenStart();
            }
        }
        binned->nextSweep(vg2Nominal, vaNominal);

        // Bin width: use at least 0.25V, and never smaller than the configured gridStep.
        double binWidth = 0.25;
        if (ref) {
            const double step = std::fabs(ref->getGridStep());
            if (step > binWidth) {
                binWidth = step;
            }
        }
        if (!(binWidth > 0.0)) {
            binWidth = 0.25;
        }

        struct Acc {
            double sumVg1 = 0.0;
            double sumVa = 0.0;
            double sumIa = 0.0;
            double sumVg2 = 0.0;
            double sumIg2 = 0.0;
            int n = 0;
        };

        std::map<long long, Acc> bins;

        for (Measurement *m : src) {
            if (!m) continue;
            for (int si = 0; si < m->count(); ++si) {
                Sweep *sw = m->at(si);
                if (!sw) continue;
                for (int sj = 0; sj < sw->count(); ++sj) {
                    Sample *s = sw->at(sj);
                    if (!s) continue;
                    const double vg1 = s->getVg1();
                    const double va = s->getVa();
                    const double ia = s->getIa();
                    const double vg2 = s->getVg2();
                    const double ig2 = s->getIg2();
                    if (!std::isfinite(vg1) || !std::isfinite(va) || !std::isfinite(ia)) {
                        continue;
                    }

                    const long long key = static_cast<long long>(std::llround(vg1 / binWidth));
                    Acc &a = bins[key];
                    a.sumVg1 += vg1;
                    a.sumVa += va;
                    a.sumIa += ia;
                    if (std::isfinite(vg2)) a.sumVg2 += vg2;
                    if (std::isfinite(ig2)) a.sumIg2 += ig2;
                    a.n += 1;
                }
            }
        }

        int outPoints = 0;
        for (auto it = bins.begin(); it != bins.end(); ++it) {
            const Acc &a = it->second;
            if (a.n <= 0) {
                continue;
            }

            const double vg1 = a.sumVg1 / static_cast<double>(a.n);
            const double va = a.sumVa / static_cast<double>(a.n);
            const double ia = a.sumIa / static_cast<double>(a.n);
            const double vg2 = (a.sumVg2 != 0.0) ? (a.sumVg2 / static_cast<double>(a.n)) : vg2Nominal;
            const double ig2 = (a.sumIg2 != 0.0) ? (a.sumIg2 / static_cast<double>(a.n)) : 0.0;

            binned->addSample(new Sample(vg1, va, ia, vg2, ig2));
            outPoints++;
        }

        // Update IaMax for plotting/axis if needed.
        if (outPoints > 0) {
            double maxIa = 0.0;
            for (int si = 0; si < binned->count(); ++si) {
                Sweep *sw = binned->at(si);
                if (!sw) continue;
                for (int sj = 0; sj < sw->count(); ++sj) {
                    Sample *s = sw->at(sj);
                    if (s && std::isfinite(s->getIa())) {
                        maxIa = std::max(maxIa, s->getIa());
                    }
                }
            }
            if (maxIa > 0.0) {
                binned->setIaMax(std::max(binned->getIaMax(), maxIa * 1.05));
            }
        }

        return binned;
    };

    struct TransferCondition {
        int vaKey = 0;   // rounded volts
        int vg2Key = 0;  // rounded volts
        bool operator<(const TransferCondition &other) const {
            if (vaKey != other.vaKey) return vaKey < other.vaKey;
            return vg2Key < other.vg2Key;
        }
    };

    auto transferConditionOf = [](Measurement *m) -> TransferCondition {
        TransferCondition c;
        if (!m) return c;
        // Prefer sweep nominal metadata (it captures pentode transfer's Va/Vg2 pairing).
        if (m->count() > 0) {
            Sweep *sw = m->at(0);
            if (sw) {
                const double va = sw->getVaNominal();
                const double vg2 = sw->getVg2Nominal();
                if (std::isfinite(va))  c.vaKey = static_cast<int>(std::lround(va));
                if (std::isfinite(vg2)) c.vg2Key = static_cast<int>(std::lround(vg2));
                return c;
            }
        }
        // Fallback to measurement configuration.
        c.vaKey = static_cast<int>(std::lround(m->getAnodeStart()));
        c.vg2Key = static_cast<int>(std::lround(m->getScreenStart()));
        return c;
    };

    std::map<TransferCondition, QList<Measurement *>> transferGroups;
    for (Measurement *m : transfers) {
        if (!m) continue;
        transferGroups[transferConditionOf(m)].append(m);
    }

    processModellingTestsBinnedTransfers.clear();
    processModellingTestsBinnedTransfers.reserve(static_cast<size_t>(transferGroups.size()));

    struct TransferGroupReport {
        int vaKey = 0;
        int vg2Key = 0;
        int measurements = 0;
        int binnedPoints = 0;
    };
    QList<TransferGroupReport> transferReport;

    int transferBinnedPoints = 0;
    for (auto it = transferGroups.begin(); it != transferGroups.end(); ++it) {
        const TransferCondition &cond = it->first;
        const QList<Measurement *> &group = it->second;
        std::unique_ptr<Measurement> binned = makeBinnedTransfer(group);
        int points = 0;
        if (binned) {
            for (int si = 0; si < binned->count(); ++si) {
                Sweep *sw = binned->at(si);
                if (sw) points += sw->count();
            }
            transferBinnedPoints += points;
            model->addMeasurement(binned.get());
            processModellingTestsBinnedTransfers.emplace_back(std::move(binned));
        }
        TransferGroupReport r;
        r.vaKey = cond.vaKey;
        r.vg2Key = cond.vg2Key;
        r.measurements = group.size();
        r.binnedPoints = points;
        transferReport.append(r);
    }

    QStringList used;
    used << tr("Seed measurement: %1").arg(seedMeasurement ? seedMeasurement->measurementName() : tr("(none)"));
    used << tr("Triode seed: %1").arg(triodeModel ? tr("available") : tr("none"));
    used << tr("Included pentode anode measurements:");
    for (Measurement *m : pentodeAnodes) {
        used << tr("  - %1").arg(m ? m->measurementName() : tr("(null)"));
    }
    if (!transfers.isEmpty()) {
        used << tr("Included transfer measurements: %1 (total binned points: %2)")
                    .arg(transfers.size())
                    .arg(transferBinnedPoints);
        for (const TransferGroupReport &r : std::as_const(transferReport)) {
            used << tr("  - Va=%1V, Vg2=%2V: %3 runs -> %4 points")
                        .arg(r.vaKey)
                        .arg(r.vg2Key)
                        .arg(r.measurements)
                        .arg(r.binnedPoints);
        }
        for (Measurement *m : transfers) {
            if (m) {
                used << tr("  - %1").arg(m->measurementName());
            }
        }
    }
    if (triodeConnected) {
        used << tr("(Note) Triode-connected measurement was detected and used only for seeding: %1").arg(triodeConnected->measurementName());
    }
    qInfo().noquote() << used.join("\n");
    QMessageBox::information(this, tr("Process Modelling Tests"), used.join("\n"));

    thread = new QThread;
    model->moveToThread(thread);
    disconnect(model, nullptr, this, nullptr);
    connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    QMetaObject::invokeMethod(model, "solveThreaded");
}

// Modeller: orchestrate pentode modelling for the currently selected
// device and measurement set.
//
// High-level flow:
//  - Locate the primary PENTODE/ANODE_CHARACTERISTICS measurement; abort
//    with a message box if none is present.
//  - If the active pentode model type is SIMPLE_MANUAL_PENTODE:
//      * Build an Estimate using Estimate::estimatePentode targeting
//        GARDINER_PENTODE so the manual model shares the same heuristics
//        as the Ceres-based path.
//      * Optionally run a one-shot GardinerPentode Ceres solve to refine
//        that seed before copying parameters into SimpleManualPentode.
//      * Create a SimpleManualPentode instance, copy the fitted or
//        estimated parameters into its sliders, overlay its curves over
//        the current pentode measurement, and open the manual pentode
//        dialog so the user can tweak parameters interactively.
//  - Otherwise (Gardiner/Reefman pentode models):
//      * Build an Estimate from, in order of preference, an explicit
//        triode model in the project, an embedded triode seed on the
//        current Device, or the Device's own pentode parameters, falling
//        back to a legacy gradient-based estimate when no seed exists.
//      * Create the requested Ceres-based pentode Model, attach all
//        suitable pentode ANODE_CHARACTERISTICS measurements in the
//        project tree, and run solveThreaded() in a background QThread,
//        delivering results back via Model::modelReady/loadModel().
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
        estimate.estimatePentode(measurement, triodeModel, GARDINER_PENTODE, preferencesDialog.useSecondaryEmission());

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
        estimate.estimatePentode(measurement, triodeModel, pentodeModelType, preferencesDialog.useSecondaryEmission());
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
        estimate.estimatePentode(measurement, nullptr, pentodeModelType, preferencesDialog.useSecondaryEmission());
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
    // Skip this while a test is running (Run button checked) to avoid
    // interfering with analyser callbacks and plot updates in progress.
    const bool testRunning = (ui && ui->runButton && ui->runButton->isChecked());
    if (!testRunning) {
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
                    if (ui->processModellingTestsButton) ui->processModellingTestsButton->setVisible(false);
                } else if (project->getDeviceType() == PENTODE) {
                    ui->fitTriodeButton->setVisible(false);
                    ui->fitPentodeButton->setVisible(true);
                    if (ui->processModellingTestsButton) ui->processModellingTestsButton->setVisible(true);
                    if (pentodeModelType == SIMPLE_MANUAL_PENTODE) {
                        ensureSimplePentodeDialog();
                    }
                } else {
                    ui->fitTriodeButton->setVisible(false);
                    ui->fitPentodeButton->setVisible(false);
                    if (ui->processModellingTestsButton) ui->processModellingTestsButton->setVisible(false);
                }
            }
        } else {
            ui->fitTriodeButton->setVisible(false);
            ui->fitPentodeButton->setVisible(false);
            if (ui->processModellingTestsButton) ui->processModellingTestsButton->setVisible(false);
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
    if (measuredCurvesSecondary != nullptr && !wantVisible) {
        measuredCurvesSecondary->setVisible(false);
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
            }
        }
        if (measuredCurvesSecondary) {
            const bool triodeBVisible =
                isDoubleTriode &&
                ui->screenCheck && ui->screenCheck->isChecked() &&
                ui->measureCheck && ui->measureCheck->isChecked();
            measuredCurvesSecondary->setVisible(triodeBVisible);
        }
    }

    // Modeller tab (role 1): use currentMeasurement and any available
    // Triode B clone for secondary overlays.
    if (tabRole == 1 && currentMeasurement) {
        qInfo("ValveWorkbench::on_measureCheck_stateChanged: rebuilding Modeller measurement curves");
        if (currentMeasurement->getDeviceType() == PENTODE) {
            currentMeasurement->setShowScreen(ui->screenCheck && ui->screenCheck->isChecked());
        }
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
                bool triodeBVisible = isDoubleTriode && ui->screenCheck && ui->screenCheck->isChecked()
                                      && ui->measureCheck && ui->measureCheck->isChecked();
                measuredCurvesSecondary->setVisible(triodeBVisible);
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
        // For Triode CC, reset manual Headroom (Vpk) whenever Max Sym Swing is
        // toggled so that helper-derived symmetric/max swing becomes the
        // effective headroom source again when headroom == 0, mirroring the
        // SE output stage behaviour.
        t->setParameter(TRI_CC_HEADROOM, 0.0);
        t->setSymSwingEnabled(enabled);
        t->plot(&plot);
        t->updateUI(circuitLabels, circuitValues);
        updateHeadroomWaveformView(t);
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
        updateHeadroomWaveformView(t);
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

    // If a datasheet reference operating point is available for triodes,
    // pre-fill the triode test conditions so Compare uses the same Va/Vg
    // that Health and datasheet comparisons use.
    if (project->getDeviceType() == TRIODE) {
        double va0 = 0.0;
        double vg0 = 0.0;
        double ia0 = 0.0;
        double gm0 = 0.0;
        double mu0 = 0.0;
        double rp0 = 0.0;
        if (ensureDatasheetRefPoint(va0, vg0, ia0, gm0, mu0, rp0)) {
            dialog.setTriodeTestConditions(va0, vg0);
        }
    }

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

    int exportDeviceType = deviceType;
    const int exportModelType = toExport->getType();
    switch (exportModelType) {
    case REEFMAN_DERK_PENTODE:
    case REEFMAN_DERK_E_PENTODE:
    case EXTRACT_DERK_E_PENTODE:
    case GARDINER_PENTODE:
    case SIMPLE_MANUAL_PENTODE:
        exportDeviceType = PENTODE;
        break;
    case SIMPLE_TRIODE:
    case KOREN_TRIODE:
    case COHEN_HELIE_TRIODE:
        exportDeviceType = TRIODE;
        break;
    default:
        break;
    }

    // Prefer an analyser measurement (if available) so exported presets carry
    // real sweep ranges/limits and can overlay measured curves in Designer.
    // For pentodes, pick the anode measurement whose screen voltage is
    // closest to the current analyser screen setting (fallback: highest Vg2).
    Measurement *measForExport = nullptr;
    if (exportDeviceType == PENTODE && currentProject != nullptr) {
        // Option 2: treat the intended screen voltage as the active Designer
        // output-stage VS (when an output circuit is selected). Fall back to
        // the Analyser screenStart otherwise.
        double targetVg2 = screenStart;
        {
            int circuitType = -1;
            if (ui && ui->circuitSelection) {
                circuitType = ui->circuitSelection->currentData().toInt();
            }
            if (circuitType >= 0 && circuitType < circuits.size()) {
                Circuit *c = circuits.at(circuitType);
                if (c) {
                    int vsIndex = -1;
                    switch (circuitType) {
                    case SINGLE_ENDED_OUTPUT:        vsIndex = SE_VS;   break;
                    case ULTRALINEAR_SINGLE_ENDED:   vsIndex = SEUL_VB; break;
                    case PUSH_PULL_OUTPUT:           vsIndex = PP_VS;   break;
                    case ULTRALINEAR_PUSH_PULL:      vsIndex = PPUL_VB; break;
                    default:                         vsIndex = -1;      break;
                    }

                    if (vsIndex >= 0) {
                        const double designerVs = c->getParameter(vsIndex);
                        if (std::isfinite(designerVs) && designerVs > 0.0) {
                            targetVg2 = designerVs;
                        }
                    }
                }
            }
        }

        auto representativeVg2 = [](Measurement *m) -> double {
            if (!m) return 0.0;
            if (std::isfinite(m->getScreenStart()) && m->getScreenStart() > 0.0) {
                return m->getScreenStart();
            }
            if (m->count() > 0) {
                Sweep *s = m->at(0);
                if (s && std::isfinite(s->getVg2Nominal()) && s->getVg2Nominal() > 0.0) {
                    return s->getVg2Nominal();
                }
            }
            return 0.0;
        };

        Measurement *best = nullptr;
        double bestScore = std::numeric_limits<double>::infinity();
        double bestVg2 = 0.0;

        const int children = currentProject->childCount();
        for (int i = 0; i < children; ++i) {
            QTreeWidgetItem *child = currentProject->child(i);
            if (!child || child->type() != TYP_MEASUREMENT) continue;
            Measurement *m = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (!m) continue;
            if (m->getDeviceType() != PENTODE || m->getTestType() != ANODE_CHARACTERISTICS) continue;

            const double vg2 = representativeVg2(m);
            if (!(vg2 > 0.0)) continue;

            if (std::isfinite(targetVg2) && targetVg2 > 0.0) {
                const double score = std::fabs(vg2 - targetVg2);
                if (!best || score < bestScore) {
                    best = m;
                    bestScore = score;
                    bestVg2 = vg2;
                }
            } else {
                if (!best || vg2 > bestVg2) {
                    best = m;
                    bestVg2 = vg2;
                }
            }
        }

        if (currentMeasurement &&
            currentMeasurement->getDeviceType() == PENTODE &&
            currentMeasurement->getTestType() == ANODE_CHARACTERISTICS) {
            const double currVg2 = representativeVg2(currentMeasurement);
            if (best) {
                if (std::isfinite(targetVg2) && targetVg2 > 0.0) {
                    if (std::fabs(currVg2 - targetVg2) <= std::fabs(bestVg2 - targetVg2)) {
                        best = currentMeasurement;
                    }
                } else {
                    if (currVg2 >= bestVg2) {
                        best = currentMeasurement;
                    }
                }
            } else {
                best = currentMeasurement;
            }
        }

        measForExport = best;
    } else {
        if (currentMeasurement &&
            currentMeasurement->getDeviceType() == exportDeviceType &&
            currentMeasurement->getTestType() == ANODE_CHARACTERISTICS) {
            measForExport = currentMeasurement;
        } else {
            measForExport = findMeasurement(exportDeviceType, ANODE_CHARACTERISTICS);
        }
    }

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
    double iaMaxHardwareOut = 5.0;
    double paMaxOut = 1.125;

    if (std::isfinite(anodeStop) && anodeStop > 0.0) {
        vaMaxOut = anodeStop;
    }
    if (std::isfinite(iaMax) && iaMax > 0.0) {
        iaMaxOut = iaMax;
    }
    iaMaxHardwareOut = iaMaxOut;
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
        iaMaxHardwareOut = iaMaxOut;
        if (std::isfinite(measPMax) && measPMax > 0.0) {
            paMaxOut = measPMax;
        }
    }

    if (exportDeviceType == PENTODE) {
        // Give pentode Designer circuits enough voltage headroom.
        if (vaMaxOut < 500.0) {
            vaMaxOut = 500.0;
        }

        // Derive vg1Max from the analyser grid stop magnitude so that
        // Designer plots use a comparable grid-voltage family to the
        // measured curves (e.g. 0 .. -40 V for a 6L6GC).
        double vg1MaxOut = 0.0;
        double vgStartOut = gridStart;
        double vgStopOut  = gridStop;
        if (measForExport) {
            vgStartOut = measForExport->getGridStart();
            vgStopOut  = measForExport->getGridStop();
        }
        vg1MaxOut = std::max(std::fabs(vgStartOut), std::fabs(vgStopOut));
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
        // the analyser cannot exceed the 50 mA capability of the hardware.
        if (!(iaMaxOut > 0.0)) {
            iaMaxOut = 5.0; // conservative fallback if analyser Ia is unset
        }
        iaMaxHardwareOut = iaMaxOut;
        if (iaMaxHardwareOut > 50.0) {
            iaMaxHardwareOut = 50.0;
        }
    }

    root["vaMax"] = vaMaxOut;
    if (exportDeviceType == TRIODE) {
        root["vg1Max"] = 5.0;
    }
    root["iaMax"] = iaMaxOut;
    root["paMax"] = paMaxOut;

    // Device type string for presets
    if (exportDeviceType == TRIODE) {
        root["deviceType"] = "TRIODE";
    } else if (exportDeviceType == PENTODE) {
        root["deviceType"] = "PENTODE";
    } else if (exportDeviceType == DOUBLE_TRIODE) {
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
        limitsObj["iaMax"] = iaMaxHardwareOut;
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

        // Grid range: for triode anode-characteristics presets, prefer the
        // analyser UI's positive-magnitude grid range so templates keep
        // 0..+V defaults instead of inheriting the negative measurement
        // sweep. For all other cases, use the measurement snapshot.
        if (exportDeviceType == TRIODE && measType == ANODE_CHARACTERISTICS) {
            QJsonObject gridObj;
            double gridStartOut = gridStart;
            double gridStopOut  = gridStop;
            double gridStepOut  = gridStep;

            // Most analyser templates use positive magnitudes for "-ve Grid Voltage".
            // If both bounds are negative, clamp the domain to 0..|stop|.
            if (gridStartOut < 0.0 && gridStopOut <= 0.0) {
                gridStartOut = 0.0;
                if (gridStopOut < 0.0) {
                    gridStopOut = -gridStopOut;
                }
            }

            if (gridStartOut > gridStopOut) {
                std::swap(gridStartOut, gridStopOut);
            }

            gridStepOut = std::fabs(gridStepOut);
            if (!(gridStepOut > 0.0)) {
                gridStepOut = 0.5; // conservative fallback
            }

            gridObj.insert("start", gridStartOut);
            gridObj.insert("step",  gridStepOut);
            gridObj.insert("stop",  gridStopOut);
            snapshot.insert("grid", gridObj);
        } else {
            snapshot.insert("grid",   makeRangeFromMeas(&Measurement::getGridStart,
                                                         &Measurement::getGridStop,
                                                         &Measurement::getGridStep));
        }

        snapshot.insert("screen", makeRangeFromMeas(&Measurement::getScreenStart,
                                                     &Measurement::getScreenStop,
                                                     &Measurement::getScreenStep));

        QJsonObject testLimits;
        double testIaMaxOut = measForExport->getIaMax();
        if (exportDeviceType == PENTODE && testIaMaxOut > 50.0) {
            testIaMaxOut = 50.0;
        }
        testLimits.insert("iaMax", testIaMaxOut);
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
        QJsonObject spiceObj = buildSpiceBlockForModel(toExport, exportDeviceType, deviceName);
        if (!spiceObj.isEmpty()) {
            root["spice"] = spiceObj;
        }
    }

    // If a Cohen-Helie triode model exists in the project, embed its
    // parameters as a 'triodeModel' block so future pentode fits can reuse
    // the same triode seed without re-running the triode fit.
    if (exportDeviceType == PENTODE && currentProject != nullptr) {
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
