#pragma once

#include "circuit.h"
#include <QVector>

// Parameters for the triode DC cathode follower circuit. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum eTriodeDCCathodeFollowerParameter {
    TRI_DCCF_VB = 0,   // Supply voltage (V)
    TRI_DCCF_RK,       // Cathode resistor Rk (Ohms)
    TRI_DCCF_RA,       // Anode resistor Ra (Ohms)
    TRI_DCCF_RL,       // Load impedance Rl (Ohms)
    TRI_DCCF_VK,       // Stage 1 cathode bias Vk (V)
    TRI_DCCF_VA,       // Stage 1 anode voltage Va (V)
    TRI_DCCF_IA,       // Stage 1 anode current Ia (mA)
    TRI_DCCF_GAIN,     // Overall gain (displayed value)
    TRI_DCCF_VK2,      // Follower cathode voltage Vk2 (V)
    TRI_DCCF_IK2       // Follower current Ik2 (mA)
};

class TriodeDCCathodeFollower : public Circuit
{
    Q_OBJECT
public:
    TriodeDCCathodeFollower();

    // UIBridge interface
    void updateUI(QLabel *labels[], QLineEdit *values[]) override;

    // Circuit interface
    void plot(Plot *plot) override;
    int getDeviceType(int index) override;

    // Designer circuits do not currently participate in the project tree, so
    // provide a trivial implementation to satisfy UIBridge's interface.
    QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) override;

protected:
    void update(int index) override;

private:
    void calculateLoadLines(QVector<QPointF> &anodeData, QVector<QPointF> &cathodeData);
    QPointF findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
                                 const QPointF &line2Start, const QPointF &line2End) const;
    QPointF findOperatingPoint(const QVector<QPointF> &anodeData,
                               const QVector<QPointF> &cathodeData) const;
};
