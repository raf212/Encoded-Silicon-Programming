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

    bool NeuromorphicSpaceTimeFabricCoordinator::CheckAndStoreAPrebuildCellInSlab_(size_t idx, packed64_t packed_cell) noexcept
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

    bool NeuromorphicSpaceTimeFabricCoordinator::MakeCheckAndStoreAFabricControlValidCell_(
        FabricMetaIndicies fabric_meta_idx, uint64_t value32_or_64, 
        PackedMode cell_mode, tag8_t mode_sub_class, PackedCellDataType cell_data_type, 
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

        if (static_cast<size_t>(fabric_meta_idx) < SlabCellCount_)
        {
            //MakeInitialValidPackedCell::already checks validity
            StorePackedCellUnchecked_(static_cast<size_t>(fabric_meta_idx), a_valid_fabric_meta_cell32);
            return true;
        }

        return false;
        
    }


    void NeuromorphicSpaceTimeFabricCoordinator::StoreNewDefaultMeta48_(FabricMetaIndicies fabric_meta_idx, uint64_t value) noexcept
    {
        if (static_cast<size_t>(fabric_meta_idx) >= APCDataStructure::METACELL_COUNT)
        {
            return;
        }
        MakeCheckAndStoreAFabricControlValidCell_(fabric_meta_idx, value & MaskLowNBits(CLK_B48), PackedMode::MODE_48);
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::UpdateValidPairedOccupancyApproximation_(
        PackedCellLocalityTypes desired_occupancy_of_locality, uint64_t desired_occupancy_value,
        bool force_update, clk16_t pair_version
    ) noexcept
    {
        static constexpr uint8_t MAX_TRIES = 128;

        const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(desired_occupancy_of_locality);

        if (force_update && desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
        {
            return false;
        }

        const std::pair<packed64_t, packed64_t> low32_and_probable_high32 = PairedVersionedCellModelOfMode32::GetPairOfLow32FAndHigh32SFromUnsigned64(
            desired_occupancy_value, pair_version,
            PackedCellLocalityTypes::PUBLISHED, PackedCellOwnership::NEUROMORPHIC_SPACE_TIME_FABRIC,
            APCPagedNodeSegmentClasses::CONTROL_SLOT
        );

        auto ForceUpdate = [&](){
            StorePackedCellUnchecked_(static_cast<size_t>(desired_occupancy_low_idx), low32_and_probable_high32.first);
            StorePackedCellUnchecked_(static_cast<size_t>(desired_occupancy_low_idx) + 1, low32_and_probable_high32.second);
            return true;
        };
        
        if (force_update)
        {
            return ForceUpdate();
        }

        const PackedCell64_t::AuthoritiveCellView low32_half_view{};
        const PackedCell64_t::AuthoritiveCellView high32_half_view{};
        const std::optional<uint64_t> maybe_occupancy = ReadOccupancyApproxFromPairedIfValid(desired_occupancy_of_locality, &low32_half_view, &high32_half_view);

        if (!maybe_occupancy|| *maybe_occupancy == PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            return ForceUpdate();
        }
    
        if (low32_half_view.LocalityOfCell == PackedCellLocalityTypes::CLAIMED || high32_half_view.LocalityOfCell == PackedCellLocalityTypes::CLAIMED)
        {
            return false;
        }

        if (*maybe_occupancy!= PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            //just cmpx  low
            if (*maybe_occupancy < IN_CELL_VALUE_MODE32_SENTINAL && low32_and_probable_high32.second == PackedCell64_t::PACKED_CELL_SENTINAL)
            {
                packed64_t expected = low32_half_view.RawCell;
                packed64_t desired = low32_and_probable_high32.first;
                for (size_t i = 0; i < MAX_TRIES; i++)
                {
                    if (SlabBasePtr_[static_cast<size_t>(desired_occupancy_low_idx)].compare_exchange_strong(
                        expected, desired, OnExchangeSuccess, OnExchangeFailure
                    ))
                    {
                        StorePackedCellUnchecked_(static_cast<size_t>(desired_occupancy_low_idx) + 1, low32_and_probable_high32.second);
                        return true;
                    }
                }
                //intehrate failure count and AtomicAdaptiveBackoff
                return false;
            }

            if ((low32_half_view.IsCellValid && high32_half_view.IsCellValid) || desired_occupancy_value >= IN_CELL_VALUE_MODE32_SENTINAL)
            {
                packed64_t expected_low = low32_half_view.RawCell;
                packed64_t desired_buffer_low = PackedCell64_t::SetLocalityInPacked(low32_and_probable_high32.first, PackedCellLocalityTypes::CLAIMED);
                packed64_t expected_high = high32_half_view.RawCell;
                packed64_t desired_buffer_high = low32_and_probable_high32.second;

                auto RestoreLow = [&]()
                {
                    StorePackedCellUnchecked_(static_cast<size_t>(desired_occupancy_low_idx), low32_half_view.RawCell);
                    return false;
                };

                for (size_t i = 0; i < MAX_TRIES; i++)
                {
                    if (SlabBasePtr_[static_cast<size_t>(desired_occupancy_low_idx)].compare_exchange_strong(
                        expected_low, desired_buffer_low, OnExchangeSuccess, OnExchangeFailure
                    ))
                    {
                        for (size_t j = 0; j < MAX_TRIES; j++)
                        {
                            if (SlabBasePtr_[static_cast<size_t>(desired_occupancy_low_idx) + 1].compare_exchange_strong(
                                expected_high, desired_buffer_high, OnExchangeSuccess, OnExchangeFailure
                            ))
                            {
                                return RestoreLow();
                            }
                        }

                        return RestoreLow();
                    }
                }
                return false;
            }
            return false;
        }
        return ForceUpdate(); 
    }



    void NeuromorphicSpaceTimeFabricCoordinator::WriceFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept
    {
        StoreNewDefaultMeta48_(FabricMetaIndicies::MAGIC, APCDataStructure::FABRIC_MAGIC);
        StoreNewDefaultMeta48_(FabricMetaIndicies::VERSION, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultMeta48_(FabricMetaIndicies::FLAGS, UNSIGNED_ZERO);
        StoreNewDefaultMeta48_(FabricMetaIndicies::SLAB_ID, static_cast<uint64_t>(SlabId_));

        StoreNewDefaultMeta48_(FabricMetaIndicies::TOTAL_CELLS, static_cast<uint64_t>(SlabCellCount_));
        StoreNewDefaultMeta48_(FabricMetaIndicies::CONTROL_CELLS_OF_FABRIC, static_cast<uint64_t>(SegmentPoolBegin_));
        StoreNewDefaultMeta48_(FabricMetaIndicies::SLOT_COUNT, static_cast<uint64_t>(SlotCount_));
        StoreNewDefaultMeta48_(FabricMetaIndicies::SLOT_CELL_COUNT, static_cast<uint64_t>(SlotCellCount_));
        StoreNewDefaultMeta48_(FabricMetaIndicies::SEGMENT_POOL_BEGIN_IDX, static_cast<uint64_t>(SegmentPoolBegin_));
        StoreNewDefaultMeta48_(FabricMetaIndicies::SEGMENT_POOL_END_IDX, static_cast<uint64_t>(SegmentPoolEnd_));
        StoreNewDefaultMeta48_(FabricMetaIndicies::FREE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        StoreNewDefaultMeta48_(FabricMetaIndicies::RETIRE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        StoreNewDefaultMeta48_(FabricMetaIndicies::RELATION_FREE_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        
        StoreNewDefaultMeta48_(FabricMetaIndicies::GLOBAL_EPOCH48, 1u);
        StoreNewDefaultMeta48_(FabricMetaIndicies::MIN_SAFE_EPOCH48, UNSIGNED_ZERO);

        StoreNewDefaultMeta48_(FabricMetaIndicies::NEXT_BRANCH_ID, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultMeta48_(FabricMetaIndicies::NEXT_RELATION_ID, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultMeta48_(FabricMetaIndicies::NEXT_DEVICE_VIEW_ID, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultMeta48_(FabricMetaIndicies::FABRIC_CLOCK16, 1u);
        StoreNewDefaultMeta48_(FabricMetaIndicies::WORK_WRITE_CURSOR, UNSIGNED_ZERO);
        StoreNewDefaultMeta48_(FabricMetaIndicies::WORK_READ_CURSOR, UNSIGNED_ZERO);
        StoreNewDefaultMeta48_(FabricMetaIndicies::READY_WRITE_CURSOR, UNSIGNED_ZERO);
        StoreNewDefaultMeta48_(FabricMetaIndicies::READY_READ_CURSOR, UNSIGNED_ZERO);

        StoreNewDefaultMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_BEGIN, static_cast<uint64_t>(table_directory_begin));
        StoreNewDefaultMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_END, static_cast<uint32_t>(table_directory_end));
        StoreNewDefaultMeta48_(FabricMetaIndicies::TABLE_COUNT, static_cast<uint64_t>(TableIdOfAPCFabric::COUNT));
        StoreNewDefaultMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_VERSION, APCDataStructure::BRANCH_VERSION);




    }



}
