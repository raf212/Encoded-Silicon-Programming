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
        ResetScalarsofTheFabric_();
    }

    packed64_t NeuromorphicSpaceTimeFabricCoordinator::GetTotalRawCellUnchackedCell(size_t idx) noexcept
    {
        if (!SlabBasePtr_ || idx >= SlabCellCount_)
        {
            return APCDataStructure::PACKED_CELL_SENTENAL;
        }
        return SlabBasePtr_[idx].load(MoLoad_);
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::GetMetaCellView(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept
    {
        if (static_cast<size_t>(fabric_meta_idx) >= APCDataStructure::METACELL_COUNT || !SlabBasePtr_)
        {
            return false;
        }

        const packed64_t packed_meta_cell = SlabBasePtr_[static_cast<size_t>(fabric_meta_idx)].load(MoLoad_);
        meta_cell_view_address = PackedCell64_t::GetAuthoritiveViewsForACell(packed_meta_cell);

        return true;        
    }



    // std::optional<uint64_t> NeuromorphicSpaceTimeFabricCoordinator::ReadOccupancyApproxFromPairedIfValid(PackedCellLocalityTypes desired_occupancy_class) noexcept
    // {
    //     const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(desired_occupancy_class);
    //     if (desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
    //     {
    //         return std::nullopt;
    //     }

    //     const packed64_t raw_occ_low = GetTotalRawCellUnchackedCell(static_cast<size_t>(desired_occupancy_low_idx));
    //     const packed64_t raw_occ_high = GetTotalRawCellUnchackedCell(static_cast<size_t>(desired_occupancy_low_idx) + 1);

    //     const PackedCell64_t::AuthoritiveCellView low_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(raw_occ_low);
    //     const PackedCell64_t::AuthoritiveCellView high_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(raw_occ_high);

    //     if (low_cell_view.RawCell == PackedCell64_t::PACKED_CELL_SENTINAL && high_cell_view.RawCell == PackedCell64_t::PACKED_CELL_SENTINAL)
    //     {
    //         return PackedCell64_t::PACKED_CELL_SENTINAL;
    //     }

    //     if (
    //         high_cell_view.RawCell == PackedCell64_t::PACKED_CELL_SENTINAL && 
    //         low_cell_view.IsCellValid && 
    //         low_cell_view.SubClassOfMode32 == SubClassesOfMode32::LOW_OF_PAIRED_CELL && 
    //         low_cell_view.Priority == PriorityPhysics::VERSION_DEPENDENCY
    //     )
    //     {
    //         return static_cast<uint64_t>(*low_cell_view.CellValue32);
    //     }

    //     if (
    //         low_cell_view.IsCellValid && high_cell_view.IsCellValid &&
    //         low_cell_view.SubClassOfMode32 == SubClassesOfMode32::LOW_OF_PAIRED_CELL && high_cell_view.SubClassOfMode32 == SubClassesOfMode32::HIGH_OF_PAIRED_CELL &&
    //         low_cell_view.Priority == PriorityPhysics::VERSION_DEPENDENCY && high_cell_view.Priority == PriorityPhysics::VERSION_DEPENDENCY &&

    //     )
    //     {
    //         /* code */
    //     }
        


    //     return std::nullopt;
    // }



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

    //     if ((slab_id & Subdivision2x16Plus2x8InternalMode48CellModel::MASK_LOW_4) == UNSIGNED_ZERO)
    //     {
    //         slab_id = APCDataStructure::BRANCH_VERSION;
    //     }
        
    //     SlabId_ = static_cast<uint8_t>(slab_id & Subdivision2x16Plus2x8InternalMode48CellModel::MASK_LOW_4);
    //     SlotCount_ = slot_count,
    //     SlotCellCount_ = slot_cell_count,
    //     ThreadTableCapacity_ = std::max<uint32_t>(1u, fabric_thread_capacity);

    //     const size_t global_config_begin = UNSIGNED_ZERO;
    //     const size_t global_config_end = APCDataStructure::METACELL_COUNT;

    //     size_t cursor = DefaultFabricAlignment16Cell_(global_config_end);
    //     const size_t table_directory_begin = cursor;
    //     const size_t table_directory_end = table_directory_begin + static_cast<size_t>(TableIdOfAPCFabric::COUNT) * SLOT_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(table_directory_end);
    //     const size_t slot_directory_begin = cursor;
    //     const size_t slot_directory_end =  slot_directory_begin + (static_cast<size_t>(slot_count) * SLOT_RECORD_WIDTH_OF_FABRIC);

    //     HashBucketCount_ = NextPowerOf2Unsigned32_(
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

    //     RelationRecordCount_ = NextPowerOf2Unsigned32_(std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, static_cast<uint32_t>(slot_count) * 4u));
    //     const size_t relation_begin = cursor;
    //     const size_t relation_end = relation_begin + static_cast<size_t>(RelationRecordCount_) * RELATION_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(relation_end);
    //     const size_t free_retire_begin = cursor;
    //     const size_t free_retire_end = free_retire_begin + static_cast<size_t>(slot_count) * QUEUE_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(free_retire_end);
    //     const size_t ready_begin = cursor;
    //     const size_t ready_end = ready_begin + static_cast<size_t>(slot_count) * QUEUE_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(ready_end);
    //     const size_t work_begin = cursor;
    //     const size_t work_end = work_begin + static_cast<size_t>(slot_count) * WORK_RECORD_WIDTH_OF_FABRIC;

    //     DeviceViewRecordCount_ = NextPowerOf2Unsigned32_(std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, slot_count));
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

    //     const packed64_t idle_free32 = PackedCell64_t::MakeInitialValidPackedCell(PackedMode::MODE_32);

        

        
    // }


}
