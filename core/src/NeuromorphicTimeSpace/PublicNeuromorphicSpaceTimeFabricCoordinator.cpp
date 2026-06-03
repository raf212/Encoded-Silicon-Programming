#pragma once

#include "NeuromorphicTimeSpace/NeuromorphicSpaceTimeFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    void NeuromorphicSpaceTimeFabricCoordinator::ShutDownFabric() noexcept
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

    constexpr packed64_t NeuromorphicSpaceTimeFabricCoordinator::ReadCompletePackedCellDirectly(
        size_t slab_index, std::optional<PackedCellLocalityTypes> invalid_cell_locality
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        const packed64_t desired_cell_raw = SlabBasePtr_[slab_index];
        if (invalid_cell_locality.has_value() && PackedCell64_t::ExtractLocalityFromPacked(desired_cell_raw) != *invalid_cell_locality)
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }
        return desired_cell_raw;
    } 

    constexpr packed64_t NeuromorphicSpaceTimeFabricCoordinator::AtomicallyLoadReadCompletePackedCell(
        size_t slab_index, std::optional<PackedCellLocalityTypes> invalid_cell_locality
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        std::atomic_ref<const packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        const packed64_t desired_cell_raw = packed_cell_ref.load(MoLoad_);
        if (invalid_cell_locality.has_value() && PackedCell64_t::ExtractLocalityFromPacked(desired_cell_raw) != *invalid_cell_locality)
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }
        return desired_cell_raw;
    }

    constexpr void NeuromorphicSpaceTimeFabricCoordinator::StorePackedCellUncheckedDirectly(size_t slab_index, packed64_t packed_cell) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return;
        }
        SlabBasePtr_[slab_index] = packed_cell;
    }

    constexpr void NeuromorphicSpaceTimeFabricCoordinator::AtomicallyStorePackedCellUnchecked(
        size_t slab_index, packed64_t packed_cell,
        std::memory_order mem_order
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return;
        }
        std::atomic_ref<packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        packed_cell_ref.store(packed_cell, mem_order);
        packed_cell_ref.notify_all();
    }

    constexpr bool NeuromorphicSpaceTimeFabricCoordinator::AtomicallyCompareAndExchangeStrongPackedCell(
        size_t slab_index, 
        packed64_t& expected_packed_cell, 
        packed64_t desired_packed_cell,
        std::memory_order mem_order_success,
        std::memory_order mem_order_failure
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return false;
        }
        std::atomic_ref<packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        return packed_cell_ref.compare_exchange_strong(expected_packed_cell, desired_packed_cell, mem_order_success, mem_order_failure);
    }

/// checked--------------------------------------------------------




    // bool NeuromorphicSpaceTimeFabricCoordinator::GetMetaCellView(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept
    // {
    //     if (static_cast<size_t>(fabric_meta_idx) >= APCDataStructure::METACELL_COUNT || !SlabBasePtr_)
    //     {
    //         return false;
    //     }

    //     const packed64_t packed_meta_cell = SlabBasePtr_[static_cast<size_t>(fabric_meta_idx)].load(MoLoad_);
    //     meta_cell_view_address = PackedCell64_t::GetAuthoritiveViewsForACell(packed_meta_cell);

    //     return true;        
    // }



    std::optional<uint64_t> NeuromorphicSpaceTimeFabricCoordinator::ReadOccupancyApproxFromPairedIfValid(
        PackedCellLocalityTypes desired_occupancy_class,
        const PackedCell64_t::AuthoritiveCellView* low_half_view_ptr,
        const PackedCell64_t::AuthoritiveCellView* high_half_view_ptr
    ) noexcept
    {
        const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(desired_occupancy_class);
        if (desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
        {
            return std::nullopt;
        }

        const packed64_t raw_occ_low = AtomicallyLoadReadCompletePackedCell(static_cast<size_t>(desired_occupancy_low_idx));
        const packed64_t raw_occ_high = AtomicallyLoadReadCompletePackedCell(static_cast<size_t>(desired_occupancy_low_idx) + 1);

        return PairedVersionedCellModelOfMode32::GetFullUnsigned64FromPairedVersionedCell(raw_occ_low, raw_occ_high, low_half_view_ptr, high_half_view_ptr);
    }


    // bool NeuromorphicSpaceTimeFabricCoordinator::InitializeFabric(
    //     uint16_t slot_count,
    //     size_t slot_cell_count,
    //     uint8_t slab_id,
    //     uint32_t fabric_thread_capacity
    // ) noexcept
    // {
    //     bool expected_init = false;
    //     if (!InitializationInProgress_.compare_exchange_strong(
    //         expected_init,
    //         true,
    //         std::memory_order_acq_rel,
    //         std::memory_order_acquire
    //     ))
    //     {
    //         return false;
    //     }

    //     auto ReleseInitGuard = [this]() noexcept
    //     {
    //         InitializationInProgress_.store(false, std::memory_order_release);
    //     };

    //     ShutDownFabric();

    //     if (slot_count == UNSIGNED_ZERO || slot_count == APCDataStructure::APC_INDEX_SENTINAL)
    //     {
    //         ReleseInitGuard();
    //         return false;
    //     }

    //     if ((slab_id & Clock16Subdivision1x8Plus2x4InMode32CellModel::MASK_LOW_4) == UNSIGNED_ZERO)
    //     {
    //         slab_id = APCDataStructure::BRANCH_VERSION;
    //     }
        
    //     SlabId_ = static_cast<uint8_t>(slab_id & Clock16Subdivision1x8Plus2x4InMode32CellModel::MASK_LOW_4);
    //     SlotCount_ = slot_count,
    //     SlotCellCount_ = slot_cell_count,
    //     ThreadTableCapacity_ = std::max<uint32_t>(1u, fabric_thread_capacity);

    //     const size_t global_config_begin = UNSIGNED_ZERO;
    //     const size_t global_config_end = APCDataStructure::METACELL_COUNT;

    //     size_t cursor = DefaultFabricAlignment16Cell_(global_config_end);
    //     const size_t table_directory_begin = cursor;
    //     const size_t table_directory_end = table_directory_begin + static_cast<size_t>(FabricTableSegmentClasses::COUNT) * SLOT_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(table_directory_end);
    //     const size_t slot_directory_begin = cursor;
    //     const size_t slot_directory_end =  slot_directory_begin + (static_cast<size_t>(slot_count) * SLOT_RECORD_WIDTH_OF_FABRIC);

    //     HashBucketCount_ = CoreOfFabricCoordinator::NextPowerOf2Unsigned32_(
    //         std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, static_cast<uint32_t>(slot_count) * HASH_BUCKED_WIDTH_OF_FABRIC)
    //     );
    //     cursor = DefaultFabricAlignment16Cell_(slot_directory_end);
    //     const size_t hash_span = static_cast<size_t>(HashBucketCount_) * HASH_BUCKED_WIDTH_OF_FABRIC;
    //     const size_t branch_hash_begin = cursor;
    //     const size_t branch_hash_end = branch_hash_begin + hash_span;

    //     cursor = DefaultFabricAlignment16Cell_(branch_hash_end);
    //     const size_t logical_hash_begin = cursor;
    //     const size_t logical_hash_end = logical_hash_begin + hash_span;

    //     cursor = DefaultFabricAlignment16Cell_(logical_hash_end);
    //     const size_t shared_hash_begin = cursor;
    //     const size_t shared_hash_end = shared_hash_begin + hash_span;

    //     RelationRecordCount_ = CoreOfFabricCoordinator::NextPowerOf2Unsigned32_(std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, static_cast<uint32_t>(slot_count) * 4u));
    //     const size_t edge_of_fabric_table_begin = cursor;
    //     const size_t edge_of_fabric_table_end = edge_of_fabric_table_begin + static_cast<size_t>(RelationRecordCount_) * RELATION_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(edge_of_fabric_table_end);
    //     const size_t free_retire_begin = cursor;
    //     const size_t free_retire_end = free_retire_begin + static_cast<size_t>(slot_count) * QUEUE_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(free_retire_end);
    //     const size_t ready_begin = cursor;
    //     const size_t ready_end = ready_begin + static_cast<size_t>(slot_count) * QUEUE_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(ready_end);
    //     const size_t work_begin = cursor;
    //     const size_t work_end = work_begin + static_cast<size_t>(slot_count) * WORK_RECORD_WIDTH_OF_FABRIC;

    //     DeviceViewRecordCount_ = CoreOfFabricCoordinator::NextPowerOf2Unsigned32_(std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, slot_count));
    //     cursor = DefaultFabricAlignment16Cell_(work_end);
    //     const size_t device_view_begin = cursor;
    //     const size_t device_view_end = device_view_begin + static_cast<size_t>(DeviceViewRecordCount_) * DEVICE_VIEW_WIDTH_OF_APC_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(device_view_end);
    //     const size_t thread_table_begin = cursor;
    //     const size_t thread_table_end = thread_table_begin + static_cast<size_t>(ThreadTableCapacity_) *  THREAD_TABLE_RECORD_WIDTH;

    //     cursor = DefaultFabricAlignment16Cell_(thread_table_end);
    //     SegmentPoolBegin_ = DefaultFabricAlignment16Cell_(std::max(cursor, DEFAULT_FABRIC_CONTROLIO_LENGTH));
    //     SegmentPoolEnd_ = SegmentPoolBegin_ + static_cast<size_t>(slot_count) * slot_cell_count;

    //     if (SegmentPoolEnd_ >= PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT)
    //     {
    //         ResetScalarsofTheFabric_();
    //         ReleseInitGuard();
    //         return false;
    //     }

    //     SlabCellCount_ = SegmentPoolEnd_;

    //     const packed64_t idle_free32 = PackedCell64_t::MakeInitialFabricValidPackedCell(PackedMode::MODE_32_ATOMIC_GUARANTEED);

    //     for (size_t i = 0; i < SlabCellCount_; i++)
    //     {
    //         SlabBasePtr_[i].store(idle_free32, MoStoreSeq_);
    //     }

    //     WriteFabricMetaHeader_(table_directory_begin, table_directory_end);

    //     WriteDirectoryEntry_(FabricTableSegmentClasses::GLOBAL_AND_CONFIG, global_config_begin, global_config_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::TABLE_DIRECTORY, table_directory_begin, table_directory_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::SLOT_DIRECTORY, slot_directory_begin, slot_directory_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::BRANCH_HASH, branch_hash_begin, branch_hash_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::LOGICAL_HASH, logical_hash_begin, logical_hash_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::SHARED_HASH, shared_hash_begin, shared_hash_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::EDGE_TABLE, edge_of_fabric_table_begin, edge_of_fabric_table_end, APCDataStructure::BRANCH_VERSION);      
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::FREE_RETIRE_TABLE, free_retire_begin, free_retire_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::READY_QUEUE, ready_begin, ready_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::WORK_QUEUE, work_begin, work_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::DEVICE_VIEW_TABLE, device_view_begin, device_view_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::THREAD_TABLE, thread_table_begin, thread_table_end, APCDataStructure::BRANCH_VERSION);
    //     WriteDirectoryEntry_(FabricTableSegmentClasses::SEGMENT_POOL, SegmentPoolBegin_, SegmentPoolEnd_, APCDataStructure::BRANCH_VERSION);

    //     //hash table init
    //     InitializeHashTable_(FabricTableSegmentClasses::BRANCH_HASH);
    //     InitializeHashTable_(FabricTableSegmentClasses::LOGICAL_HASH);
    //     InitializeHashTable_(FabricTableSegmentClasses::SHARED_HASH);

        

    //     //continue

    //     return true;
    // }


}
