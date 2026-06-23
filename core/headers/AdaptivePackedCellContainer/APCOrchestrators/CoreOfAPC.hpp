
#pragma once 
#include <array>
#include <utility>
#include "../../PackedCell/InternalCellModes/Mode48CellModels.hpp"
#include "../../PackedCell/InternalCellModes/Mode32CellModels.hpp"

namespace PredictedAdaptedEncoding
{
    enum class MetaIndexOfAPCNode : size_t
    {
        // identity
        MAGIC_ID = 0,
        MANAGER_CONTROL_FLAGS = 1,
        SEGMENT_CONF_FLAGS = 2,

        // logical-node identity
        BRANCH_ID = 3,
        LOGICAL_NODE_ID = 4,
        SHARED_ID = 5,
        SHARED_PREVIOUS_ID = 6,
        SHARED_NEXT_ID = 7,

        // runtime-control
        BRANCH_DEPTH = 8,
        MAX_DEPTH = 9,
        BRANCH_PRIORITY = 10,
        CURRENT_ACTIVE_THREADS = 11,
        SPLIT_THRESHOLD_PERCENTAGE = 12,
        SEGMENT_KIND = 13,

        // payload / capacity
        TOTAL_CAPACITY_OF_THIS_SEGEMENT = 14,

        // timing
        LOCAL_CLOCK48 = 15,
        LAST_SPLIT_EPOCH = 16,

        // region summary
        REGION_DIR_COUNT = 17,
        REGION_SIZE = 18,
        REGION_COUNT = 19,
        PAGED_NODE_READY_BIT = 20,
        PRODUCER_BLOCK_SIZE = 21,
        BACKGROUND_EPOCH_ADVANCE_MS = 22,
        DEFINED_MODE_OF_CURRENT_APC = 23,
        RETIRE_BRANCH_THRASHOLD = 24,
        PRODUCER_CURSOR_PLACEMENT = 25,
        CONSUMER_CURSORE_PLACEMENT = 26,
        CURRENTLY_OWNED = 27,
        TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH = 28,
        NODE_GROUP_SIZE = 29,
        NODE_AUX_PARAM_U32 = 30,

        // graph ports
        FEEDFORWARD_IN_TARGET_ID = 31,
        FEEDFORWARD_OUT_TARGET_ID = 32,
        FEEDBACKWARD_IN_TARGET_ID = 33,
        FEEDBACKWARD_OUT_TARGET_ID = 34,
        LATERAL_0_TARGET_ID = 35,
        LATERAL_1_TARGET_ID = 36,
        NODE_ROLE_FLAGS_RESERVED = 37,
        LAST_ACCEPTED_FEED_FORWARD_CLOCK16 = 38,
        LAST_EMITTED_FEED_FORWARD_CLOCK16 = 39,
        LAST_ACCEPTED_FEED_BACKWARD_CLOCK16 = 40,
        LAST_EMITTED_FEED_BACKWARD_CLOCK16 = 41,
        NODE_COMPUTE_KIND = 42,

        // payload bounds versions
        MESSAGE_FEEDFORWARD_BOUNDS_VERSION = 43,
        MESSAGE_FEEDBACKWARD_BOUNDS_VERSION = 44,
        LATERAL_BOUNDS_VERSION = 45,
        STATE_BOUNDS_VERSION = 46,
        ERROR_BOUNDS_VERSION = 47,
        EDGE_DESCRIPTIOR_BOUNDS_VERSION = 48,
        WEIGHT_BOUNDS_VERSION = 49,
        AUX_BOUNDS_VERSION = 50,
        HETEROGENOUS_RAW_MEMORY_BOUNDS_VERSION = 51,
        SLOT_TABLE_DESCRIPTOR_BOUNDS_VERSION = 52,
        PAIRED_POINTER_IN_MEMORY_BOUNDS_VERSION = 53,
        UNDEFINED_BOUNDS_VERSION = 54,
        FREE_BOUNDS_VERSION = 55,
        GLOBAL_CURRENT_VERSION = 56,

        // region occupancy
        REGION_OCCUPANCY_NONE = 57,
        REGION_OCCUPANCY_FF = 58,
        REGION_OCCUPANCY_FB = 59,
        REGION_OCCUPANCY_LATERAL = 60,
        REGION_OCCUPANCY_STATE = 61,
        REGION_OCCUPANCY_ERROR = 62,
        REGION_OCCUPANCY_EDGE = 63,
        REGION_OCCUPANCY_WEIGHT = 64,
        REGION_OCCUPANCY_CONTROL = 65,
        REGION_OCCUPANCY_AUX = 66,
        REGION_OCCUPANCY_HETEROGENOUS_MEMORY_MAYBE_PAIRED_POINTER_OR_RAW_APC_SEGMENT = 67,
        REGION_OCCUPANCY_PAIRED_POINTER_LOCAL_MEMORY = 68,
        REGION_OCCUPANCY_PAIRED_POINTER_DISTANCE_MEMORY = 69,
        REGION_OCCUPANCY_FREE = 70,
        REGION_OCCUPANCY_UNDEFINED = 71,
        REGION_OCCUPANCY_NANNULL = 72,

        // retire epoch
        RETIRE_EPOCH_LOW32 = 73,
        RETIRE_EPOCH_HIGH32 = 74,

        // combined occupancy
        COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48 = 75,

        // edge / weight tables
        EDGE_TABLE_COUNT = 76,
        WEIGHT_TABLE_COUNT = 77,

        // amount / end marker
        EOF_APC_HEADER = 95
    };

    struct APCDataStructure 
    {
        static constexpr size_t METACELL_COUNT = 96;
        static constexpr uint32_t BRANCH_MAGIC = 0x41504342u;//big-endian
        static constexpr uint32_t EOF_HEADER = 0x72616600;//big-endian
        static constexpr uint16_t BRANCH_VERSION = 1u;
        static constexpr packed64_t PACKED_CELL_SENTENAL = UINT64_MAX;
        static constexpr uint32_t APC_MAX_LENGTH_OR_COUNTER = UINT16_MAX - 1;
        static constexpr uint32_t APC_INDEX_SENTINAL = UINT16_MAX;
        static constexpr uint32_t BRANCH_SENTINAL = IN_CELL_VALUE_MODE32_SENTINAL;
        static constexpr size_t APC_CACHELINE_SIZE = 64u;
        static constexpr size_t APC_SIZE_SENTINAL = SIZE_MAX;

        static constexpr uint32_t FABRIC_MAGIC = 0x41504643u;
        static constexpr uint32_t FABRIC_META_EOF = 0x41474946u;

protected:
        static constexpr bool DoseU32FitsInU16_(uint32_t value) noexcept
        {
            return value <= APC_MAX_LENGTH_OR_COUNTER;
        }

        static packed64_t* AllocateAlignedRawPackedCells_(size_t count)
        {
            if (count == UNSIGNED_ZERO)
            {
                return nullptr;
            }

            void* raw_ptr = nullptr;

            try
            {
                raw_ptr = ::operator new[](sizeof(packed64_t) * count, std::align_val_t{APC_CACHELINE_SIZE});
            }
            catch(...)
            {
                return nullptr;
            }
            
            auto* packed_cells = static_cast<packed64_t*>(raw_ptr);
            for (size_t i = 0; i < count; i++)
            {
                packed_cells[i] = PackedCell64_t::PACKED_CELL_SENTINAL;
            }
            return packed_cells;
        }

        static constexpr void FreeAlignedRawPackedCells_(packed64_t* backing_ptr) noexcept
        {
            if (!backing_ptr)
            {
                return;
            }
            ::operator delete[](static_cast<void*>(backing_ptr), std::align_val_t{APC_CACHELINE_SIZE});
        }
    };

    struct ContainerConf
    {
        PackedMode InitialMode = PackedMode::MODEL32;
        size_t ProducerBlockSize = MIN_PRODUCER_BLOCK_SIZE;
        size_t RegionSize = MIN_REGION_SIZE;
        uint32_t RetireBatchThreshold = MIN_RETIRE_BATCH_THRESHOLD;
        uint32_t BackgroundEpochAdvanceMS = MIN_BACKGROUND_EPOCH_MS;
        bool EnableBranching = true;
        uint32_t BranchSplitThresholdPercentage = INITIAL_BRANCH_SPLIT_THRESHOLD_PERCENTAGE;
        uint32_t BranchMaxDepth = MAX_BRANCH_DEPTH;
        size_t BranchMinChildCapacity = MINIMUM_BRANCH_CAPACITY;
        uint32_t NodeGroupSize = 1u;

        enum class APCSegmentExtendOrder : uint8_t
        {
            FIFO = 0,
            PRIORITY = 1,
            RANDOM = 2
        };
    };


    enum class PublishStatus : uint8_t
    {
        OK = 0,
        FULL = 1,
        INVALID = 2
    };

    struct PublishResult
    {
        PublishStatus ResultStatus{PublishStatus::INVALID};
        size_t Index{APCDataStructure::APC_SIZE_SENTINAL};
    };

}