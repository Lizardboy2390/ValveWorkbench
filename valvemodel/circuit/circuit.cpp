#include "circuit.h"

#include <QApplication>

bool no_bg = false;
int vecgetnumber = 0;
double v2dat = 0.0;
bool has_break = false;
bool errorflag = false;

extern bool no_bg;
extern int vecgetnumber;
extern double v2dat;
extern bool has_break;
extern bool errorflag;

Circuit::Circuit()
{

}

void Circuit::setOverlaysVisible(bool visible)
{
    if (anodeLoadLine) anodeLoadLine->setVisible(visible);
    if (cathodeLoadLine) cathodeLoadLine->setVisible(visible);
    if (acSignalLine) acSignalLine->setVisible(visible);
    if (opMarker) opMarker->setVisible(visible);
}

void Circuit::resetOverlays()
{
    anodeLoadLine = nullptr;
    cathodeLoadLine = nullptr;
    acSignalLine = nullptr;
    opMarker = nullptr;
}

void Circuit::setParameter(int index, double value)
{
    parameter[index]->setValue(value);

    update(index);
}

double Circuit::getParameter(int index)
{
    return parameter[index]->getValue();
}

void Circuit::setDevice1(Device *newDevice1)
{
    device1 = newDevice1;
}

void Circuit::setDevice2(Device *newDevice2)
{
    device2 = newDevice2;
}

bool Circuit::waitSimulationEnd()
{
    bool completed = false;
    for (int i=0;i < 20;i++) {
#if defined(__MINGW32__) || defined(_MSC_VER)
        Sleep (100);
#else
        usleep (100000);
#endif
        QApplication::processEvents(QEventLoop::AllEvents);

        if (no_bg) {
            completed = true;
            break;
        }
    }

    return (completed);
}