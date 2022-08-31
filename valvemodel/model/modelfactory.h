#pragma once

#include "simpletriode.h"
#include "korentriode.h"
#include "cohenhelietriode.h"
#include "cohenheliepentode.h"

class ModelFactory
{
public:
    static Model *createModel(int modelType);
};

