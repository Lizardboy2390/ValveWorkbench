#pragma once

#include "circuit.h"

// Parameters for the two-stage Triode CC + DC Cathode Follower circuit.
// The first N entries are user-editable inputs; the remaining are computed outputs.
enum eTriodeCcDccfTwoStageParameter {
    TCC_DCCF_VB1 = 0,   // Stage 1 supply voltage Vb1 (V)
    TCC_DCCF_RA1,       // Stage 1 anode resistor Ra1 (Ohms)
    TCC_DCCF_RK1,       // Stage 1 cathode resistor Rk1 (Ohms)
    TCC_DCCF_VB2,       // Stage 2 supply voltage Vb2 (V)
    TCC_DCCF_RA2,       // Stage 2 anode resistor Ra2 (Ohms)
    TCC_DCCF_RK2,       // Stage 2 cathode resistor Rk2 (Ohms)
    TCC_DCCF_RL2,       // Stage 2 load resistor Rl2 (Ohms)

    TCC_DCCF_VA1,       // Stage 1 anode voltage Va1 (V)
    TCC_DCCF_VK1,       // Stage 1 cathode voltage Vk1 (V)
    TCC_DCCF_IA1,       // Stage 1 anode current Ia1 (mA)
    TCC_DCCF_HEADROOM2, // Stage 2 headroom at anode (Vpk)
    TCC_DCCF_GAIN1,     // Stage 1 gain (approx Av)

    TCC_DCCF_VA2,       // Stage 2 anode voltage Va2 (V)
    TCC_DCCF_VK2,       // Stage 2 cathode/output voltage Vk2 (V)
    TCC_DCCF_IK2,       // Stage 2 cathode current Ik2 (mA)
    TCC_DCCF_GAIN2      // Stage 2 follower gain (approx Av)
};

class TriodeCcDccfTwoStage : public Circuit
{
    Q_OBJECT
public:
    TriodeCcDccfTwoStage();

    // UIBridge / Circuit interface
    void updateUI(QLabel *labels[], QLineEdit *values[]) override;
    void plot(Plot *plot) override;
    int getDeviceType(int index) override;
    QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) override;

protected:
    void update(int index) override;

private:
    bool computeStage1(double &va1, double &vk1, double &ia1_mA, double &gain1);
    bool computeStage2(double va1,
                       double &va2,
                       double &vk2,
                       double &ik2_mA,
                       double &gain2);

    bool computeFollowerHeadroomHarmonicCurrents(double headroomVpk,
                                                 double &Ia,
                                                 double &Ib,
                                                 double &Ic,
                                                 double &Id,
                                                 double &Ie) const;

    bool simulateFollowerHarmonicsTimeDomain(double headroomVpk,
                                             double &hd2,
                                             double &hd3,
                                             double &hd4,
                                             double &hd5,
                                             double &thd) const;

    double followerThdPct = 0.0;
};
