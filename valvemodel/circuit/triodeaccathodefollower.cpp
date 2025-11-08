#include "triodeaccathodefollower.h"

#include <QtMath>
#include <QGraphicsTextItem>
#include <algorithm>
#include <limits>

TriodeACCathodeFollower::TriodeACCathodeFollower()
{
    // Inputs
    parameter[ACCF_VB] = new Parameter("Supply voltage (V)", 300.0);
    parameter[ACCF_RK] = new Parameter("Cathode resistor (Ω)", 1000.0);
    parameter[ACCF_RA] = new Parameter("Anode resistor (Ω)", 100000.0);
    parameter[ACCF_RL] = new Parameter("Load impedance (Ω)", 1000000.0);
    parameter[ACCF_RG] = new Parameter("Grid resistor (Ω)", 1000000.0);

    // Outputs
    parameter[ACCF_VK] = new Parameter("Bias voltage (V)", 0.0);
    parameter[ACCF_VA] = new Parameter("Anode voltage (V)", 0.0);
    parameter[ACCF_IA] = new Parameter("Anode current (mA)", 0.0);
    parameter[ACCF_RA_INT] = new Parameter("Internal resistance (Ω)", 0.0);
    parameter[ACCF_GAIN] = new Parameter("Gain (unbypassed)", 0.0);
    parameter[ACCF_GAIN_B] = new Parameter("Gain (bypassed)", 0.0);
    parameter[ACCF_MU] = new Parameter("Mu (unitless)", 0.0);
    parameter[ACCF_GM] = new Parameter("Transconductance (mA/V)", 0.0);
    // New computed outputs
    parameter[ACCF_ZIN] = new Parameter("Input impedance (Ω)", 0.0);
    parameter[ACCF_ZO]  = new Parameter("Output impedance (Ω)", 0.0);
}

void TriodeACCathodeFollower::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs (first 5)
    for (int i = 0; i < 5; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            if (i == ACCF_VB) {
                labels[i]->setText("B+ supply (V)");
                values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            } else if (i == ACCF_RA) {
                labels[i]->setText("Anode resistor Ra (kΩ)");
                values[i]->setText(QString::number(parameter[i]->getValue() / 1000.0, 'f', 1));
            } else if (i == ACCF_RL) {
                labels[i]->setText("Load impedance RL (kΩ)");
                values[i]->setText(QString::number(parameter[i]->getValue() / 1000.0, 'f', 1));
            } else if (i == ACCF_RG) {
                labels[i]->setText("Grid resistor Rg (kΩ)");
                values[i]->setText(QString::number(parameter[i]->getValue() / 1000.0, 'f', 1));
            } else if (i == ACCF_RK) {
                labels[i]->setText("Cathode resistor Rk (Ω)");
                values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            } else {
                labels[i]->setText(parameter[i]->getName());
                values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            }
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false); // ensure inputs (including Rg) are editable
        }
    }

    // Outputs (5..13)
    for (int i = 5; i < 14; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            QString labelText;
            switch (i) {
                case ACCF_VK:    labelText = "Cathode DC Vk (V):"; break;
                case ACCF_VA:    labelText = "Valve Va (V):"; break;
                case ACCF_IA:    labelText = "Anode current Ia (mA):"; break;
                case ACCF_RA_INT:labelText = "Internal ra (Ω):"; break;
                case ACCF_GAIN:  labelText = "Voltage gain:"; break;
                case ACCF_GAIN_B:labelText = ""; break;
                case ACCF_MU:    labelText = "μ (unitless):"; break;
                case ACCF_GM:    labelText = "gm (mA/V):"; break;
                case ACCF_ZIN:   labelText = "Input impedance Zin (kΩ):"; break;
                case ACCF_ZO:    labelText = "Output impedance Zo (kΩ):"; break;
            }
            if (i == ACCF_GAIN_B) {
                labels[i]->setVisible(false);
                values[i]->setVisible(false);
            } else {
                labels[i]->setText(labelText);
                if (device1 == nullptr) {
                    values[i]->setText("N/A");
                } else {
                    if (i == ACCF_GAIN) {
                        double gain = (sensitivityGainMode == 1) ? parameter[ACCF_GAIN_B]->getValue() : parameter[ACCF_GAIN]->getValue();
                        values[i]->setText(QString::number(gain, 'f', 1));
                    } else if (i == ACCF_RA_INT) {
                        values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 0));
                    } else if (i == ACCF_GM) {
                        values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 2));
                    } else if (i == ACCF_ZIN || i == ACCF_ZO) {
                        // Show kΩ with 1 decimal
                        values[i]->setText(QString::number(parameter[i]->getValue() / 1000.0, 'f', 1));
                    } else {
                        values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
                    }
                }
                if (i == ACCF_RA_INT) { values[i]->setMinimumWidth(80); values[i]->setMaximumWidth(110); }
                labels[i]->setVisible(true);
                values[i]->setVisible(true);
                values[i]->setReadOnly(true);
            }
        }
    }

    // Row 12: Input sensitivity (Vpp), only when sym swing is enabled
    if (labels[12] && values[12]) {
        labels[12]->setText("Input sensitivity (Vpp):");
        if (device1 == nullptr) {
            values[12]->setText("N/A");
        } else {
            if (showSymSwing && lastSymVpp > 0.0) {
                double gain = (sensitivityGainMode == 1) ? parameter[ACCF_GAIN_B]->getValue() : parameter[ACCF_GAIN]->getValue();
                double vpp_in = (std::isfinite(gain) && std::abs(gain) > 1e-12) ? (lastSymVpp / std::abs(gain)) : 0.0;
                values[12]->setText(QString::number(vpp_in, 'f', 1));
            } else {
                values[12]->setText("");
            }
        }
        values[12]->setStyleSheet("color: rgb(100,149,237);");
        labels[12]->setVisible(true);
        values[12]->setVisible(true);
        values[12]->setReadOnly(true);
    }
}

static inline QPointF segInter(const QPointF &a1, const QPointF &a2, const QPointF &b1, const QPointF &b2, bool &ok)
{
    double x1=a1.x(), y1=a1.y(), x2=a2.x(), y2=a2.y();
    double x3=b1.x(), y3=b1.y(), x4=b2.x(), y4=b2.y();
    double den = (x1-x2)*(y3-y4)-(y1-y2)*(x3-x4);
    if (std::abs(den) < 1e-12) { ok=false; return QPointF(); }
    double px = ((x1*y2 - y1*x2)*(x3 - x4) - (x1 - x2)*(x3*y4 - y3*x4)) / den;
    double py = ((x1*y2 - y1*x2)*(y3 - y4) - (y1 - y2)*(x3*y4 - y3*x4)) / den;
    ok = (px >= std::min(x1,x2)-1e-9 && px <= std::max(x1,x2)+1e-9 &&
          px >= std::min(x3,x4)-1e-9 && px <= std::max(x3,x4)+1e-9);
    return QPointF(px, py);
}

void TriodeACCathodeFollower::plot(Plot *plot)
{
    lastSymVpp = 0.0;
    if (anodeLoadLine) { plot->getScene()->removeItem(anodeLoadLine); delete anodeLoadLine; anodeLoadLine=nullptr; }
    if (cathodeLoadLine) { plot->getScene()->removeItem(cathodeLoadLine); delete cathodeLoadLine; cathodeLoadLine=nullptr; }
    if (acSignalLine) { plot->getScene()->removeItem(acSignalLine); delete acSignalLine; acSignalLine=nullptr; }
    if (opMarker) { plot->getScene()->removeItem(opMarker); delete opMarker; opMarker=nullptr; }
    if (symSwingGroup) { plot->getScene()->removeItem(symSwingGroup); delete symSwingGroup; symSwingGroup=nullptr; }
    if (sensitivityGroup) { plot->getScene()->removeItem(sensitivityGroup); delete sensitivityGroup; sensitivityGroup=nullptr; }
    if (swingGroup) { plot->getScene()->removeItem(swingGroup); delete swingGroup; swingGroup=nullptr; }
    if (paLimitGroup) { plot->getScene()->removeItem(paLimitGroup); delete paLimitGroup; paLimitGroup=nullptr; }

    if (!device1) return;

    const double vb = parameter[ACCF_VB]->getValue();
    const double ra = parameter[ACCF_RA]->getValue();
    const double rk = parameter[ACCF_RK]->getValue();
    const double rl = parameter[ACCF_RL]->getValue();
    const double rg = parameter[ACCF_RG]->getValue();

    // Axes based on device limits
    const double xStop = std::max(10.0, device1->getVaMax());
    const double yStop = std::max(1.0, device1->getIaMax());
    const double xMajor = std::max(1.0, xStop / 10.0);
    const double yMajor = std::max(0.1, yStop / 10.0);
    plot->setAxes(0.0, xStop, xMajor, 0.0, yStop, yMajor);

    // Build anode "DC" load line: Va = Vb - Ia*(Ra+Rk)
    QVector<QPointF> aPts;
    {
        const int n=64;
        for (int i=0;i<n;i++) {
            double ia_mA = (yStop * i) / (n-1);
            double ia_A = ia_mA/1000.0;
            double va = std::clamp(vb - ia_A*(ra+rk), 0.0, xStop);
            aPts.push_back(QPointF(va, ia_mA));
        }
    }

    // Build cathode bias curve using device model: Vg = -Ia*Rk, Va = f(Ia,Vg)
    QVector<QPointF> kPts;
    {
        const int n=64;
        for (int i=0;i<n;i++) {
            double ia_mA = (yStop * i) / (n-1);
            double vg = - (ia_mA/1000.0) * rk;
            double va = device1->anodeVoltage(ia_mA, std::clamp(vg, -device1->getVg1Max(), 0.0));
            if (std::isfinite(va)) {
                va = std::clamp(va, 0.0, xStop);
                kPts.push_back(QPointF(va, ia_mA));
            }
        }
    }

    // Find OP by crude segment intersection
    QPointF op(0,0);
    bool found=false;
    if (aPts.size() < 2 || kPts.size() < 2) {
        // Not enough data to proceed
        return;
    }
    for (int i=0;i+1<aPts.size() && !found;i++) {
        for (int j=0;j+1<kPts.size() && !found;j++) {
            bool ok=false; QPointF p = segInter(aPts[i], aPts[i+1], kPts[j], kPts[j+1], ok);
            if (ok && std::isfinite(p.x()) && std::isfinite(p.y())) { op=p; found=true; }
        }
    }
    if (!found) {
        // Simple fallback: choose mid
        op = aPts.value(aPts.size()/2, QPointF(0,0));
    }
    if (!std::isfinite(op.x()) || !std::isfinite(op.y())) {
        return;
    }

    // Update outputs
    parameter[ACCF_VK]->setValue((op.y()/1000.0)*rk);
    parameter[ACCF_VA]->setValue(op.x());
    parameter[ACCF_IA]->setValue(op.y());

    // Derive small-signal params at OP via finite differences (similar to Triode CC)
    double mu=0.0, ra_int=0.0, gm_mA_per_V=0.0;
    if (std::isfinite(op.x()) && std::isfinite(op.y())) {
        const double ia_mA = op.y();
        const double vg0 = - (op.y()/1000.0) * rk;
        const double dIa_mA = std::max(0.1, std::abs(ia_mA) * 0.01);
        const double dVg = std::max(0.01, std::abs(vg0) * 0.02);
        const double vgMin = -device1->getVg1Max();
        const double vgPlus = std::clamp(vg0 + dVg, vgMin, 0.0);
        const double vgMinus = std::clamp(vg0 - dVg, vgMin, 0.0);

        double va_plusIa = device1->anodeVoltage(ia_mA + dIa_mA, vg0);
        double va_minusIa = device1->anodeVoltage(ia_mA - dIa_mA, vg0);
        if (std::isfinite(va_plusIa) && std::isfinite(va_minusIa) && dIa_mA>0.0) {
            double dVa_dIa_V_per_mA = (va_plusIa - va_minusIa)/(2.0*dIa_mA);
            ra_int = dVa_dIa_V_per_mA * 1000.0;
        }

        double va_plusVg = device1->anodeVoltage(ia_mA, vgPlus);
        double va_minusVg = device1->anodeVoltage(ia_mA, vgMinus);
        if (std::isfinite(va_plusVg) && std::isfinite(va_minusVg) && (vgPlus-vgMinus)!=0.0) {
            double dVa_dVg = (va_plusVg - va_minusVg) / (vgPlus - vgMinus);
            mu = -dVa_dVg;
        }
        if (ra_int>0.0) {
            gm_mA_per_V = (mu/ra_int) * 1000.0; // mA/V because ra in ohms
        }
    }
    if (!std::isfinite(mu) || mu<=0.0) mu=100.0;
    if (!std::isfinite(ra_int) || ra_int<=0.0) ra_int=1000.0;
    if (!std::isfinite(gm_mA_per_V) || gm_mA_per_V<0.0) gm_mA_per_V=0.0;
    parameter[ACCF_MU]->setValue(mu);
    parameter[ACCF_RA_INT]->setValue(ra_int);
    parameter[ACCF_GM]->setValue(gm_mA_per_V);

    // Gains using device-derived mu and ra
    // External AC load parallel (avoid division by zero when ra==0 or rl==0)
    double r_ac = 0.0;
    if (ra > 0.0 && rl > 0.0) {
        r_ac = 1.0 / (1.0/ra + 1.0/rl);
    } else if (ra > 0.0) {
        r_ac = ra;
    } else {
        r_ac = rl; // if ra<=0, treat external load as RL only
    }
    double gain_unb = mu * rl / (rl + ra + (mu + 1.0) * rk);
    double gain_b = mu * rl / (rl + ra + ra_int);
    if (!std::isfinite(gain_unb)) gain_unb=0.0;
    if (!std::isfinite(gain_b)) gain_b=0.0;
    parameter[ACCF_GAIN]->setValue(gain_unb);
    parameter[ACCF_GAIN_B]->setValue(gain_b);

    // Compute Zin (bootstrapped) and Zout approximations
    double Zload_cf = (sensitivityGainMode == 1) ? rl : ( (rl>0 && rk>0) ? (1.0 / (1.0/rl + 1.0/rk)) : rl );
    if (Zload_cf <= 0.0) Zload_cf = rl;
    double A_cath = 0.0;
    if (std::isfinite(mu) && std::isfinite(ra_int) && Zload_cf > 0.0) {
        A_cath = (mu * Zload_cf) / (ra_int + (mu + 1.0) * Zload_cf);
        if (!std::isfinite(A_cath)) A_cath = 0.0;
    }
    double Zin = (rg > 0.0) ? (rg / std::max(1e-6, (1.0 - A_cath))) : 0.0;
    double gm_A_per_V = gm_mA_per_V / 1000.0;
    double Zo0 = 0.0;
    if (sensitivityGainMode == 1) {
        Zo0 = (gm_A_per_V > 0.0) ? (1.0 / gm_A_per_V) : 0.0;
    } else {
        Zo0 = (ra_int / (mu + 1.0)) + rk;
    }
    double Zo = Zo0;
    if (Zo > 0.0 && rl > 0.0) {
        Zo = (Zo0 * rl) / (Zo0 + rl);
    }
    if (!std::isfinite(Zin) || Zin < 0.0) Zin = 0.0;
    if (!std::isfinite(Zo)  || Zo  < 0.0) Zo  = 0.0;
    parameter[ACCF_ZIN]->setValue(Zin);
    parameter[ACCF_ZO]->setValue(Zo);

    // (ACCF) Do not draw the green DC anode line; follower view focuses on cathode bias curve and AC line

    // Draw cathode bias curve (purple), clipped left/right
    {
        cathodeLoadLine = new QGraphicsItemGroup();
        QPen pen(QColor::fromRgb(128,0,128)); pen.setWidth(2);
        const double leftClipX = std::max(5.0, xStop*0.03);
        const double rightClipX = std::max(leftClipX+1.0, xStop - std::max(5.0, xStop*0.03));
        for (int i=0;i+1<kPts.size();++i) {
            QPointF s0=kPts[i], e0=kPts[i+1];
            if ((s0.x()<leftClipX && e0.x()<leftClipX) || (s0.x()>rightClipX && e0.x()>rightClipX)) continue;
            QPointF s=s0, e=e0;
            if (s.x()<leftClipX && e.x()>leftClipX) {
                double t=(leftClipX - s.x())/(e.x()-s.x()); double y=s.y()+t*(e.y()-s.y()); s.setX(leftClipX); s.setY(y);
            }
            if (s.x()<rightClipX && e.x()>rightClipX) {
                double t=(rightClipX - s.x())/(e.x()-s.x()); double y=s.y()+t*(e.y()-s.y()); e.setX(rightClipX); e.setY(y);
            }
            if (e.x()<=leftClipX || s.x()>=rightClipX) continue;
            auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen);
            if (seg) cathodeLoadLine->addToGroup(seg);
        }
        plot->getScene()->addItem(cathodeLoadLine);
    }

    // OP marker (red dot)
    {
        opMarker = new QGraphicsItemGroup();
        QPen pen(Qt::red); pen.setWidth(4);
        auto *seg = plot->createSegment(op.x()-2, op.y(), op.x()+2, op.y(), pen);
        if (seg) opMarker->addToGroup(seg);
        seg = plot->createSegment(op.x(), op.y()-2, op.x(), op.y()+2, pen);
        if (seg) opMarker->addToGroup(seg);
        plot->getScene()->addItem(opMarker);
    }

    // AC small-signal line through OP with follower slope = -1000 / (Ra + (Rk || RL))
    double slope = 0.0;
    {
        double rkrl = (rk > 0.0 && rl > 0.0) ? (rk * rl) / (rk + rl) : (rk > 0.0 ? rk : rl);
        double r_acf = std::max(1.0, ra + std::max(0.0, rkrl));
        slope = -1000.0 / r_acf;
        acSignalLine = new QGraphicsItemGroup();
        QPen pen(QColor::fromRgb(190, 190, 0)); pen.setWidth(2);
        if (std::isfinite(slope)) {
            double x1=0.0, y1=op.y()+slope*(x1-op.x());
            double x2=xStop, y2=op.y()+slope*(x2-op.x());
            auto *seg = plot->createSegment(x1, y1, x2, y2, pen);
            if (seg) acSignalLine->addToGroup(seg);
        }
        plot->getScene()->addItem(acSignalLine);
    }

    // Brown max swing row (vertical cutoff line and labels)
    double yAtCut=0.0, vaCut=-1.0;
    double vaZero = std::numeric_limits<double>::quiet_NaN();
    {
        // intersection with Vg=0 curve approximated by finding where vg=0 along AC line: Vg = -Ia*Rk => Ia=0 when vg=0 at cathode follower? We reuse cutoff by scanning device at vg=0 along AC line
        // Scan along Va on AC line and find where grid reaches 0 (approx where device Ia at Vg=0 equals line Ia)
        const int n=200;
        double lastVa=0.0, lastF=0.0; bool has=false;
        for (int i=0;i<=n;i++) {
            double va = (xStop*i)/n;
            double ia_line = op.y() + slope*(va - op.x());
            if (ia_line < 0) continue;
            double ia_mA = ia_line;
            double ia_dev = device1->anodeCurrent(va, 0.0); // mA at Vg=0
            double f = ia_dev - ia_mA;
            if (i>0 && f*lastF <= 0.0) {
                double t = lastF/(lastF - f);
                vaCut = lastVa + t*(va - lastVa);
                yAtCut = op.y() + slope*(vaCut - op.x());
                has=true; break;
            }
            lastVa=va; lastF=f;
        }

        if (std::isfinite(slope) && std::abs(slope) > 1e-9) {
            vaZero = op.x() - op.y()/slope;
            vaZero = std::clamp(vaZero, 0.0, xStop);
        }

        swingGroup = new QGraphicsItemGroup();
        QPen swingPen(QColor::fromRgb(165,42,42)); swingPen.setWidth(2);
        if (vaCut >= 0.0 && std::isfinite(yAtCut)) {
            if (auto *vline = plot->createSegment(vaCut, 0.0, vaCut, yAtCut, swingPen)) swingGroup->addToGroup(vline);
            auto *labelCut = plot->createLabel(vaCut, -yMajor*1.8, vaCut, QColor::fromRgb(165,42,42));
            if (labelCut) { QPointF p=labelCut->pos(); double w=labelCut->boundingRect().width(); labelCut->setPos(p.x()-5.0 - w/2.0, p.y()); swingGroup->addToGroup(labelCut);}        
        }
        if (std::isfinite(vaZero)) {
            auto *labelZero = plot->createLabel(vaZero, -yMajor*1.8, vaZero, QColor::fromRgb(165,42,42));
            if (labelZero) { QPointF p=labelZero->pos(); double w=labelZero->boundingRect().width(); labelZero->setPos(p.x()-5.0 - w/2.0, p.y()); swingGroup->addToGroup(labelZero);}        
        }
        if (vaCut >= 0.0 && std::isfinite(vaZero)) {
            double swing = std::abs(vaZero - vaCut);
            double mid = 0.5*(vaZero + vaCut);
            auto *labelSwing = plot->createLabel(mid, -yMajor*1.8, swing, QColor::fromRgb(165,42,42));
            if (labelSwing) { QPointF p=labelSwing->pos(); double w=labelSwing->boundingRect().width(); labelSwing->setPos(p.x()-5.0 - w/2.0, p.y()); swingGroup->addToGroup(labelSwing);}        
        }
        if (!swingGroup->childItems().isEmpty()) plot->getScene()->addItem(swingGroup); else { delete swingGroup; swingGroup=nullptr; }
    }

    // Light blue symmetric swing row (ticks + center Vpp + end labels)
    {
        // Determine right limit: Pa and Vg=0 boundaries
        const double paMaxW = device1->getPaMax();
        double vaPa = xStop;
        if (paMaxW > 0.0) {
            // choose Va where Pa intersects Ia=0..yStop by scanning
            vaPa = xStop;
        }
        // reuse vaZero and vaCut as left/right bounds around OP
        if (vaCut >= 0.0 && std::isfinite(slope)) {
            // compute symmetric span only if both sides around OP are valid
            double vpkLeft = op.x() - vaCut;           // limited by Vgk -> 0
            double vpkRight = (std::isfinite(vaZero) ? (vaZero - op.x()) : 0.0); // limited by Ia -> 0
            if (std::isfinite(vpkLeft) && std::isfinite(vpkRight)) {
                double vpk = std::min(vpkLeft, vpkRight);
                if (vpk > 0) {
                    double leftX = op.x() - vpk;
                    double rightX = op.x() + vpk;
                    symSwingGroup = new QGraphicsItemGroup();
                    QPen tickPen(QColor::fromRgb(100,149,237)); tickPen.setWidth(2);
                    if (leftX >= 0.0 && leftX < op.x()) { if (auto *lt = plot->createSegment(leftX, 0.0, leftX, op.y()+slope*(leftX-op.x()), tickPen)) symSwingGroup->addToGroup(lt); }
                    if ((std::isfinite(vaZero) ? (rightX <= vaZero) : (rightX <= xStop)) && rightX > op.x()) {
                        if (auto *rt = plot->createSegment(rightX, 0.0, rightX, op.y()+slope*(rightX-op.x()), tickPen)) symSwingGroup->addToGroup(rt);
                    }

                    // End labels (light blue) on same row as center label
                    const double yRow = -yMajor*2.4;
                    if (leftX >= 0.0) {
                        auto *lLbl = plot->createLabel(leftX, yRow, leftX, QColor::fromRgb(100,149,237));
                        if (lLbl) { QPointF p=lLbl->pos(); double w=lLbl->boundingRect().width(); lLbl->setPos(p.x()-5.0 - w/2.0, p.y()); symSwingGroup->addToGroup(lLbl);}        
                    }
                    if ((std::isfinite(vaZero) ? (rightX <= vaZero) : (rightX <= xStop))) {
                        auto *rLbl = plot->createLabel(rightX, yRow, rightX, QColor::fromRgb(100,149,237));
                        if (rLbl) { QPointF p=rLbl->pos(); double w=rLbl->boundingRect().width(); rLbl->setPos(p.x()-5.0 - w/2.0, p.y()); symSwingGroup->addToGroup(rLbl);}        
                    }

                    const double vpp = 2.0 * vpk; lastSymVpp = vpp;
                    auto *cLbl = plot->createLabel(op.x(), yRow, vpp, QColor::fromRgb(100,149,237));
                    if (cLbl) { QPointF p=cLbl->pos(); double w=cLbl->boundingRect().width(); cLbl->setPos(p.x()-5.0 - w/2.0, p.y()); symSwingGroup->addToGroup(cLbl);}        
                    if (!symSwingGroup->childItems().isEmpty()) plot->getScene()->addItem(symSwingGroup); else { delete symSwingGroup; symSwingGroup=nullptr; }
                }
            }
        }
    }

    // Pa max dashed red starting inside visible y-range
    if (device1) {
        const double paMaxW = device1->getPaMax();
        if (paMaxW > 0.0) {
            paLimitGroup = new QGraphicsItemGroup();
            QPen paPen(QColor::fromRgb(180,0,0)); paPen.setStyle(Qt::DashLine); paPen.setWidth(2);
            const double xEnter = std::max(1e-6, std::min(xStop, (yStop > 0.0 ? (1000.0 * paMaxW / yStop) : xStop)));
            const int segs=60;
            double prevX=xEnter; double prevY=std::min(yStop, 1000.0 * paMaxW / prevX);
            for (int i=1;i<=segs;i++) {
                double t = double(i)/segs;
                double x = xEnter + (xStop - xEnter) * t;
                double y = (x>0.0) ? std::min(yStop, 1000.0 * paMaxW / x) : yStop;
                auto *seg = plot->createSegment(prevX, prevY, x, y, paPen);
                if (seg) paLimitGroup->addToGroup(seg);
                prevX=x; prevY=y;
            }
            if (!paLimitGroup->childItems().isEmpty()) plot->getScene()->addItem(paLimitGroup); else { delete paLimitGroup; paLimitGroup=nullptr; }
        }
    }
}

int TriodeACCathodeFollower::getDeviceType(int index)
{
    // Follow Triode CC convention: primary device is a triode, no secondary device
    if (index == 1) return 1; // triode device type id
    return -1; // no second device
}

void TriodeACCathodeFollower::update(int index)
{
    Q_UNUSED(index);
}

QTreeWidgetItem *TriodeACCathodeFollower::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *root = new QTreeWidgetItem(parent, QStringList() << "AC Cathode Follower");
    if (parent) parent->addChild(root);
    return root;
}
