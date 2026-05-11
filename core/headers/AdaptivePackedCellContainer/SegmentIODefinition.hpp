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

    enum class APCNodeComputeKind : uint32_t
    {
        NONE = 0u,
        GENERATOR_UINT32 = 1u,
        SQUARE_UINT32 = 2u,
        ADD_UINT32 = 3u,
        DIV_UINT32 = 4u,
        BIDIRECTIONAL_PREDECTIVE = 6u,
        GENERIC_VECTOR = 7u
    };


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

    packed64_t PackValue32InPackedCellwithClock16_(
        val32_t value32,
        PriorityPhysics priority,
        PackedCellLocalityTypes locality = PackedCellLocalityTypes::ST_PUBLISHED,
        APCPagedNodeRelMaskClasses page_class = APCPagedNodeRelMaskClasses::NONE,
        RelOffsetMode32 reloffset_mode32 = RelOffsetMode32::RELOFFSET_GENERIC_VALUE,
        PackedCellDataType dtype = PackedCellDataType::UnsignedPCellDataType,
        PackedCellNodeAuthority node_authority = PackedCellNodeAuthority::IDLE_OR_FREE
    ) noexcept
    {
        if (OwnedMasterClockConfPtr_)
        {
            return OwnedMasterClockConfPtr_->ComposeValue32WithCurrentThreadStamp16(value32, page_class, priority, locality, reloffset_mode32, dtype);
        }
        meta16_t strl_moded32 = PackedCell64_t::MakeInCellMetaForMode_32t(priority, node_authority, locality, page_class, reloffset_mode32, dtype);
        return PackedCell64_t::ComposeValue32u_64(value32, UNSIGNED_ZERO, strl_moded32);
    }

    void WriteBrenchMeta32_(
        MetaIndexOfAPCNode idx,
        uint32_t value32,
        PriorityPhysics priority = PriorityPhysics::IDLE,
        APCPagedNodeRelMaskClasses rel_mask4 = APCPagedNodeRelMaskClasses::CONTROL_SLOT
    ) noexcept
    {
        size_t index = static_cast<size_t>(idx);
        if (!ValidMetaIdx(idx))
        {
            return;
        }
        BackingPtr[index].store(PackValue32InPackedCellwithClock16_(value32, priority, PackedCellLocalityTypes::ST_PUBLISHED, rel_mask4), MoStoreSeq_);
        BackingPtr[index].notify_all();
    }

    void WritBranchMeta48_(
        MetaIndexOfAPCNode idx,
        uint64_t raw48_value,
        APCPagedNodeRelMaskClasses page_class = APCPagedNodeRelMaskClasses::CONTROL_SLOT,
        PriorityPhysics priority = PriorityPhysics::DEFAULT_PRIORITY,
        RelOffsetMode48 rel_offset = RelOffsetMode48::THREE_16_BIT_SUB_DIVISION
    ) noexcept
    {
        size_t index = static_cast<size_t>(idx);
        if (!ValidMetaIdx(idx))
        {
            return;
        }
        const meta16_t meta16 = PackedCell64_t::MakeInCellMetaForMode_48t(priority, PackedCellNodeAuthority::IDLE_OR_FREE, PackedCellLocalityTypes::ST_PUBLISHED, page_class, rel_offset);
        const packed64_t packed_cell = PackedCell64_t::ComposeCLK48u_64(raw48_value & MaskLowNBits(CLK_B48), meta16);
        BackingPtr[index].store(packed_cell, MoStoreSeq_);
        BackingPtr[index].notify_all();
    }


    bool TurnOnMultipleSegmentFlagsAtOnce_(uint32_t use_or_between_flags = UNSIGNED_ZERO) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(use_or_between_flags, UNSIGNED_ZERO, MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
    }

    bool UpdateAPCModeFlagsInHeader_(uint32_t flags_to_turn_on = UNSIGNED_ZERO, uint32_t flags_to_turn_off = UNSIGNED_ZERO, MetaIndexOfAPCNode desired_flag_idx = MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS) noexcept;

    bool WriteBoundsPairToHeader_(const LayoutBoundsOfSingleRelNodeClass layout_bound) noexcept;

    void BuidDefaultLayoutPlan_(CompleteAPCNodeRegionsLayout& full_layout) noexcept;

    void InitDefaultAPCSegmentedNodeLayout_() noexcept;


    bool WriteAllRegionsLayoutToHeader_(const CompleteAPCNodeRegionsLayout& full_layout) noexcept;

    bool TurnOnReadyBitForDesiredPagedNode_(APCPagedNodeRelMaskClasses desired_region_class) noexcept;

    bool ClearTheDesiredPagedNodeReadyBit_(APCPagedNodeRelMaskClasses desired_region_class) noexcept;

    bool ClearMultipleControlFlags_(uint32_t use_or_between_flags = UNSIGNED_ZERO) noexcept
    {
        return UpdateAPCModeFlagsInHeader_(UNSIGNED_ZERO, use_or_between_flags);
    }

    bool ResetALLOccupancy16x3ModelToZero_() noexcept
    {
        if (!IsBound())
        {
            return false;
        }
        WritBranchMeta48_(MetaIndexOfAPCNode::COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48, UNSIGNED_ZERO);

        for (uint8_t i = 0; i < APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses; i++)
        {
            const APCPagedNodeRelMaskClasses blind_page = static_cast<APCPagedNodeRelMaskClasses>(i);
            const MetaIndexOfAPCNode idx = APCAndPagedNodeHelpers::GetOccupancyMetIndexByRegionClass(blind_page);
            if (!ValidMetaIdx(idx))
            {
                continue;
            }
            WritBranchMeta48_(idx, UNSIGNED_ZERO);
        }
        WriteExactMetaCellJustNewValue(
            MetaIndexOfAPCNode::PAGED_NODE_READY_BIT,
            UNSIGNED_ZERO
        );
        return true;
    }

    
public:
    packed64_t PackPureClock48AsPackedCell(
        std::optional<uint64_t> clock48 = std::nullopt,
        PriorityPhysics priority = PriorityPhysics::IDLE,
        PackedCellLocalityTypes locality = PackedCellLocalityTypes::ST_PUBLISHED,
        APCPagedNodeRelMaskClasses page_class = APCPagedNodeRelMaskClasses::CONTROL_SLOT,
        RelOffsetMode48 reloffset = RelOffsetMode48::RELOFFSET_PURE_TIMER,
        PackedCellDataType dtype = PackedCellDataType::UnsignedPCellDataType,
        PackedCellNodeAuthority node_authority = PackedCellNodeAuthority::IDLE_OR_FREE
    ) noexcept;

    void WriteOrUpdateMetaClock48(PriorityPhysics priority = PriorityPhysics::IDLE, std::optional<uint64_t>meta_clock_48 = std::nullopt) noexcept;

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

    //continue here
    void InitLogicalNodeIdentity(
        uint32_t logical_node_id,
        uint32_t shared_id,
        bool is_root_shared
    ) noexcept;

    void InitNodeSemantics(
        APCNodeComputeKind compute_kind_of_node,
        uint32_t aux_param_uint32 = UNSIGNED_ZERO
    ) noexcept;


    void InitRootOrChildBranch(
        uint32_t branch_id,
        uint32_t logical_node_id,
        uint32_t shared_id,
        size_t total_capacity,
        const ContainerConf& container_configuration,
        bool is_root_shared = true,
        APCNodeComputeKind node_compute_kind = APCNodeComputeKind::NONE,
        uint32_t aux_param_uint32 = UNSIGNED_ZERO,
        uint32_t branch_depth = UNSIGNED_ZERO,
        uint8_t branch_priority = ZERO_PRIORITY,
        PriorityPhysics write_cell_priority = PriorityPhysics::IDLE

    ) noexcept;

    val32_t ReadMetaCellValue32(MetaIndexOfAPCNode idx) noexcept;

    void TouchLocalMetaClock48() noexcept;

    bool TryIncrementOrDecrementActiveThreadCount(int8_t change_count) noexcept;

    bool TryMarkSplitInFlight() noexcept;

    bool ShouldSplitNow() noexcept;

    bool TryBindPortTarget(MetaIndexOfAPCNode port_meta_idx, uint32_t target_branch_id) noexcept;

    uint32_t TotalCASFailForThisBranchIncreaseAndGet(uint32_t increment) noexcept;

    bool SetLayOutBounds(
        APCPagedNodeRelMaskClasses page_class, 
        uint16_t begain_index,
        uint16_t end_index,
        bool caller_already_holds_flag = false,
        std::optional<uint16_t> version_number = std::nullopt
    ) noexcept;

    std::optional<LayoutBoundsOfSingleRelNodeClass> ReadLayoutBounds(APCPagedNodeRelMaskClasses desired_rel_mask) noexcept;
    std::optional<CompleteAPCNodeRegionsLayout> ReadAndGetFullRegionLayout_() noexcept;


    bool TrySetLayoutMutationInFlight() noexcept;

    bool TryExtendASegmentInOwnAPC(APCPagedNodeRelMaskClasses desired_rel_mask, uint32_t wanted_amount, ContainerConf::APCSegmentExtendOrder desired_apc_order) noexcept;

    clk16_t ReadLastAcceptedClok16ForThisSegment(APCPagedNodeRelMaskClasses region_kind) noexcept;
    clk16_t ReadLastEmittedClok16ForThisSegment(APCPagedNodeRelMaskClasses region_kind) noexcept;

    bool WriteExactMetaCellJustNewValue(MetaIndexOfAPCNode idx, uint32_t value) noexcept;

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
        return (ReadMetaCellValue32(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS) & static_cast<uint32_t>(flag)) != 0u;
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
        return (ReadMetaCellValue32(MetaIndexOfAPCNode::MANAGER_CONTROL_FLAGS) & static_cast<uint32_t>(desired_manager_contgrol_flag)) != UNSIGNED_ZERO;
    }
    
    void SetGraphNodeFlag() noexcept
    {
        TurnOnASegmentFlag(ControlEnumOfAPCSegment::IS_GRAPH_NODE);
    }

    uint32_t GetTotalCapacityForThisAPC() noexcept
    {
        return ReadMetaCellValue32(MetaIndexOfAPCNode::TOTAL_CAPACITY_OF_THIS_SEGEMENT);
    }

    uint32_t MaxDepthRead() noexcept
    {
        return ReadMetaCellValue32(MetaIndexOfAPCNode::MAX_DEPTH);
    }

    uint32_t CurrentBranchDepthRead() noexcept
    {
        return ReadMetaCellValue32(MetaIndexOfAPCNode::BRANCH_DEPTH);
    }

    size_t PayloadCapacityFromHeader() noexcept
    {
        const uint32_t payload_begain = METACELL_COUNT;
        const uint32_t payload_end  = GetTotalCapacityForThisAPC();
        if (payload_end > payload_begain)
        {
            return static_cast<size_t>(payload_end - payload_begain);
        }
        return UNSIGNED_ZERO;
    }

    void MakeAPCBranchOwned() noexcept
    {
        WriteBrenchMeta32_(MetaIndexOfAPCNode::CURRENTLY_OWNED, 1u, PriorityPhysics::IMPORTANT);
    }


    void ResetTotalCASFailureForThisBranch(PriorityPhysics priority = PriorityPhysics::IDLE) noexcept
    {
        WriteBrenchMeta32_(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH, UNSIGNED_ZERO, priority);
    }

    bool SetSegmentRegionKind(APCPagedNodeRelMaskClasses region_kind) noexcept
    {
        const uint32_t current_segment_kind = ReadMetaCellValue32(MetaIndexOfAPCNode::SEGMENT_KIND);
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
    
    packed64_t ReadRegionOccupancyCombinedCell(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        return ReadFullMetaCell(
            APCAndPagedNodeHelpers::GetOccupancyMetIndexByRegionClass(page_class)
        );
    }

    uint16_t ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes locality_type) noexcept
    {
        const std::optional<uint16_t> desired_occupancy =  GetOccuupancyFromPackedCellMode48(
                ReadCentralAPCOccupancyCellForThisPagedNode(),
                locality_type,
                static_cast<uint16_t>(PayloadCapacityFromHeader())
            );
        return desired_occupancy ? *desired_occupancy : UNSIGNED_ZERO;
    }

    bool GetPublishedClaimedFaultyFromCentral(
        uint16_t& published_occupancy,
        uint16_t& claimed_occupancy,
        uint16_t& faulty_occupancy
    )
    {
        const packed64_t central_occupancy_cell = ReadCentralAPCOccupancyCellForThisPagedNode();
        const uint64_t raw48 = PackedCell64_t::ExtractClk48(central_occupancy_cell);
        bool ok = ExtractLowMidHighFromMode48_(raw48, published_occupancy, claimed_occupancy, faulty_occupancy);
        return ok;
    }

    uint16_t ReadRegionOccupancyOfALocality(PackedCellLocalityTypes locality_type, APCPagedNodeRelMaskClasses page_class) noexcept;

    uint16_t ReadPublishedOccupancyOfAPageClass(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(PackedCellLocalityTypes::ST_PUBLISHED, page_class);
    }

    uint16_t ReadIdleOccupancyOfAPAgeClass(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(PackedCellLocalityTypes::ST_IDLE, page_class);
    }


    uint16_t ReadClaimedOccupancyOfAPAgeClass(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(PackedCellLocalityTypes::ST_CLAIMED, page_class);
    }
    

    uint16_t ReadFaultyOccupancyOfAPAgeClass(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        return ReadRegionOccupancyOfALocality(PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY, page_class);
    }

    uint16_t ReadTotalUsedOccupancyOfARegion(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        return ReadPublishedOccupancyOfAPageClass(page_class) +
            ReadClaimedOccupancyOfAPAgeClass(page_class) +
            ReadFaultyOccupancyOfAPAgeClass(page_class);
    }

    bool CasUpdateOccupancy3x16ThreeSubdivisionCell(
        PackedCellLocalityTypes from_locality,
        PackedCellLocalityTypes to_locality,
        std::optional<APCPagedNodeRelMaskClasses> page_class = std::nullopt
    ) noexcept;

    bool APPLYCentralAndRegionOccupancyTransitionCell(
        packed64_t old_cell,
        packed64_t new_cell,
        APCPagedNodeRelMaskClasses physical_page_class = APCPagedNodeRelMaskClasses::NANNULL
    ) noexcept;

    bool RefreshReadyBitForRegionFromOccupancy(APCPagedNodeRelMaskClasses page_class) noexcept;

    uint16_t ReadTotalOccuPancyOfAnyPageClass(APCPagedNodeRelMaskClasses page_class = APCPagedNodeRelMaskClasses::NANNULL) noexcept
    {

        const packed64_t packed_cell = page_class != APCPagedNodeRelMaskClasses::NANNULL ?
            ReadRegionOccupancyCombinedCell(page_class) : ReadCentralAPCOccupancyCellForThisPagedNode();

        const uint16_t full_combined_occupancy = GetTootalOccupancyFromPackedCell(packed_cell);
        return full_combined_occupancy;
    }
    
};

}
