#pragma once 
#include "AdaptivePackedCellContainer/SegmentIODefinition.hpp"
# include <functional>

namespace PredictedAdaptedEncoding
{

    enum class APCFabricSlotState : uint32_t
    {
        EMPTY = 0,
        REUSEABLE = 1,
        ALLOCATED = 2,
        REGISTERED = 3,
        ACTIVE = 4,
        QUIESCING = 5,
        DEAD = 6,
        RETIRED_WAITING_EPOCH = 7
    };

    enum class IntraRelationshipKindOfAPCFabric : uint32_t
    {
        NONE = 0,
        SHARED_NEXT = 1u,
        SHARED_PREVIOUS = 1u << 2,
        FEEDFORWARD_OUT = 1u << 3,
        FEEDFORWARD_IN = 1u << 4,
        FEEDBACKWARD_OUT = 1u << 5,
        FEEDBACKWARD_IN = 1u << 6,
        LATERAL_0 = 1u << 7,
        LATERAL_1 = 1u << 8,
        STATE_BIND = 1u << 9,
        ERROR_BIND = 1u << 10,
        AUX_BIND = 1u << 11,
        RETIRE_LINK = 1u << 12,
        DEVICE_VIEW_LINK = 1u << 13
    };

    enum class APCFebricClockPolicy : uint16_t
    {
        ACCEPT_NEWER_ONLY = 0, 
        ACCEPT_EQUAL_OR_NEWER = 1,
        ACCEPT_ANY_BUT_MARK_ORDER = 2,
        STRICT_WINDOW_CLOCK16 = 3
    };

    enum class APCFabricDeviceHint : uint16_t
    {
        HOST_FAST_CPU = 0,
        SYCL_SHARED_USM = 1,
        SYCL_DEVICE_USM = 2,
        ONEDPL_PARALLEL_SCAN_SORT_REDUCE = 3,
        ONEDNN_PRIMITIVE = 4,
        ONEMKL_DENSE = 5,
        ONEMKL_SPARSE = 6
    };

    enum class APCFabricWorkKind : uint32_t
    {
        NONE = 0u,
        CREATE_SHARED_SEGMENT = 1u,
        REBUILD_READY_MASK = 2u,
        REBUILD_CHAIN_METADATA = 3u,
        BUILD_FLOAT32_VIEW = 4u,
        RETIRE_HANDLE = 5u
    };

    struct HandleOfAPC
    {
        uint32_t SlabId{UNSIGNED_ZERO};
        uint32_t SlotIndex{APCDataStructure::BRANCH_SENTINAL};
        uint32_t Genaration{UNSIGNED_ZERO};
        uint32_t flags{UNSIGNED_ZERO};
        uint32_t BranchId{UNSIGNED_ZERO};
        uint32_t LogicalId{UNSIGNED_ZERO};
        uint32_t ShharedId{UNSIGNED_ZERO};

        bool IsInvalidHandler() const noexcept
        {
            return SlotIndex != APCDataStructure::BRANCH_SENTINAL && Genaration != UNSIGNED_ZERO && BranchId != UNSIGNED_ZERO;
        }
    };

    struct HashEntryOfAPC
    {
        std::atomic<uint32_t> HashKey{UNSIGNED_ZERO};
        std::atomic<uint64_t> PackedHandle{UNSIGNED_ZERO};

        void RestHashEntryOfAPC() noexcept
        {
            PackedHandle.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            HashKey.store(UNSIGNED_ZERO, MoStoreUnSeq_);
        }
    };

    struct SlotRecordOfAPCFabric
    {
        std::atomic<uint32_t> StateOfSlot{static_cast<uint32_t>(APCFabricSlotState::EMPTY)};
        std::atomic<uint32_t> BranchId{UNSIGNED_ZERO};
        std::atomic<uint32_t> LogicalId{UNSIGNED_ZERO};
        std::atomic<uint32_t> SharedId{UNSIGNED_ZERO};
        std::atomic<uint32_t> IntraRelationshipFlags{UNSIGNED_ZERO};
        std::atomic<uint32_t> Generation{UNSIGNED_ZERO};
        std::atomic<uint32_t> RetireEpochLow32{UNSIGNED_ZERO};
        std::atomic<size_t> NextFree{SIZE_MAX};
        std::atomic<uintptr_t> ObjectProbableAPCPtr{UNSIGNED_ZERO};
        std::atomic<uint32_t> FirstOutboundRelation{UNSIGNED_ZERO};
        std::atomic<uint32_t> FirstInboundRelation{UNSIGNED_ZERO};

        void ResetSlotRecord() noexcept
        {
            BranchId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            LogicalId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            SharedId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            IntraRelationshipFlags.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            RetireEpochLow32.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            FirstInboundRelation.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            StateOfSlot.store(static_cast<uint32_t>(APCFabricSlotState::REUSEABLE), MoStoreUnSeq_);
        }
    };

}


