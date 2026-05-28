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

    void NeuromorphicSpaceTimeFabricCoordinator::ResetScalarsofTheFabric_() noexcept
    {
        SlabBasePtr_ = nullptr;
        SlabCellCount_ = UNSIGNED_ZERO;
        SlotCellCount_ = UNSIGNED_ZERO;
        SlotCount_ = UNSIGNED_ZERO;
        SlabId_ = APCDataStructure::BRANCH_VERSION;
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

    uint32_t NeuromorphicSpaceTimeFabricCoordinator::NextPowerOf2Unsigned32_(uint32_t given_value) noexcept
    {
        if (given_value <= 2u)
        {
            return 2u;
        }
        --given_value;
        given_value |= given_value >> 1u;
        given_value |= given_value >> 2u;
        given_value |= given_value >> 4u;
        given_value |= given_value >> 8u;
        given_value |= given_value >> 16u;

        return given_value + 1u;
    }

    void NeuromorphicSpaceTimeFabricCoordinator::StorePackedCellUnchecked_(size_t idx, packed64_t packed_cell) noexcept
    {
        SlabBasePtr_[idx].store(packed_cell, MoStoreSeq_);
        SlabBasePtr_[idx].notify_all();
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::StoreAValidPackedCell_(size_t idx, packed64_t packed_cell) noexcept
    {
        if (!PackedCell64_t::IsThisCellValid(packed_cell))
        {
            return false;
        }

        if (!SlabBasePtr_ || idx  >= SlabCellCount_)
        {
            return false;
        }

        StorePackedCellUnchecked_(idx, packed_cell);

        return true;
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::StoreAValidFabricMetaCellOnly_(
        FabricMetaIndicies fabric_meta_idx, uint64_t value32_or_64, 
        PackedMode cell_mode, tag8_t mode_sub_class,PackedCellDataType cell_data_type, 
        PackedCellLocalityTypes locality_of_cell, PriorityPhysics priority, clk16_t extended_meta_value
    ) noexcept
    {
        const packed64_t a_valid_fabric_meta_cell32 = PackedCell64_t::MakeInitialValidPackedCell(
            cell_mode, locality_of_cell, PackedCellOwnership::NEUROMORPHIC_SPACE_TIME_FABRIC,
            APCPagedNodeSegmentClasses::CONTROL_SLOT, cell_data_type, value32_or_64, extended_meta_value,
            priority,(cell_mode == PackedMode::MODE_32) ? static_cast<SubClassesOfMode32>(mode_sub_class) : SubClassesOfMode32::SELF_CLASS,
            (cell_mode == PackedMode::MODE_48) ? static_cast<SubClassesOfMode48>(mode_sub_class) : SubClassesOfMode48::SELF_CLASS
        );

        if (a_valid_fabric_meta_cell32 == PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            return false;
        }

        if (static_cast<size_t>(fabric_meta_idx) < APCDataStructure::METACELL_COUNT)
        {
            //MakeInitialValidPackedCell::already checks validity
            StorePackedCellUnchecked_(static_cast<size_t>(fabric_meta_idx), a_valid_fabric_meta_cell32);
            return true;
        }

        return false;
        
    }

    void NeuromorphicSpaceTimeFabricCoordinator::WriteValidPairedEpoch_(FabricMetaIndicies meta_idx, uint64_t epoch_value) noexcept
    {
        const std::pair<packed64_t, packed64_t> low_high_split = PairedCellModelOfMode32::GetPairOfLow32FAndHigh32SFromUnsigned64(
            epoch_value, PackedCellLocalityTypes::IDLE, PackedCellOwnership::NEUROMORPHIC_SPACE_TIME_FABRIC      
        );

        FabricMetaIndicies desired_low_idx = FabricMetaIndicies::EOF_FABRIC_HEADER;

        switch (meta_idx)
        {
        case FabricMetaIndicies::GLOBAL_EPOCH_LOW32 :
        case FabricMetaIndicies::GLOBAL_EPOCH_HIGH32 :
            desired_low_idx = FabricMetaIndicies::GLOBAL_EPOCH_LOW32;
            break;

        case FabricMetaIndicies::MIN_SAFE_EPOCH_LOW32 :
        case FabricMetaIndicies::MIN_SAFE_EPOCH_HIGH32 :
            desired_low_idx = FabricMetaIndicies::MIN_SAFE_EPOCH_LOW32;
            break;

        default:
            return;
        }

        StorePackedCellUnchecked_(static_cast<size_t>(desired_low_idx), low_high_split.first);
        StorePackedCellUnchecked_(static_cast<size_t>(desired_low_idx) + 1, low_high_split.second);
    }


}
