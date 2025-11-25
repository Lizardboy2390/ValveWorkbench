#include "modelfactory.h"

Model *ModelFactory::createModel(int modelType)
{
    switch ((eModelType) modelType) {
    case SIMPLE_TRIODE:
        return new SimpleTriode();
    case KOREN_TRIODE:
        return new KorenTriode();
    case COHEN_HELIE_TRIODE:
        return new CohenHelieTriode();
    case REEFMAN_DERK_PENTODE:
        return new ReefmanPentode(DERK);
    case REEFMAN_DERK_E_PENTODE:
         return new ReefmanPentode(DERK_E);
    case EXTRACT_DERK_E_PENTODE:
        return new ExtractModelPentode();
    case GARDINER_PENTODE:
        return new GardinerPentode();
    case SIMPLE_MANUAL_PENTODE:
        return new SimpleManualPentode();
    }

    return nullptr;
}
