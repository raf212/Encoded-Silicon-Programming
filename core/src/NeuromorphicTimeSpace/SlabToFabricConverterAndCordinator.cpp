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
        PerAPCRuntimeCellCount_ = UNSIGNED_ZERO;
        CountOfAPC_ = UNSIGNED_ZERO;
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



    constexpr void SlabToFabricConverterAndCordinator::Zero4LocalityBasedOccupancyOfFabric_() noexcept
    {
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::IDLE, UNSIGNED_ZERO, true, APCDataStructure::BRANCH_VERSION);
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::PUBLISHED, UNSIGNED_ZERO, true, APCDataStructure::BRANCH_VERSION);
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::CLAIMED, UNSIGNED_ZERO, true, APCDataStructure::BRANCH_VERSION);
        UpdateValidPairedOccupancyApproxAtomically_(LocalityPolicy::FAULTY, UNSIGNED_ZERO, true, APCDataStructure::BRANCH_VERSION);
    }


    constexpr void SlabToFabricConverterAndCordinator::InitializeCompleateFabricMetaIndices_(size_t table_directory_begin, size_t table_directory_end) noexcept
    {
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::MAGIC, APCDataStructure::FABRIC_MAGIC);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::VERSION, APCDataStructure::BRANCH_VERSION);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::FLAGS, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::SLAB_ID, static_cast<uint64_t>(SlabId_));

        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::TOTAL_CELLS, static_cast<uint64_t>(SlabCellCount_));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::CONTROL_CELLS_OF_FABRIC, static_cast<uint64_t>(SegmentPoolBegin_));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::APC_DESCRIPTION_COUNT, static_cast<uint64_t>(CountOfAPC_));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::PER_APC_RUNTIME_CELL_COUNT, static_cast<uint64_t>(PerAPCRuntimeCellCount_));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::SEGMENT_POOL_BEGIN_IDX, static_cast<uint64_t>(SegmentPoolBegin_));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::SEGMENT_POOL_END_IDX, static_cast<uint64_t>(SegmentPoolEnd_));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::TOTAL_APC_IN_USE , UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RETIRE_SLOT_HEAD, PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RELATION_FREE_HEAD, UNSIGNED_ZERO);
        
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::GLOBAL_EPOCH48, 1u);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::MIN_SAFE_EPOCH48, UNSIGNED_ZERO);

        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::NEXT_BRANCH_ID, APCDataStructure::BRANCH_VERSION);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::NEXT_RELATION_ID, APCDataStructure::BRANCH_VERSION);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::NEXT_DEVICE_VIEW_ID, APCDataStructure::BRANCH_VERSION);
        
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::FABRIC_CLOCK16, 1u);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::WORK_WRITE_CURSOR, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::WORK_READ_CURSOR, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::READY_WRITE_CURSOR, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::READY_READ_CURSOR, UNSIGNED_ZERO);

        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RECORD_BOOK_OF_TSC_BEGIN, static_cast<uint64_t>(table_directory_begin));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RECORD_BOOK_OF_TSC_END, static_cast<uint32_t>(table_directory_end));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::TABLE_DIRECTORY_COUNT, static_cast<uint64_t>(FabricTableSegmentClasses::NULLNAN));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::TABLE_DIRECTORY_VERSION, APCDataStructure::BRANCH_VERSION);

        Zero4LocalityBasedOccupancyOfFabric_();

        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::CAS_FAILURE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::ERROR_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RETIRE_SLOT_HEAD, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::LIVE_SLOT_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::HASH_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::HASH_COMPACTION_COUNT, UNSIGNED_ZERO);

        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::WORK_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::READY_QUEUE_OCCUPANCY, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::BACKOFF_SPIN_LIMIT, 16u);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::BACKOFF_YIELD_LIMIT, 64u);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::INITIALIZATION_STATE, static_cast<uint64_t>(LocalityPolicy::PUBLISHED));
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::HAS_COMPACTION_INFLIGHT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RELATION_RECLAIM_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::WORK_QUEUE_DROPPED_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::THREAD_TABLE_CAPACITY, ThreadTableCapacity_);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::THREAD_ACTIVE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::THREAD_REGISTRATION_FAILURE, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RELATION_TOMBSTONE_COUNT, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::RELATION_UNLINK_FAILURES, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::WORK_QUEUE_CLAIM_FAILURES, UNSIGNED_ZERO);
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::EOF_FABRIC_HEADER, APCDataStructure::FABRIC_META_EOF);
    }





    void SlabToFabricConverterAndCordinator::InitializeHashTable_(FabricTableSegmentClasses hash_table) noexcept
    {
        if (!CoreOfFabricCoordinator::IsValidHashTable(hash_table))
        {
            return;
        }

        RecordBookTablesBoundsCarrier return_bounds{};
        bool bounds_ok = GetValidSlabRangeTripletFromRecordBookOfFTSC(hash_table, return_bounds);
        if (!bounds_ok)
        {
            return;
        }

        
        const packed64_t idle_key_value = HashTableConf::MakeHashKeyOrValueCell(UNSIGNED_ZERO, hash_table, LocalityPolicy::IDLE);
        const packed64_t prob_distance_lock_cell_idle = HashTableConf::MakeHashProbDistanceCellWithSaftyLock(
            UNSIGNED_ZERO, UNSIGNED_ZERO, UNSIGNED_ZERO,
            hash_table, 
            LocalityPolicy::IDLE
        );

        if (
            idle_key_value == PackedCell64_t::PACKED_CELL_SENTINAL ||
            prob_distance_lock_cell_idle == PackedCell64_t::PACKED_CELL_SENTINAL 
        )
        {
            return;
        }

        for (
            size_t idx = return_bounds.BeginIndex; 
            idx + HASH_BUCKED_WIDTH_OF_FABRIC <= static_cast<size_t>(return_bounds.EndIndex);
            idx += HASH_BUCKED_WIDTH_OF_FABRIC
        )
        {
            StorePackedCellUncheckedDirectly(idx + static_cast<size_t>(HashTableInternalIndexing::KEY_INDEX), idle_key_value);
            StorePackedCellUncheckedDirectly(idx + static_cast<size_t>(HashTableInternalIndexing::VALUE_INDEX), idle_key_value);
            StorePackedCellUncheckedDirectly(idx + static_cast<size_t>(HashTableInternalIndexing::PROB_DISTANCE_LOCK), prob_distance_lock_cell_idle);
        }
    }



    void SlabToFabricConverterAndCordinator::InitializeAPCDescriptorTable_() noexcept
    {
        const uint8_t version = APCDataStructure::BRANCH_VERSION;
        const packed64_t idle_apc_cell = PackedCell64_t::MakeTypedAPCValidPackedCell(
            TypeFamily::VALUE32,
            AccessContractOfValue::CLAIMED_GURDED,
            APCPagedNodeSegmentClasses::UNDEFINED,
            LocalityPolicy::IDLE,
            InternalDataTypePolicy::UnsignedPCellDataType,
            AttributePolicy::SELF_CONTAINED_DATA_OR_MODEL,
            UNSIGNED_ZERO,
            UNSIGNED_ZERO
        );

        for (uint64_t desc_idx = 0; desc_idx < CountOfAPC_; desc_idx++)
        {
            const APCDescriptorRange self_range = ReadRangeForASingleAPCSlotFromAPCDescriptor_(desc_idx);
            if (!self_range.IsVAlid)
            {
                continue;
            }
            
            const APCDescriptorRange segment_pool_range = GetSegmentPoolBegainEndForSingleAPCDescription_(desc_idx);
            if (!segment_pool_range.IsVAlid)
            {
                continue;
            }

            const APCDescriptorRange next_segment_pool_range = GetSegmentPoolBegainEndForSingleAPCDescription_(desc_idx + 1);

            DescriptionOfAPC::SingleAPCDescriptionCellBuffer desired_buffer = DescriptionOfAPC::MakeADefaultAPCDescription(
                desc_idx,
                static_cast<uint64_t>(segment_pool_range.BeginIndex),
                static_cast<uint64_t>(segment_pool_range.EndIndex),
                static_cast<uint64_t>(next_segment_pool_range.BeginIndex),
                version,
                LocalityPolicy::PUBLISHED                
            );

            bool buffer_ok = DescriptionOfAPC::ValidateSingleAPCDescriptionBuffer(
                desired_buffer,
                true,
                OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC,
                DescriptionOfAPC::StateOfSingleAPCDescription::RECORD_WITH_SEGMENT_POOL
            );
            if (!buffer_ok)
            {
                continue;
            }

            if (!ForceMemCopyFromArray_(self_range.BeginIndex, static_cast<size_t>(APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX), desired_buffer))
            {
                continue;
            }
            
            for (size_t seg_idx = segment_pool_range.BeginIndex; seg_idx <= segment_pool_range.EndIndex; seg_idx++)
            {
                StorePackedCellUncheckedDirectly(seg_idx, idle_apc_cell);
            }
            
        }
        
        MakeAndStoreFabricMetaValue48_(FabricMetaIndicies::TOTAL_APC_IN_USE, UNSIGNED_ZERO);
        
    }


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
