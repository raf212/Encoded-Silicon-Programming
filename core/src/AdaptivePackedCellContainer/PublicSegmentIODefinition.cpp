#include "APCSegmentsCausalCordinator.hpp"
#include "PackedCellContainerManager.hpp"
#include <iostream>

namespace PredictedAdaptedEncoding
{

    uint16_t SegmentIODefinition::ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes locality_type) noexcept
    {
        const uint16_t total_capacity = static_cast<uint16_t>(
            std::min<size_t>(GetTotalCapacityForThisAPC(), APC_MAX_LENGTH_OR_COUNTER)
        );

        const std::optional<uint16_t> desired_occupancy =  GetOccuupancyFromPackedCellMode48(
                ReadCentralAPCOccupancyCellForThisPagedNode(),
                locality_type,
                total_capacity
            );
        return desired_occupancy ? *desired_occupancy : UNSIGNED_ZERO;
    }

    val32_t SegmentIODefinition::ReadMetaCellValue32(MetaIndexOfAPCNode idx) noexcept
    {
        if (!ValidMetaIdx(idx) || idx == MetaIndexOfAPCNode::LOCAL_CLOCK48)
        {
            return UNSIGNED_ZERO;
        }
        size_t index = static_cast<size_t>(idx);
        return PackedCell64_t::ExtractValue32(BackingPtr[index].load(MoLoad_));
    }

    void SegmentIODefinition::TouchLocalMetaClock48() noexcept
    {
        if (!OwnedMasterClockConfPtr_)
        {
            return;
        }
        OwnedMasterClockConfPtr_->TouchSegmentLocalClock48HighPriority();
    }

    packed64_t SegmentIODefinition::PackPureClock48AsPackedCell(
        std::optional<uint64_t> clock48,
        PriorityPhysics priority,
        PackedCellLocalityTypes locality,
        APCPagedNodeSegmentClasses page_class,
        RelOffsetMode48 reloffset,
        PackedCellDataType dtype,
        PackedCellNodeAuthority node_authority
    ) noexcept
    {
        if ((reloffset != RelOffsetMode48::RELOFFSET_PURE_TIMER))
        {
            return PackedCell64_t::MakeFaultyCell();
        }
        
        if (OwnedMasterClockConfPtr_)
        {
            return OwnedMasterClockConfPtr_->ComposePureClockCell48();
        }
        
        meta16_t strl_clock48 = PackedCell64_t::MakeInCellMetaForMode_48t(priority, node_authority, locality, page_class, reloffset, dtype);
        if (clock48)
        {
            return PackedCell64_t::ComposeCLK48u_64(clock48.value(), strl_clock48);
        }
        Timer48 now_timer;
        return PackedCell64_t::ComposeCLK48u_64((now_timer.NowTicks() & MaskLowNBits(CLK_B48)), strl_clock48);
    }

    void SegmentIODefinition::WriteOrUpdateMetaClock48(PriorityPhysics priority, std::optional<uint64_t>meta_clock_48 ) noexcept
    {
        size_t idx = static_cast<size_t>(MetaIndexOfAPCNode::LOCAL_CLOCK48);
        packed64_t wanted_cell = PackPureClock48AsPackedCell(meta_clock_48, priority, PackedCellLocalityTypes::ST_PUBLISHED);
        BackingPtr[idx].store(wanted_cell, MoStoreSeq_);
        BackingPtr[idx].notify_all();
    }

    bool SegmentIODefinition::JustUpdateValueOfMeta32(
        MetaIndexOfAPCNode idx,
        uint32_t expected_value,
        uint32_t desired_value,
        bool refresh_clock16
    ) noexcept
    {
        if (!ValidMetaIdx(idx) || idx == MetaIndexOfAPCNode::LOCAL_CLOCK48)
        {
            return false;
        }
        const size_t index = static_cast<size_t>(idx);
        packed64_t expected_packed = BackingPtr[index].load(MoLoad_);
        if (PackedCell64_t::ExtractValue32(expected_packed) != expected_value)
        {
            return false;
        }
        if (PackedCell64_t::ExtractLocalityFromPacked(expected_packed) == PackedCellLocalityTypes::ST_CLAIMED)
        {
            return false;
        }
        meta16_t current_strl = PackedCell64_t::ExtractMeta16fromPackedCell(expected_packed);
        clk16_t current_clock16 = PackedCell64_t::ExtractClk16(expected_packed);
        if (refresh_clock16 && OwnedMasterClockConfPtr_)
        {
            current_clock16 = OwnedMasterClockConfPtr_->NowClock16();
        }
        const packed64_t desired_packed = PackedCell64_t::ComposeValue32u_64(desired_value, current_clock16, current_strl);
        return BackingPtr[index].compare_exchange_strong(
            expected_packed,
            desired_packed,
            OnExchangeSuccess,
            OnExchangeFailure
        );
        
    }

    void SegmentIODefinition::InitLogicalNodeIdentity(
        uint32_t logical_node_id,
        uint32_t shared_id,
        bool is_root_shared
    ) noexcept
    {
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LOGICAL_NODE_ID, logical_node_id, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::SHARED_ID, shared_id, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::SHARED_PREVIOUS_ID, BRANCH_SENTINAL, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::SHARED_NEXT_ID, BRANCH_SENTINAL, PriorityPhysics::IDLE);
        if (is_root_shared)
        {
            TurnOnMultipleSegmentFlagsAtOnce_(static_cast<uint32_t>(ControlEnumOfAPCSegment::IS_GRAPH_NODE) | static_cast<uint32_t>(ControlEnumOfAPCSegment::IS_SHARED_ROOT));
            ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::IS_SHARED_MAMBER);
        }
        else
        {
            TurnOnMultipleSegmentFlagsAtOnce_(static_cast<uint32_t>(ControlEnumOfAPCSegment::IS_GRAPH_NODE) | static_cast<uint32_t>(ControlEnumOfAPCSegment::IS_SHARED_MAMBER));
            ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::IS_SHARED_ROOT);    
        }
        
    }

    void SegmentIODefinition::InitNodeSemantics(
        APCNodeComputeKind compute_kind_of_node,
        uint32_t aux_param_uint32
    ) noexcept
    {
        WriteMetaCellMode32_(MetaIndexOfAPCNode::NODE_COMPUTE_KIND, static_cast<uint32_t>(compute_kind_of_node), PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::NODE_AUX_PARAM_U32, aux_param_uint32, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_FORWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_BACKWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LAST_EMITTED_FEED_FORWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LAST_EMITTED_FEED_BACKWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);

        WriteMetaCellMode32_(MetaIndexOfAPCNode::FEEDFORWARD_IN_TARGET_ID, BRANCH_SENTINAL);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::FEEDFORWARD_OUT_TARGET_ID, BRANCH_SENTINAL);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::FEEDBACKWARD_IN_TARGET_ID, BRANCH_SENTINAL);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::FEEDBACKWARD_OUT_TARGET_ID, BRANCH_SENTINAL);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LATERAL_0_TARGET_ID, BRANCH_SENTINAL);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LATERAL_1_TARGET_ID, BRANCH_SENTINAL);
    }



    void SegmentIODefinition::InitRootOrChildBranch(
        uint32_t branch_id,
        uint32_t logical_node_id,
        uint32_t shared_id,
        size_t total_capacity,
        const ContainerConf& container_configuration,
        bool is_root_shared,
        APCNodeComputeKind node_compute_kind,
        uint32_t aux_param_uint32,
        uint32_t branch_depth,
        uint8_t branch_priority,
        PriorityPhysics write_cell_priority

    ) noexcept
    {
        if (!IsBound())
        {
            return;
        }

        const uint32_t safe_capacity = static_cast<uint32_t>(std::min<size_t>(total_capacity, BRANCH_SENTINAL));

        const uint32_t region_count = container_configuration.RegionSize == 0 ? 0 : static_cast<uint32_t>((std::max<size_t>(total_capacity, METACELL_COUNT) - METACELL_COUNT + container_configuration.RegionSize - 1) / container_configuration.RegionSize);
        const uint32_t resolve_shared_id = (shared_id == UNSIGNED_ZERO || shared_id == BRANCH_SENTINAL) ? branch_id : shared_id;
        
        WriteMetaCellMode32_(MetaIndexOfAPCNode::MAGIC_ID, BRANCH_MAGIC, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::BRANCH_ID, static_cast<uint32_t>(std::min<uint32_t>(branch_id, BRANCH_SENTINAL)), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::GLOBAL_CURRENT_VERSION, BRANCH_VERSION, write_cell_priority, APCPagedNodeSegmentClasses::CONTROL_SLOT);

        WriteMetaCellMode32_(MetaIndexOfAPCNode::BRANCH_DEPTH, branch_depth, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::BRANCH_PRIORITY, branch_priority, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS, container_configuration.EnableBranching ? static_cast<uint32_t>(ControlEnumOfAPCSegment::ENABLE_BRANCHING) : static_cast<uint32_t>(ControlEnumOfAPCSegment::NONE), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::CURRENT_ACTIVE_THREADS, UNSIGNED_ZERO, write_cell_priority);
        WritBranchMeta48_(MetaIndexOfAPCNode::COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48, UNSIGNED_ZERO);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::SPLIT_THRESHOLD_PERCENTAGE, container_configuration.BranchSplitThresholdPercentage, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::SEGMENT_KIND, static_cast<uint32_t>(APCPagedNodeSegmentClasses::FREE_SLOT), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::MAX_DEPTH, container_configuration.BranchMaxDepth, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::TOTAL_CAPACITY_OF_THIS_SEGEMENT, safe_capacity, write_cell_priority);                                                                                        
        WriteMetaCellMode32_(MetaIndexOfAPCNode::LAST_SPLIT_EPOCH, UNSIGNED_ZERO, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::REGION_SIZE, static_cast<uint32_t>(container_configuration.RegionSize), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::REGION_COUNT, region_count, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT, UNSIGNED_ZERO, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::PRODUCER_BLOCK_SIZE, static_cast<uint32_t>(container_configuration.ProducerBlockSize), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::BACKGROUND_EPOCH_ADVANCE_MS, static_cast<uint32_t>(container_configuration.BackgroundEpochAdvanceMS), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::DEFINED_MODE_OF_CURRENT_APC, static_cast<uint32_t>(container_configuration.InitialMode), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::RETIRE_BRANCH_THRASHOLD, container_configuration.RetireBatchThreshold, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::PRODUCER_CURSOR_PLACEMENT, static_cast<uint32_t>(METACELL_COUNT), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::CONSUMER_CURSORE_PLACEMENT, static_cast<uint32_t>(METACELL_COUNT), write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::CURRENTLY_OWNED, UNSIGNED_ZERO, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH, UNSIGNED_ZERO, write_cell_priority);
        WriteMetaCellMode32_(MetaIndexOfAPCNode::NODE_GROUP_SIZE, container_configuration.NodeGroupSize, write_cell_priority);
        InitLogicalNodeIdentity(logical_node_id, resolve_shared_id, is_root_shared);
        InitNodeSemantics(node_compute_kind, aux_param_uint32);
        InitDefaultAPCSegmentedNodeLayout_();
        for (uint8_t i = 0; i < APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses; i++)
        {
            WriteMetaCellMode32_(static_cast<MetaIndexOfAPCNode>(static_cast<size_t>(MetaIndexOfAPCNode::REGION_OCCUPANCY_NONE) + i), 
                            UNSIGNED_ZERO, write_cell_priority, APCPagedNodeSegmentClasses::CONTROL_SLOT
                        );
        }

        ResetALLOccupancy16x3ModelToZero_();
        
        WriteMetaCellMode32_(MetaIndexOfAPCNode::EOF_APC_HEADER, EOF_HEADER, write_cell_priority);
#ifndef NDEBUG
{
    auto layout = ReadAndGetFullRegionLayout_(false);
    if (!layout)
    {
        std::cerr << "[APC INIT BUG] layout version validation failed after init\n";
        std::terminate();
    }
}
#endif
    }

    bool SegmentIODefinition::TryIncrementOrDecrementActiveThreadCount(int8_t change_count) noexcept
    {
        ///for now
        if (change_count < 0)
        {
            change_count = -1;
        }
        else if (change_count > 0)
        {
            change_count = 1;
        }
        else
        {
            return true;
        }
        ///
        
        
        while (true)
        {
            uint32_t current_thread_count = ReadMetaCellValue32(MetaIndexOfAPCNode::CURRENT_ACTIVE_THREADS);
            if (current_thread_count == UINT32_MAX)
            {
                return false;
            }

            if (change_count < 0 && current_thread_count == 0)
            {
                return false;
            }

            const uint32_t desired = static_cast<uint32_t>(static_cast<int64_t>(current_thread_count) + static_cast<int64_t>(change_count));
            
            if (JustUpdateValueOfMeta32(MetaIndexOfAPCNode::CURRENT_ACTIVE_THREADS, current_thread_count, desired))
            {
                return true;
            }
        }
    }

    bool SegmentIODefinition::TryBindPortTarget(MetaIndexOfAPCNode port_meta_idx, uint32_t target_branch_id) noexcept
    {
        if (target_branch_id == BRANCH_SENTINAL)
        {
            return false;
        }
        while (true)
        {
            const uint32_t current_meta_value = ReadMetaCellValue32(port_meta_idx);
            if (current_meta_value == target_branch_id)
            {
                return true;
            }
            if (current_meta_value != BRANCH_SENTINAL)
            {
                return false;
            }
            if (JustUpdateValueOfMeta32(port_meta_idx, current_meta_value, target_branch_id))
            {
                return true;
            }
        }
    }

    bool SegmentIODefinition::ShouldSplitNow() noexcept
    {
        const val32_t split_threshold = ReadMetaCellValue32(MetaIndexOfAPCNode::SPLIT_THRESHOLD_PERCENTAGE);
        const val32_t max_depth = ReadMetaCellValue32(MetaIndexOfAPCNode::MAX_DEPTH);
        const val32_t depth_of_current_branch = ReadMetaCellValue32(MetaIndexOfAPCNode::BRANCH_DEPTH);
        if (depth_of_current_branch >= max_depth)
        {
            return false;
        }
        auto maybe_compleate_layout = ReadAndGetFullRegionLayout_();
        if (!maybe_compleate_layout)
        {
            return false;
        }

        CompleteAPCNodeRegionsLayout compleate_layout_of_this_apc = *maybe_compleate_layout;
        for (size_t rel_class = 0; rel_class < APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses; rel_class++)
        {
            const LayoutBoundsOfSingleRelNodeClass* current_layout = compleate_layout_of_this_apc.GetALayoutByRelMask(static_cast<APCPagedNodeSegmentClasses>(rel_class));
            if (!current_layout || current_layout->IsEmpty())
            {
                continue;
            }

            const uint32_t payload_span = current_layout->GetPayloadSpan();
            if (payload_span == 0)
            {
                continue;
            }
            
            const uint16_t total_used_region_occupancy = ReadTotalUsedOccupancyOfARegion(current_layout->PAGE_LAYOUT_CLASS);

            const uint32_t pressure = static_cast<uint32_t>(static_cast<uint64_t>(total_used_region_occupancy) * 100 / payload_span);
            if (pressure >= split_threshold)
            {
                return true;
            }
            
        }
        return false;
        
    }


    bool SegmentIODefinition::TryMarkSplitInFlight() noexcept
    {
        const uint32_t bit = static_cast<uint32_t>(ControlEnumOfAPCSegment::SPLIT_INFLIGHT);
        while (true)
        {
            const uint32_t current_flags = ReadMetaCellValue32(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
            if ((current_flags & bit) != UNSIGNED_ZERO)
            {
                return false;
            }
            const uint32_t desired_flags = current_flags | bit;
            if (JustUpdateValueOfMeta32(
                MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS,
                current_flags,
                desired_flags,
                false
            ))
            {
                return true;
            }
            if (AdaptiveBackoffOfAPCPtr_)
            {
                AdaptiveBackoffOfAPCPtr_->AdaptiveBackOffPacked(
                    ReadFullMetaCell(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS)
                );
            }
            
            
        }
        
    }

    uint32_t SegmentIODefinition::TotalCASFailForThisBranchIncreaseAndGet(uint32_t increment) noexcept
    {
        while (true)
        {
            val32_t current_total_cas_failure = ReadMetaCellValue32(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH);
            if (current_total_cas_failure == BRANCH_SENTINAL)
            {
                return BRANCH_SENTINAL;
            }
            
            if (JustUpdateValueOfMeta32(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH, current_total_cas_failure, current_total_cas_failure + increment))
            {
                return current_total_cas_failure + increment;
            }   
        }
    }




    std::optional<LayoutBoundsOfSingleRelNodeClass>
    SegmentIODefinition::ReadLayoutBoundsAndVersion(
        APCPagedNodeSegmentClasses page_class,
        bool caller_holds_the_flag
    ) noexcept
    {
        if (page_class == APCPagedNodeSegmentClasses::CONTROL_SLOT)
        {
            return GetVirtualControlSlotLayout_();
        }
        
        const bool valid_layout_class = APCAndPagedNodeHelpers::IsTrackedOccupancyPageClass(page_class);
        
        if (!valid_layout_class)
        {
            return std::nullopt;
        }

        const MetaIndexOfAPCNode layout_idx =
            LayoutBoundsOfSingleRelNodeClass::GetLayoutCellMetaIndexForPageClass(
                page_class
            );

        if (!ValidMetaIdx(layout_idx) ||
            layout_idx == MetaIndexOfAPCNode::EOF_APC_HEADER)
        {
            return std::nullopt;
        }

        const packed64_t layout_cell = ReadFullMetaCell(layout_idx);

        uint16_t begin16 = UNSIGNED_ZERO;
        uint16_t end16 = UNSIGNED_ZERO;
        uint16_t version16 = UNSIGNED_ZERO;

        if (!ExtractLayoutModel_BegainL_EndM_VersionH(
                layout_cell,
                begin16,
                end16,
                version16))
        {
            return std::nullopt;
        }

        const uint32_t total_capacity = GetTotalCapacityForThisAPC();

        if (begin16 < METACELL_COUNT ||
            end16 < begin16 ||
            end16 > total_capacity ||
            version16 == UNSIGNED_ZERO ||
            version16 == APC_INDEX_SENTINAL)
        {
            return std::nullopt;
        }

        if (!caller_holds_the_flag)
        {
            const std::optional<uint16_t> global_version =
                ReadGlobalLayoutVersion_();

            if (!global_version.has_value() || *global_version != version16)
            {
                return std::nullopt;
            }
        }

        LayoutBoundsOfSingleRelNodeClass out_layout{};
        out_layout.BeginIndex = begin16;
        out_layout.EndIndex = end16;
        out_layout.VersionNumber = version16;
        out_layout.PAGE_LAYOUT_CLASS = page_class;

        const uint32_t payload_capacity =
            total_capacity > METACELL_COUNT
                ? total_capacity - METACELL_COUNT
                : UNSIGNED_ZERO;

        if (payload_capacity > UNSIGNED_ZERO)
        {
            out_layout.SetOrResetPercentage(payload_capacity);
        }

        return out_layout;
    }

    bool SegmentIODefinition::SetLayOutBounds(
        APCPagedNodeSegmentClasses page_class, 
        uint16_t begain_index,
        uint16_t end_index,
        bool caller_already_holds_flag,
        std::optional<uint16_t> version_number
    ) noexcept
    {
        if (!APCAndPagedNodeHelpers::IsDataConsumablePageClass(page_class))
        {
            return false;
        }
        
        if (begain_index > end_index || begain_index < METACELL_COUNT || end_index > GetTotalCapacityForThisAPC())
        {
            return false;
        }
        
        const MetaIndexOfAPCNode layout_idx = LayoutBoundsOfSingleRelNodeClass::GetLayoutCellMetaIndexForPageClass(page_class);
        if (!ValidMetaIdx(layout_idx))
        {
            return false;
        }

        bool owns_layout_flag = false;
        
        if (caller_already_holds_flag)
        {
            if (!IsLayoutMutationFlagActive())
            {
                return false;
            }        
        }
        else
        {
            if (!TrySetLayoutMutationInFlight())
            {
                return false;
            }
            owns_layout_flag = true;
        }

        
        auto ClearLayoutFlagSB = [this, owns_layout_flag]() noexcept
        {
            if (owns_layout_flag)
            {
                ClearOneControlEnumFlagOfAPC(
                    ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT
                );            
            }
        };

        const uint16_t resolved_version = (version_number.has_value() && *version_number > UNSIGNED_ZERO && *version_number != APC_INDEX_SENTINAL) ? 
                                            *version_number : ReadGlobalLayoutVersion_().value_or(static_cast<uint16_t>(BRANCH_VERSION));
        if (resolved_version == UNSIGNED_ZERO || resolved_version == APC_INDEX_SENTINAL)
        {
            ClearLayoutFlagSB();
            return false;
        }
        

        packed64_t observed_layout = ReadFullMetaCell(layout_idx);

        while (true)
        {
            if (!IsLayoutMutationFlagActive())
            {
                ClearLayoutFlagSB();
                return false;
            }
            
            packed64_t desired_layout = ComposeLayoutModelof16x3(begain_index, end_index, resolved_version, page_class);
            packed64_t expected_layout_cell = observed_layout;

            if (BackingPtr[static_cast<size_t>(layout_idx)].compare_exchange_strong(
                    expected_layout_cell,
                    desired_layout,
                    OnExchangeSuccess,
                    OnExchangeFailure))
            {
                BackingPtr[static_cast<size_t>(layout_idx)].notify_all();
                
                ClearLayoutFlagSB();
                return true;
            }
            observed_layout = expected_layout_cell;
            if (AdaptiveBackoffOfAPCPtr_)
            {
                AdaptiveBackoffOfAPCPtr_->AdaptiveBackOffPacked(observed_layout);
            }
        }
        
        
        
    }

    bool SegmentIODefinition::TrySetLayoutMutationInFlight() noexcept
    {
        const uint32_t bit = static_cast<uint32_t>(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);

        while (true)
        {
            const uint32_t current_flags = ReadMetaCellValue32(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
            if (current_flags == BRANCH_SENTINAL)
            {
                return false;
            }
            if ((current_flags & bit) != UNSIGNED_ZERO)
            {
                return false;
            }
            const uint32_t desired_flags = current_flags | bit;
            if (JustUpdateValueOfMeta32(
                MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS,
                current_flags,
                desired_flags,
                false
            ))
            {
                return true;
            }
            if (AdaptiveBackoffOfAPCPtr_)
            {
                AdaptiveBackoffOfAPCPtr_->AdaptiveBackOffPacked(
                    ReadFullMetaCell(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS)
                );
            }
        }
    }

    bool SegmentIODefinition::TryExtendASegmentInOwnAPC(APCPagedNodeSegmentClasses desired_rel_mask, uint32_t wanted_amount, ContainerConf::APCSegmentExtendOrder desired_apc_order) noexcept
    {
        if (wanted_amount == 0)
        {
            return true;
        }
        if (!IsBound() || !APCAndPagedNodeHelpers::IsDataConsumablePageClass(desired_rel_mask))
        {
            return false;
        }
        if (!TrySetLayoutMutationInFlight())
        {
            return false;
        }

        auto ClearIfOwned = [this]() noexcept
        {
            ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
        };

        auto FailedWrite = [&]()noexcept -> bool
        {
            ClearIfOwned();
            return false;
        };
        
        auto maybe_current_complete_node_layout = ReadAndGetFullRegionLayout_(true);
        if (!maybe_current_complete_node_layout)
        {
            return FailedWrite();
        }

        CompleteAPCNodeRegionsLayout current_complete_layout = *maybe_current_complete_node_layout;

        LayoutBoundsOfSingleRelNodeClass* target_layout_of_increment = current_complete_layout.GetALayoutByRelMask(desired_rel_mask);
        LayoutBoundsOfSingleRelNodeClass* free_slot_layout = current_complete_layout.GetALayoutByRelMask(APCPagedNodeSegmentClasses::FREE_SLOT);

        if (!target_layout_of_increment || !free_slot_layout)
        {
            return FailedWrite();
        }

        const uint32_t payload_begain = METACELL_COUNT;
        const uint32_t payload_end = GetTotalCapacityForThisAPC();
        if (!target_layout_of_increment->IsValid(payload_begain, payload_end) || !free_slot_layout->IsValid(payload_begain, payload_end))
        {
            return FailedWrite();
        }
        

        auto IsLayoutValid = [&](CompleteAPCNodeRegionsLayout& compleate_layout_address) noexcept->bool
        {
            auto ordered_array = compleate_layout_address.OrderedViewsFIFO();
            uint32_t cursor = payload_begain;
            for (const auto* one_layout : ordered_array)
            {
                if (!one_layout)
                {
                    return false;
                }
                if (!one_layout->IsValid(payload_begain, payload_end))
                {
                    return false;
                }
                if (one_layout->BeginIndex != cursor)
                {
                    return false;
                }
                if (one_layout->EndIndex < one_layout->BeginIndex)
                {
                    return false;
                }
                cursor = one_layout->EndIndex;
            }
            return cursor == payload_end;
        };

        auto TryFromSpecificNeighbor = [&](LayoutBoundsOfSingleRelNodeClass& candidate_neighbor) noexcept->bool
        {
            if (candidate_neighbor.PAGE_LAYOUT_CLASS == desired_rel_mask)
            {
                return false;
            }
            if (candidate_neighbor.IsEmpty())
            {
                return false;
            }
            CompleteAPCNodeRegionsLayout trial_layout = current_complete_layout;
            LayoutBoundsOfSingleRelNodeClass* trial_target = trial_layout.GetALayoutByRelMask(desired_rel_mask);
            LayoutBoundsOfSingleRelNodeClass* trial_neighbor = trial_layout.GetALayoutByRelMask(candidate_neighbor.PAGE_LAYOUT_CLASS);
            if (!trial_target || !trial_neighbor)
            {
                return false;
            }
            
            bool grown = false;

            if (trial_target->CanBorrowRightFrom(*trial_neighbor))
            {
                grown = trial_target->TryGrowRight(wanted_amount, *trial_neighbor);
            }
            else if (trial_target->CanBorrowLeftFrom(*trial_neighbor))
            {
                grown = trial_target->TryGrowLeft(wanted_amount, *trial_neighbor);
            }
            if (!grown)
            {
                return false;
            }
            if(!IsLayoutValid(trial_layout))
            {
                return false;
            }
            current_complete_layout = trial_layout;
            return true;
        };

        //first try against free
        if (TryFromSpecificNeighbor(*free_slot_layout))
        {
            const bool ok = WriteAllRegionsLayoutToHeader_(
                current_complete_layout,
                std::nullopt,
                true
            );

            ClearIfOwned();

            return ok;
        }
        
        std::array<LayoutBoundsOfSingleRelNodeClass*, (CompleteAPCNodeRegionsLayout::CURRENT_TOTAL_APC_REL_NODE_CLASSES)> candidates{};
        size_t count = 0;
        {
            auto all_layouts = current_complete_layout.OrderedViewsFIFO();
            for (auto* one_layout : all_layouts)
            {
                if (!one_layout || one_layout->PAGE_LAYOUT_CLASS == desired_rel_mask)
                {
                    continue;
                }

                const bool touches_target = target_layout_of_increment->CanBorrowRightFrom(*one_layout) || target_layout_of_increment->CanBorrowLeftFrom(*one_layout);
                if (touches_target)
                {
                    candidates[count++] = one_layout;
                }
            }
        }


        if (desired_apc_order == ContainerConf::APCSegmentExtendOrder::PRIORITY)
        {
            std::sort(candidates.begin(), candidates.begin() + count, 
                [&](const auto* priority_layout_1, const auto* priority_layout_2) noexcept
                {
                    return priority_layout_1->GetPayloadSpan() > priority_layout_2->GetPayloadSpan();
                }
            );
        }

        for (size_t i = 0; i < count; i++)
        {
            if (!candidates[i])
            {
                continue;
            }
            if (TryFromSpecificNeighbor(*candidates[i]))
            {
                const bool ok = WriteAllRegionsLayoutToHeader_(
                    current_complete_layout,
                    std::nullopt,
                    true
                );

                ClearIfOwned();
                return ok;
            }
        }
        return FailedWrite();
    }

    clk16_t SegmentIODefinition::ReadLastAcceptedClok16ForThisSegment(APCPagedNodeSegmentClasses region_kind) noexcept
    {
        switch (region_kind)
        {
            case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE :
                return static_cast<clk16_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_BACKWARD_CLOCK16));
        
        default:
            return static_cast<clk16_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_FORWARD_CLOCK16));
        }
    }

    clk16_t SegmentIODefinition::ReadLastEmittedClok16ForThisSegment(APCPagedNodeSegmentClasses region_kind) noexcept
    {
        switch (region_kind)
        {
            case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE :
                return static_cast<clk16_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::LAST_EMITTED_FEED_BACKWARD_CLOCK16));
        
        default:
            return static_cast<clk16_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::LAST_EMITTED_FEED_FORWARD_CLOCK16));
        }
    }

    bool SegmentIODefinition::WriteExactMetaCellJustNewValue(MetaIndexOfAPCNode idx, uint32_t value) noexcept
    {
        while (true)
        {
            const uint32_t current_value = ReadMetaCellValue32(idx);
            if (current_value == value)
            {
                return true;
            }
            if (JustUpdateValueOfMeta32(idx, current_value, value))
            {
                return true;
            }
        }
    }

    uint16_t SegmentIODefinition::ReadRegionOccupancyOfALocality(PackedCellLocalityTypes locality_type, APCPagedNodeSegmentClasses page_class) noexcept
    {

        if (!APCAndPagedNodeHelpers::IsTrackedOccupancyPageClass(page_class))
        {
            return UNSIGNED_ZERO;
        }
        const packed64_t packed_cell = ReadRegionOccupancyCombinedCell(page_class);

        uint16_t max_for_a_page = UNSIGNED_ZERO;
        if (page_class == APCPagedNodeSegmentClasses::CONTROL_SLOT)
        {
            max_for_a_page = static_cast<uint16_t>(
                std::min<size_t>(METACELL_COUNT, APC_MAX_LENGTH_OR_COUNTER)
            );
        }
        else
        {
            std::optional<LayoutBoundsOfSingleRelNodeClass> maybe_bounds_of_the_page_class = ReadLayoutBoundsAndVersion(page_class);
            if (!maybe_bounds_of_the_page_class || maybe_bounds_of_the_page_class->IsEmpty())
            {
                return UNSIGNED_ZERO;
            }
            max_for_a_page = static_cast<uint16_t>(
                maybe_bounds_of_the_page_class->GetPayloadSpan(),
                APC_MAX_LENGTH_OR_COUNTER
            );
        }
        
        const std::optional<uint16_t> desired_region_occupancy = GetOccuupancyFromPackedCellMode48(
            packed_cell,
            locality_type,
            max_for_a_page
        );

        return desired_region_occupancy ? *desired_region_occupancy : UNSIGNED_ZERO;
    } 


    bool SegmentIODefinition::CasUpdateOccupancy3x16ThreeSubdivisionCell__(
        PackedCellLocalityTypes from_locality,
        PackedCellLocalityTypes to_locality,
        std::optional<APCPagedNodeSegmentClasses> page_class,
        PackedCellLocalityTypes control_or_meta_cells_own_locality,
        bool is_this_cell_central_occupancy_counter
    ) noexcept
    {
        if (from_locality == to_locality)
        {
            return true;
        }

        if (is_this_cell_central_occupancy_counter)
        {
            if (page_class.has_value())
            {
                return false;
            }
        }
        else
        {
            if (!page_class.has_value() || !APCAndPagedNodeHelpers::IsTrackedOccupancyPageClass(*page_class))
            {
                return false;
            }
        }
        const MetaIndexOfAPCNode meta_idx = page_class.has_value() ? APCAndPagedNodeHelpers::GetOccupancyMetIndexByRegionClass(*page_class) : MetaIndexOfAPCNode::COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48;

        if (!ValidMetaIdx(meta_idx))
        {
            return false;
        }
        
        packed64_t observed_cell = ReadFullMetaCell(meta_idx);
        while (true)
        {

            const PackedCell64_t::AuthoritiveCellView occupancy_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(observed_cell);

            if (!APCAndPagedNodeHelpers::DoseThisCellUpdateableAsOccupancy16x3(occupancy_cell_view))
            {
                return false;
            }

            //return addresses
            uint16_t published_count = UNSIGNED_ZERO;
            uint16_t claimed_count = UNSIGNED_ZERO;
            uint16_t faulty_count = UNSIGNED_ZERO;
            //

            const uint64_t raw48 = PackedCell64_t::ExtractClk48(observed_cell);

            if (!ExtractLowMidHighFromMode48_(raw48, published_count, claimed_count, faulty_count))
            {
                return false;
            }

            auto DecrementLocalityCount = [&](PackedCellLocalityTypes locality) noexcept->bool
            {
                switch (locality)
                {
                case PackedCellLocalityTypes::ST_IDLE :
                    return true;

                case PackedCellLocalityTypes::ST_PUBLISHED :
                    if (published_count > UNSIGNED_ZERO)
                    {
                        --published_count;
                        return true;
                    }
                    return false;

                case PackedCellLocalityTypes::ST_CLAIMED :
                    if (claimed_count > UNSIGNED_ZERO)
                    {
                        --claimed_count;
                        return true;
                    }
                    return false;

                case PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY :
                    if (faulty_count > UNSIGNED_ZERO)
                    {
                        --faulty_count;
                        return true;
                    }
                    return false;
                    
                default:
                    return false;
                }
            };

            auto IncrementLocalityCount = [&](PackedCellLocalityTypes locality) noexcept
            {
                switch (locality)
                {
                case PackedCellLocalityTypes::ST_IDLE :
                    return true;
                case PackedCellLocalityTypes::ST_PUBLISHED :
                    if (published_count < APC_MAX_LENGTH_OR_COUNTER)
                    {
                        published_count++;
                        return true;
                    }
                    return false;
                case PackedCellLocalityTypes::ST_CLAIMED :
                    if (claimed_count < APC_MAX_LENGTH_OR_COUNTER)
                    {
                        claimed_count++;
                        return true;
                    }
                    return false;
                case PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY :
                    if (faulty_count < APC_MAX_LENGTH_OR_COUNTER)
                    {
                        faulty_count++;
                        return true;
                    }
                    return false;
                default:
                    return false;
                }
            };

            const bool decrement_ok = DecrementLocalityCount(from_locality);
            const bool increment_ok = IncrementLocalityCount(to_locality);
            if (!increment_ok || !decrement_ok)
            {
                return false;
            }

            const packed64_t desired_cell = ComposeAPCOccupancyModel_16x3_48t(
                published_count, claimed_count, faulty_count, 
                APCPagedNodeSegmentClasses::CONTROL_SLOT,
                control_or_meta_cells_own_locality
            );

            packed64_t expected_cell = observed_cell;

            if (BackingPtr[static_cast<size_t>(meta_idx)].compare_exchange_strong(expected_cell, desired_cell, OnExchangeSuccess, OnExchangeFailure))
            {
                BackingPtr[static_cast<size_t>(meta_idx)].notify_all();
                return true;
            }
            observed_cell = expected_cell;

            if (AdaptiveBackoffOfAPCPtr_)
            {
                AdaptiveBackoffOfAPCPtr_->AdaptiveBackOffPacked(observed_cell);
            }
            else
            {
                std::this_thread::yield();
            }
            
        }
        
    }

    bool SegmentIODefinition::ApplyCentralAndRegionOccupancyTransitionCell(
        packed64_t old_cell,
        packed64_t new_cell,
        APCPagedNodeSegmentClasses physical_page_class
    ) noexcept
    {
        const PackedCellLocalityTypes from_locality = PackedCell64_t::ExtractLocalityFromPacked(old_cell);
        const PackedCellLocalityTypes to_locality = PackedCell64_t::ExtractLocalityFromPacked(new_cell);
        if (from_locality == to_locality)
        {
            return true;
        }

        if (!APCAndPagedNodeHelpers::IsTrackedOccupancyPageClass(physical_page_class))
        {
            return false;
        }
        


        
        const bool region_ok = CasUpdateOccupancy3x16ThreeSubdivisionCell__(from_locality, to_locality, physical_page_class, PackedCellLocalityTypes::ST_PUBLISHED, false);

        if (!region_ok)
        {
            return false;
        }

        const bool central_ok = CasUpdateOccupancy3x16ThreeSubdivisionCell__(from_locality, to_locality, std::nullopt, PackedCellLocalityTypes::ST_PUBLISHED, true);

        if (!central_ok)
        {
            CasUpdateOccupancy3x16ThreeSubdivisionCell__(
                to_locality,
                from_locality,
                physical_page_class,
                PackedCellLocalityTypes::ST_PUBLISHED,
                false
            );
            return false;
        }

        RefreshReadyBitForRegionFromOccupancy(physical_page_class);
        return region_ok;
    }

    bool SegmentIODefinition::RefreshReadyBitForRegionFromOccupancy(APCPagedNodeSegmentClasses page_class) noexcept
    {
        if (!APCAndPagedNodeHelpers::IsDataConsumablePageClass(page_class))
        {
            return true;
        }
        const uint32_t published = ReadPublishedOccupancyOfAPageClass(page_class);
        if (published > UNSIGNED_ZERO)
        {
            return TurnOnReadyBitForDesiredPagedNode_(page_class);
        }
        return ClearTheDesiredPagedNodeReadyBit_(page_class);
    }

    size_t SegmentIODefinition::PayloadCapacityFromHeader() noexcept
    {
        const uint32_t payload_begain = METACELL_COUNT;
        const uint32_t payload_end  = GetTotalCapacityForThisAPC();
        if (payload_end > payload_begain)
        {
            return static_cast<size_t>(payload_end - payload_begain);
        }
        return UNSIGNED_ZERO;
    }

    uint16_t SegmentIODefinition::ReadTotalOccuPancyOfAnyPageClass(APCPagedNodeSegmentClasses page_class) noexcept
    {

        const packed64_t packed_cell = page_class != APCPagedNodeSegmentClasses::NANNULL ?
            ReadRegionOccupancyCombinedCell(page_class) : ReadCentralAPCOccupancyCellForThisPagedNode();

        const uint16_t full_combined_occupancy = GetTootalOccupancyFromPackedCell(packed_cell);
        return full_combined_occupancy;
    }
}