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
    case COHEN_HELIE_PENTODE:
    case BEAM_TETRODE:
    case SECONDARY_EMISSION_PENTODE:
    case SECONDARY_EMISSION_BEAM_TETRODE:
        return new CohenHeliePentode(modelType);
    }

    return nullptr;
}
