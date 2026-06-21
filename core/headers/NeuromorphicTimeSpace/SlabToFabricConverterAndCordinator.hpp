#pragma once 
#include "FabricTableConstructors/HashTablesConstructor.h"

namespace PredictedAdaptedEncoding
{
    
    
    class SlabToFabricConverterAndCordinator : public HashTablesConstructor
    {
    private:

        packed64_t* AllocatePackedCellRaw_(size_t count_of_cells) noexcept;
        
        void FreeRawPackedCells_(packed64_t*packed_cell_memory_ptr, size_t packed_cell_count) noexcept;

        void ResetScalarsofTheFabric_() noexcept;

        void InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept;

        /// @brief CALLES: 4xUpdateValidPairedOccupancyApproxAtomically_ ON: Each LocalityPolicy
        constexpr void Zero4LocalityBasedOccupancyOfFabric_() noexcept;

        /// @brief BUILD: & INITIALIZED: All The APC Handle Descriptor With Segment Pool <-  CONSISTING: Packed CEll -> PacvkedMode::VALUE32
        void InitializeAPCDescriptorTable_() noexcept;

        /// @brief INITIALIZES: All FabricMetaIndicies
        /// @param table_directory_begin 
        /// @param table_directory_end 
        constexpr void InitializeCompleateFabricMetaIndices_(size_t table_directory_begin, size_t table_directory_end) noexcept;

    public:
        SlabToFabricConverterAndCordinator(/* args */) noexcept = default;

        ~SlabToFabricConverterAndCordinator() noexcept
        {
            ShutDownFabric();
        }

        SlabToFabricConverterAndCordinator(const SlabToFabricConverterAndCordinator&) = delete;
        SlabToFabricConverterAndCordinator& operator = (const SlabToFabricConverterAndCordinator&) = delete;

        void ShutDownFabric() noexcept;

        // bool InitializeFabric(
        //     uint16_t slot_count,
        //     size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
        //     uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
        //     uint32_t fabric_thread_capacity = DEFAULT_THREAD_TABLE_CAPACITY
        // ) noexcept
        // {
        //     bool expected = false;
        //     if (!InitializationInProgress_.compare_exchange_strong(expected, true, MoClaimSuccess, MoClaimFailure))
        //     {
        //         return false;
        //     }

        //     struct InitGuardSTFC
        //     {
        //         SlabToFabricConverterAndCordinator* SelfPtr{};
        //         bool SuccesInit{false};


        //         ~InitGuardSTFC()
        //         {
        //             if (!SuccesInit && SelfPtr)
        //             {
        //                 SelfPtr->FabricInitialized_.store(false, MoStoreSeq_);
        //             }
                    
        //             if (SelfPtr)
        //             {
        //                 SelfPtr->InitializationInProgress_.store(false, MoStoreSeq_);
        //             }
        //         }
        //     } internal_init_guard{this, false};
            
        //     ShutDownFabric();

        //     InitializationInProgress_.store(true, MoStoreSeq_);

        //     if (slot_count == UNSIGNED_ZERO || slot_cell_count > APCDataStructure::APC_MAX_LENGTH_OR_COUNTER)
        //     {
        //         return false;
        //     }
            
        //     if (slot_cell_count < MINIMUM_BRANCH_CAPACITY)
        //     {
        //         slot_cell_count = MINIMUM_BRANCH_CAPACITY;
        //     }

        //     CountOfAPC_ = static_cast<uint64_t>(slot_count);
        //     PerAPCRuntimeCellCount_ = static_cast<uint64_t>(slot_cell_count);
        //     SlabId_ = slab_id == UNSIGNED_ZERO ? APCDataStructure::BRANCH_VERSION : slab_id;
        //     ThreadTableCapacity_ = fabric_thread_capacity == UNSIGNED_ZERO ? DEFAULT_THREAD_TABLE_CAPACITY : fabric_thread_capacity;

        //     HashBucketCount_ = HashHelpers::BucketCountForExpectedEntries(CountOfAPC_);

        //     if (HashBucketCount_ == UNSIGNED_ZERO || HashBucketCount_ >= APC_FABRIC_INDEX_SENTINAL)
        //     {
        //         return false;
        //     }
            
        //     RelationRecordCount_ = HashHelpers::NextPowerOf2Unsigned48_(std::max<uint64_t>(HashHelpers::MIN_LIMIT_POW_OF_2, CountOfAPC_ * HashHelpers::DEFAULT_TABLE_TAILROOM_MULT));
        //     DeviceViewRecordCount_ = HashHelpers::NextPowerOf2Unsigned48_(std::max<uint64_t>(HashHelpers::MIN_LIMIT_POW_OF_2, CountOfAPC_ )); // NO EXTRA TAILROOM

        //     size_t cursor = DefaultFabricAlignment16Cell_(APCDataStructure::METACELL_COUNT);
        //     const size_t record_book_begin = cursor;
        //     const size_t record_book_end = record_book_begin + static_cast<size_t>(RecordBookConf::RECORD_BOOK_INTERNAL_SEGMENT_COUNT) * RECORD_BOOK_WIDTH;

        //     cursor = DefaultFabricAlignment16Cell_(record_book_end);
        //     const size_t apc_description_begin = cursor;
        //     const size_t apc_description_end = apc_description_begin + static_cast<size_t>(CountOfAPC_ * APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX);

        //     cursor = DefaultFabricAlignment16Cell_(apc_description_end);
        //     const size_t branch_hash_begin = cursor;
        //     const size_t branch_hash_end = branch_hash_begin + static_cast<size_t>(HashBucketCount_ * HASH_BUCKED_WIDTH_OF_FABRIC);

        //     cursor = DefaultFabricAlignment16Cell_(branch_hash_end);
        //     const size_t logical_hash_begin = cursor;
        //     const size_t logical_hash_end = logical_hash_begin + static_cast<size_t>(HashBucketCount_ * HASH_BUCKED_WIDTH_OF_FABRIC);
            
        //     cursor = DefaultFabricAlignment16Cell_(logical_hash_end);
        //     const size_t shared_hash_begin = cursor;
        //     const size_t shared_hash_end = shared_hash_begin + static_cast<size_t>(HashBucketCount_ * HASH_BUCKED_WIDTH_OF_FABRIC);

        //     cursor = DefaultFabricAlignment16Cell_(shared_hash_end);
        //     const size_t edge_table_begin = cursor;
        //     const size_t edge_table_end = edge_table_begin + static_cast<size_t>(CountOfAPC_ * QUEUE_RECORD_WIDTH_OF_FABRIC);

        //     cursor = DefaultFabricAlignment16Cell_(edge_table_end);
        //     const size_t free_list_begin = cursor;
        //     const size_t free_list_end = free_list_begin + static_cast<size_t>(CountOfAPC_ * QUEUE_RECORD_WIDTH_OF_FABRIC);

        //     cursor = DefaultFabricAlignment16Cell_()

        // }

    };

    

}
