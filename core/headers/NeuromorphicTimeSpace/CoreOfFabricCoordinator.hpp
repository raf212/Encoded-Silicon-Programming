#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{

    class AdaptivePackedCellContainer;
    static constexpr uint32_t APC_FABRIC_HASH_TOMBSTONE_KEY = IN_CELL_VALUE_MODE32_SENTINAL;
    static constexpr uint32_t DEFAULT_HAS_CONST_1 = 0x7feb352du;
    static constexpr uint32_t DEFAULT_HAS_CONST_2 = 0x846ca68bu;

    static constexpr size_t DEFAULT_FABRIC_CONTROLIO_LENGTH = 1024u;
    static constexpr size_t TABLE_ENTRY_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t HASH_BUCKED_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t SLOT_RECORD_WIDTH_OF_FABRIC = 12u;
    static constexpr size_t RELATION_WIDTH_OF_FABRIC = 8u;
    static constexpr size_t QUEUE_RECORD_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t WORK_RECORD_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t DEVICE_VIEW_WIDTH_OF_APC_FABRIC = 8u;
    static constexpr size_t THREAD_TABLE_RECORD_WIDTH = 4u;
    static constexpr size_t DEFAULT_THREAD_SLOT_OF_FABRIC = 256u;
    
    enum class HandleStateOfAPCFabric : uint8_t
    {
        NONE = 0x0u,
        APC_SEGMENT = 0x1u,
        FABRIC_ROOT = 0x2u,
        RELATION_RECORD = 0x3u,
        DEVICE_VIEW = 0x4u,
        RETIRED_SLOT = 0x5u
    };

    // enum class SLotStateOfAPCFabric : uint32_t
    // {
    //     FREE = 0,
    //     CLAIMED = 1,
    //     LIVE = 2,
    //     FAULTY = 3
    // };

    enum class SlotCellTypeOfAPCFabric : size_t
    {
        STATE = 0,
        OWNER_BRANCH = 1,
        GENERATION = 2,
        BEGIN = 3,
        END = 4,
        LOGICAL_ID = 5,
        SHARED_ID = 6,
        RELATION_HEADs = 7,
        RETIRE_EPOCH_LOW32 = 8,
        RETIRE_EPOCH_HIGH32 = 9,
        NEXT_HANDLE = 10,
        SLOT_FLAGS = 11
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
        COMPUTE_SAFE_EPOCH = 11
    };

    enum class StateOfFabricThread : uint32_t
    {
        FREE = 0,
        REGISTERED = 1u,
        IN_CRITICAL = 2u
    };

    enum class ThreadCellTypeOfAPCFabric : size_t
    {
        EPOCH_LOW32 = 0,
        EPOCH_HIGH32 = 1,
        WAIT_TOKEN = 2,
        THREAD_STATE = 3
    };

    enum class WorkCellClassesOfAPCFabric : uint16_t
    {
        EMPTY = 0,
        CLAIMED = 1,
        PUBLISHED = 2
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

        //4 pairs of PackedCellLocalityTypes + PriorityPhysics::VERSION_DEPENDENCY based occupancy
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

    struct AllocatorOfAPCFabricCells
    {
        using AllocateFunction = std::atomic<packed64_t>* (*)(
            size_t count_of_packed_cell, size_t alignment, void* user
        ) noexcept;

        using FreeFunction = void (*)(
            std::atomic<packed64_t>* packed_cell_storage_ptr, 
            size_t count_of_cell, size_t alignment, void*user
        ) noexcept;

        AllocateFunction AllocatePackedCellStorage{nullptr};
        FreeFunction FreePackedCellStorage{nullptr};
        void* User{nullptr};
        size_t Alignment{BIT_LENGTH_OF_A_PACKED_CELL};

        static void DefaultFreeAtomicCells(
            std::atomic<packed64_t>* packed_cell_storage_ptr, 
            size_t count_of_cell, size_t alignment, void*
        ) noexcept
        {
            if (!packed_cell_storage_ptr)
            {
                return;
            }
            for (size_t i = 0; i < count_of_cell; i++)
            {
                packed_cell_storage_ptr[i].~atomic<packed64_t>();

            }

            if (alignment < alignof(std::atomic<packed64_t>))
            {
                alignment = alignof(std::atomic<packed64_t>);
            }
            ::operator delete[](packed_cell_storage_ptr, std::align_val_t(alignment));
        }
    };

    struct CoreOfFabricCoordinator
    {

        static constexpr FabricMetaIndicies GetDesiredLowIdxOfOccupancyPairFromLocality(PackedCellLocalityTypes locality) noexcept
        {
            switch (locality)
            {
            case PackedCellLocalityTypes::IDLE :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_IDLE_LOW32;

            case PackedCellLocalityTypes::PUBLISHED :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_PUBLISHED_LOW32;

            case PackedCellLocalityTypes::CLAIMED :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_CLAIMED_LOW32;

            case PackedCellLocalityTypes::FAULTY :
                return FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_FAULTY_LOW32;

            default:
                return FabricMetaIndicies::EOF_FABRIC_HEADER;
            }
        }


        static constexpr uint32_t GetWidthOfValidFebricTable(FabricTableSegmentClasses table_idintity) noexcept
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

            
            default:
                return 0u;
            }
        }

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


        static constexpr packed64_t MakeANEncodedHandlerCellForFabric(
            uint32_t slot_index, 
            uint8_t slab_id, uint8_t generation, 
            HandleStateOfAPCFabric handle_state = HandleStateOfAPCFabric::APC_SEGMENT,
            FabricTableSegmentClasses fabric_segment_class = FabricTableSegmentClasses::GLOBAL_AND_CONFIG,
            PackedCellLocalityTypes locality_of_cell = PackedCellLocalityTypes::IDLE
        ) noexcept
        {
            const uint16_t external_handle = Clock16Subdivision1x8Plus2x4InMode32CellModel::Pack1x8Plus2x4InUnsigned16_(slab_id, generation, static_cast<uint8_t>(handle_state));

            const packed64_t packed_cell = PackedCell64_t::MakeInitialFabricValidPackedCell(
                PackedMode::MODE_32, locality_of_cell, fabric_segment_class,
                PackedCellDataType::UnsignedPCellDataType, 
                static_cast<uint64_t>(slot_index), external_handle,
                PriorityPhysics::VERSION_DEPENDENCY,
                SubClassesOfMode32::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4
            );

            return packed_cell;
        }

    };
    
}
