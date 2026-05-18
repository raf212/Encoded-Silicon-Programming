#pragma once 
#include "APCSegmentsCausalCordinator.hpp"
#include "NeuromorphicAPCFabricCordinator.hpp"


namespace PredictedAdaptedEncoding
{

    bool NeuromorphicAPCFabricCordinator::UsePreAllocatedNodePoolOfAPC(size_t requested_apc_size) noexcept
    {
        const size_t slot_capacity = (
            requested_apc_size > UNSIGNED_ZERO && requested_apc_size <= APCDataStructure::APC_MAX_LENGTH_OR_COUNTER
        ) ? std::max<size_t>(requested_apc_size, MINIMUM_BRANCH_CAPACITY) : MINIMUM_BRANCH_CAPACITY;

        const size_t slab_bytes = (requested_apc_size > APCDataStructure::APC_MAX_LENGTH_OR_COUNTER) ?
            std::max<size_t>(requested_apc_size, DEFAULT_FABRIC_SLBM_BYTES) :  DEFAULT_FABRIC_SLBM_BYTES;

        return InitializeStorageFabric(slot_capacity, slab_bytes);
    }


}


