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
        HOST_PROCESSOR = 0,
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

    struct RelationRecordOfAPCFabric
    {
        std::atomic<uint64_t> SourcePackedHandle{UNSIGNED_ZERO};
        std::atomic<uint64_t> TargetPackedHandle{UNSIGNED_ZERO};
        std::atomic<uint32_t> SourceBranchId{UNSIGNED_ZERO};
        std::atomic<uint32_t> TargetBranchId{UNSIGNED_ZERO};
        std::atomic<uint32_t> WeightBranchOrViewId{UNSIGNED_ZERO};
        std::atomic<uint32_t> TensorReferenceId{UNSIGNED_ZERO};
        std::atomic<uint32_t> OutBoundNextRelation{APCDataStructure::BRANCH_SENTINAL};
        std::atomic<uint32_t> InBoundNextRelation{APCDataStructure::BRANCH_SENTINAL};
        std::atomic<uint32_t> FreeNextRelation{APCDataStructure::BRANCH_SENTINAL};
        std::atomic<uint32_t> RetireEpochLow32{UNSIGNED_ZERO};
        std::atomic<uint32_t> IntraRelationshipFlags{UNSIGNED_ZERO};

        IntraRelationshipKindOfAPCFabric RelationKind{IntraRelationshipKindOfAPCFabric::NONE};
        APCPagedNodeSegmentClasses SourceRegion{APCPagedNodeSegmentClasses::NANNULL};
        APCPagedNodeSegmentClasses TargetRegion{APCPagedNodeSegmentClasses::NANNULL};
        PriorityPhysics PriorityPolicy{PriorityPhysics::INHERIT_SOURCE_PRIORITY};
        APCFebricClockPolicy ClockPolicy{APCFebricClockPolicy::ACCEPT_NEWER_ONLY};
        APCFabricDeviceHint DeviceHint{APCFabricDeviceHint::HOST_PROCESSOR};

        void ResetRelationRecord() noexcept
        {
            SourcePackedHandle.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            TargetPackedHandle.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            SourceBranchId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            TargetBranchId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            WeightBranchOrViewId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            TensorReferenceId.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            OutBoundNextRelation.store(APCDataStructure::BRANCH_SENTINAL, MoStoreUnSeq_);
            InBoundNextRelation.store(APCDataStructure::BRANCH_SENTINAL, MoStoreUnSeq_);
            RetireEpochLow32.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            RelationKind = IntraRelationshipKindOfAPCFabric::NONE;
            IntraRelationshipFlags.store(UNSIGNED_ZERO, MoStoreUnSeq_);
            SourceRegion = APCPagedNodeSegmentClasses::NANNULL;
            TargetRegion = APCPagedNodeSegmentClasses::NANNULL;
            PriorityPolicy = PriorityPhysics::INHERIT_SOURCE_PRIORITY;
            ClockPolicy = APCFebricClockPolicy::ACCEPT_NEWER_ONLY;
            DeviceHint = APCFabricDeviceHint::HOST_PROCESSOR;

        }


    };

    struct SharedChainRecordOfAPCFabric
    {
        std::atomic<uint32_t> SharedId{UNSIGNED_ZERO};
        std::atomic<uint64_t> HeadPackedHandle{UNSIGNED_ZERO};
        std::atomic<uint64_t> TailPackedHandle{UNSIGNED_ZERO};
        std::atomic<uint32_t> SegmentCount{UNSIGNED_ZERO};
        std::atomic<uint32_t> TotalPayload{UNSIGNED_ZERO};
        std::atomic<uint32_t> ActivePayload{UNSIGNED_ZERO};
        std::atomic<uint32_t> Pressure{UNSIGNED_ZERO};
        std::atomic<uint32_t> FirstRelatgionOfAPC{APCDataStructure::METACELL_COUNT};
        std::atomic<uint32_t> IntraRelationshipFlags{UNSIGNED_ZERO};
    };

    struct FabricViewOfAPC
    {
        std::atomic<uint64_t> ViewId{UNSIGNED_ZERO};
        std::atomic<uint64_t> SourcePackedHandle{UNSIGNED_ZERO};
        APCPagedNodeSegmentClasses Region{APCPagedNodeSegmentClasses::NANNULL};
        APCFabricDeviceHint TargetDevice{APCFabricDeviceHint::HOST_PROCESSOR};
        std::atomic<uint32_t> Count{UNSIGNED_ZERO};
        std::atomic<uint32_t> Capacity{UNSIGNED_ZERO};
        std::atomic<uint32_t> ClockSnapshot{UNSIGNED_ZERO};
        std::atomic<uint32_t> MetaSnapshot{UNSIGNED_ZERO};
        std::atomic<uint32_t> FirstCellOffset{UNSIGNED_ZERO};
    };

    struct WorkRecordOfAPCFabric
    {
        std::atomic<uint32_t> BranchId{UNSIGNED_ZERO};
        std::atomic<uint32_t> WorkKind{UNSIGNED_ZERO};
        std::atomic<uint32_t> PageClass{UNSIGNED_ZERO};
        std::atomic<uint64_t> PackedHandle{UNSIGNED_ZERO};
    };


    ///old PackedCellContainerManager
    class AdaptivePackedCellContainer;

    class PackedCellContainerManager
    {
        public :
            struct ThreadHandlePCCM
            {
                size_t QSBRIdx = SIZE_MAX;
                std::atomic<uint64_t>* WaitSlotPtr = nullptr;
                uint64_t NodeTokenOfAPC = 0;
            };

            static PackedCellContainerManager& Instance()
            {
                // intentionally leaked singleton to avoid static-destruction order problems
                static PackedCellContainerManager* inst = []() {
                    return new PackedCellContainerManager();
                }();
                return *inst;
            }

            void StartAPCManager();
            void StopAPCManager();
            ThreadHandlePCCM RegisterAPCThread();
            void UnRegisterAPCThread(const ThreadHandlePCCM& thread_handle) noexcept;
            void EnterCriticalContainer(const ThreadHandlePCCM& thread_handle) noexcept;
            void ExtitCriticalContainer(const ThreadHandlePCCM& thread_handle) noexcept;

            void NotifySlotIdxOfAPC(size_t idx, uint64_t thread_token) noexcept;
            void NotifyAllActiveAPCThreads(uint64_t thread_token) noexcept;

            void RegisterAPCFromManager_(AdaptivePackedCellContainer* adaptive_p_c_ptr) noexcept;
            void UnRegisterAPCFromManager_(AdaptivePackedCellContainer* adaptive_p_c_ptr) noexcept;
            void RequestAPCSegmentCreationFromManager_(AdaptivePackedCellContainer* adaptive_p_c_ptr) noexcept;
            void ReclaimationRequestOfAPCSegmentFromManager_(AdaptivePackedCellContainer* adaptive_p_c_ptr) noexcept;
            AdaptivePackedCellContainer* GetAPCPtrFromBranchId(uint32_t) noexcept;
            uint64_t ComputeMinThreadEpoch() const noexcept;


            void PushTOAPCManagerStack_(
                std::atomic<AdaptivePackedCellContainer*>& head_stack,
                AdaptivePackedCellContainer* apc_ptr, bool is_cleanup_stack
            ) noexcept;

            AdaptivePackedCellContainer* PopAllAPC_S(std::atomic<AdaptivePackedCellContainer*>& head_stack) noexcept
            {
                return head_stack.exchange(nullptr, std::memory_order_acq_rel);
            }

            AtomicAdaptiveBackoff& GetManagersAdaptiveBackoff() noexcept
            {
                return AdaptiveBackOffOfAPCManager_;
            }

            AtomicAdaptiveBackoff::PCBDecision GetCellsAdaptiveBackoffFromManager(packed64_t packed_cell) noexcept
            {
                return AdaptiveBackOffOfAPCManager_.AdaptiveBackOffPacked(packed_cell);
            }


            void UsePreAllocatedNodePoolOfAdaptivePackedCellContainer(size_t pool_size_of_preallocated_adaptive_packed_cell_container) noexcept;

        private :
            PackedCellContainerManager();
            ~PackedCellContainerManager();
            PackedCellContainerManager(const PackedCellContainerManager&) = delete;
            PackedCellContainerManager& operator=(const PackedCellContainerManager&) = delete;

            std::atomic<AdaptivePackedCellContainer*> RegistryHeadAPC_{nullptr};
            std::atomic<AdaptivePackedCellContainer*> WorkStackHeadAPC_{nullptr};
            std::atomic<AdaptivePackedCellContainer*> CleanupStackHeadAPC_{nullptr};

            std::atomic<size_t>ThreadFreelistHead_{SIZE_MAX};
            size_t MaxThreads_ = 4096;
            size_t  ThreadTableCapacity_{0};
            std::unique_ptr<std::atomic<size_t>[]> ThreadNextIdxPtr_;
            std::unique_ptr<std::atomic<uint64_t>[]> ThreadEpochArrayPtr_;
            std::unique_ptr<std::atomic<uint64_t>[]> ThreadWaitSlotArrayPtr_;
            // bool UseNodePool_ = false;

            std::atomic<bool> RunningManager_{false};
            std::thread ManagerThread_;
            std::atomic<uint64_t>GlobalEpoch_{1};

            AtomicAdaptiveBackoff AdaptiveBackOffOfAPCManager_;
            std::atomic<uint64_t> ManagerWakeCounter_{0};


            size_t AllocateThreadSlots_() noexcept;
            void FreeThreadSlots_(size_t idx) noexcept;
            void PushFreeThreadIndex_(size_t idx) noexcept;
            size_t PopFreeThreadIndex_() noexcept;


            void ManagerManinLoop_() noexcept;
            void ProcessRemainingWorkOfAPC_(AdaptivePackedCellContainer* batch_head_of_adaptive_packed_cell_container_ptr, uint64_t min_epoch = 64) noexcept;
            void ProcessCleanUpBatchOfAdaptivePackedCellContainer_(AdaptivePackedCellContainer* batch_head_of_adaptive_packed_cell_container, uint64_t min_epoch) noexcept;
            void TryCompactRegistryInPlace_() noexcept;
            static constexpr uint64_t THREAD_SENTINEL_ = std::numeric_limits<uint64_t>::max();

    };
}


