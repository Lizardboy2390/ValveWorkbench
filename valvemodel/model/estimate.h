#pragma once

#include <QTreeWidgetItem>

#include "../ui/uibridge.h"

#include "../data/measurement.h"

#include "cohenhelietriode.h"

class CohenHelieTriode;

class Estimate : UIBridge
{
    Q_OBJECT
public:
    Estimate();

    double getMu() const;
    void setMu(double newMu);
    double getKg1() const;
    void setKg1(double newKg1);
    double getX() const;
    void setX(double newX);
    double getKp() const;
    void setKp(double newKp);
    double getKvb() const;
    void setKvb(double newKvb);
    double getKvb1() const;
    void setKvb1(double newKvb1);
    double getVct() const;
    void setVct(double newVct);
    double getKg2() const;
    void setKg2(double newKg2);
    double getA() const;
    void setA(double newA);
    double getAlpha() const;
    void setAlpha(double newAlpha);
    double getBeta() const;
    void setBeta(double newBeta);
    double getGamma() const;
    void setGamma(double newGamma);
    double getOmega() const;
    void setOmega(double newOmega);
    double getLambda() const;
    void setLambda(double newLambda);
    double getNu() const;
    void setNu(double newNu);
    double getS() const;
    void setS(double newS);
    double getAp() const;
    void setAp(double newAp);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);

    virtual void updateProperties(QTableWidget *properties);

    void estimateTriode(Measurement *measurement);
    void estimatePentode(Measurement *measurement, CohenHelieTriode *trideModel, int modelType, bool secondaryEmission = false);

    double anodeCurrent(double va, double vg1);

    QGraphicsItemGroup *plotModel(Plot *plot, Measurement *measurement);

    double getPsi() const;
    void setPsi(double newPsi);

protected:
    double mu = 40.0;
    double kg1 = 0.5;
    double x = 1.4;
    double kp = 500;
    double kvb = 300;
    double kvb1 = 10;
    double vct = 0.2;

    double kg2 = 2.0;
    double a = 0.0;
    double alpha = 0.1;
    double beta = 0.05;
    double gamma = 0.01;
    double psi = 3.5;

    double omega = 0.0;
    double lambda = 10.0;
    double nu = 1.0;
    double s = 0.01;
    double ap = 0.03;

    double findVa(Sweep * sweep, double ia);
    double findIa(Sweep * sweep, double va);

    void estimateMu(Measurement *measurement);
    void estimateKg1X(Measurement *measurement);
    void estimateKp(Measurement *measurement);
    void estimateKvbKvb1(Measurement *measurement);
    void estimateKg2(Measurement *measurement, CohenHelieTriode *triodeModel);
    void estimateA(Measurement *measurement, CohenHelieTriode *triodeModel);
    void estimateAlphaBeta(Measurement *measurement, CohenHelieTriode *triodeModel, int modelType);
    void estimateBetaGamma(Measurement *measurement, CohenHelieTriode *triodeModel);
};

