#include "gardinerpentode.h"

GardinerPentode::GardinerPentode(int newType) : type(newType)
{

}

int GardinerPentode::getType() const
{
    return type;
}

void GardinerPentode::setType(int newType)
{
    type = newType;
}
