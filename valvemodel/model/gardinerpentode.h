#ifndef GARDINERPENTODE_H
#define GARDINERPENTODE_H

#include "gardinertriode.h"

class GardinerPentode : public GardinerTriode
{
public:
    GardinerPentode(int newType = GARDINER_PENTODE);

    int getType() const;
    void setType(int newType);

protected:
    int type;
};

#endif // GARDINERPENTODE_H
