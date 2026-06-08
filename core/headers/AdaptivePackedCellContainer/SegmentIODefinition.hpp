#pragma once
#include "PackedCell/CoreCellDefination.hpp"
#include "APCSpikeController/AtomicAdaptiveBackoff.hpp"

namespace PredictedAdaptedEncoding
{


class SegmentIODefinition : public APCDataStructure
{
public:
    SegmentIODefinition() noexcept = default;
    
    std::atomic<packed64_t>* BackingPtr{nullptr};

    enum class ControlEnumOfAPCSegment : uint32_t
    {
        NONE = 0u,
        ENABLE_BRANCHING = 1u << 0,
        HAS_REGION_INDEX =  1u << 1,
        SATURATED = 1u << 2,
        SPLIT_INFLIGHT = 1u << 3,
        IS_GRAPH_NODE = 1u << 4,
        IS_SHARED_ROOT = 1u << 5,
        IS_SHARED_MAMBER = 1u << 6,
        HAS_SHARED_NEXT = 1u << 7,
        HAS_SHARED_PREVIOUS = 1u << 8,
        HAS_LAYOUT_DIR = 1u << 9,
        HAS_EDGE_TABLE = 1u << 10,
        HAS_WEIGHT_TABLE = 1u << 11,
        LAYOUT_MUTATION_INFLIGHT = 1u << 12
    };

    enum class ManagerControlFlagBits : uint32_t
    {
        NONE = 0u,
        REGISTERED_APC = 1U << 0,
        DEAD_APC = 1U << 1,
        RECLAIMATION_REQUST_FOR_JUST_THIS_APC = 1u << 2,
        RECLAIMATION_REQUEST_FOR_WHOLE_CHAIN = 1u << 3,
        REQUEST_NEW_SEGMENTATION = 1u << 4,
        IN_WORK_STACK = 1u << 5,
        IN_CLEANUP_STACK = 1u << 6
    };

    bool ValidMetaIdx(MetaIndexOfAPCNode idx) noexcept
    {
        return BackingPtr && static_cast<size_t>(idx) < BranchCapacity_ && static_cast<size_t>(idx) < METACELL_COUNT;
    }

    packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept
    {
        if (ValidMetaIdx(idx))
        {
            return BackingPtr[static_cast<size_t>(idx)].load(MoLoad_);
        }
        return PACKED_CELL_SENTENAL;
    }
    
protected:
    Timer48 LocalTimer48_;
    AtomicAdaptiveBackoff* AdaptiveBackoffOfAPCPtr_{nullptr};
    std::unique_ptr<MasterClockConf> OwnedMasterClockConfPtr_;
    size_t BranchCapacity_{0};

    bool TurnOnMultipleSegmentFlagsAtOnce_(uint32_t use_or_between_flags = UNSIGNED_ZERO) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(use_or_between_flags, UNSIGNED_ZERO, MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
    }

    bool UpdateAPCModeFlagsInHeader_(uint32_t flags_to_turn_on = UNSIGNED_ZERO, uint32_t flags_to_turn_off = UNSIGNED_ZERO, MetaIndexOfAPCNode desired_flag_idx = MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS) noexcept;

    bool WriteBoundsPairToHeader_(
        const LayoutBoundsOfSingleRelNodeClass layout_bound,
        std::optional<uint16_t> version_number = std::nullopt,
        bool caller_holds_the_flag = false
    ) noexcept;

    void BuidDefaultLayoutPlan_(CompleteAPCNodeRegionsLayout& full_layout) noexcept;

    void InitDefaultAPCSegmentedNodeLayout_() noexcept;

    std::optional<uint16_t> ReadGlobalLayoutVersion_() noexcept;

    bool WriteGlobalLayoutVersion_(uint16_t layout_version) noexcept;
    
    std::optional<uint16_t> NextGlobalLayoutVersion_() noexcept;

    bool WriteAllRegionsLayoutToHeader_(
        const CompleteAPCNodeRegionsLayout& full_layout,
        std::optional<uint16_t> forced_version_number = std::nullopt,
        bool caller_holds_the_flag = false
    ) noexcept;

    bool TurnOnReadyBitForDesiredPagedNode_(APCPagedNodeSegmentClasses desired_region_class) noexcept;

    bool ClearTheDesiredPagedNodeReadyBit_(APCPagedNodeSegmentClasses desired_region_class) noexcept;

    bool ClearMultipleControlFlags_(uint32_t use_or_between_flags = UNSIGNED_ZERO) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(UNSIGNED_ZERO, use_or_between_flags);
    }

    bool ResetALLOccupancy16x3ModelToZero_() noexcept;

    std::optional<LayoutBoundsOfSingleRelNodeClass> GetVirtualControlSlotLayout_() noexcept;

    bool ApplyRegionalMigrationOccupancyTransitionCell(
        LocalityPolicy from_locality_of_source_cell,
        LocalityPolicy destination_locality_of_source_cell,
        APCPagedNodeSegmentClasses source_page_class,
        APCPagedNodeSegmentClasses destination_page_class
    ) noexcept;

    /// @brief APC META USES TypeFamily::VALUE32 path WITH::AccessContractOfValue
    /// @param idx 
    /// @param value32 
    /// @param priority 
    /// @param page_class 
    void WriteMetaCellMode32_(
        MetaIndexOfAPCNode idx,
        uint32_t value32,
        PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST,
        APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::CONTROL_SLOT
    ) noexcept
    {
        size_t index = static_cast<size_t>(idx);
        if (!ValidMetaIdx(idx))
        {
            return;
        }

        const packed64_t packed_cell = PackedCell64_t::MakeTypedAPCValidPackedCell(
            TypeFamily::VALUE32,
            AccessContractOfValue::CAS_RMW,
            page_class,
            LocalityPolicy::PUBLISHED,
            InternalDataTypePolicy::UnsignedPCellDataType,
            priority,
            value32,
            UNSIGNED_ZERO
        );


        BackingPtr[index].store(packed_cell, MoStoreSeq_);
        BackingPtr[index].notify_all();
    }

    void WrireAPCMetaModel_48t(
        MetaIndexOfAPCNode idx,
        uint64_t raw48_value,
        Model48Subclass sub_class = Model48Subclass::SELF_CLASS,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED,
        InternalDataTypePolicy dtype = InternalDataTypePolicy::UnsignedPCellDataType,
        PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST
    ) noexcept
    {
        size_t index = static_cast<size_t>(idx);
        if (!ValidMetaIdx(idx))
        {
            return;
        }
        const meta16_t meta16 = PackedCell64_t::MakeMeta16ForAnyOwnerAndItsClassModel_48t(
            OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            static_cast<tag8_t>(APCPagedNodeSegmentClasses::CONTROL_SLOT),
            sub_class, priority, locality, dtype
        );
        const packed64_t packed_cell = PackedCell64_t::Compose48BitFamilyPackedCell(raw48_value & MaskLowNBits(FAMILY_48_BIT_LEN), meta16);
        BackingPtr[index].store(packed_cell, MoStoreSeq_);
        BackingPtr[index].notify_all();
    }


private:

    bool CasUpdateOccupancy3x16ThreeSubdivisionCell__(
        LocalityPolicy from_locality,
        LocalityPolicy to_locality,
        std::optional<APCPagedNodeSegmentClasses> page_class = std::nullopt,
        LocalityPolicy control_cells_own_locality = LocalityPolicy::PUBLISHED,
        bool is_this_cell_central_occupancy_counter = false
    ) noexcept;

    
public:
    packed64_t PackPureClock48AsPackedCell(
        std::optional<uint64_t> clock48 = std::nullopt,
        PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED,
        APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::CONTROL_SLOT
    ) noexcept;

    void WriteOrUpdateMetaClock48(PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST, std::optional<uint64_t>meta_clock_48 = std::nullopt) noexcept;

    bool JustUpdateValueOfMeta32(
        MetaIndexOfAPCNode idx,
        uint32_t expected_value,
        uint32_t desired_value,
        bool refresh_clock16 = true
    ) noexcept;


    bool IsBound() const noexcept
    {
        return BackingPtr != nullptr && BranchCapacity_ >= METACELL_COUNT;
    }

    size_t PayloadCapacity() const noexcept
    {
        return BranchCapacity_ > METACELL_COUNT ? (BranchCapacity_ - METACELL_COUNT) : 0u;
    }

    void InitLogicalNodeIdentity(
        uint32_t logical_node_id,
        uint32_t shared_id,
        bool is_root_shared
    ) noexcept;

    void InitNodeSemantics(
        uint32_t aux_param_uint32 = UNSIGNED_ZERO
    ) noexcept;


    void InitRootOrChildBranch(
        uint32_t branch_id,
        uint32_t logical_node_id,
        uint32_t shared_id,
        size_t total_capacity,
        const ContainerConf& container_configuration,
        bool is_root_shared = true,
        uint32_t aux_param_uint32 = UNSIGNED_ZERO,
        uint32_t branch_depth = UNSIGNED_ZERO,
        uint8_t branch_priority = UNSIGNED_ZERO,
        PriorityPolicy write_cell_priority = PriorityPolicy::PRESSURE_FIRST

    ) noexcept;

    val32_t ReadMetaCellFamily32(MetaIndexOfAPCNode idx) noexcept;

    void TouchLocalMetaClock48() noexcept;

    bool TryIncrementOrDecrementActiveThreadCount(int8_t change_count) noexcept;

    bool TryMarkSplitInFlight() noexcept;

    bool ShouldSplitNow() noexcept;

    bool TryBindPortTarget(MetaIndexOfAPCNode port_meta_idx, uint32_t target_branch_id) noexcept;

    uint32_t TotalCASFailForThisBranchIncreaseAndGet(uint32_t increment) noexcept;

    bool SetLayOutBounds(
        APCPagedNodeSegmentClasses page_class, 
        uint16_t begain_index,
        uint16_t end_index,
        bool caller_already_holds_flag = false,
        std::optional<uint16_t> version_number = std::nullopt
    ) noexcept;

    std::optional<LayoutBoundsOfSingleRelNodeClass> ReadLayoutBoundsAndVersion(APCPagedNodeSegmentClasses desired_rel_mask, bool caller_holds_the_flag = false) noexcept;
    std::optional<CompleteAPCNodeRegionsLayout> ReadAndGetFullRegionLayout_(bool caller_holds_layout_flag = false) noexcept;


    bool TrySetLayoutMutationInFlight() noexcept;

    bool TryExtendASegmentInOwnAPC(APCPagedNodeSegmentClasses desired_rel_mask, uint32_t wanted_amount, ContainerConf::APCSegmentExtendOrder desired_apc_order) noexcept;

    clk16_t ReadLastAcceptedClok16ForThisSegment(APCPagedNodeSegmentClasses region_kind) noexcept;
    clk16_t ReadLastEmittedClok16ForThisSegment(APCPagedNodeSegmentClasses region_kind) noexcept;
    size_t PayloadCapacityFromHeader() noexcept;

    bool ApplyCentralAndRegionOccupancyTransitionCell(
        packed64_t old_cell,
        packed64_t new_cell,
        APCPagedNodeSegmentClasses physical_page_class = APCPagedNodeSegmentClasses::NULLNAN
    ) noexcept;

    bool RefreshReadyBitForRegionFromOccupancy(APCPagedNodeSegmentClasses page_class) noexcept;

    uint16_t ReadTotalOccuPancyOfAnyPageClass(APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::NULLNAN) noexcept;
    
    bool WriteExactMetaCellJustNewValue(MetaIndexOfAPCNode idx, uint32_t value) noexcept;

    bool ValidateAPCOccupancyInvarient() noexcept;

    bool  TryBindShareNext(uint32_t shared_next_id) noexcept
    {
        return TryBindPortTarget(MetaIndexOfAPCNode::SHARED_NEXT_ID, shared_next_id);
    }

    bool TryBindSharedPrevious(uint32_t shared_previous_id) noexcept
    {
        return TryBindPortTarget(MetaIndexOfAPCNode::SHARED_PREVIOUS_ID, shared_previous_id);
    }

    bool TurnOnASegmentFlag(ControlEnumOfAPCSegment desired_segment_flag) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(static_cast<uint32_t>(desired_segment_flag), UNSIGNED_ZERO, MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
    }

    bool HasThisControlEnumFlag(ControlEnumOfAPCSegment flag) noexcept
    {
        return (ReadMetaCellFamily32(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS) & static_cast<uint32_t>(flag)) != 0u;
    }

    bool ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment desired_control_flag) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(UNSIGNED_ZERO, static_cast<uint32_t>(desired_control_flag), MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
    }

    bool TurnOnAManagerControlFlag(ManagerControlFlagBits desired_manager_control_flag) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(static_cast<uint32_t>(desired_manager_control_flag), UNSIGNED_ZERO, MetaIndexOfAPCNode::MANAGER_CONTROL_FLAGS);
    }

    bool ClearOneManagerControlFlag(ManagerControlFlagBits desired_manager_control_flag) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(UNSIGNED_ZERO, static_cast<uint32_t>(desired_manager_control_flag), MetaIndexOfAPCNode::MANAGER_CONTROL_FLAGS);
    }

    bool HasThisManageControlFlag(ManagerControlFlagBits desired_manager_contgrol_flag) noexcept
    {
        return (ReadMetaCellFamily32(MetaIndexOfAPCNode::MANAGER_CONTROL_FLAGS) & static_cast<uint32_t>(desired_manager_contgrol_flag)) != UNSIGNED_ZERO;
    }

    bool IsLayoutMutationFlagActive() noexcept
    {
        return HasThisControlEnumFlag(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
    }
    
    void SetGraphNodeFlag() noexcept
    {
        TurnOnASegmentFlag(ControlEnumOfAPCSegment::IS_GRAPH_NODE);
    }

    uint32_t GetTotalCapacityForThisAPC() noexcept
    {
        return ReadMetaCellFamily32(MetaIndexOfAPCNode::TOTAL_CAPACITY_OF_THIS_SEGEMENT);
    }

    uint32_t MaxDepthRead() noexcept
    {
        return ReadMetaCellFamily32(MetaIndexOfAPCNode::MAX_DEPTH);
    }

    uint32_t CurrentBranchDepthRead() noexcept
    {
        return ReadMetaCellFamily32(MetaIndexOfAPCNode::BRANCH_DEPTH);
    }

    void MakeAPCBranchOwned() noexcept
    {
        WriteMetaCellMode32_(MetaIndexOfAPCNode::CURRENTLY_OWNED, 1u);
    }


    void ResetTotalCASFailureForThisBranch(PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST) noexcept
    {
        WriteMetaCellMode32_(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH, UNSIGNED_ZERO, priority);
    }

    bool SetSegmentRegionKind(APCPagedNodeSegmentClasses region_kind) noexcept
    {
        const uint32_t current_segment_kind = ReadMetaCellFamily32(MetaIndexOfAPCNode::SEGMENT_KIND);
        return JustUpdateValueOfMeta32(MetaIndexOfAPCNode::SEGMENT_KIND, current_segment_kind, static_cast<uint32_t>(region_kind));
    }

    std::atomic<packed64_t>* GetAPCBackinghPtr() noexcept
    {
        if (BackingPtr)
        {
            return BackingPtr;
        }
        return nullptr;
    }

    packed64_t ReadCentralAPCOccupancyCellForThisPagedNode() noexcept
    {
        return ReadFullMetaCell(MetaIndexOfAPCNode::COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48);
    }
    
    packed64_t ReadRegionOccupancyCombinedCell(APCPagedNodeSegmentClasses page_class) noexcept
    {
        return ReadFullMetaCell(
            APCAndPagedNodeHelpers::GetOccupancyMetIndexByRegionClass(page_class)
        );
    }

    uint16_t ReadCentralAPCOccupancyOfALocality(LocalityPolicy locality_type) noexcept;

    bool GetPublishedClaimedFaultyFromCentral(
        uint16_t& published_occupancy,
        uint16_t& claimed_occupancy,
        uint16_t& faulty_occupancy
    )
    {
        const packed64_t central_occupancy_cell = ReadCentralAPCOccupancyCellForThisPagedNode();
        const uint64_t raw48 = PackedCell64_t::ExtractModel48(central_occupancy_cell);
        bool ok = Subdevision16x3InternalMode48CellModel::ExtractLowMidHighFromMode48_(raw48, published_occupancy, claimed_occupancy, faulty_occupancy);
        return ok;
    }

    uint16_t ReadRegionOccupancyOfALocality(LocalityPolicy locality_type, APCPagedNodeSegmentClasses page_class) noexcept;

    uint16_t ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(LocalityPolicy::PUBLISHED, page_class);
    }

    uint16_t ReadIdleOccupancyOfAPAgeClass(APCPagedNodeSegmentClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(LocalityPolicy::IDLE, page_class);
    }


    uint16_t ReadClaimedOccupancyOfAPAgeClass(APCPagedNodeSegmentClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(LocalityPolicy::CLAIMED, page_class);
    }
    

    uint16_t ReadFaultyOccupancyOfAPAgeClass(APCPagedNodeSegmentClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(LocalityPolicy::FAULTY, page_class);
    }

    uint16_t ReadTotalUsedOccupancyOfARegion(APCPagedNodeSegmentClasses page_class) noexcept
    {
        return ReadPublishedOccupancyOfAPageClass(page_class) +
            ReadClaimedOccupancyOfAPAgeClass(page_class) +
            ReadFaultyOccupancyOfAPAgeClass(page_class);
    }

    void BindExternalStorage_(std::atomic<packed64_t>* packed_ptr, size_t cell_count) noexcept
    {
        BackingPtr = packed_ptr;
        BranchCapacity_ = cell_count;
    }

    void UnbindExternalStorage_() noexcept
    {
        BackingPtr = nullptr;
        BranchCapacity_ = UNSIGNED_ZERO;
    }

};

}
