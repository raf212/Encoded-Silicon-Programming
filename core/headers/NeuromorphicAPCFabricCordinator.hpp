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

    class AdaptivePackedCellContainer;

    class NeuromorphicAPCFabricCordinator
    {
        public :

            static constexpr size_t DEFAULT_FABRIC_SLAM_BYTES = 1 << 20;
            static constexpr size_t DEFAULT_TABLE_CAPACITY = 4096;
            static constexpr uint32_t DEFAULT_RELATION_CAPACITY = 16384u;
            static constexpr uint32_t DEFAULT_WORK_RING_CAPACITY = 8192u;
            static constexpr uint32_t DEFAULT_ACTIVE_VIEW_CAPACITY = 1024u;
            static constexpr uint32_t DEFAULT_ACTIVE_VIEW_CELL_CAPACITY = 65536u;

            bool IsFabricInitialized() const noexcept;
            
            bool IsFabricShuttingDown() const noexcept;

            size_t FebricSlotCapacityCell() noexcept;

            bool InitializeStorageFabric(
                size_t slot_cell_capacity = MINIMUM_BRANCH_CAPACITY,
                size_t slab_bytes = DEFAULT_FABRIC_SLAM_BYTES,
                uint32_t branch_table_capacity = DEFAULT_TABLE_CAPACITY,
                uint32_t logical_table_capacity = DEFAULT_TABLE_CAPACITY,
                uint32_t shared_table_capacity = DEFAULT_TABLE_CAPACITY,
                uint32_t relation_capacity = DEFAULT_RELATION_CAPACITY,
                uint32_t work_Ring_capacity = DEFAULT_WORK_RING_CAPACITY,
                uint32_t view_capacity = DEFAULT_ACTIVE_VIEW_CAPACITY,
                uint32_t view_cell_capacity = DEFAULT_ACTIVE_VIEW_CELL_CAPACITY
            ); 

            AdaptivePackedCellContainer* AllocateAPCObjectFromFabricManager() noexcept;
            
            bool BindExternalRootAPC(AdaptivePackedCellContainer* apc_ptr, size_t wanted_capacity) noexcept;

            HandleOfAPC ResolveBranchHandle(uint32_t branch_id) noexcept;

            AdaptivePackedCellContainer* ResolveAPCPtr(HandleOfAPC handle) noexcept;

            uint32_t CreateRelation(
                HandleOfAPC source_handle,
                HandleOfAPC target_handle,
                IntraRelationshipKindOfAPCFabric relation_kind,                                                                                                                                                                                                                                                     
                APCPagedNodeSegmentClasses source_region,
                APCPagedNodeSegmentClasses target_kind,
                PriorityPhysics priority_policy = PriorityPhysics::INHERIT_SOURCE_PRIORITY,
                APCFabricDeviceHint device_hint = APCFabricDeviceHint::HOST_PROCESSOR,
                uint32_t weight_or_view_id = UNSIGNED_ZERO,
                uint32_t tensor_referance_id = UNSIGNED_ZERO
            ) noexcept;

            bool LinkSharedNext(HandleOfAPC source, HandleOfAPC target) noexcept;

            SharedChainRecordOfAPCFabric* ResolveSharedChainRecordPtr(uint32_t shared_id) noexcept;

            bool UpdateSharedChainAfterAppend(HandleOfAPC root, HandleOfAPC tail, HandleOfAPC child) noexcept;

            bool EnqueueFabricWork(
                uint32_t branch_id,
                WorkRecordOfAPCFabric work_kind,
                APCPagedNodeSegmentClasses region_kind = APCPagedNodeSegmentClasses::FREE_SLOT,
                uint64_t packed_handle = UNSIGNED_ZERO
            );

            uint32_t BuildFloat32Activeview(
                HandleOfAPC source,
                APCPagedNodeSegmentClasses page_class,
                APCFabricDeviceHint target_device = APCFabricDeviceHint::SYCL_SHARED_USM  
            ) noexcept;

            FabricViewOfAPC* GetActiveAPCViewPtr(uint32_t view_id) noexcept;

            uint32_t* GetCellIdxPtr(uint32_t view_id) noexcept;

            float* GetViewFloat32Ptr(uint32_t view_idd) noexcept;

            packed64_t* GetViewOfOrriginalPackedPtr(uint32_t view_id) noexcept;

            uint16_t* GetViewClock16Ptr(uint32_t view_id) noexcept;

            uint16_t* GetViewMeta16Ptr(uint32_t view_id) noexcept;

        private:

            size_t FabricSlotCapacityCells_{UNSIGNED_ZERO};
            size_t FabricSlotCount_{UNSIGNED_ZERO};
            size_t FabricSlabCells_{UNSIGNED_ZERO};
            uint32_t BranchTableCapacity_{UNSIGNED_ZERO};
            uint32_t LogicalTableCapacity_{UNSIGNED_ZERO};
            uint32_t SharedTableCapacity_{UNSIGNED_ZERO};
            uint32_t RelationTableCapacity_{UNSIGNED_ZERO};
            uint32_t WorkingTableCapacity_{UNSIGNED_ZERO};
            uint32_t ActiveViewCapacity_{UNSIGNED_ZERO};
            uint32_t CellInActiveCapacity{UNSIGNED_ZERO};

            std::unique_ptr<std::atomic<packed64_t>[]> FabricCellsPtrs_;
            std::unique_ptr<AdaptivePackedCellContainer[]> FabricObjectPoolPtrs_;
            std::unique_ptr<SlotRecordOfAPCFabric[]> SlotTablePtrs_;
            std::unique_ptr<HashEntryOfAPC[]> BranchTablePtrs_;
            std::unique_ptr<HashEntryOfAPC[]> LogicalTablePtrs_;
            std::unique_ptr<SharedChainRecordOfAPCFabric[]> SharedTablePtrs_;
            std::unique_ptr<RelationRecordOfAPCFabric[]> RelationTablePtrs_;
            std::unique_ptr<WorkRecordOfAPCFabric[]> WorkRingPtrs_;
            std::unique_ptr<FabricViewOfAPC[]> FabricActiveViewPtrs_;

            std::unique_ptr<uint32_t[]> CellIndicesViewPtrs_;
            std::unique_ptr <float []> Float32CellViewPtrs_;
            std::unique_ptr<packed64_t[]> OriginalPacked64ViewPtrs_;
            std::unique_ptr<uint16_t[]> Clock16ViewPtrs_;
            std::unique_ptr<uint16_t[]> Meta16ViewPtrs_;

            std::atomic<bool> FabricInitialized_{false};
            std::atomic<bool> FabricShuttingDown_{false};
            std::atomic<uint64_t> WorkWriteCursor_{UNSIGNED_ZERO};
            std::atomic<uint64_t> WorkReadCursor_{UNSIGNED_ZERO};
            std::atomic<uint32_t> VuewCellCursor_{UNSIGNED_ZERO};
            std::atomic<size_t> FreeSlotHeadOfFabricPtr_{SIZE_MAX};
            std::atomic<uint32_t> RelationFreeHead_{APCDataStructure::BRANCH_SENTINAL};
            std::atomic<uint32_t> NextValidId_{APCDataStructure::BRANCH_VERSION};

            static uint32_t HashUnsigned32_(uint32_t given_value) noexcept;

            static uint32_t NextPowerOf2Unsigned32_(uint32_t given_value) noexcept;

            size_t PopFreeSlot_() noexcept;

            void PushFreeSlot_(size_t slot) noexcept;

            bool InsertAHash_(
                HashEntryOfAPC* table_ptr, uint32_t capacity, 
                uint32_t hash_key, uint64_t packed_handle
            ) noexcept;

            uint64_t LookupAHashKey_(
                HashEntryOfAPC* table_ptr, uint32_t capacity,
                uint32_t key
            ) noexcept;

            bool EraseAHash_(
                HashEntryOfAPC* table_ptr, uint32_t capacity,
                uint32_t key, uint64_t expected_handle = UNSIGNED_ZERO
            ) noexcept;

            FabricViewOfAPC* GetOtCreateSharedChain_(uint32_t shared_id) noexcept;

            uint32_t PopFreeRelationSlot_() noexcept;

            uint32_t PushFreeRelationSlot_(uint32_t slot) noexcept;

            void ResetRelationSlot_(uint32_t slot) noexcept;

            bool EnqueueFabricWork_(
                uint32_t branch_id, APCFabricWorkKind work_kind,
                APCPagedNodeSegmentClasses page_class, uint64_t packed_handle
            ) noexcept;


            bool DequeueFabricwork_(APCFabricWorkKind& out_record) noexcept;

            void ProcessFabricWork_(const WorkRecordOfAPCFabric& work_record, uint64_t min_epoch) noexcept;

    };
}


