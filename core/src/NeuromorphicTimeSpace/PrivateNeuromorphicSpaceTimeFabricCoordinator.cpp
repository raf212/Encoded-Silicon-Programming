#pragma once

#include "NeuromorphicTimeSpace/NeuromorphicSpaceTimeFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    void NeuromorphicSpaceTimeFabricCoordinator::FreeAtomicCells_(std::atomic<packed64_t>* packed_cell_memory_ptr) noexcept
    {
        AllocatorOfAPCFabricCells::FreeFunction free_function = AllocatorOfFabric_.FreePackedCellStorage ?
                            AllocatorOfFabric_.FreePackedCellStorage : &AllocatorOfAPCFabricCells::DefaultFreeAtomicCells;
        const size_t alignment = AllocatorOfFabric_.Alignment ? AllocatorOfFabric_.Alignment : BIT_LENGTH_OF_A_PACKED_CELL;
        free_function(packed_cell_memory_ptr, SlabCellCount_, alignment, AllocatorOfFabric_.User);
    }

    void NeuromorphicSpaceTimeFabricCoordinator::ResetScalarsofTheFabric() noexcept
    {
        SlabBasePtr_ = nullptr;
        SlabCellCount_ = UNSIGNED_ZERO;
        SlotCellCount_ = UNSIGNED_ZERO;
        SlotCount_ = UNSIGNED_ZERO;
        SlabId_ = BRANCH_VERSION;
        SegmentPoolBegin_ = MINIMUM_BRANCH_CAPACITY;
        SegmentPoolEnd_ = MINIMUM_BRANCH_CAPACITY;
        HashBucketCount_ = UNSIGNED_ZERO;
        RelationRecordCount_ = UNSIGNED_ZERO;
        DeviceViewRecordCount_ = UNSIGNED_ZERO;
        ThreadTableCapacity_  = UNSIGNED_ZERO;
        for (auto& table_cache : TableCache_)
        {
            table_cache = CacheEntryOfFabricTable{};
        }
        FabricInitialized_.store(false, MoStoreSeq_);
    }


}
