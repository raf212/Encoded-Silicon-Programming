#pragma once

#include "NeuromorphicTimeSpace/NeuromorphicSpaceTimeFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    void NeuromorphicSpaceTimeFabricCoordinator::ShutDownFabric() noexcept
    {
        FabricInitialized_.store(false, MoStoreSeq_);
        if (SlabBasePtr_)
        {
            FreeAtomicCells_(SlabBasePtr_);
        }
        ResetScalarsofTheFabric();
    }


}
