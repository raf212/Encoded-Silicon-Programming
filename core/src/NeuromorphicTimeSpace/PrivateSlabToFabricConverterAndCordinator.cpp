#pragma once

#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{

    packed64_t* SlabToFabricConverterAndCordinator::AllocatePackedCellRaw_(size_t count_of_cells) noexcept
    {
        auto allocation_function = AllocatorOfFabric_.AllocatePackedCellStorage ? 
            AllocatorOfFabric_.AllocatePackedCellStorage : &RawPackedCellAllocator::DefaultAllocateAtomicCells;
        
        size_t alignment = AllocatorOfFabric_.Alignment ? AllocatorOfFabric_.Alignment : BIT_LENGTH_OF_A_PACKED_CELL;
        alignment = std::max<size_t>(alignment, alignof(packed64_t));
        alignment = std::max<size_t>(alignment, BIT_LENGTH_OF_A_PACKED_CELL);

        return allocation_function(count_of_cells, alignment, AllocatorOfFabric_.User);

    }


    void SlabToFabricConverterAndCordinator::FreeRawPackedCells_(packed64_t* packed_cell_memory_ptr, size_t packed_cell_count) noexcept
    {
        RawPackedCellAllocator::FreeFunction free_function = AllocatorOfFabric_.FreePackedCellStorage ?
                            AllocatorOfFabric_.FreePackedCellStorage : &RawPackedCellAllocator::DefaultFreeAtomicCells;
        size_t alignment = AllocatorOfFabric_.Alignment ? AllocatorOfFabric_.Alignment : BIT_LENGTH_OF_A_PACKED_CELL;
        alignment = std::max<size_t>(alignment, alignof(packed64_t));
        alignment = std::max<size_t>(alignment, BIT_LENGTH_OF_A_PACKED_CELL);

        free_function(packed_cell_memory_ptr, packed_cell_count, alignment, AllocatorOfFabric_.User);
    }

    void SlabToFabricConverterAndCordinator::ResetScalarsofTheFabric_() noexcept
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


    constexpr void SlabToFabricConverterAndCordinator::MakeAndStoreFabricMetaValue48(
        FabricMetaIndicies fabric_meta_idx, 
        uint64_t value, 
        LocalityPolicy cell_locality,
        AccessContractOfValue access_contract,
        PriorityPolicy priority
    )noexcept
    {
        const size_t slab_index = static_cast<size_t>(fabric_meta_idx);
        if (slab_index >= APCDataStructure::METACELL_COUNT)
        {
            return;
        }

        const packed64_t desired_packed_cell = PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48, access_contract,
            FabricTableSegmentClasses::GENERIC_CONTROL,
            cell_locality,
            InternalDataTypePolicy ::UnsignedPCellDataType,
            priority,
            value
        );

        StorePackedCellUncheckedDirectly(slab_index, desired_packed_cell);
        
    }

    //Integrate AtomicAdaptiveBackoff
    // add CAS_FAILURE_COUNT
    constexpr bool SlabToFabricConverterAndCordinator::UpdateValidPairedOccupancyApproxAtomically_(
        LocalityPolicy desired_occupancy_of_locality, uint64_t desired_occupancy_value,
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
            LocalityPolicy::PUBLISHED,
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
    
        if (low32_half_view.LocalityOfCell == LocalityPolicy::CLAIMED || high32_half_view.LocalityOfCell == LocalityPolicy::CLAIMED)
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
                    if (CompareExchangeStrongFromFabric(low_idx, expected, desired))
                    {
                        AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
                        return true;
                    }

                    if (PackedCell64_t::ExtractLocalityPolicy(expected) == LocalityPolicy::CLAIMED)
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
                const packed64_t desired_claimed_low = PackedCell64_t::SetLocalityInPacked(low32_and_probable_high32.first, LocalityPolicy::CLAIMED);

                auto RestoreLow = [&]()
                {
                    AtomicallyStorePackedCellUnchecked(low_idx, low32_half_view.RawCell);
                    return false;
                };

                for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
                {
                    if (CompareExchangeStrongFromFabric(low_idx, expected_low, desired_claimed_low))
                    {
                        packed64_t expected_high = high32_half_view.RawCell;

                        for (size_t j = 0; j < DEFAULT_MAX_TRIES; j++)
                        {
                            if (CompareExchangeStrongFromFabric(high_idx, expected_high, low32_and_probable_high32.second))
                            {
                                AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
                                return true;
                            }

                            if (PackedCell64_t::ExtractLocalityPolicy(expected_high) == LocalityPolicy::CLAIMED)
                            {
                                return RestoreLow();
                            }
                        }

                        return RestoreLow();
                    }

                    if (PackedCell64_t::ExtractLocalityPolicy(expected_low) == LocalityPolicy::CLAIMED)
                    {
                        return false;
                    }
                }
            }

            return false;
        }

        return ForceUpdate(); 
    }

    constexpr void SlabToFabricConverterAndCordinator::ResetAll4TypesOfOccupancyMetaData_() noexcept
    {
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::IDLE, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::PUBLISHED, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::CLAIMED, UNSIGNED_ZERO, true);
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::FAULTY, UNSIGNED_ZERO, true);
    }


    constexpr void SlabToFabricConverterAndCordinator::WriteFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept
    {
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::MAGIC, APCDataStructure::FABRIC_MAGIC);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::VERSION, APCDataStructure::BRANCH_VERSION);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::FLAGS, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::SLAB_ID, static_cast<uint64_t>(SlabId_));

        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::TOTAL_CELLS, static_cast<uint64_t>(SlabCellCount_));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::CONTROL_CELLS_OF_FABRIC, static_cast<uint64_t>(SegmentPoolBegin_));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::SLOT_COUNT, static_cast<uint64_t>(SlotCount_));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::SLOT_CELL_COUNT, static_cast<uint64_t>(SlotCellCount_));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::SEGMENT_POOL_BEGIN_IDX, static_cast<uint64_t>(SegmentPoolBegin_));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::SEGMENT_POOL_END_IDX, static_cast<uint64_t>(SegmentPoolEnd_));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::FREE_SLOT_HEAD, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RETIRE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RELATION_FREE_HEAD, UNSIGNED_ZERO);
        
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::GLOBAL_EPOCH48, 1u);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::MIN_SAFE_EPOCH48, UNSIGNED_ZERO);

        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::NEXT_BRANCH_ID, APCDataStructure::BRANCH_VERSION);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::NEXT_RELATION_ID, APCDataStructure::BRANCH_VERSION);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::NEXT_DEVICE_VIEW_ID, APCDataStructure::BRANCH_VERSION);
        
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::FABRIC_CLOCK16, 1u);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::WORK_WRITE_CURSOR, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::WORK_READ_CURSOR, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::READY_WRITE_CURSOR, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::READY_READ_CURSOR, UNSIGNED_ZERO);

        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RECORD_BOOK_OF_TSC_BEGIN, static_cast<uint64_t>(table_directory_begin));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RECORD_BOOK_OF_TSC_END, static_cast<uint32_t>(table_directory_end));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::TABLE_DIRECTORY_COUNT, static_cast<uint64_t>(FabricTableSegmentClasses::COUNT));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::TABLE_DIRECTORY_VERSION, APCDataStructure::BRANCH_VERSION);

        ResetAll4TypesOfOccupancyMetaData_();

        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::CAS_FAILURE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::ERROR_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RETIRE_SLOT_HEAD, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::LIVE_SLOT_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::HASH_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::HASH_COMPACTION_COUNT, UNSIGNED_ZERO);

        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::WORK_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::READY_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::BACKOFF_SPIN_LIMIT, 16u);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::BACKOFF_YIELD_LIMIT, 64u);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::INITIALIZATION_STATE, static_cast<uint64_t>(LocalityPolicy::PUBLISHED));
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::HAS_COMPACTION_INFLIGHT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RELATION_RECLAIM_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::WORK_QUEUE_DROPPED_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::THREAD_TABLE_CAPACITY, ThreadTableCapacity_);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::THREAD_ACTIVE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::THREAD_REGISTRATION_FAILURE, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RELATION_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::RELATION_UNLINK_FAILURES, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::WORK_QUEUE_CLAIM_FAILURES, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48(FabricMetaIndicies::EOF_FABRIC_HEADER, APCDataStructure::FABRIC_META_EOF);
    }


    constexpr size_t SlabToFabricConverterAndCordinator::ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(
        OriginOfRecord table_class
    ) noexcept
    {
        if (!CoreOfFabricCoordinator::IsValidFabricTable(table_class))
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }

        /// ALways same derives from -> FabricMetaIndicies
        const packed64_t directory_begin_cell = ReadCompletePackedCellDirectly(static_cast<size_t>(FabricMetaIndicies::RECORD_BOOK_OF_TSC_BEGIN));

        if (CoreOfFabricCoordinator::IsThisValidRecordBookPackedCell(directory_begin_cell))
        {
            const size_t base_origin_table_idx = static_cast<size_t>(PackedCell64_t::ExtractRaw48FamilyBits(directory_begin_cell));

            return base_origin_table_idx + (static_cast<size_t>(table_class) * RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC);        
        }
        
        return APCDataStructure::APC_SIZE_SENTINAL;

    }

    constexpr bool SlabToFabricConverterAndCordinator::WriteARecordBookOfTSCEntry_(
        FabricTableSegmentClasses table_class, 
        size_t begin, size_t end, 
        uint8_t slab_id
    ) noexcept
    {
        if (!CoreOfFabricCoordinator::IsValidFabricTable(table_class))
        {
            return false;
        }

        if (!SlabBasePtr_)
        {
            return false;
        }

        const size_t base_idx = ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(table_class);
        if (base_idx == APCDataStructure::APC_SIZE_SENTINAL || (base_idx + RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC > SlabCellCount_))
        {
            return false;
        }

        const packed64_t begin48_cell = FabricCellConf::MakeRecordBookCellOfTSC(
            static_cast<uint64_t>(begin)
        );

        const packed64_t end48_cell =  FabricCellConf::MakeRecordBookCellOfTSC(
            static_cast<uint64_t>(end)
        );

        const packed64_t safty_lock_for_this = FabricCellConf::MakeRecordBookSaftyLock(
            begin, end, table_class, 
            LocalityPolicy::PUBLISHED, slab_id
        );

        const FTSC_SlabRangeTripletFrom_RecordBookOfFTSC desired_record_triplet {begin48_cell, end48_cell, safty_lock_for_this};

        std::optional<uint64_t> full_width_validated = CoreOfFabricCoordinator::ValidateAFabricTableRangeStruct(desired_record_triplet, table_class);
        if (full_width_validated.has_value() && full_width_validated.value() > UNSIGNED_ZERO)
        {
            AtomicallyStorePackedCellUnchecked(
                base_idx + static_cast<size_t>(RecordBookInternalIndexing::BEGIN48), 
                begin48_cell
            );
            
            AtomicallyStorePackedCellUnchecked(
                base_idx + static_cast<size_t>(RecordBookInternalIndexing::END48), 
                end48_cell
            );                
            
            AtomicallyStorePackedCellUnchecked(
                base_idx + static_cast<size_t>(RecordBookInternalIndexing::META32), 
                safty_lock_for_this
            );

            return true;
        }
        
        return false;
        
    }


    std::optional<FTSC_SlabRangeTripletFrom_RecordBookOfFTSC> SlabToFabricConverterAndCordinator::GetValidSlabRangeTripletFromRecordBookOfFTSC(FabricTableSegmentClasses table_class) noexcept
    {
        if (!CoreOfFabricCoordinator::IsValidFabricTable(table_class))
        {
            return std::nullopt;
        }

        const size_t begin_of_desired_table = ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(table_class) + static_cast<size_t>(RecordBookInternalIndexing::BEGIN48);
        const size_t end_idx = begin_of_desired_table + static_cast<size_t>(RecordBookInternalIndexing::END48);
        const size_t safty_lock_meta_cell = begin_of_desired_table + static_cast<size_t>(RecordBookInternalIndexing::META32);
        if (end_idx >= SlabCellCount_ || begin_of_desired_table < APCDataStructure::METACELL_COUNT)
        {
            return std::nullopt;
        }

        const FTSC_SlabRangeTripletFrom_RecordBookOfFTSC triplet{ 
            ReadCompletePackedCellDirectly(begin_of_desired_table),
            ReadCompletePackedCellDirectly(end_idx),
            ReadCompletePackedCellDirectly(safty_lock_meta_cell)
        };

        std::optional<uint64_t> full_width_validated = CoreOfFabricCoordinator::ValidateAFabricTableRangeStruct(triplet, table_class);
        if (full_width_validated.has_value() && full_width_validated.value() > UNSIGNED_ZERO)
        {
            return triplet;
        }

        return std::nullopt;
    }
        

    void SlabToFabricConverterAndCordinator::IdleAFabricTableClassRangesMemory_(FabricTableSegmentClasses table_class) noexcept
    {
        const std::optional<FTSC_SlabRangeTripletFrom_RecordBookOfFTSC> maybe_table_range_triplet = GetValidSlabRangeTripletFromRecordBookOfFTSC(table_class);
        if (!maybe_table_range_triplet.has_value())
        {
            return;
        }
        const FTSC_SlabRangeTripletFrom_RecordBookOfFTSC table_range_triplet = *maybe_table_range_triplet;
        const std::optional<packed64_t> maybe_width_masked = CoreOfFabricCoordinator::ValidateAFabricTableRangeStruct(table_range_triplet, table_class);
        if (!maybe_width_masked.has_value() && maybe_width_masked.value() <= UNSIGNED_ZERO)
        {
            return;
        }
        

        const packed64_t idle_table_cell = PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48, AccessContractOfValue::ATOMIC_SLNAPSHOT,
            table_class, LocalityPolicy::IDLE,
            InternalDataTypePolicy::UnsignedPCellDataType
        );

        for (size_t idx = PackedCell64_t::ExtractRaw48FamilyBits(table_range_triplet.BeginIdxRawType48Cell); idx < PackedCell64_t::ExtractRaw48FamilyBits(table_range_triplet.EndIdxRawType48Cell); idx++)
        {
            StorePackedCellUncheckedDirectly(idx, idle_table_cell);
        }
        
    }

    void SlabToFabricConverterAndCordinator::InitializeHashTable_(FabricTableSegmentClasses hash_table) noexcept
    {

        const packed64_t desired_idle_hash_key_value_cell = FabricCellConf::MakeHashKeyOrValueCell(UNSIGNED_ZERO, hash_table, LocalityPolicy::IDLE);
        if (desired_idle_hash_key_value_cell == PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            return;
        }

        std::optional<FTSC_SlabRangeTripletFrom_RecordBookOfFTSC> desired_hash_table_triplet = GetValidSlabRangeTripletFromRecordBookOfFTSC(hash_table);

        if (!desired_hash_table_triplet.has_value())
        {
            return;
        }

        const size_t begin_idx_of_the_hash_table = PackedCell64_t::ExtractRaw48FamilyBits(desired_hash_table_triplet.value().BeginIdxRawType48Cell);
        const size_t end_idx_of_the_hash_table = PackedCell64_t::ExtractRaw48FamilyBits(desired_hash_table_triplet.value().EndIdxRawType48Cell);

        const packed64_t idle_key_value = FabricCellConf::MakeHashKeyOrValueCell(UNSIGNED_ZERO, hash_table, LocalityPolicy::IDLE);

        const packed64_t prob_distance_lock_cell_idle = FabricCellConf::MakeHashProbDistanceCellWithSaftyLock(
            UNSIGNED_ZERO, UNSIGNED_ZERO, UNSIGNED_ZERO,
            hash_table
        );

        if (
            idle_key_value == PackedCell64_t::PACKED_CELL_SENTINAL ||
            prob_distance_lock_cell_idle == PackedCell64_t::PACKED_CELL_SENTINAL 
        )
        {
            return;
        }

        for (size_t idx = begin_idx_of_the_hash_table; idx < end_idx_of_the_hash_table; idx += HASH_BUCKED_WIDTH_OF_FABRIC)
        {
            StorePackedCellUncheckedDirectly(idx + static_cast<size_t>(HashTableInternalIndexing::KEY_INDEX), idle_key_value);
            StorePackedCellUncheckedDirectly(idx + static_cast<size_t>(HashTableInternalIndexing::VALUE_INDEX), idle_key_value);
            StorePackedCellUncheckedDirectly(idx + static_cast<size_t>(HashTableInternalIndexing::PROB_DISTANCE_LOCK), prob_distance_lock_cell_idle);
        }
    }

    // size_t SlabToFabricConverterAndCordinator::GetSlotCellTypeIdxInFabric_(uint32_t slot, APCDescriptotCellType slot_type) noexcept
    // {
    //     CacheEntryOfFabricTable slot_directory_cache_entry;
    //     bool ok = GetFabricTableCache(FabricTableSegmentClasses::APC_DESCRIPTOR, slot_directory_cache_entry);
    //     if (!ok)
    //     {
    //         return APCDataStructure::APC_SIZE_SENTINAL;
    //     }
    //     return slot_directory_cache_entry.BeginIdx + 
    //         static_cast<size_t>(slot) * APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC +
    //         static_cast<size_t>(slot_type);
    // }

    // void SlabToFabricConverterAndCordinator::MakeAndStoreASlotDirectoryCell_(
    //     uint32_t slot, 
    //     APCDescriptotCellType slote_state,
    //     uint32_t value32, 
    //     clk16_t extended_meta_value,
    //     LocalityPolicy locality_of_cell
    // ) noexcept
    // {
    //     const size_t slot_idx = GetSlotCellTypeIdxInFabric_(slot, slote_state);
    //     if (slot_idx == APCDataStructure::PACKED_CELL_SENTENAL)
    //     {
    //         return;
    //     }

    //     MakeAndStoreDirectlyAFabricOwnedCell_(
    //         slot_idx, value32,
    //         FabricTableSegmentClasses::APC_DESCRIPTOR,
    //         ModelFamily::MODEL32, extended_meta_value, static_cast<tag8_t>(Model32Subclass::SELF_CLASS),
    //         InternalDataTypePolicy::UnsignedPCellDataType, locality_of_cell,
    //         PriorityPolicy::INFLUENCED
    //     );
    // }

    // void SlabToFabricConverterAndCordinator::InitializeSlotDirectory_() noexcept
    // {
        
    // }


}
