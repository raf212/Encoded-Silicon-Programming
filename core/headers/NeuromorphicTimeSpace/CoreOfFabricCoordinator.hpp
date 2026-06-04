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
    static constexpr size_t TABLE_ENTRY_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t HASH_BUCKED_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t SLOT_RECORD_WIDTH_OF_FABRIC = 12u;
    static constexpr size_t RELATION_WIDTH_OF_FABRIC = 8u;
    static constexpr size_t QUEUE_RECORD_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t WORK_RECORD_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t DEVICE_VIEW_WIDTH_OF_APC_FABRIC = 8u;
    static constexpr size_t THREAD_TABLE_RECORD_WIDTH = 4u;
    static constexpr size_t DEFAULT_THREAD_SLOT_OF_FABRIC = 256u;

    enum class ReadContractResultOfPackedCell : uint8_t
    {
        UNDEFINED_ERROR = 0,

        SUCCESSFUL_SINGLE_CELL_NON_ATOMIC_READ = 1,
        SUCCESSFUL_SINGLE_CELL_ATOMIC_READ = 2,
        SUCCESSFUL_PAIRED_VERSION_MATCHED_NON_ATOMIC_READ = 3,
        SUCCESSFUL_PAIRED_VERSION_MATCHED_ATOMIC_READ = 4,

        CLAIMED_CONTRACT_BREACHED = 5,
        FAULTY_CONTRACT_BREACHED = 6,
        VERSION_CONTRACT_BREACHED = 7,
        MODE_CONTRACT_BREACHED = 8,
        SUB_CLASS_CONTRACT_BREACHED = 9,
        DTATA_TYPE_CONTRACT_BREACHED = 10,
        MULTIPLE_CONTRACT_BREACHED = 11,

        SEGMENT_IO_NODE_SEGMENT_CLASS_CONTRACT_BREACHED = 12,
        FABRIC_TABLE_CLASS_CONTRACT_BREACHED = 13,

        CELL_READ_CONTAINS_PACKED_CELL_SENTINAL = 14

    };
    
    enum class HandleStateOfAPCFabric : uint8_t
    {
        NONE = 0x0u,
        APC_SEGMENT = 0x1u,
        FABRIC_ROOT = 0x2u,
        RELATION_RECORD = 0x3u,
        DEVICE_VIEW = 0x4u,
        RETIRED_SLOT = 0x5u,
        HASH_VALUE = 0x6u,
        FABRIC_TABLE = 0x7u
    };

    enum class TableEntryCellTypeOfFabric : size_t
    {
        BEGIN48 = 0,
        END48 = 1
    };

    enum class HashCellOfFabridc : size_t
    {
        KEY = 0,
        VALUE_HANDLE = 1,
    };

    enum class SlotCellTypeOfAPCFabric : size_t
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
        NANNULL = 11
    };

    enum class RelationCellOfAPCFabric : size_t
    {
        SOURCE_BRANCH = 0,
        TARGET_BRANCH = 1,
        KIND_OF_BRANCH = 2,
        REGION_POLICY = 3,
        NEXT_OUT = 4,
        NEXT_IN = 5,
        WEIGHTOR_VIEW = 6,
        CLOCK_PRIORITY_DEVICE_VIEW = 7
    };

    enum class RelationKindOfFabric : size_t
    {
        NONE = 0,
        SHARED_NEXT,
        SHARED_PREVIOUS,
        FEED_FORWAED_IN,
        FEED_FORWARD_OUT,
        FEED_BACKWARD_IN,
        FEED_BACKWARD_OUT,
        LATERAL_0,
        LATERAL_1,
        STATE_BIND,
        ERROR_BIND,
        AUX_BIND,
        DEVICE_VIEW_LINK,
        RETIRE_LINK
    };

    enum class DeviceHintOfNeuromorphicFabric : uint16_t
    {
        HOST_FAST_CPU = 0,
        SYCL_SHARED_USM = 1,
        SYCL_DEVICE_USM = 2,
        ONEDPL_PARALLEL_SCAN_SORT_REDUCE = 3,
        ONEDNN_PRIMITIVE = 4,
        ONEMKL_DENSE = 5,
        ONEMKL_SPARSE = 6,
        PYTORCH_CPP_TENSOR_VIEW = 7
    };

    enum class WorkKindOfAPCFabric : uint16_t
    {
        NONE = 0,
        CREATE_SHARED_SEGMENT = 1,
        REBUILD_READY_MASK = 2,
        REBUILD_CHAIN_METADATA = 3,
        REBUILD_FLOAT32_VIEW = 4,
        RETIRE_HANDLE = 5,
        RECYCLE_RETIRED = 6,
        COMPACT_REGION_FOR_DEVICE = 7,
        COMPACT_HASH_TABLE = 8,
        COMPACT_ALL_HASH_TABLES = 9,
        RELATION_GC = 10,
        COMPUTE_SAFE_EPOCH = 11,
        MIGRATE_OR_GROW_FABRIC = 12
    };

    enum class ThreadCellTypeOfAPCFabric : size_t
    {
        EPOCH48 = 0,
        WAIT_TOKEN = 2,
        THREAD_STATE = 3,
        THREAD_FLAGS = 4
    };

    enum class FabricMetaIndicies : size_t
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
        
        TABLE_DIRECTORY_BEGIN = 36,
        TABLE_DIRECTORY_END = 37,
        TABLE_DIRECTORY_COUNT = 38,
        TABLE_DIRECTORY_VERSION = 39,

        //4 pairs of LocalityPolicy + PriorityPolicy::VERSIONED based occupancy
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

    struct ThreadHandleOfAPCFabric
    {
        uint32_t ThreadIndex{IN_CELL_VALUE_MODE32_SENTINAL};
        uint32_t TokenOfThreadHandle{UNSIGNED_ZERO};

        bool IsThisValidThreadhandle() const noexcept
        {
            return ThreadIndex != IN_CELL_VALUE_MODE32_SENTINAL && TokenOfThreadHandle != UNSIGNED_ZERO;
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

    struct CoreOfFabricCoordinator
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
            given_value ^= given_value >> CLK_B16;
            given_value *= DEFAULT_HAS_CONST_1;
            given_value ^= given_value >> (CLK_B16 - 1);
            given_value *= DEFAULT_HAS_CONST_2;
            given_value ^=  given_value >> CLK_B16;
            return given_value;
        }

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


        static constexpr uint32_t GetWidthOfValidFabricTable(FabricTableSegmentClasses table_idintity) noexcept
        {
            switch (table_idintity)
            {
            case FabricTableSegmentClasses::TABLE_DIRECTORY:
                return static_cast<uint32_t>(TABLE_ENTRY_WIDTH_OF_FABRIC);
            
            case FabricTableSegmentClasses::SLOT_DIRECTORY:
                return static_cast<uint32_t>(SLOT_RECORD_WIDTH_OF_FABRIC);
            
            case FabricTableSegmentClasses::BRANCH_HASH:
            case FabricTableSegmentClasses::SHARED_HASH:
            case FabricTableSegmentClasses::LOGICAL_HASH:
                return static_cast<uint32_t>(HASH_BUCKED_WIDTH_OF_FABRIC);
            
            case FabricTableSegmentClasses::EDGE_TABLE:
                return static_cast<uint32_t>(RELATION_WIDTH_OF_FABRIC);

            case FabricTableSegmentClasses::FREE_RETIRE_TABLE:
            case FabricTableSegmentClasses::READY_QUEUE:
                return static_cast<uint32_t>(QUEUE_RECORD_WIDTH_OF_FABRIC);

            case FabricTableSegmentClasses::WORK_QUEUE:
                return static_cast<uint32_t>(WORK_RECORD_WIDTH_OF_FABRIC);

            case FabricTableSegmentClasses::DEVICE_VIEW_TABLE:
                return static_cast<uint32_t>(DEVICE_VIEW_WIDTH_OF_APC_FABRIC);

            case FabricTableSegmentClasses::THREAD_TABLE:
                return static_cast<uint32_t>(THREAD_TABLE_RECORD_WIDTH);

            case FabricTableSegmentClasses::SEGMENT_POOL:
                return MINIMUM_BRANCH_CAPACITY;
            
            default:
                return 0u;
            }
        }

        /// @brief Creates an unsigned fabric-owned cell.
        ///
        /// Call flow:
        ///   This->MakeInitialFabricValidPackedCell()
        ///     -> MakeInitialValidBlindPackedCell()
        ///     -> MakeAnUncheckedCellBasedOnMode_::MakeInCellMetaForMode + ComposeValue
        ///
        /// @return packed64_t or PACKED_CELL_SENTINAL
        static constexpr packed64_t MakeAValidFabricMode32UnsignedCell(
            uint32_t value,
            clk16_t external_meta_or_prob_distance,
            FabricTableSegmentClasses table_class,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            PriorityPolicy priority = PriorityPolicy::VERSIONED,
            Model32Subclass subclass_of_mode32 = Model32Subclass::SELF_CLASS,
            BehaveOfMode32 runtime_contract = BehaveOfMode32::VALUE32
        )
        {
            return PackedCell64_t::MakeInitialFabricValidPackedCell(
                static_cast<PackedMode>(runtime_contract),
                cell_locality, 
                table_class, 
                InternalDataTypePolicy::UnsignedPCellDataType,
                value, 
                external_meta_or_prob_distance, 
                priority, 
                subclass_of_mode32,
                Model48Subclass::SELF_CLASS
            );
        }

        static constexpr packed64_t MakeAValidFabricMode48UnsignedCell(
            uint64_t value,
            FabricTableSegmentClasses table_class,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            PriorityPolicy priority = PriorityPolicy::VERSIONED,
            Model48Subclass subclass_of_mode48 = Model48Subclass::SELF_CLASS,
            BehaveOfMode48 cell_behaviour = BehaveOfMode48::VALUE48
        )
        {
            return PackedCell64_t::MakeInitialFabricValidPackedCell(
                static_cast<PackedMode>(cell_behaviour),
                cell_locality, 
                table_class, 
                InternalDataTypePolicy::UnsignedPCellDataType,
                value, 
                UNSIGNED_ZERO, 
                priority, 
                Model32Subclass::SELF_CLASS, 
                subclass_of_mode48
            );
        }

        static constexpr packed64_t MakeADirectoryEntryCellForFabric(
            uint32_t value,
            TableEntryCellTypeOfFabric table_cell_type,
            FabricTableSegmentClasses identity_of_desired_directory,
            uint8_t version = static_cast<uint8_t>(APCDataStructure::BRANCH_VERSION),
            LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED
        ) noexcept
        {
            const uint16_t external_handle = Clock16Subdivision1x8Plus2x4InMode32CellModel::Pack1x8Plus2x4InUnsigned16_(
                version, static_cast<tag8_t>(table_cell_type), static_cast<tag8_t>(identity_of_desired_directory)
            );

            const packed64_t packed_cell = MakeAValidFabricMode32UnsignedCell(
                value, external_handle, FabricTableSegmentClasses::TABLE_DIRECTORY,
                cell_locality, PriorityPolicy::VERSIONED, 
                Model32Subclass::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4,
                BehaveOfMode32::MODEL32
            );
            return packed_cell;
        }


        static constexpr packed64_t MakeANEncodedHandlerCellForFabric(
            uint32_t slot_index, 
            uint8_t slab_id, uint8_t generation, 
            HandleStateOfAPCFabric handle_state = HandleStateOfAPCFabric::APC_SEGMENT,
            FabricTableSegmentClasses fabric_segment_class = FabricTableSegmentClasses::GLOBAL_AND_CONFIG,
            LocalityPolicy locality_of_cell = LocalityPolicy::IDLE
        ) noexcept
        {
            const uint16_t external_handle = Clock16Subdivision1x8Plus2x4InMode32CellModel::Pack1x8Plus2x4InUnsigned16_(slab_id, generation, static_cast<uint8_t>(handle_state));
            const packed64_t packed_cell = MakeAValidFabricMode32UnsignedCell(
                static_cast<uint64_t>(slot_index), 
                external_handle,
                fabric_segment_class, 
                locality_of_cell, 
                PriorityPolicy::VERSIONED,
                Model32Subclass::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4
            );

            return packed_cell;
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
    
}
