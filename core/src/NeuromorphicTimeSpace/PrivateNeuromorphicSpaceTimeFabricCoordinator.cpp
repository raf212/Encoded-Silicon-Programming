#pragma once

#include "NeuromorphicTimeSpace/NeuromorphicSpaceTimeFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    packed64_t* NeuromorphicSpaceTimeFabricCoordinator::AllocatePackedCellRaw_(size_t count_of_cells) noexcept
    {
        auto allocation_function = AllocatorOfFabric_.AllocatePackedCellStorage ? 
            AllocatorOfFabric_.AllocatePackedCellStorage : &RawPackedCellAllocator::DefaultAllocateAtomicCells;
        
        size_t alignment = AllocatorOfFabric_.Alignment ? AllocatorOfFabric_.Alignment : BIT_LENGTH_OF_A_PACKED_CELL;
        alignment = std::max<size_t>(alignment, alignof(packed64_t));
        alignment = std::max<size_t>(alignment, BIT_LENGTH_OF_A_PACKED_CELL);

        return allocation_function(count_of_cells, alignment, AllocatorOfFabric_.User);

    }


    void NeuromorphicSpaceTimeFabricCoordinator::FreeRawPackedCells_(packed64_t* packed_cell_memory_ptr, size_t packed_cell_count) noexcept
    {
        RawPackedCellAllocator::FreeFunction free_function = AllocatorOfFabric_.FreePackedCellStorage ?
                            AllocatorOfFabric_.FreePackedCellStorage : &RawPackedCellAllocator::DefaultFreeAtomicCells;
        size_t alignment = AllocatorOfFabric_.Alignment ? AllocatorOfFabric_.Alignment : BIT_LENGTH_OF_A_PACKED_CELL;
        alignment = std::max<size_t>(alignment, alignof(packed64_t));
        alignment = std::max<size_t>(alignment, BIT_LENGTH_OF_A_PACKED_CELL);

        free_function(packed_cell_memory_ptr, packed_cell_count, alignment, AllocatorOfFabric_.User);
    }

    void NeuromorphicSpaceTimeFabricCoordinator::ResetScalarsofTheFabric_() noexcept
    {
        SlabBasePtr_ = nullptr;
        SlabCellCount_ = UNSIGNED_ZERO;
        SlotCellCount_ = UNSIGNED_ZERO;
        SlotCount_ = UNSIGNED_ZERO;
        SlabId_ = APCDataStructure::BRANCH_VERSION;

        SegmentPoolBegin_ = APCDataStructure::METACELL_COUNT;
        SegmentPoolEnd_ = APCDataStructure::METACELL_COUNT;

        HashBucketCount_ = UNSIGNED_ZERO;
        RelationRecordCount_ = UNSIGNED_ZERO;
        DeviceViewRecordCount_ = UNSIGNED_ZERO;
        ThreadTableCapacity_  = UNSIGNED_ZERO;

        FabricInitialized_.store(false, MoStoreSeq_);
        InitializationInProgress_.store(false, MoStoreSeq_);
    }



    constexpr bool NeuromorphicSpaceTimeFabricCoordinator::MakeAndStoreDirectlyAFabricOwnedCell_(
        size_t idx, uint64_t value32_or_64, 
        FabricTableSegmentClasses fabric_segment_class,
        PackedMode cell_mode, clk16_t extended_meta_value,
        tag8_t mode_sub_class, PackedCellDataType cell_data_type, 
        PackedCellLocalityTypes locality_of_cell, 
        CellMapAndPriority priority
    ) noexcept
    {
        const packed64_t a_valid_fabric_meta_cell32 = PackedCell64_t::MakeInitialFabricValidPackedCell(
            cell_mode, locality_of_cell, 
            fabric_segment_class, cell_data_type, 
            value32_or_64, extended_meta_value,
            priority,(cell_mode == PackedMode::MODE_32_ATOMIC_GUARANTEED) ? static_cast<SubClassesOfMode32>(mode_sub_class) : SubClassesOfMode32::SELF_CLASS,
            (cell_mode == PackedMode::MODE_48_ATOMIC_GUARANTEED) ? static_cast<SubClassesOfMode48>(mode_sub_class) : SubClassesOfMode48::SELF_CLASS
        );

        if (a_valid_fabric_meta_cell32 == PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            return false;
        }

        //MakeInitialFabricValidPackedCell::already checks validity
        StorePackedCellUncheckedDirectly(idx, a_valid_fabric_meta_cell32);
        return true;
    }

    constexpr bool NeuromorphicSpaceTimeFabricCoordinator::StoreFebricControlMeta48Directly_(
        FabricMetaIndicies fabric_meta_idx, 
        uint64_t value, 
        PackedCellLocalityTypes cell_locality,
        SubClassesOfMode48 subclass48,
        CellMapAndPriority priority
    )noexcept
    {
        const size_t slab_index = static_cast<size_t>(fabric_meta_idx);
        if (slab_index >= APCDataStructure::METACELL_COUNT)
        {
            return false;
        }

        return MakeAndStoreDirectlyAFabricOwnedCell_(
            slab_index,
            value,
            FabricTableSegmentClasses::GENERIC_CONTROL,
            PackedMode::MODE_48_ATOMIC_GUARANTEED,
            UNSIGNED_ZERO,
            static_cast<tag8_t>(subclass48),
            PackedCellDataType::UnsignedPCellDataType,
            cell_locality,
            priority
        );
    }

    //Integrate AtomicAdaptiveBackoff
    // add CAS_FAILURE_COUNT
    constexpr bool NeuromorphicSpaceTimeFabricCoordinator::UpdateValidPairedOccupancyApproxAtomically_(
        PackedCellLocalityTypes desired_occupancy_of_locality, uint64_t desired_occupancy_value,
        bool force_update, clk16_t pair_version
    ) noexcept
    {
        const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(desired_occupancy_of_locality);

        if (force_update && desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
        {
            return false;
        }

        const size_t low_idx = static_cast<size_t>(desired_occupancy_low_idx);
        const size_t high_idx = low_idx + 1;

        const std::pair<packed64_t, packed64_t> low32_and_probable_high32 = PairedVersionedCellModelOfMode32::GetPairOfLow32FAndHigh32SFromUnsigned64ForFabric(
            desired_occupancy_value, pair_version,
            PackedCellLocalityTypes::PUBLISHED,
            FabricTableSegmentClasses::GENERIC_CONTROL
        );

        auto ForceUpdate = [&](){
            AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
            AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
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
                const packed64_t desired = low32_and_probable_high32.first;
                for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
                {
                    if (AtomicallyCompareAndExchangeStrongPackedCell(low_idx, expected, desired))
                    {
                        AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
                        return true;
                    }

                    if (PackedCell64_t::ExtractLocalityFromPacked(expected) == PackedCellLocalityTypes::CLAIMED)
                    {
                        return false;
                    }
                    
                }
                //intehrate failure count and AtomicAdaptiveBackoff
                return false;
            }

            //double cas 
            if ((low32_half_view.IsCellValid && high32_half_view.IsCellValid) || desired_occupancy_value >= IN_CELL_VALUE_MODE32_SENTINAL)
            {
                packed64_t expected_low = low32_half_view.RawCell;
                const packed64_t desired_claimed_low = PackedCell64_t::SetLocalityInPacked(low32_and_probable_high32.first, PackedCellLocalityTypes::CLAIMED);

                auto RestoreLow = [&]()
                {
                    AtomicallyStorePackedCellUnchecked(low_idx, low32_half_view.RawCell);
                    return false;
                };

                for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
                {
                    if (AtomicallyCompareAndExchangeStrongPackedCell(low_idx, expected_low, desired_claimed_low))
                    {
                        packed64_t expected_high = high32_half_view.RawCell;

                        for (size_t j = 0; j < DEFAULT_MAX_TRIES; j++)
                        {
                            if (AtomicallyCompareAndExchangeStrongPackedCell(high_idx, expected_high, low32_and_probable_high32.second))
                            {
                                AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
                                return true;
                            }

                            if (PackedCell64_t::ExtractLocalityFromPacked(expected_high) == PackedCellLocalityTypes::CLAIMED)
                            {
                                return RestoreLow();
                            }
                        }

                        return RestoreLow();
                    }

                    if (PackedCell64_t::ExtractLocalityFromPacked(expected_low) == PackedCellLocalityTypes::CLAIMED)
                    {
                        return false;
                    }
                }
            }

            return false;
        }

        return ForceUpdate(); 
    }

    constexpr void NeuromorphicSpaceTimeFabricCoordinator::ResetAll4TypesOfOccupancyMetaData_() noexcept
    {
        UpdateValidPairedOccupancyApproxAtomically_(PackedCellLocalityTypes::IDLE, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproxAtomically_(PackedCellLocalityTypes::PUBLISHED, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproxAtomically_(PackedCellLocalityTypes::CLAIMED, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproxAtomically_(PackedCellLocalityTypes::FAULTY, UNSIGNED_ZERO, true);
    }


    constexpr void NeuromorphicSpaceTimeFabricCoordinator::WriteFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept
    {
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::MAGIC, APCDataStructure::FABRIC_MAGIC);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::VERSION, APCDataStructure::BRANCH_VERSION);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::FLAGS, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::SLAB_ID, static_cast<uint64_t>(SlabId_));

        StoreFebricControlMeta48Directly_(FabricMetaIndicies::TOTAL_CELLS, static_cast<uint64_t>(SlabCellCount_));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::CONTROL_CELLS_OF_FABRIC, static_cast<uint64_t>(SegmentPoolBegin_));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::SLOT_COUNT, static_cast<uint64_t>(SlotCount_));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::SLOT_CELL_COUNT, static_cast<uint64_t>(SlotCellCount_));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::SEGMENT_POOL_BEGIN_IDX, static_cast<uint64_t>(SegmentPoolBegin_));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::SEGMENT_POOL_END_IDX, static_cast<uint64_t>(SegmentPoolEnd_));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::FREE_SLOT_HEAD, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::RETIRE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::RELATION_FREE_HEAD, UNSIGNED_ZERO);
        
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::GLOBAL_EPOCH48, 1u);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::MIN_SAFE_EPOCH48, UNSIGNED_ZERO);

        StoreFebricControlMeta48Directly_(FabricMetaIndicies::NEXT_BRANCH_ID, APCDataStructure::BRANCH_VERSION);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::NEXT_RELATION_ID, APCDataStructure::BRANCH_VERSION);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::NEXT_DEVICE_VIEW_ID, APCDataStructure::BRANCH_VERSION);
        
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::FABRIC_CLOCK16, 1u);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::WORK_WRITE_CURSOR, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::WORK_READ_CURSOR, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::READY_WRITE_CURSOR, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::READY_READ_CURSOR, UNSIGNED_ZERO);

        StoreFebricControlMeta48Directly_(FabricMetaIndicies::TABLE_DIRECTORY_BEGIN, static_cast<uint64_t>(table_directory_begin));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::TABLE_DIRECTORY_END, static_cast<uint32_t>(table_directory_end));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::TABLE_DIRECTORY_COUNT, static_cast<uint64_t>(FabricTableSegmentClasses::COUNT));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::TABLE_DIRECTORY_VERSION, APCDataStructure::BRANCH_VERSION);

        ResetAll4TypesOfOccupancyMetaData_();

        StoreFebricControlMeta48Directly_(FabricMetaIndicies::CAS_FAILURE_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::ERROR_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::RETIRE_SLOT_HEAD, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::LIVE_SLOT_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::HASH_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::HASH_COMPACTION_COUNT, UNSIGNED_ZERO);

        StoreFebricControlMeta48Directly_(FabricMetaIndicies::WORK_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::READY_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::BACKOFF_SPIN_LIMIT, 16u);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::BACKOFF_YIELD_LIMIT, 64u);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::INITIALIZATION_STATE, static_cast<uint64_t>(PackedCellLocalityTypes::PUBLISHED));
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::HAS_COMPACTION_INFLIGHT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::RELATION_RECLAIM_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::WORK_QUEUE_DROPPED_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::THREAD_TABLE_CAPACITY, ThreadTableCapacity_);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::THREAD_ACTIVE_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::THREAD_REGISTRATION_FAILURE, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::RELATION_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::RELATION_UNLINK_FAILURES, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::WORK_QUEUE_CLAIM_FAILURES, UNSIGNED_ZERO);
        StoreFebricControlMeta48Directly_(FabricMetaIndicies::EOF_FABRIC_HEADER, APCDataStructure::FABRIC_META_EOF);
    }


    constexpr size_t NeuromorphicSpaceTimeFabricCoordinator::GetTableDirectoryCellSlabIndex_(
        FabricTableSegmentClasses table_class, 
        TableEntryCellTypeOfFabric entry_type, 
        std::optional<PackedCellLocalityTypes> invalid_cell_locality
    ) noexcept
    {
        const size_t table_meta_index = static_cast<size_t>(table_class);
        if (!CoreOfFabricCoordinator::IsValidFabricTable(table_class))
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }
        const packed64_t directory_begin_cell = ReadCompletePackedCellDirectly(table_meta_index, invalid_cell_locality);
        
        const size_t base_idx = static_cast<size_t>(PackedCell64_t::ExtractClk48(directory_begin_cell));
        return base_idx + (table_meta_index * TABLE_ENTRY_WIDTH_OF_FABRIC) + static_cast<size_t>(entry_type);

    }

    // bool NeuromorphicSpaceTimeFabricCoordinator::WriteDirectoryEntry_(FabricTableSegmentClasses table_class, size_t begin, size_t end, uint16_t version) noexcept
    // {

    //     if (!CoreOfFabricCoordinator::IsValidFabricTable(table_class))
    //     {
    //         return false;
    //     }

    //     // Is  begin < APCDataStructure::METACELL_COUNT-> This check valid ??
    //     if (!SlabBasePtr_ || begin > end || end > SlabCellCount_ || begin < APCDataStructure::METACELL_COUNT)
    //     {
    //         return false;
    //     }
        
    //     const size_t width = CoreOfFabricCoordinator::GetWidthOfValidFabricTable(table_class);
    //     if (width == UNSIGNED_ZERO)
    //     {
    //         return false;
    //     }
        
    //     const size_t base = GetTableDirectoryCellSlabIndex_(table_class, TableEntryCellTypeOfFabric::BEGIN48, PackedCellLocalityTypes::CLAIMED);

    //     if (base + TABLE_ENTRY_WIDTH_OF_FABRIC > SlabCellCount_)
    //     {
    //         return false;
    //     }
        
        
    //     MakeAndStoreDirectlyAFabricOwnedCell_(base + 0u, static_cast<uint64_t>(begin), table_id, PackedMode::MODE_32_ATOMIC_GUARANTEED, version, 
    //         UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType, PackedCellLocalityTypes::PUBLISHED,
    //         CellMapAndPriority::VERSION_AND_CLAIMED_CAS_DEPENDENT
    //     );

    //     MakeAndStoreDirectlyAFabricOwnedCell_(base + 1u, static_cast<uint64_t>(end), table_id, PackedMode::MODE_32_ATOMIC_GUARANTEED, version,
    //         UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType, PackedCellLocalityTypes::PUBLISHED,
    //         CellMapAndPriority::VERSION_AND_CLAIMED_CAS_DEPENDENT
    //     );

    //     MakeAndStoreDirectlyAFabricOwnedCell_(base + 2u, static_cast<uint64_t>(directory_width), table_id, PackedMode::MODE_32_ATOMIC_GUARANTEED, version,
    //         UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType, PackedCellLocalityTypes::PUBLISHED,
    //         CellMapAndPriority::VERSION_AND_CLAIMED_CAS_DEPENDENT
    //     );
    // }



    // void NeuromorphicSpaceTimeFabricCoordinator::InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept
    // {
    //     CacheEntryOfFabricTable desired_cache_entry;
    //     const bool ok = GetFabricTableCache(table_class, desired_cache_entry);
    //     if (!ok)
    //     {
    //         return;
    //     }
    //     for (size_t i = desired_cache_entry.BeginIdx; i < desired_cache_entry.EndIdx; i++)
    //     {
    //         MakeAndStoreDirectlyAFabricOwnedCell_(
    //             i + 0u, UNSIGNED_ZERO, table_class, PackedMode::MODE_32_ATOMIC_GUARANTEED, 
    //             desired_cache_entry.VersionCount, UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType,
    //             PackedCellLocalityTypes::IDLE, CellMapAndPriority::VERSION_AND_CLAIMED_CAS_DEPENDENT
    //         );
    //         MakeAndStoreDirectlyAFabricOwnedCell_(
    //             i + 1u, IN_CELL_VALUE_MODE32_SENTINAL, table_class, PackedMode::MODE_32_ATOMIC_GUARANTEED, 
    //             desired_cache_entry.VersionCount, UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType,
    //             PackedCellLocalityTypes::IDLE, CellMapAndPriority::VERSION_AND_CLAIMED_CAS_DEPENDENT
    //         );
    //     }
        
    // }

    // size_t NeuromorphicSpaceTimeFabricCoordinator::GetSlotCellTypeIdxInFabric_(uint32_t slot, SlotCellTypeOfAPCFabric slot_type) noexcept
    // {
    //     CacheEntryOfFabricTable slot_directory_cache_entry;
    //     bool ok = GetFabricTableCache(FabricTableSegmentClasses::SLOT_DIRECTORY, slot_directory_cache_entry);
    //     if (!ok)
    //     {
    //         return APCDataStructure::APC_SIZE_SENTINAL;
    //     }
    //     return slot_directory_cache_entry.BeginIdx + 
    //         static_cast<size_t>(slot) * SLOT_RECORD_WIDTH_OF_FABRIC +
    //         static_cast<size_t>(slot_type);
    // }

    void NeuromorphicSpaceTimeFabricCoordinator::MakeAndStoreASlotDirectoryCell_(
        uint32_t slot, 
        SlotCellTypeOfAPCFabric slote_state,
        uint32_t value32, 
        clk16_t extended_meta_value,
        PackedCellLocalityTypes locality_of_cell
    ) noexcept
    {
        const size_t slot_idx = GetSlotCellTypeIdxInFabric_(slot, slote_state);
        if (slot_idx == APCDataStructure::PACKED_CELL_SENTENAL)
        {
            return;
        }

        MakeAndStoreDirectlyAFabricOwnedCell_(
            slot_idx, value32,
            FabricTableSegmentClasses::SLOT_DIRECTORY,
            PackedMode::MODE_32_ATOMIC_GUARANTEED, extended_meta_value, static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType::UnsignedPCellDataType, locality_of_cell,
            CellMapAndPriority::VERSION_AND_CLAIMED_CAS_DEPENDENT
        );
    }

    // void NeuromorphicSpaceTimeFabricCoordinator::InitializeSlotDirectory_() noexcept
    // {
    //     for (uint32_t slot = 0; slot < SlotCount_; slot++)
    //     {
    //         const uint8_t generation = APCDataStructure::BRANCH_VERSION;
    //         const packed64_t next_handle = (slot + 1u < SlotCount_) ? CoreOfFabricCoordinator::MakeANEncodedHandlerCellForFabric(
    //             slot + 1u, generation, SlabId_, HandleStateOfAPCFabric::APC_SEGMENT, 
    //             FabricTableSegmentClasses::SLOT_DIRECTORY
    //         ) : PackedCell64_t::PACKED_CELL_SENTINAL;


    //         MakeAndStoreASlotDirectoryCell_(slot, SlotCellTypeOfAPCFabric::STATE, static_cast<uint64_t>(PackedCellLocalityTypes::IDLE), generation);
    //         MakeAndStoreASlotDirectoryCell_(slot, SlotCellTypeOfAPCFabric::OWNER_BRANCH, UNSIGNED_ZERO, generation);
    //         MakeAndStoreASlotDirectoryCell_(slot, SlotCellTypeOfAPCFabric::GENERATION, generation, generation);
    //         MakeAndStoreASlotDirectoryCell_(slot, SlotCellTypeOfAPCFabric::STATE, static_cast<uint64_t>(PackedCellLocalityTypes::IDLE), generation);
    //         MakeAndStoreASlotDirectoryCell_(slot, SlotCellTypeOfAPCFabric::STATE, static_cast<uint64_t>(PackedCellLocalityTypes::IDLE), generation);
    //         MakeAndStoreASlotDirectoryCell_(slot, SlotCellTypeOfAPCFabric::STATE, static_cast<uint64_t>(PackedCellLocalityTypes::IDLE), generation);

    //     }
        
    // }


}
