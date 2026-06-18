#pragma once

#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{

    void SlabToFabricConverterAndCordinator::ShutDownFabric() noexcept
    {
        if (InitializationInProgress_.load(MoLoad_))
        {

        }
        FabricInitialized_.store(false, MoStoreSeq_);
        packed64_t* old_ptr = SlabBasePtr_;
        const size_t old_count = SlabCellCount_;
        SlabBasePtr_ = nullptr;
        SlabCellCount_ = UNSIGNED_ZERO;
        if (old_ptr)
        {
            FreeRawPackedCells_(old_ptr, old_count);
        }
        ResetScalarsofTheFabric_();
    }


}
