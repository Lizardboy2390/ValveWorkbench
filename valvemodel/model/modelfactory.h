#pragma once

#include "simpletriode.h"
#include "korentriode.h"
#include "cohenhelietriode.h"
#include "reefmanpentode.h"
#include "gardinerpentode.h"
#include "simplemanualpentode.h"
#include "extractmodelpentode.h"

class ModelFactory
{
public:
    static Model *createModel(int modelType);
};

