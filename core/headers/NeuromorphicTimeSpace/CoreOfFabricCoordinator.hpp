#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{

    class AdaptivePackedCellContainer;
    static constexpr uint64_t APC_FABRIC_UINT48_MAX = PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT;
    static constexpr uint32_t APC_FABRIC_HASH_EMPTY_KEY = 0u;
    static constexpr uint32_t APC_FABRIC_HASH_TOMBSTONE_KEY = IN_CELL_VALUE_MODE32_SENTINAL;
    static constexpr uint32_t DEFAULT_HAS_CONST_1 = 0x7feb352du;
    static constexpr uint32_t DEFAULT_HAS_CONST_2 = 0x846ca68bu;

    static constexpr size_t DEFAULT_FABRIC_CONTROLIO_LENGTH = 1024u;

    /// @brief should be 3
    static constexpr size_t RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC = 3u;

    /// @brief should be 3
    static constexpr size_t HASH_BUCKED_WIDTH_OF_FABRIC = 3u;

    static constexpr size_t SLOT_RECORD_WIDTH_OF_FABRIC = 12u;
    static constexpr size_t RELATION_WIDTH_OF_FABRIC = 8u;
    static constexpr size_t QUEUE_RECORD_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t WORK_RECORD_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t DEVICE_VIEW_WIDTH_OF_APC_FABRIC = 8u;
    static constexpr size_t THREAD_TABLE_RECORD_WIDTH = 4u;
    static constexpr size_t DEFAULT_THREAD_SLOT_OF_FABRIC = 256u;


    enum class RecordBookInternalIndexing : tag8_t
    {
        BEGIN48 = 0,
        END48 = 1,
        META32 = 2
    };

    enum class HashTableInternalIndexing : tag8_t
    {
        KEY_INDEX = 0,
        VALUE_INDEX = 1,
        PROB_DISTANCE_LOCK = 2
    };

    struct FTSC_SlabRangeTripletFrom_RecordBookOfFTSC
    {
        packed64_t BeginIdxRawType48Cell = UNSIGNED_ZERO;
        packed64_t EndIdxRawType48Cell = UNSIGNED_ZERO;
        packed64_t WidthVersionOriginSafty = UNSIGNED_ZERO;
    };
    static_assert(sizeof(FTSC_SlabRangeTripletFrom_RecordBookOfFTSC) == RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC * sizeof(packed64_t));
    static_assert(alignof(FTSC_SlabRangeTripletFrom_RecordBookOfFTSC) == alignof(packed64_t));


    struct HashKeyValueDistanceTriplet
    {
        uint64_t HashValue = UNSIGNED_ZERO;
        uint64_t HashKey = UNSIGNED_ZERO;
        uint16_t ProbDistance = UNSIGNED_ZERO;
        bool IsValid = false;
    };
    static_assert(sizeof(HashKeyValueDistanceTriplet) == HASH_BUCKED_WIDTH_OF_FABRIC * sizeof(uint64_t));
    static_assert(alignof(HashKeyValueDistanceTriplet) == alignof(uint64_t));


    enum class SlotCellTypeOfAPCFabric : uint8_t
    {
        STATE = 0,
        OWNER_BRANCH = 1,
        GENERATION = 2,
        BEGIN = 3,
        END = 4,
        LOGICAL_ID = 5,
        SHARED_ID = 6,
        RELATION_HEADS = 7,
        RETIRE_EPOCH48 = 8,
        NEXT_HANDLE = 9,
        SLOT_FLAGS = 10,
        UNASSIGNED_UNUSED_NANNULL = 11
    };


    enum class FabricMetaIndicies : uint8_t
    {
        MAGIC = 0,
        VERSION = 1,
        FLAGS = 2,
        SLAB_ID = 3,
        TOTAL_CELLS = 4,
        CONTROL_CELLS_OF_FABRIC = 5,
        SLOT_COUNT = 6,
        SLOT_CELL_COUNT = 7,
        SEGMENT_POOL_BEGIN_IDX = 8,
        SEGMENT_POOL_END_IDX = 9,
        
        FREE_SLOT_HEAD = 10,
        RETIRE_SLOT_HEAD = 11,
        RELATION_FREE_HEAD = 12,
        RESERVED_13 = 13,
        RESERVED_14 = 14,
        NEXT_BRANCH_ID = 15,
        NEXT_RELATION_ID = 16,
        NEXT_DEVICE_VIEW_ID = 17,


        WORK_WRITE_CURSOR = 18,
        WORK_READ_CURSOR = 19,
        READY_WRITE_CURSOR = 20,
        READY_READ_CURSOR = 21,

        //COUNTS
        GLOBAL_EPOCH48 = 22,
        MIN_SAFE_EPOCH48 = 23,
        RELATION_RECLAIM_COUNT = 24,
        WORK_QUEUE_DROPPED_COUNT = 25,
        THREAD_ACTIVE_COUNT = 26,
        THREAD_REGISTRATION_FAILURE = 27,
        RELATION_TOMBSTONE_COUNT = 28,
        RELATION_UNLINK_FAILURES = 29,
        WORK_QUEUE_CLAIM_FAILURES = 30,
        CAS_FAILURE_COUNT = 31,
        ERROR_COUNT = 32,
        RETIRED_COUNT = 33,
        LIVE_SLOT_COUNT = 34,
        FABRIC_CLOCK16 = 35,
        ///end count
        
        RECORD_BOOK_OF_TSC_BEGIN = 36,
        RECORD_BOOK_OF_TSC_END = 37,
        TABLE_DIRECTORY_COUNT = 38,
        TABLE_DIRECTORY_VERSION = 39,

        //4 pairs of LocalityPolicy + PriorityPolicy::INFLUENCED based occupancy
        FABRIC_OCCUPANCY_APPROXIMATION_IDLE_LOW32 = 40,
        FABRIC_OCCUPANCY_APPROXIMATION_IDLE_HIGH32 = 41,
        FABRIC_OCCUPANCY_APPROXIMATION_PUBLISHED_LOW32 = 42,
        FABRIC_OCCUPANCY_APPROXIMATION_PUBLISHED_HIGH32 = 43,
        FABRIC_OCCUPANCY_APPROXIMATION_CLAIMED_LOW32 = 44,
        FABRIC_OCCUPANCY_APPROXIMATION_CLAIMED_HIGH32 = 45,
        FABRIC_OCCUPANCY_APPROXIMATION_FAULTY_LOW32 = 46,
        FABRIC_OCCUPANCY_APPROXIMATION_FAULTY_HIGH32 = 47,
        //end pair

        HASH_TOMBSTONE_COUNT = 48,
        HASH_COMPACTION_COUNT = 49,
        WORK_QUEUE_OCCUPANCY = 50,
        READY_QUEUE_OCCUPANCY = 51,

        BACKOFF_SPIN_LIMIT = 52,
        BACKOFF_YIELD_LIMIT = 53,
        INITIALIZATION_STATE = 54,
        HAS_COMPACTION_INFLIGHT = 55,

        THREAD_TABLE_CAPACITY = 56,


        RESERVED_57_UPTO_94 = 57,

        EOF_FABRIC_HEADER = 95

    };

    struct HashHelpers
    {
        static constexpr uint64_t NextPowerOf2Unsigned32_(uint64_t given_value) noexcept
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
            given_value |= given_value >> 32u;

            return given_value + 1u;
        }

        static constexpr uint32_t HashUnsigned32_(uint32_t given_value) noexcept
        {
            given_value ^= given_value >> LOW16_BIT_LEN;
            given_value *= DEFAULT_HAS_CONST_1;
            given_value ^= given_value >> (LOW16_BIT_LEN - 1);
            given_value *= DEFAULT_HAS_CONST_2;
            given_value ^=  given_value >> LOW16_BIT_LEN;
            return given_value;
        }
    };



    // struct HashPackedCellTripletCarrier
    // {
    //     packed64_t BeginIdxRawType48Cell = UNSIGNED_ZERO;
    //     packed64_t EndIdxRawType48Cell = UNSIGNED_ZERO;
    //     packed64_t WidthVersionOriginSafty = UNSIGNED_ZERO;
    // };
    // static_assert(sizeof(HashPackedCellTripletCarrier) == RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC * sizeof(packed64_t));
    // static_assert(alignof(HashPackedCellTripletCarrier) == alignof(packed64_t));


    struct CoreOfFabricCoordinator
    {

        static constexpr uint8_t EACH_TABLE_RECORD_SENTINAL = UINT8_MAX;



        static constexpr bool IsValidFabricTable(FabricTableSegmentClasses table_class) noexcept
        {
            return table_class > FabricTableSegmentClasses::NONE &&
                table_class < FabricTableSegmentClasses::COUNT;
        }

        static constexpr bool IsValidHashTable(FabricTableSegmentClasses table_class) noexcept
        {
            return table_class == FabricTableSegmentClasses::BRANCH_HASH ||
                table_class == FabricTableSegmentClasses::LOGICAL_HASH ||
                table_class == FabricTableSegmentClasses::SHARED_HASH;
        }

        static constexpr bool IsQueueTable(FabricTableSegmentClasses table_class) noexcept
        {
            return table_class == FabricTableSegmentClasses::FREE_RETIRE_TABLE ||
                table_class == FabricTableSegmentClasses::READY_QUEUE ||
                table_class == FabricTableSegmentClasses::WORK_QUEUE;
        }

        static constexpr FabricMetaIndicies GetDesiredLowIdxOfOccupancyPairFromLocality(LocalityPolicy locality) noexcept
        {
            switch (locality)
            {
            case LocalityPolicy::IDLE :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_IDLE_LOW32;

            case LocalityPolicy::PUBLISHED :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_PUBLISHED_LOW32;

            case LocalityPolicy::CLAIMED :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_CLAIMED_LOW32;

            case LocalityPolicy::FAULTY :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_FAULTY_LOW32;

            default:
                return FabricMetaIndicies::EOF_FABRIC_HEADER;
            }
        }


        static constexpr uint8_t GetWidthOfValidFabricTable(FabricTableSegmentClasses table_idintity) noexcept
        {
            switch (table_idintity)
            {
            case FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES:
                return static_cast<uint8_t>(RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC);
            
            case FabricTableSegmentClasses::APC_DESCRIPTOR:
                return static_cast<uint8_t>(SLOT_RECORD_WIDTH_OF_FABRIC);
            
            case FabricTableSegmentClasses::BRANCH_HASH:
            case FabricTableSegmentClasses::SHARED_HASH:
            case FabricTableSegmentClasses::LOGICAL_HASH:
                return static_cast<uint8_t>(HASH_BUCKED_WIDTH_OF_FABRIC);
            
            case FabricTableSegmentClasses::EDGE_TABLE:
                return static_cast<uint8_t>(RELATION_WIDTH_OF_FABRIC);

            case FabricTableSegmentClasses::FREE_RETIRE_TABLE:
            case FabricTableSegmentClasses::READY_QUEUE:
                return static_cast<uint8_t>(QUEUE_RECORD_WIDTH_OF_FABRIC);

            case FabricTableSegmentClasses::WORK_QUEUE:
                return static_cast<uint8_t>(WORK_RECORD_WIDTH_OF_FABRIC);

            case FabricTableSegmentClasses::DEVICE_VIEW_TABLE:
                return static_cast<uint8_t>(DEVICE_VIEW_WIDTH_OF_APC_FABRIC);

            case FabricTableSegmentClasses::THREAD_TABLE:
                return static_cast<uint8_t>(THREAD_TABLE_RECORD_WIDTH);

            case FabricTableSegmentClasses::SEGMENT_POOL:
                return UNSIGNED_ZERO;
            
            default:
                return EACH_TABLE_RECORD_SENTINAL;
            }
        }

        static constexpr bool IsThisValidRecordBookPackedCell(
            packed64_t packed_cell,
            PackedCell64_t::AuthoritiveCellView* return_cell_view = nullptr
        ) noexcept
        {
            PackedCell64_t::AuthoritiveCellView desired_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
            const bool ok = IsTheCellConsumeableAsRecordBookCellOfTSC(desired_cell_view);
            if (return_cell_view)
            {
                *return_cell_view = desired_cell_view;
            }
            return ok;
        }

        static constexpr bool IsTheCellConsumeableAsRecordBookCellOfTSC(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {

            if (!a_cell_view.IsCellValid)
            {
                return false;
            }
            
            if (
                a_cell_view.CellOwnership != OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC ||
                a_cell_view.FabricTableSegmentClass != FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES ||
                a_cell_view.CellValueDataType != InternalDataTypePolicy::UnsignedPCellDataType 
            )
            {
                return false;
            }
            
            
            switch (a_cell_view.CellMode)
            {
            case PackedMode::VALUE48:
                return a_cell_view.ContractOfValue != AccessContractOfValue::UNASSIGNED_UNUSED_NANNULL &&
                    a_cell_view.Raw48BitInCellData < PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT;
            case PackedMode::MODEL32:

                return a_cell_view.SubClassOfModel32 == Model32Subclass::UNCLOCKED_1x8_PLUS_2x4 &&
                    a_cell_view.Raw32BitInCellData < IN_CELL_VALUE_MODE32_SENTINAL;

            default:
                return false;
            }            
        }

        static constexpr std::optional<uint64_t> ValidateAFabricTableRangeStruct(const FTSC_SlabRangeTripletFrom_RecordBookOfFTSC& provided_range_triplet) noexcept
        {
            const PackedCell64_t::AuthoritiveCellView auth_view_of_begin_idx = PackedCell64_t::GetAuthoritiveViewsForACell(provided_range_triplet.BeginIdxRawType48Cell);
            const PackedCell64_t::AuthoritiveCellView auth_view_of_end_idx = PackedCell64_t::GetAuthoritiveViewsForACell(provided_range_triplet.EndIdxRawType48Cell);
            const PackedCell64_t::AuthoritiveCellView auth_view_of_safty_meta = PackedCell64_t::GetAuthoritiveViewsForACell(provided_range_triplet.WidthVersionOriginSafty);


            if (!auth_view_of_begin_idx.IsCellValid || !auth_view_of_begin_idx.IsCellValid || !auth_view_of_safty_meta.IsCellValid)
            {
                return std::nullopt;
            }

            if (
                !IsTheCellConsumeableAsRecordBookCellOfTSC(auth_view_of_begin_idx) || 
                !IsTheCellConsumeableAsRecordBookCellOfTSC(auth_view_of_end_idx) ||
                !IsTheCellConsumeableAsRecordBookCellOfTSC(auth_view_of_safty_meta)
            )
            {
                return std::nullopt;
            }

            if (auth_view_of_begin_idx.Raw48BitInCellData < APCDataStructure::METACELL_COUNT || 
                auth_view_of_begin_idx.Raw48BitInCellData >= auth_view_of_end_idx.Raw48BitInCellData ||
                auth_view_of_begin_idx.CellMode != PackedMode::MODEL32 ||
                auth_view_of_safty_meta.InCellClock16 == UNSIGNED_ZERO
            )
            {
                return std::nullopt;
            }

            const uint8_t record_width = Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractLowest8Bit_(auth_view_of_safty_meta.InCellClock16);
            const OriginOfRecord origin_table = static_cast<OriginOfRecord>(Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractMid4Bit_(auth_view_of_safty_meta.InCellClock16));

            const uint64_t full_width = (auth_view_of_end_idx.Raw48BitInCellData) - (auth_view_of_begin_idx.Raw48BitInCellData);
            if ((static_cast<uint32_t>(full_width) !=  auth_view_of_safty_meta.Raw32BitInCellData))
            {
                return std::nullopt;
            }
            
            if (origin_table != FabricTableSegmentClasses::SEGMENT_POOL)
            {
                if (
                    record_width == UNSIGNED_ZERO ||
                    (full_width % RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC) != UNSIGNED_ZERO
                )
                {
                    return std::nullopt;
                }
            }
            

            return full_width;
        }

        static constexpr bool IsHashPackedCellRuntimeAccessable(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {
            if (
                !a_cell_view.IsCellValid || 
                a_cell_view.CellOwnership != OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC ||
                a_cell_view.CellValueDataType != InternalDataTypePolicy::UnsignedPCellDataType
            )
            {
                return false;
            }

            if (
                !IsValidHashTable(a_cell_view.FabricTableSegmentClass) ||
                a_cell_view.LocalityOfCell == LocalityPolicy::CLAIMED ||
                a_cell_view.Raw48BitInCellData == PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT
            )
            {
                return false;
            }

            switch (a_cell_view.CellMode)
            {
            case PackedMode::VALUE48:
                return a_cell_view.ContractOfValue == AccessContractOfValue::CLAIMED_GURDED;

            case PackedMode::MODEL48:
                return a_cell_view.SubClassOfModel48 == Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL;
                
            default:
                return false;
            }
            
        }


        //kept for safty
        static constexpr bool IsThisFebricMetaIdxAValidIncrementalCountType(FabricMetaIndicies meta_idx) noexcept
        {
            switch (meta_idx)
            {
            //21 for now skeletorn
            case FabricMetaIndicies::GLOBAL_EPOCH48:
            case FabricMetaIndicies::MIN_SAFE_EPOCH48:
            case FabricMetaIndicies::NEXT_DEVICE_VIEW_ID:
            case FabricMetaIndicies::RELATION_RECLAIM_COUNT:
            case FabricMetaIndicies::WORK_QUEUE_DROPPED_COUNT:
            case FabricMetaIndicies::THREAD_ACTIVE_COUNT:
            case FabricMetaIndicies::THREAD_REGISTRATION_FAILURE:
            case FabricMetaIndicies::RELATION_TOMBSTONE_COUNT:
            case FabricMetaIndicies::RELATION_UNLINK_FAILURES:
            case FabricMetaIndicies::WORK_QUEUE_CLAIM_FAILURES:
            case FabricMetaIndicies::CAS_FAILURE_COUNT:
            case FabricMetaIndicies::ERROR_COUNT:
            case FabricMetaIndicies::RETIRED_COUNT:
            case FabricMetaIndicies::LIVE_SLOT_COUNT:
            case FabricMetaIndicies::FABRIC_CLOCK16:
                return true;
            
            default:
                return false;
            }
        }

    };


    struct RawPackedCellAllocator
    {
        using AllocateFunction = packed64_t* (*)(
            size_t count_of_packed_cell, size_t alignment, void* user
        ) noexcept;

        using FreeFunction = void (*)(
            packed64_t* packed_cell_storage_ptr, 
            size_t count_of_cell, size_t alignment, void*user
        ) noexcept;

        AllocateFunction AllocatePackedCellStorage{nullptr};
        FreeFunction FreePackedCellStorage{nullptr};
        void* User{nullptr};
        size_t Alignment{BIT_LENGTH_OF_A_PACKED_CELL};

        static size_t AlignBiteCount_(size_t bytes, size_t alignment) noexcept
        {
            if (alignment == UNSIGNED_ZERO)
            {
                return bytes;
            }

            const size_t remaining_bytes = bytes % alignment;
            return remaining_bytes == UNSIGNED_ZERO ? bytes : bytes + (alignment - remaining_bytes);
        }

        static packed64_t* DefaultAllocateAtomicCells(
            size_t count_of_packed_cell, size_t alignment, void*
        ) noexcept
        {
            if (count_of_packed_cell == UNSIGNED_ZERO)
            {
                return nullptr;
            }

            alignment = std::max<size_t>(alignment, alignof(packed64_t));
            const size_t byte_count = sizeof(packed64_t) * count_of_packed_cell;
            const size_t aligned_bytes = AlignBiteCount_(byte_count, alignment);

#if defined(_MSC_VER)

            void* raw_packed_cell_memory = _aligned_malloc(aligned_bytes, alignment);
#else
            void* raw_packed_cell_memory = std::aligned_alloc(alignment, aligned_bytes);
#endif
            if (!raw_packed_cell_memory)
            {
                return nullptr;
            }
            std::memset(raw_packed_cell_memory, UNSIGNED_ZERO, aligned_bytes);
            return static_cast<packed64_t*>(raw_packed_cell_memory);
            
        }

        static void DefaultFreeAtomicCells(
            packed64_t* packed_cell_storage_ptr, 
            size_t, size_t, void*
        ) noexcept
        {
            if (!packed_cell_storage_ptr)
            {
                return;
            }
#if defined(_MSC_VER)
            _aligned_free(packed_cell_storage_ptr);
#else
            std::free(packed_cell_storage_ptr);
#endif
        }
    };
    
}
