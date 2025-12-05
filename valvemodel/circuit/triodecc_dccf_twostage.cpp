#include "triodecc_dccf_twostage.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>

TriodeCcDccfTwoStage::TriodeCcDccfTwoStage()
{
    // Stage 1 inputs
    parameter[TCC_DCCF_VB1] = new Parameter("Stage 1 supply Vb1 (V)", 300.0);
    parameter[TCC_DCCF_RA1] = new Parameter("Stage 1 anode resistor Ra1 (\u03a9)", 100000.0);
    parameter[TCC_DCCF_RK1] = new Parameter("Stage 1 cathode resistor Rk1 (\u03a9)", 1500.0);

    // Stage 2 inputs
    parameter[TCC_DCCF_VB2] = new Parameter("Stage 2 supply Vb2 (V)", 300.0);
    parameter[TCC_DCCF_RA2] = new Parameter("Stage 2 anode resistor Ra2 (\u03a9)", 100000.0);
    parameter[TCC_DCCF_RK2] = new Parameter("Stage 2 cathode resistor Rk2 (\u03a9)", 1500.0);
    parameter[TCC_DCCF_RL2] = new Parameter("Stage 2 load resistor Rl2 (\u03a9)", 82000.0);

    // Stage 1 outputs
    parameter[TCC_DCCF_VA1]   = new Parameter("Stage 1 anode voltage Va1 (V)", 0.0);
    parameter[TCC_DCCF_VK1]   = new Parameter("Stage 1 cathode voltage Vk1 (V)", 0.0);
    parameter[TCC_DCCF_IA1]   = new Parameter("Stage 1 anode current Ia1 (mA)", 0.0);
    parameter[TCC_DCCF_GAIN1] = new Parameter("Stage 1 gain (approx Av)", 0.0);

    // Stage 2 outputs
    parameter[TCC_DCCF_VA2]   = new Parameter("Stage 2 anode voltage Va2 (V)", 0.0);
    parameter[TCC_DCCF_VK2]   = new Parameter("Stage 2 cathode voltage Vk2 (V)", 0.0);
    parameter[TCC_DCCF_IK2]   = new Parameter("Stage 2 cathode current Ik2 (mA)", 0.0);
    parameter[TCC_DCCF_GAIN2] = new Parameter("Stage 2 gain (approx Av)", 0.0);

    // Stage 2 headroom at the anode (Vpk) driving follower THD calculations.
    parameter[TCC_DCCF_HEADROOM2] = new Parameter("Stage 2 headroom (Vpk)", 0.0);
}

int TriodeCcDccfTwoStage::getDeviceType(int index)
{
    // Single triode device reused for both stages
    if (index == 1) {
        return TRIODE;
    }
    return -1;
}

QTreeWidgetItem *TriodeCcDccfTwoStage::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    // Designer-only circuit; it does not appear in the project tree.
    return nullptr;
}

void TriodeCcDccfTwoStage::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first 7 fields, editable
    for (int i = 0; i < 7; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);
            labels[i]->setStyleSheet(QString());
            values[i]->setStyleSheet(QString());
        }
    }

    auto setOutputRow = [&](int row,
                            const QString &labelText,
                            int paramIndex,
                            int decimals)
    {
        if (!labels[row] || !values[row]) {
            return;
        }

        labels[row]->setText(labelText);
        if (!device1) {
            values[row]->setText("N/A");
        } else if (paramIndex >= 0 && parameter[paramIndex]) {
            values[row]->setText(QString::number(parameter[paramIndex]->getValue(), 'f', decimals));
        } else {
            values[row]->setText("-");
        }

        labels[row]->setVisible(true);
        values[row]->setVisible(true);
        values[row]->setReadOnly(true);
        labels[row]->setStyleSheet(QString());
        values[row]->setStyleSheet(QString());
    };

    // Stage 1 DC outputs (we no longer show Stage 1 gain to free space)
    setOutputRow(7,  "Stage 1 anode voltage Va1 (V):",   TCC_DCCF_VA1, 3);
    setOutputRow(8,  "Stage 1 cathode voltage Vk1 (V):", TCC_DCCF_VK1, 3);
    setOutputRow(9,  "Stage 1 anode current Ia1 (mA):",  TCC_DCCF_IA1, 3);

    // Stage 2 headroom (manual Vpk at anode driving follower THD)
    if (labels[10] && values[10]) {
        labels[10]->setText("Stage 2 headroom (Vpk):");

        double headroom = 0.0;
        if (parameter[TCC_DCCF_HEADROOM2]) {
            headroom = parameter[TCC_DCCF_HEADROOM2]->getValue();
        }
        values[10]->setText(QString::number(headroom, 'f', 1));

        QString style;
        if (headroom > 0.0) {
            // Manual override active: bright blue, matching other Designer circuits.
            style = "color: rgb(0,0,255);";
        }
        labels[10]->setStyleSheet(style);
        values[10]->setStyleSheet(style);

        labels[10]->setVisible(true);
        values[10]->setVisible(true);
        values[10]->setReadOnly(false);
    }

    // Stage 2 DC outputs
    setOutputRow(11, "Stage 2 anode voltage Va2 (V):",       TCC_DCCF_VA2,   3);
    setOutputRow(12, "Stage 2 cathode voltage Vk2 (V):",     TCC_DCCF_VK2,   3);
    setOutputRow(13, "Stage 2 cathode current Ik2 (mA):",    TCC_DCCF_IK2,   3);
    setOutputRow(14, "Stage 2 gain (approx Av):",            TCC_DCCF_GAIN2, 3);

    // Stage 2 THD at headroom (%) – display-only VTADIY-style metric.
    if (labels[15] && values[15]) {
        const double headroom = parameter[TCC_DCCF_HEADROOM2]
                                    ? parameter[TCC_DCCF_HEADROOM2]->getValue()
                                    : 0.0;

        if (!device1 || headroom <= 0.0 || !(followerThdPct > 0.0) || !std::isfinite(followerThdPct)) {
            // No valid THD metric: hide the row to avoid stale values.
            values[15]->setText(QString());
            labels[15]->setStyleSheet(QString());
            values[15]->setStyleSheet(QString());
            labels[15]->setVisible(false);
            values[15]->setVisible(false);
        } else {
            labels[15]->setText("Stage 2 THD at headroom (%):");
            values[15]->setText(QString::number(followerThdPct, 'f', 1));

            // Manual headroom only for now: bright blue.
            const QString style = "color: rgb(0,0,255);";
            labels[15]->setStyleSheet(style);
            values[15]->setStyleSheet(style);

            labels[15]->setVisible(true);
            values[15]->setVisible(true);
            values[15]->setReadOnly(true);
        }
    }
}

bool TriodeCcDccfTwoStage::computeStage1(double &va1, double &vk1, double &ia1_mA, double &gain1)
{
    gain1 = 0.0;

    if (!device1) {
        return false;
    }

    const double vb1 = parameter[TCC_DCCF_VB1]->getValue();
    const double ra1 = parameter[TCC_DCCF_RA1]->getValue();
    const double rk1 = parameter[TCC_DCCF_RK1]->getValue();

    if (vb1 <= 0.0 || rk1 <= 0.0) {
        return false;
    }

    // Simple DC load line from (0, Ia_max) to (Vb1, 0) using (Ra1+Rk1).
    const double denom = (ra1 + rk1 > 0.0) ? (ra1 + rk1) : rk1;
    const double iaMax_mA = vb1 / denom * 1000.0;

    // Build cathode line as in TriodeDCCathodeFollower: Ia from Vg/Rk1.
    const double vgMax = device1->getVg1Max();
    const int steps = 100;

    QVector<QPointF> cathodeData;
    cathodeData.reserve(steps);
    for (int j = 1; j <= steps; ++j) {
        const double vg_mag = vgMax * static_cast<double>(j) / steps; // positive magnitude
        const double ia_mA = vg_mag * 1000.0 / rk1;
        if (ia_mA <= 0.0) {
            continue;
        }

        const double va = device1->anodeVoltage(ia_mA, -vg_mag);
        if (std::isfinite(va) && va > 0.0001) {
            cathodeData.push_back(QPointF(va, ia_mA));
        }
    }

    if (cathodeData.size() < 2) {
        return false;
    }

    // Anode line endpoints
    const QPointF aStart(0.0, iaMax_mA);
    const QPointF aEnd(vb1, 0.0);

    auto findIntersection = [](const QPointF &l1s, const QPointF &l1e,
                               const QPointF &l2s, const QPointF &l2e) -> QPointF {
        const double x1 = l1s.x(), y1 = l1s.y();
        const double x2 = l1e.x(), y2 = l1e.y();
        const double x3 = l2s.x(), y3 = l2s.y();
        const double x4 = l2e.x(), y4 = l2e.y();

        const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(denom) < 1e-12) {
            return QPointF(-1.0, -1.0);
        }

        const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;
        if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0) {
            return QPointF(-1.0, -1.0);
        }

        const double ix = x1 + t * (x2 - x1);
        const double iy = y1 + t * (y2 - y1);
        return QPointF(ix, iy);
    };

    QPointF op(-1.0, -1.0);
    for (int j = 0; j < cathodeData.size() - 1; ++j) {
        const QPointF cStart = cathodeData[j];
        const QPointF cEnd   = cathodeData[j + 1];
        const QPointF ip     = findIntersection(aStart, aEnd, cStart, cEnd);
        if (ip.x() >= 0.0 && ip.y() >= 0.0) {
            op = ip;
            break;
        }
    }

    if (!(op.x() >= 0.0 && op.y() >= 0.0)) {
        return false;
    }

    va1    = op.x();
    ia1_mA = op.y();
    const double ia1_A = ia1_mA / 1000.0;
    vk1    = ia1_A * rk1;

    // Approximate Stage 1 gain using the same approach as TriodeDCCathodeFollower.
    double mu = 0.0;
    double ar = 0.0;

    if (std::isfinite(va1) && std::isfinite(ia1_A)) {
        const double vg0 = -ia1_A * rk1; // self-bias grid voltage

        const double dVg = std::max(0.01, std::abs(vg0) * 0.02);
        const double vgMin = -device1->getVg1Max();
        const double vgPlus  = std::clamp(vg0 + dVg, vgMin, 0.0);
        const double vgMinus = std::clamp(vg0 - dVg, vgMin, 0.0);

        const double va_plusVg  = device1->anodeVoltage(ia1_mA, vgPlus);
        const double va_minusVg = device1->anodeVoltage(ia1_mA, vgMinus);
        if (std::isfinite(va_plusVg) && std::isfinite(va_minusVg) && (vgPlus - vgMinus) != 0.0) {
            const double dVa_dVg = (va_plusVg - va_minusVg) / (vgPlus - vgMinus);
            mu = -dVa_dVg;
        }

        const double ia1_mA_at_vb   = device1->anodeCurrent(vb1, -vk1);
        const double ia1_mA_at_vb10 = device1->anodeCurrent(vb1 - 10.0, -vk1);
        const double dIa_mA = ia1_mA_at_vb - ia1_mA_at_vb10;
        if (std::isfinite(ia1_mA_at_vb) && std::isfinite(ia1_mA_at_vb10) && std::abs(dIa_mA) > 1e-9) {
            ar = 10000.0 / dIa_mA;
        }
    }

    if (!std::isfinite(mu) || mu <= 0.0) {
        mu = 100.0;
    }
    if (!std::isfinite(ar) || ar <= 0.0) {
        ar = 1000.0;
    }

    const double re = ra1; // simple approximation: load is Ra1 only here
    double gain_bypassed = 0.0;
    if (re > 0.0 && std::isfinite(re)) {
        gain_bypassed = mu * re / (re + ar);
    }
    if (!std::isfinite(gain_bypassed)) {
        gain_bypassed = 0.0;
    }

    gain1 = gain_bypassed;
    return true;
}

bool TriodeCcDccfTwoStage::computeStage2(double va1,
                                         double &va2,
                                         double &vk2,
                                         double &ik2_mA,
                                         double &gain2)
{
    gain2 = 0.0;

    if (!device1) {
        return false;
    }

    const double vb2 = parameter[TCC_DCCF_VB2]->getValue();
    const double ra2 = parameter[TCC_DCCF_RA2]->getValue();
    const double rk2 = parameter[TCC_DCCF_RK2]->getValue();
    const double rl2 = parameter[TCC_DCCF_RL2]->getValue();

    if (vb2 <= 0.0 || ra2 <= 0.0 || rk2 <= 0.0 || rl2 <= 0.0) {
        return false;
    }

    // Effective cathode network for DC (Rk2 || Rl2), similar in spirit to the
    // Stage 1 treatment in TriodeDCCathodeFollower.
    const double rk_eq = (rk2 * rl2) / (rk2 + rl2);
    if (!(rk_eq > 0.0) || !std::isfinite(rk_eq)) {
        return false;
    }

    // Stage 2 anode line in Va-Ia space: approximate using Ra2 + rk_eq in the
    // denominator, mirroring the Stage 1 load line style.
    const double denom = ra2 + rk_eq;
    if (!(denom > 0.0)) {
        return false;
    }
    const double iaMax_mA = vb2 / denom * 1000.0;
    const QPointF aStart(0.0, iaMax_mA);
    const QPointF aEnd(vb2, 0.0);

    // Build a Stage 2 "tube + cathode network" curve in Va-Ia space by
    // sweeping Ia2 and using the triode model to obtain Va2 for the given
    // grid-to-cathode voltage set by Stage 1's anode (va1).
    QVector<QPointF> cathodeData2;
    const int steps = 120;
    const double iaStep_mA = iaMax_mA / static_cast<double>(steps);

    for (int j = 1; j <= steps; ++j) {
        const double ia_mA = iaStep_mA * static_cast<double>(j);
        if (ia_mA <= 0.0) {
            continue;
        }

        const double ia_A  = ia_mA / 1000.0;
        const double vk_eq = ia_A * rk_eq;         // cathode potential from RK2||RL2
        const double vgk   = va1 - vk_eq;          // grid-to-cathode voltage

        // Clamp grid bias into the model's supported range, matching other
        // triode circuits (negative for normal operation, up to 0 V).
        const double vg_min   = -device1->getVg1Max();
        const double vg_param = std::clamp(vgk, vg_min, 0.0);

        const double va_model = device1->anodeVoltage(ia_mA, vg_param);
        if (std::isfinite(va_model) && va_model > 0.0001 && va_model <= vb2 * 2.0) {
            cathodeData2.push_back(QPointF(va_model, ia_mA));
        }
    }

    if (cathodeData2.size() < 2) {
        return false;
    }

    auto findIntersection = [](const QPointF &l1s, const QPointF &l1e,
                               const QPointF &l2s, const QPointF &l2e) -> QPointF {
        const double x1 = l1s.x(), y1 = l1s.y();
        const double x2 = l1e.x(), y2 = l1e.y();
        const double x3 = l2s.x(), y3 = l2s.y();
        const double x4 = l2e.x(), y4 = l2e.y();

        const double d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(d) < 1e-12) {
            return QPointF(-1.0, -1.0);
        }

        const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / d;
        const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / d;
        if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0) {
            return QPointF(-1.0, -1.0);
        }

        const double ix = x1 + t * (x2 - x1);
        const double iy = y1 + t * (y2 - y1);
        return QPointF(ix, iy);
    };

    QPointF op(-1.0, -1.0);
    for (int j = 0; j < cathodeData2.size() - 1; ++j) {
        const QPointF cStart = cathodeData2[j];
        const QPointF cEnd   = cathodeData2[j + 1];
        const QPointF ip     = findIntersection(aStart, aEnd, cStart, cEnd);
        if (ip.x() >= 0.0 && ip.y() >= 0.0) {
            op = ip;
            break;
        }
    }

    if (!(op.x() >= 0.0 && op.y() >= 0.0)) {
        return false;
    }

    va2    = op.x();
    ik2_mA = op.y();
    const double ia2_A = ik2_mA / 1000.0;
    vk2    = ia2_A * rk_eq;

    if (!(vk2 >= 0.0) || !std::isfinite(vk2)) {
        return false;
    }

    // First-order follower gain approximation; can be refined later if needed.
    if (rk2 > 0.0 && rl2 > 0.0) {
        gain2 = rl2 / (rl2 + rk2);
    } else {
        gain2 = 0.0;
    }
    if (!std::isfinite(gain2) || gain2 < 0.0) {
        gain2 = 0.0;
    }

    return true;
}

void TriodeCcDccfTwoStage::update(int index)
{
    Q_UNUSED(index);

    // Reset cached follower THD for this update pass.
    followerThdPct = 0.0;

    if (!device1) {
        for (int i = TCC_DCCF_VA1; i <= TCC_DCCF_GAIN2; ++i) {
            if (parameter[i]) {
                parameter[i]->setValue(0.0);
            }
        }
        return;
    }

    double va1 = 0.0, vk1 = 0.0, ia1_mA = 0.0, gain1 = 0.0;
    if (!computeStage1(va1, vk1, ia1_mA, gain1)) {
        for (int i = TCC_DCCF_VA1; i <= TCC_DCCF_GAIN2; ++i) {
            if (parameter[i]) {
                parameter[i]->setValue(0.0);
            }
        }
        return;
    }

    parameter[TCC_DCCF_VA1]->setValue(va1);
    parameter[TCC_DCCF_VK1]->setValue(vk1);
    parameter[TCC_DCCF_IA1]->setValue(ia1_mA);
    parameter[TCC_DCCF_GAIN1]->setValue(gain1);

    double va2 = 0.0, vk2 = 0.0, ik2_mA = 0.0, gain2 = 0.0;
    if (!computeStage2(va1, va2, vk2, ik2_mA, gain2)) {
        parameter[TCC_DCCF_VA2]->setValue(0.0);
        parameter[TCC_DCCF_VK2]->setValue(0.0);
        parameter[TCC_DCCF_IK2]->setValue(0.0);
        parameter[TCC_DCCF_GAIN2]->setValue(0.0);
        return;
    }

    parameter[TCC_DCCF_VA2]->setValue(va2);
    parameter[TCC_DCCF_VK2]->setValue(vk2);
    parameter[TCC_DCCF_IK2]->setValue(ik2_mA);
    parameter[TCC_DCCF_GAIN2]->setValue(gain2);

    // Compute Stage 2 follower THD at the requested headroom (Vpk at anode).
    double headroomVpk = 0.0;
    if (parameter[TCC_DCCF_HEADROOM2]) {
        headroomVpk = parameter[TCC_DCCF_HEADROOM2]->getValue();
    }

    if (headroomVpk > 0.0) {
        double hd2 = 0.0;
        double hd3 = 0.0;
        double hd4 = 0.0;
        double hd5 = 0.0;
        double thd = 0.0;

        if (simulateFollowerHarmonicsTimeDomain(headroomVpk,
                                                hd2,
                                                hd3,
                                                hd4,
                                                hd5,
                                                thd)) {
            if (std::isfinite(thd) && thd >= 0.0) {
                followerThdPct = thd;
            }
        }
    }
}

void TriodeCcDccfTwoStage::plot(Plot *plot)
{
    if (!device1) {
        return;
    }

    // Clear previous overlays for this circuit.
    if (anodeLoadLine) {
        plot->getScene()->removeItem(anodeLoadLine);
        delete anodeLoadLine;
        anodeLoadLine = nullptr;
    }
    if (cathodeLoadLine) {
        plot->getScene()->removeItem(cathodeLoadLine);
        delete cathodeLoadLine;
        cathodeLoadLine = nullptr;
    }
    if (acSignalLine) {
        plot->getScene()->removeItem(acSignalLine);
        delete acSignalLine;
        acSignalLine = nullptr;
    }
    if (opMarker) {
        plot->getScene()->removeItem(opMarker);
        delete opMarker;
        opMarker = nullptr;
    }

    // Ensure axes are set if this is the first overlay on the shared scene,
    // mirroring the behaviour of other Designer circuits.
    const double vaMax = device1->getVaMax();
    const double iaMax = device1->getIaMax();
    const double xMajor = std::max(5.0, vaMax / 10.0);
    const double yMajor = std::max(0.5, iaMax / 10.0);

    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, vaMax, xMajor, 0.0, iaMax, yMajor);
    }

    const double vb2 = parameter[TCC_DCCF_VB2]->getValue();
    const double ra2 = parameter[TCC_DCCF_RA2]->getValue();
    const double rk2 = parameter[TCC_DCCF_RK2]->getValue();
    const double rl2 = parameter[TCC_DCCF_RL2]->getValue();

    if (!(vb2 > 0.0) || !(ra2 > 0.0) || !(rk2 > 0.0) || !(rl2 > 0.0)) {
        return;
    }

    // Use the Stage 1 anode as the DC-coupled grid drive for Stage 2.
    const double va1 = parameter[TCC_DCCF_VA1]->getValue();
    if (!(va1 > 0.0) || !std::isfinite(va1)) {
        return;
    }

    const double rk_eq = (rk2 * rl2) / (rk2 + rl2);
    if (!(rk_eq > 0.0) || !std::isfinite(rk_eq)) {
        return;
    }

    // Stage 2 anode DC load line: from (0, Ia_max) to (Vb2, 0).
    const double denom = ra2 + rk_eq;
    if (!(denom > 0.0)) {
        return;
    }
    const double iaMax_mA = vb2 / denom * 1000.0;
    const QPointF aStart(0.0, iaMax_mA);
    const QPointF aEnd(vb2, 0.0);

    // Build Stage 2 "tube + cathode" curve in Va-Ia space using the same
    // construction as computeStage2.
    QVector<QPointF> cathodeData2;
    const int steps = 120;
    const double iaStep_mA = iaMax_mA / static_cast<double>(steps);

    for (int j = 1; j <= steps; ++j) {
        const double ia_mA = iaStep_mA * static_cast<double>(j);
        if (ia_mA <= 0.0) {
            continue;
        }

        const double ia_A  = ia_mA / 1000.0;
        const double vk_eq = ia_A * rk_eq;
        const double vgk   = va1 - vk_eq; // grid-to-cathode voltage

        const double vg_min   = -device1->getVg1Max();
        const double vg_param = std::clamp(vgk, vg_min, 0.0);

        const double va_model = device1->anodeVoltage(ia_mA, vg_param);
        if (std::isfinite(va_model) && va_model > 0.0001 && va_model <= vb2 * 2.0) {
            cathodeData2.push_back(QPointF(va_model, ia_mA));
        }
    }

    if (cathodeData2.size() < 2) {
        return;
    }

    auto findIntersection = [](const QPointF &l1s, const QPointF &l1e,
                               const QPointF &l2s, const QPointF &l2e) -> QPointF {
        const double x1 = l1s.x(), y1 = l1s.y();
        const double x2 = l1e.x(), y2 = l1e.y();
        const double x3 = l2s.x(), y3 = l2s.y();
        const double x4 = l2e.x(), y4 = l2e.y();

        const double d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(d) < 1e-12) {
            return QPointF(-1.0, -1.0);
        }

        const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / d;
        const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / d;
        if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0) {
            return QPointF(-1.0, -1.0);
        }

        const double ix = x1 + t * (x2 - x1);
        const double iy = y1 + t * (y2 - y1);
        return QPointF(ix, iy);
    };

    QPointF op(-1.0, -1.0);
    for (int j = 0; j < cathodeData2.size() - 1; ++j) {
        const QPointF cStart = cathodeData2[j];
        const QPointF cEnd   = cathodeData2[j + 1];
        const QPointF ip     = findIntersection(aStart, aEnd, cStart, cEnd);
        if (ip.x() >= 0.0 && ip.y() >= 0.0) {
            op = ip;
            break;
        }
    }

    // Draw Stage 2 anode load line (green).
    anodeLoadLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 128, 0));
        pen.setWidth(2);
        QGraphicsLineItem *seg = plot->createSegment(aStart.x(), aStart.y(), aEnd.x(), aEnd.y(), pen);
        if (seg) {
            anodeLoadLine->addToGroup(seg);
        }
        if (!anodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(anodeLoadLine);
        } else {
            delete anodeLoadLine;
            anodeLoadLine = nullptr;
        }
    }

    // Draw Stage 2 tube+cathode curve (blue).
    cathodeLoadLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 0, 128));
        pen.setWidth(2);

        for (int j = 0; j < cathodeData2.size() - 1; ++j) {
            const QPointF s = cathodeData2[j];
            const QPointF e = cathodeData2[j + 1];
            QGraphicsLineItem *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen);
            if (seg) {
                cathodeLoadLine->addToGroup(seg);
            }
        }
        if (!cathodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(cathodeLoadLine);
        } else {
            delete cathodeLoadLine;
            cathodeLoadLine = nullptr;
        }
    }

    // Follower load line (cyan) based on Stage 2 load resistor.
    if (vb2 > 0.0 && rl2 > 0.0) {
        acSignalLine = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 191, 191));
        pen.setWidth(2);

        const double ik2_ref_mA = vb2 * 1000.0 / rl2;
        QGraphicsLineItem *seg = plot->createSegment(0.0, ik2_ref_mA, vb2, 0.0, pen);
        if (seg) {
            acSignalLine->addToGroup(seg);
        }
        if (!acSignalLine->childItems().isEmpty()) {
            plot->getScene()->addItem(acSignalLine);
        } else {
            delete acSignalLine;
            acSignalLine = nullptr;
        }
    }

    // Stage 2 operating point marker (red cross), if intersection was found.
    if (op.x() >= 0.0 && op.y() >= 0.0) {
        opMarker = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(255, 0, 0));
        pen.setWidth(2);

        const double d = 5.0;
        QGraphicsLineItem *h = plot->createSegment(op.x() - d, op.y(), op.x() + d, op.y(), pen);
        QGraphicsLineItem *v = plot->createSegment(op.x(), op.y() - d, op.x(), op.y() + d, pen);
        if (h) opMarker->addToGroup(h);
        if (v) opMarker->addToGroup(v);

        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker;
            opMarker = nullptr;
        }
    }
}

bool TriodeCcDccfTwoStage::computeFollowerHeadroomHarmonicCurrents(double headroomVpk,
                                                                    double &Ia,
                                                                    double &Ib,
                                                                    double &Ic,
                                                                    double &Id,
                                                                    double &Ie) const
{
    Ia = Ib = Ic = Id = Ie = 0.0;

    if (!device1 || headroomVpk <= 0.0) {
        return false;
    }

    const double vb2    = parameter[TCC_DCCF_VB2] ? parameter[TCC_DCCF_VB2]->getValue() : 0.0;
    const double va2_op = parameter[TCC_DCCF_VA2] ? parameter[TCC_DCCF_VA2]->getValue() : 0.0;
    const double vk2    = parameter[TCC_DCCF_VK2] ? parameter[TCC_DCCF_VK2]->getValue() : 0.0;
    const double va1    = parameter[TCC_DCCF_VA1] ? parameter[TCC_DCCF_VA1]->getValue() : 0.0;

    if (!(vb2 > 0.0) || !(va2_op > 0.0)) {
        return false;
    }

    const double maxHeadroom = 0.9 * std::max(1.0, std::min(vb2, device1->getVaMax()));
    const double vpk = std::min(std::max(0.0, headroomVpk), maxHeadroom);
    if (vpk <= 0.0) {
        return false;
    }

    const double vgMin      = -device1->getVg1Max();
    const double vgridkRaw  = va1 - vk2;
    const double vg0        = std::clamp(vgridkRaw, vgMin, 0.0);

    auto sampleCurrent = [&](double va) -> double {
        const double vaClamped = std::clamp(va, 0.0, device1->getVaMax());
        const double ia_mA     = device1->anodeCurrent(vaClamped, vg0);
        if (!std::isfinite(ia_mA) || ia_mA < 0.0) {
            return 0.0;
        }
        return ia_mA / 1000.0;
    };

    const double va0  = va2_op + vpk;
    const double va1p = va2_op + 0.5 * vpk;
    const double va2p = va2_op;
    const double va3  = va2_op - 0.5 * vpk;
    const double va4  = va2_op - vpk;

    Ia = sampleCurrent(va0);
    Ib = sampleCurrent(va1p);
    Ic = sampleCurrent(va2p);
    Id = sampleCurrent(va3);
    Ie = sampleCurrent(va4);

    if (!(Ia > 0.0) || !(Ic > 0.0)) {
        return false;
    }

    return true;
}

bool TriodeCcDccfTwoStage::simulateFollowerHarmonicsTimeDomain(double headroomVpk,
                                                                double &hd2,
                                                                double &hd3,
                                                                double &hd4,
                                                                double &hd5,
                                                                double &thd) const
{
    hd2 = hd3 = hd4 = hd5 = thd = 0.0;

    if (!device1 || headroomVpk <= 0.0) {
        return false;
    }

    double Ia = 0.0;
    double Ib = 0.0;
    double Ic = 0.0;
    double Id = 0.0;
    double Ie = 0.0;
    if (!computeFollowerHeadroomHarmonicCurrents(headroomVpk,
                                                 Ia,
                                                 Ib,
                                                 Ic,
                                                 Id,
                                                 Ie)) {
        return false;
    }

    double samples[5];
    samples[0] = Ia;
    samples[1] = Ib;
    samples[2] = Ic;
    samples[3] = Id;
    samples[4] = Ie;

    const int sampleCount = 512;
    const double twoPi = 6.28318530717958647692;

    double a[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double b[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    for (int k = 0; k < sampleCount; ++k) {
        const double phase = twoPi * static_cast<double>(k) / static_cast<double>(sampleCount);
        const double u     = phase / twoPi;

        const double pos    = u * 5.0;
        const double indexF = std::floor(pos);
        int i0              = static_cast<int>(indexF);
        if (i0 < 0) {
            i0 = 0;
        }
        if (i0 >= 5) {
            i0 = i0 % 5;
        }
        const int i1   = (i0 + 1) % 5;
        const double frac = pos - indexF;

        const double ip = samples[i0] + (samples[i1] - samples[i0]) * frac;

        const double window = 0.5 * (1.0 - std::cos(twoPi * static_cast<double>(k) /
                                                   static_cast<double>(sampleCount - 1)));
        const double v = ip * window;

        for (int n = 1; n <= 5; ++n) {
            const double angle = static_cast<double>(n) * phase;
            const double c     = std::cos(angle);
            const double s     = std::sin(angle);
            a[n] += v * c;
            b[n] += v * s;
        }
    }

    const double scale = 2.0 / static_cast<double>(sampleCount);
    double A[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    for (int n = 1; n <= 5; ++n) {
        a[n] *= scale;
        b[n] *= scale;
        A[n] = std::sqrt(a[n] * a[n] + b[n] * b[n]);
    }

    const double fundamental = A[1];
    if (!(fundamental > 0.0) || !std::isfinite(fundamental)) {
        return false;
    }

    const double invFund = 100.0 / fundamental;
    hd2 = A[2] * invFund;
    hd3 = A[3] * invFund;
    hd4 = A[4] * invFund;
    hd5 = A[5] * invFund;

    if (!std::isfinite(hd2) || hd2 < 0.0) hd2 = 0.0;
    if (!std::isfinite(hd3) || hd3 < 0.0) hd3 = 0.0;
    if (!std::isfinite(hd4) || hd4 < 0.0) hd4 = 0.0;
    if (!std::isfinite(hd5) || hd5 < 0.0) hd5 = 0.0;

    thd = std::sqrt(hd2 * hd2 + hd3 * hd3 + hd4 * hd4 + hd5 * hd5);
    if (!std::isfinite(thd) || thd < 0.0) {
        thd = 0.0;
    }

    return true;
}
