#include "circuit.h"

Circuit::Circuit()
{

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
