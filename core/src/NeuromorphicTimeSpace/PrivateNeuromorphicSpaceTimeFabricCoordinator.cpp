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

        AtomicallyStorePackedCellUnchecked(idx, packed_cell);

        return true;
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::MakeCheckAndStoreAFabricControlValidCell_(
        FabricMetaIndicies fabric_meta_idx, uint64_t value32_or_64, 
        FabricTableSegmentClasses fabric_segment_class,
        PackedMode cell_mode, clk16_t extended_meta_value, 
        tag8_t mode_sub_class, 
        PackedCellDataType cell_data_type, PackedCellLocalityTypes locality_of_cell, 
        PriorityPhysics priority
    ) noexcept
    {
        const size_t true_index = static_cast<size_t>(fabric_meta_idx);
        if (true_index < APCDataStructure::METACELL_COUNT)
        {
            return MakeAndStoreAFabricOwnedCell_(true_index, value32_or_64, fabric_segment_class,
                cell_mode, extended_meta_value, mode_sub_class, 
                cell_data_type, locality_of_cell,
                priority
            );           
        }

        return false;
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::MakeAndStoreAFabricOwnedCell_(
        size_t idx, uint64_t value32_or_64, 
        FabricTableSegmentClasses fabric_segment_class,
        PackedMode cell_mode, clk16_t extended_meta_value,
        tag8_t mode_sub_class, PackedCellDataType cell_data_type, 
        PackedCellLocalityTypes locality_of_cell, 
        PriorityPhysics priority
    ) noexcept
    {
        const packed64_t a_valid_fabric_meta_cell32 = PackedCell64_t::MakeInitialFabricValidPackedCell(
            cell_mode, locality_of_cell, 
            fabric_segment_class, cell_data_type, 
            value32_or_64, extended_meta_value,
            priority,(cell_mode == PackedMode::MODE_32) ? static_cast<SubClassesOfMode32>(mode_sub_class) : SubClassesOfMode32::SELF_CLASS,
            (cell_mode == PackedMode::MODE_48) ? static_cast<SubClassesOfMode48>(mode_sub_class) : SubClassesOfMode48::SELF_CLASS
        );

        if (a_valid_fabric_meta_cell32 == PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            return false;
        }

        if (idx < SlabCellCount_)
        {
            //MakeInitialFabricValidPackedCell::already checks validity
            AtomicallyStorePackedCellUnchecked(idx, a_valid_fabric_meta_cell32);
            return true;
        }

        return false;
    }

    void NeuromorphicSpaceTimeFabricCoordinator::StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies fabric_meta_idx, uint64_t value, FabricTableSegmentClasses fabric_segment_class) noexcept
    {
        if (static_cast<size_t>(fabric_meta_idx) >= APCDataStructure::METACELL_COUNT)
        {
            return;
        }
        MakeCheckAndStoreAFabricControlValidCell_(
            fabric_meta_idx, value & MaskLowNBits(CLK_B48), 
            fabric_segment_class, PackedMode::MODE_48
        );
    }

    // //Integrate AtomicAdaptiveBackoff
    // // add CAS_FAILURE_COUNT
    // bool NeuromorphicSpaceTimeFabricCoordinator::UpdateValidPairedOccupancyApproximation_(
    //     PackedCellLocalityTypes desired_occupancy_of_locality, uint64_t desired_occupancy_value,
    //     bool force_update, clk16_t pair_version
    // ) noexcept
    // {
    //     const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(desired_occupancy_of_locality);

    //     if (force_update && desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
    //     {
    //         return false;
    //     }

    //     const size_t low_idx = static_cast<size_t>(desired_occupancy_low_idx);
    //     const size_t high_idx = low_idx + 1;

    //     const std::pair<packed64_t, packed64_t> low32_and_probable_high32 = PairedVersionedCellModelOfMode32::GetPairOfLow32FAndHigh32SFromUnsigned64ForFabric(
    //         desired_occupancy_value, pair_version,
    //         PackedCellLocalityTypes::PUBLISHED,
    //         FabricTableSegmentClasses::GENERIC_CONTROL
    //     );

    //     auto ForceUpdate = [&](){
    //         AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
    //         AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
    //         return true;
    //     };
        
    //     if (force_update)
    //     {
    //         return ForceUpdate();
    //     }

    //     const PackedCell64_t::AuthoritiveCellView low32_half_view{};
    //     const PackedCell64_t::AuthoritiveCellView high32_half_view{};
    //     const std::optional<uint64_t> maybe_occupancy = ReadOccupancyApproxFromPairedIfValid(desired_occupancy_of_locality, &low32_half_view, &high32_half_view);

    //     if (!maybe_occupancy|| *maybe_occupancy == PackedCell64_t::PACKED_CELL_SENTINAL)
    //     {
    //         return ForceUpdate();
    //     }
    
    //     if (low32_half_view.LocalityOfCell == PackedCellLocalityTypes::CLAIMED || high32_half_view.LocalityOfCell == PackedCellLocalityTypes::CLAIMED)
    //     {
    //         return false;
    //     }

    //     if (*maybe_occupancy!= PackedCell64_t::PACKED_CELL_SENTINAL)
    //     {
    //         //just cmpx  low
    //         if (*maybe_occupancy < IN_CELL_VALUE_MODE32_SENTINAL && low32_and_probable_high32.second == PackedCell64_t::PACKED_CELL_SENTINAL)
    //         {
    //             packed64_t expected = low32_half_view.RawCell;
    //             const packed64_t desired = low32_and_probable_high32.first;
    //             for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
    //             {
    //                 if (SlabBasePtr_[low_idx].compare_exchange_strong(
    //                     expected, desired, OnExchangeSuccess, OnExchangeFailure
    //                 ))
    //                 {
    //                     AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
    //                     return true;
    //                 }

    //                 if (PackedCell64_t::ExtractLocalityFromPacked(expected) == PackedCellLocalityTypes::CLAIMED)
    //                 {
    //                     return false;
    //                 }
                    
    //             }
    //             //intehrate failure count and AtomicAdaptiveBackoff
    //             return false;
    //         }

    //         //double cas 
    //         if ((low32_half_view.IsCellValid && high32_half_view.IsCellValid) || desired_occupancy_value >= IN_CELL_VALUE_MODE32_SENTINAL)
    //         {
    //             packed64_t expected_low = low32_half_view.RawCell;
    //             const packed64_t desired_claimed_low = PackedCell64_t::SetLocalityInPacked(low32_and_probable_high32.first, PackedCellLocalityTypes::CLAIMED);

    //             auto RestoreLow = [&]()
    //             {
    //                 AtomicallyStorePackedCellUnchecked(low_idx, low32_half_view.RawCell);
    //                 return false;
    //             };

    //             for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
    //             {
    //                 if (SlabBasePtr_[low_idx].compare_exchange_strong(
    //                     expected_low, desired_claimed_low, OnExchangeSuccess, OnExchangeFailure
    //                 ))
    //                 {
    //                     packed64_t expected_high = high32_half_view.RawCell;

    //                     for (size_t j = 0; j < DEFAULT_MAX_TRIES; j++)
    //                     {
    //                         if (SlabBasePtr_[high_idx].compare_exchange_strong(
    //                             expected_high, low32_and_probable_high32.second, OnExchangeSuccess, OnExchangeFailure
    //                         ))
    //                         {
    //                             AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
    //                             return true;
    //                         }

    //                         if (PackedCell64_t::ExtractLocalityFromPacked(expected_high) == PackedCellLocalityTypes::CLAIMED)
    //                         {
    //                             return RestoreLow();
    //                         }
    //                     }

    //                     return RestoreLow();
    //                 }

    //                 if (PackedCell64_t::ExtractLocalityFromPacked(expected_low) == PackedCellLocalityTypes::CLAIMED)
    //                 {
    //                     return false;
    //                 }
    //             }
    //         }

    //         return false;
    //     }

    //     return ForceUpdate(); 
    // }

    void NeuromorphicSpaceTimeFabricCoordinator::ResetAll4TypesOfOccupancyMetaData() noexcept
    {
        UpdateValidPairedOccupancyApproximation_(PackedCellLocalityTypes::IDLE, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproximation_(PackedCellLocalityTypes::PUBLISHED, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproximation_(PackedCellLocalityTypes::CLAIMED, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproximation_(PackedCellLocalityTypes::FAULTY, UNSIGNED_ZERO, true);
    }


    void NeuromorphicSpaceTimeFabricCoordinator::WriteFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept
    {
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::MAGIC, APCDataStructure::FABRIC_MAGIC);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::VERSION, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::FLAGS, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::SLAB_ID, static_cast<uint64_t>(SlabId_));

        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::TOTAL_CELLS, static_cast<uint64_t>(SlabCellCount_));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::CONTROL_CELLS_OF_FABRIC, static_cast<uint64_t>(SegmentPoolBegin_));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::SLOT_COUNT, static_cast<uint64_t>(SlotCount_));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::SLOT_CELL_COUNT, static_cast<uint64_t>(SlotCellCount_));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::SEGMENT_POOL_BEGIN_IDX, static_cast<uint64_t>(SegmentPoolBegin_));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::SEGMENT_POOL_END_IDX, static_cast<uint64_t>(SegmentPoolEnd_));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::FREE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::RETIRE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::RELATION_FREE_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::GLOBAL_EPOCH48, 1u);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::MIN_SAFE_EPOCH48, UNSIGNED_ZERO);

        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::NEXT_BRANCH_ID, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::NEXT_RELATION_ID, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::NEXT_DEVICE_VIEW_ID, APCDataStructure::BRANCH_VERSION);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::FABRIC_CLOCK16, 1u);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::WORK_WRITE_CURSOR, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::WORK_READ_CURSOR, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::READY_WRITE_CURSOR, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::READY_READ_CURSOR, UNSIGNED_ZERO);

        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_BEGIN, static_cast<uint64_t>(table_directory_begin));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_END, static_cast<uint32_t>(table_directory_end));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_COUNT, static_cast<uint64_t>(FabricTableSegmentClasses::COUNT));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::TABLE_DIRECTORY_VERSION, APCDataStructure::BRANCH_VERSION);

        ResetAll4TypesOfOccupancyMetaData();

        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::CAS_FAILURE_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::ERROR_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::RETIRE_SLOT_HEAD, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::LIVE_SLOT_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::HASH_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::HASH_COMPACTION_COUNT, UNSIGNED_ZERO);

        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::WORK_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::READY_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::BACKOFF_SPIN_LIMIT, 16u);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::BACKOFF_YIELD_LIMIT, 64u);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::INITIALIZATION_STATE, static_cast<uint64_t>(PackedCellLocalityTypes::PUBLISHED));
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::HAS_COMPACTION_INFLIGHT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::RELATION_RECLAIM_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::WORK_QUEUE_DROPPED_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::THREAD_TABLE_CAPACITY, ThreadTableCapacity_);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::THREAD_ACTIVE_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::THREAD_REGISTRATION_FAILURE, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::RELATION_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::RELATION_UNLINK_FAILURES, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::WORK_QUEUE_CLAIM_FAILURES, UNSIGNED_ZERO);
        StoreNewDefaultFebricControlMeta48_(FabricMetaIndicies::EOF_FABRIC_HEADER, APCDataStructure::FABRIC_META_EOF);
    }


    // size_t NeuromorphicSpaceTimeFabricCoordinator::GetTableDirectoryBeginIdx_(FabricTableSegmentClasses desired_table, uint8_t part) noexcept
    // {
    //     const size_t  base_of_fabric_table = static_cast<size_t>(PackedCell64_t::ExtractValue32(
    //         AtomicallyLoadReadPackedCell(static_cast<size_t>(FabricMetaIndicies::TABLE_DIRECTORY_BEGIN))
    //     ));
    //     return base_of_fabric_table + static_cast<size_t>(desired_table) * TABLE_ENTRY_WIDTH_OF_FABRIC + static_cast<size_t>(part);
    // }

    void NeuromorphicSpaceTimeFabricCoordinator::WriteDirectoryEntry_(FabricTableSegmentClasses table_id, size_t begin, size_t end, uint16_t version) noexcept
    {
        const size_t table_index = static_cast<size_t>(table_id);
        const uint32_t directory_width = CoreOfFabricCoordinator::GetWidthOfValidFebricTable(table_id);
        if (table_index < TableCache_.size())
        {
            TableCache_[table_index] = CacheEntryOfFabricTable{begin, end, directory_width, version, true};
        }
        const size_t base = GetTableDirectoryBeginIdx_(table_id);
        
        MakeAndStoreAFabricOwnedCell_(base + 0u, static_cast<uint64_t>(begin), table_id, PackedMode::MODE_32, version, 
            UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType, PackedCellLocalityTypes::PUBLISHED,
            PriorityPhysics::VERSION_DEPENDENCY
        );

        MakeAndStoreAFabricOwnedCell_(base + 1u, static_cast<uint64_t>(end), table_id, PackedMode::MODE_32, version,
            UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType, PackedCellLocalityTypes::PUBLISHED,
            PriorityPhysics::VERSION_DEPENDENCY
        );

        MakeAndStoreAFabricOwnedCell_(base + 2u, static_cast<uint64_t>(directory_width), table_id, PackedMode::MODE_32, version,
            UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType, PackedCellLocalityTypes::PUBLISHED,
            PriorityPhysics::VERSION_DEPENDENCY
        );
    }

    bool NeuromorphicSpaceTimeFabricCoordinator::GetFabricTableCache(FabricTableSegmentClasses fabric_class, CacheEntryOfFabricTable& cache_entry) noexcept
    {
        if (fabric_class > FabricTableSegmentClasses::NONE && fabric_class < FabricTableSegmentClasses::GENERIC_CONTROL)
        {
            cache_entry = TableCache_[static_cast<size_t>(fabric_class)];
            return IsThisAValidFabricTableCacheEntry(cache_entry);
        }
        return false;
    }


    void NeuromorphicSpaceTimeFabricCoordinator::InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept
    {
        CacheEntryOfFabricTable desired_cache_entry;
        const bool ok = GetFabricTableCache(table_class, desired_cache_entry);
        if (!ok)
        {
            return;
        }
        for (size_t i = desired_cache_entry.BeginIdx; i < desired_cache_entry.EndIdx; i++)
        {
            MakeAndStoreAFabricOwnedCell_(
                i + 0u, UNSIGNED_ZERO, table_class, PackedMode::MODE_32, 
                desired_cache_entry.VersionCount, UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType,
                PackedCellLocalityTypes::IDLE, PriorityPhysics::VERSION_DEPENDENCY
            );
            MakeAndStoreAFabricOwnedCell_(
                i + 1u, IN_CELL_VALUE_MODE32_SENTINAL, table_class, PackedMode::MODE_32, 
                desired_cache_entry.VersionCount, UNSIGNED_ZERO, PackedCellDataType::UnsignedPCellDataType,
                PackedCellLocalityTypes::IDLE, PriorityPhysics::VERSION_DEPENDENCY
            );
        }
        
    }

    size_t NeuromorphicSpaceTimeFabricCoordinator::GetSlotCellTypeIdxInFabric_(uint32_t slot, SlotCellTypeOfAPCFabric slot_type) noexcept
    {
        CacheEntryOfFabricTable slot_directory_cache_entry;
        bool ok = GetFabricTableCache(FabricTableSegmentClasses::SLOT_DIRECTORY, slot_directory_cache_entry);
        if (!ok)
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }
        return slot_directory_cache_entry.BeginIdx + 
            static_cast<size_t>(slot) * SLOT_RECORD_WIDTH_OF_FABRIC +
            static_cast<size_t>(slot_type);
    }

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

        MakeAndStoreAFabricOwnedCell_(
            slot_idx, value32,
            FabricTableSegmentClasses::SLOT_DIRECTORY,
            PackedMode::MODE_32, extended_meta_value, static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType::UnsignedPCellDataType, locality_of_cell,
            PriorityPhysics::VERSION_DEPENDENCY
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


    // bool NeuromorphicSpaceTimeFabricCoordinator::DefaultCompareExchangeStrongUncheckedCell_(
    //     size_t idx,
    //     const std::pair<packed64_t, packed64_t> expectedF_desiresS,
    //     bool is_claimed_invalid
    // ) noexcept
    // {
    //     if (SlabBasePtr_ && idx >= SlabCellCount_)
    //     {
    //         return false;
    //     }

    //     packed64_t expected_cell = expectedF_desiresS.first;
    //     for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
    //     {
    //         if (SlabBasePtr_[idx].compare_exchange_strong(expected_cell, expectedF_desiresS.second, OnExchangeSuccess, OnExchangeFailure))
    //         {
    //             return true;
    //         }
            
    //         if (is_claimed_invalid && PackedCell64_t::ExtractLocalityFromPacked(expected_cell) == PackedCellLocalityTypes::CLAIMED)
    //         {
    //             return false;
    //         }

    //         //JUST AS A SLOT HOLDER AtomicAdaptiveBackoff has to be build for packed cell
    //         AdaptiveBackoffCentral_.AdaptiveBackOffPacked(expected_cell);
    //     }
        
    //     return false;
    // }

}
