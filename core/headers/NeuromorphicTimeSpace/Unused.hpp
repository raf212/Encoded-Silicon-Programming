#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{
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



}