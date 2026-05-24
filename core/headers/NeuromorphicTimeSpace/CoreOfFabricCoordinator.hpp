#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{

    class AdaptivePackedCellContainer;
    static constexpr uint32_t APC_FABRIC_HASH_TOMBSTONE_KEY = IN_CELL_VALUE_MODE32_SENTINAL;
    static constexpr uint32_t DEFAULT_HAS_CONST_1 = 0x7feb352du;
    static constexpr uint32_t DEFAULT_HAS_CONST_2 = 0x846ca68bu;

    static constexpr size_t FABRIC_CONTROLIO_LENGTH = 4096u;
    static constexpr size_t TABLE_ENTRY_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t HASH_BUCKED_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t SLOT_RECORD_WIDTH_OF_FABRIC = 12u;
    static constexpr size_t RELATION_WIDTH_OF_FABRIC = 8u;
    static constexpr size_t QUEUE_RECORD_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t WORK_RECORD_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t DEFAULT_THREAD_SLOT_OF_FABRIC = 256u;


    enum class FabricToIoLinkerClasses : uint16_t
    {
        NONE = 0,
        META_DATA = 1,
        DIRECTORY = 2,
        SLOT_TABLE = 3,
        HASH_TABLE = 4,
        RELATION_TABLE = 5,
        QUEUE_TABLE = 6,
        DEVICE_VIEW = 7,
        SEGMENT_POOL = 8,
        RETIRED_SEGMENT_OF_FABRIC = 9,
        UNDEFINED_SEGMENT_OF_FABRIC = 10
    };

    static constexpr APCPagedNodeSegmentClasses ConvertFabricToIOLinkerAsSegmentClasses(FabricToIoLinkerClasses fabric_class) noexcept
    {
        switch (fabric_class)
        {
            case FabricToIoLinkerClasses::META_DATA : 
            case FabricToIoLinkerClasses::DIRECTORY :
                return APCPagedNodeSegmentClasses::CONTROL_SLOT;
            
            case FabricToIoLinkerClasses::SLOT_TABLE :
                return APCPagedNodeSegmentClasses::SLOT_TABLE_DESCRIPTOR;

            case FabricToIoLinkerClasses::HASH_TABLE :
            case FabricToIoLinkerClasses::RELATION_TABLE :
                return APCPagedNodeSegmentClasses::EDGE_DESCRIPTOR;

            case FabricToIoLinkerClasses::QUEUE_TABLE :
                return APCPagedNodeSegmentClasses::AUX_SLOT;
            
            case FabricToIoLinkerClasses::DEVICE_VIEW:
                return APCPagedNodeSegmentClasses::HETEROGENOUS_MEMORY_MAYBE_PAIRED_POINTER_OR_RAW_APC_SEGMENT;
            
            case FabricToIoLinkerClasses::SEGMENT_POOL:
                return APCPagedNodeSegmentClasses::FREE_SLOT;
            
            case FabricToIoLinkerClasses::RETIRED_SEGMENT_OF_FABRIC:
            case FabricToIoLinkerClasses::UNDEFINED_SEGMENT_OF_FABRIC:
                return APCPagedNodeSegmentClasses::UNDEFINED;

            default:
                return APCPagedNodeSegmentClasses::NONE;
        }
    }

    
    enum class HandleStateOfAOCFabric : uint8_t
    {
        NONE = 0x0u,
        APC_SEGMENT = 0x1u,
        FABRIC_ROOT = 0x2u,
        RELATION_RECORD = 0x3u,
        DEVICE_VIEW = 0x4u,
        RETIRED_SLOT = 0x5u
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

        GLOBAL_EPOCH_LOW32 = 13,
        GLOBAL_EPOCH_HIGH32 = 14,
        MIN_SAFE_EPOCH_LOW32 = 15,
        MIN_SAFE_EPOCH_HIGH32 = 16,

        NEXT_BRANCH_ID = 17,
        NEXT_RELATION_ID = 18,
        NEXT_DEVICE_VIEW_ID = 19,
        FABRIC_CLOCK16 = 20,

        WORK_RETIRE_CURSOR = 21,
        WORK_READ_CURSOR = 22,
        READY_WRITE_CURSOR = 23,
        READY_READ_CURSOR = 24,

        TABLE_DIRECTORY_BEGIN = 25,
        TABLE_DIRECTORY_END = 26,
        TABLE_COUNT = 27,
        TABLE_DIRECTORY_VERSION = 28,

        FABRIC_OCCUPANCY_3x16_MODEl48 = 29,
        CAS_FAILURE_COUNT = 30,
        ERROR_COUNT = 31,
        RETIRED_COUYNT = 32,
        LIVE_SLOT_COUNT = 33,

        HASH_TOMBSTONE_COUNT = 34,
        HASH_COMPACTION_COUNT = 35,
        WORK_QUEUE_OCCUPANCY = 36,
        READY_QUEUE_OCCUPANCY = 37,

        BACKOFF_SPIN_LIMIT = 38,
        BACKOFF_YIELD_LIMIT = 39,
        INITIALIZATION_STATE = 40,
        HAS_COMPACTION_INFLIGHT = 41,
        RELATION_RECLAIM_COUNT = 42,
        WORK_QUEUE_DROPPED_COUNT = 43,
        THREAD_TABLE_CAPACITY = 44,
        THREAD_ACTIVE_COUNT = 45,
        THREAD_REGISTRATION_FAILURE = 46,
        RELATION_TOMBSTONE_COUNT = 47,
        RELATION_UNLINK_FAILURES = 48,
        WORK_QUEUE_CLAIM = 49,

        RESERVED_50 = 50,

        EOF_FABRIC_HEADER = 95

    };

    enum class TableIdOfAPCFabric : uint16_t //14
    {
        GLOBAL_CONFIG = 0,
        TABLE_DIRECTORY,
        SLOT_DIRECTORY,
        BRANCH_HASH,
        LOGICAL_HASH,
        SHARED_HASH,
        RELATION_TABLE,
        FREE_RETIRE_TABLE,
        READY_QUEUE,
        WORK_QUEUE,
        DEVICE_VIEW_TABLE,
        THREAD_TABLE,
        SEGMENT_POOL,
        COUNT
    };

    enum class SLotStateOfAPCFabric : uint32_t
    {
        FREE = 0,
        CLAIMED = 1,
        LIVE = 2,
        FAULTY = 3
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


}
