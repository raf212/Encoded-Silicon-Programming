#include "APCSegmentsCausalCordinator.hpp"
#include "PackedCellContainerManager.hpp"
#include <iostream>

namespace PredictedAdaptedEncoding
{

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
        APCPagedNodeRelMaskClasses page_class,
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
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LOGICAL_NODE_ID, logical_node_id, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::SHARED_ID, shared_id, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::SHARED_PREVIOUS_ID, BRANCH_SENTINAL, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::SHARED_NEXT_ID, BRANCH_SENTINAL, PriorityPhysics::IDLE);
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
        WriteBrenchMeta32_(MetaIndexOfAPCNode::NODE_COMPUTE_KIND, static_cast<uint32_t>(compute_kind_of_node), PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::NODE_AUX_PARAM_U32, aux_param_uint32, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_FORWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_BACKWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LAST_EMITTED_FEED_FORWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LAST_EMITTED_FEED_BACKWARD_CLOCK16, UNSIGNED_ZERO, PriorityPhysics::IDLE);

        WriteBrenchMeta32_(MetaIndexOfAPCNode::FEEDFORWARD_IN_TARGET_ID, BRANCH_SENTINAL);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::FEEDFORWARD_OUT_TARGET_ID, BRANCH_SENTINAL);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::FEEDBACKWARD_IN_TARGET_ID, BRANCH_SENTINAL);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::FEEDBACKWARD_OUT_TARGET_ID, BRANCH_SENTINAL);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LATERAL_0_TARGET_ID, BRANCH_SENTINAL);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LATERAL_1_TARGET_ID, BRANCH_SENTINAL);
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
        
        WriteBrenchMeta32_(MetaIndexOfAPCNode::MAGIC_ID, BRANCH_MAGIC, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::BRANCH_ID, static_cast<uint32_t>(std::min<uint32_t>(branch_id, BRANCH_SENTINAL)), write_cell_priority);

        WriteBrenchMeta32_(MetaIndexOfAPCNode::BRANCH_DEPTH, branch_depth, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::BRANCH_PRIORITY, branch_priority, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS, container_configuration.EnableBranching ? static_cast<uint32_t>(ControlEnumOfAPCSegment::ENABLE_BRANCHING) : static_cast<uint32_t>(ControlEnumOfAPCSegment::NONE), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::CURRENT_ACTIVE_THREADS, UNSIGNED_ZERO, write_cell_priority);
        WritBranchMeta48_(MetaIndexOfAPCNode::COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48, UNSIGNED_ZERO);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::SPLIT_THRESHOLD_PERCENTAGE, container_configuration.BranchSplitThresholdPercentage, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::SEGMENT_KIND, static_cast<uint32_t>(APCPagedNodeRelMaskClasses::FREE_SLOT), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::MAX_DEPTH, container_configuration.BranchMaxDepth, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::TOTAL_CAPACITY_OF_THIS_SEGEMENT, safe_capacity, write_cell_priority);                                                                                        
        WriteBrenchMeta32_(MetaIndexOfAPCNode::LAST_SPLIT_EPOCH, UNSIGNED_ZERO, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::REGION_SIZE, static_cast<uint32_t>(container_configuration.RegionSize), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::REGION_COUNT, region_count, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT, UNSIGNED_ZERO, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::PRODUCER_BLOCK_SIZE, static_cast<uint32_t>(container_configuration.ProducerBlockSize), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::BACKGROUND_EPOCH_ADVANCE_MS, static_cast<uint32_t>(container_configuration.BackgroundEpochAdvanceMS), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::DEFINED_MODE_OF_CURRENT_APC, static_cast<uint32_t>(container_configuration.InitialMode), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::RETIRE_BRANCH_THRASHOLD, container_configuration.RetireBatchThreshold, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::PRODUCER_CURSOR_PLACEMENT, static_cast<uint32_t>(METACELL_COUNT), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::CONSUMER_CURSORE_PLACEMENT, static_cast<uint32_t>(METACELL_COUNT), write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::CURRENTLY_OWNED, UNSIGNED_ZERO, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH, UNSIGNED_ZERO, write_cell_priority);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::NODE_GROUP_SIZE, container_configuration.NodeGroupSize, write_cell_priority);
        InitLogicalNodeIdentity(logical_node_id, resolve_shared_id, is_root_shared);
        InitNodeSemantics(node_compute_kind, aux_param_uint32);
        InitDefaultAPCSegmentedNodeLayout_();
        for (uint8_t i = 0; i < APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses; i++)
        {
            WriteBrenchMeta32_(static_cast<MetaIndexOfAPCNode>(static_cast<size_t>(MetaIndexOfAPCNode::REGION_OCCUPANCY_NONE) + i), 
                            UNSIGNED_ZERO, write_cell_priority, APCPagedNodeRelMaskClasses::CONTROL_SLOT
                        );
        }

        ResetALLOccupancy16x3ModelToZero_();
        
        WriteBrenchMeta32_(MetaIndexOfAPCNode::EOF_APC_HEADER, EOF_HEADER, write_cell_priority);
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
        ///
        
        
        while (true)
        {
            uint32_t current_thread_count = ReadMetaCellValue32(MetaIndexOfAPCNode::CURRENT_ACTIVE_THREADS);
            if (current_thread_count == UINT32_MAX)
            {
                return false;
            }
            if (JustUpdateValueOfMeta32(MetaIndexOfAPCNode::CURRENT_ACTIVE_THREADS, current_thread_count, current_thread_count + change_count))
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
            const LayoutBoundsOfSingleRelNodeClass* current_layout = compleate_layout_of_this_apc.GetALayoutByRelMask(static_cast<APCPagedNodeRelMaskClasses>(rel_class));
            if (!current_layout || current_layout->IsEmpty())
            {
                continue;
            }

            const uint32_t payload_span = current_layout->GetPayloadSpan();
            if (payload_span == 0)
            {
                continue;
            }
            
            const APCPagedNodeRelMaskClasses page_class = current_layout->PAGE_LAYOUT_CLASS;
            if (!APCAndPagedNodeHelpers::IsValidAccountingPageClass(page_class))
            {
                continue;
            }
            const uint16_t sum_of_total_occupancy  = ReadTotalUsedOccupancyOfARegion(page_class);
            
            if (((static_cast<uint64_t>(sum_of_total_occupancy) * 100) / payload_span) >= split_threshold)
            {
                return true;
            } 
        }
        return false;
        
    }


    bool SegmentIODefinition::TryMarkSplitInFlight() noexcept
    {
        const uint32_t current_flags = ReadMetaCellValue32(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
        if (current_flags == BRANCH_SENTINAL)
        {
            return false;
        }

        bool is_already_in_flight = HasThisControlEnumFlag(ControlEnumOfAPCSegment::SPLIT_INFLIGHT);
        if (is_already_in_flight)
        {
            return false;
        }
        return TurnOnASegmentFlag(ControlEnumOfAPCSegment::SPLIT_INFLIGHT);
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




    std::optional<LayoutBoundsOfSingleRelNodeClass> SegmentIODefinition::ReadLayoutBounds(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        const MetaIndexOfAPCNode layout_idx = LayoutBoundsOfSingleRelNodeClass::GetLayoutCellMetaIndexForPageClass(page_class);
        if (!ValidMetaIdx(layout_idx))
        {
            return std::nullopt;
        }
        const packed64_t layout_cell = ReadFullMetaCell(layout_idx);
        uint16_t begin_16, end16, version16 = 0;
        if (!ExtractLayoutModel_BegainL_EndM_VersionH(layout_cell, begin_16, end16, version16))
        {
            return std::nullopt;
        }
        
        const uint32_t total_capacity = GetTotalCapacityForThisAPC();
        if (begin_16 < METACELL_COUNT || end16 < begin_16 || end16 > total_capacity)
        {
            return std::nullopt;
        }

        LayoutBoundsOfSingleRelNodeClass out_layout{};
        out_layout.BeginIndex = begin_16;
        out_layout.EndIndex = end16;
        out_layout.VersionNumber = version16;
        out_layout.PAGE_LAYOUT_CLASS = page_class;

        const uint32_t payload_capacity = total_capacity > METACELL_COUNT ? total_capacity - METACELL_COUNT : UNSIGNED_ZERO;

        if (payload_capacity > UNSIGNED_ZERO)
        {
            out_layout.SetOrResetPercentage(payload_capacity);
        }
        return out_layout;
    }

    bool SegmentIODefinition::SetLayOutBounds(APCPagedNodeRelMaskClasses desired_rel_mask, uint32_t begin, uint32_t end) noexcept
    {
        if (begin > end || begin < METACELL_COUNT || end > GetTotalCapacityForThisAPC())
        {
            return false;
        }

        auto maybe_begain_end = GetMetaBoundsLegalPairForPageClasses(desired_rel_mask);
        if (!maybe_begain_end)
        {
            return false;
        }

        // std::pair begin_end = *maybe_begain_end; kept for learning

        const auto [begin_meta, end_meta] = *maybe_begain_end;

        const uint32_t current_begain = ReadMetaCellValue32(begin_meta);
        const uint32_t current_end = ReadMetaCellValue32(end_meta);

        return JustUpdateValueOfMeta32(begin_meta, current_begain, begin) && JustUpdateValueOfMeta32(end_meta, current_end, end);
        
    }

    bool SegmentIODefinition::TrySetLayoutMutationInFlight() noexcept
    {
        while (true)
        {
            const uint32_t current_flags = ReadMetaCellValue32(MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS);
            if (current_flags == BRANCH_SENTINAL)
            {
                return false;
            }

            if (HasThisControlEnumFlag(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT))
            {
                return false;
            }
            return TurnOnASegmentFlag(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);            
        }
    }

    bool SegmentIODefinition::TryExtendASegmentInOwnAPC(APCPagedNodeRelMaskClasses desired_rel_mask, uint32_t wanted_amount, ContainerConf::APCSegmentExtendOrder desired_apc_order) noexcept
    {
        if (wanted_amount == 0)
        {
            return true;
        }
        if (!IsBound() || desired_rel_mask == APCPagedNodeRelMaskClasses::NANNULL)
        {
            return false;
        }
        if (!TrySetLayoutMutationInFlight())
        {
            return false;
        }
        
        auto maybe_current_complete_node_layout = ReadAndGetFullRegionLayout_();
        if (!maybe_current_complete_node_layout)
        {
            ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
            return false;
        }

        CompleteAPCNodeRegionsLayout current_complete_layout = *maybe_current_complete_node_layout;

        LayoutBoundsOfSingleRelNodeClass* target_layout_of_increment = current_complete_layout.GetALayoutByRelMask(desired_rel_mask);
        LayoutBoundsOfSingleRelNodeClass* free_slot_layout = current_complete_layout.GetALayoutByRelMask(APCPagedNodeRelMaskClasses::FREE_SLOT);

        if (!target_layout_of_increment || !free_slot_layout || target_layout_of_increment->IsEmpty())
        {
            ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
            return false;
        }
        const uint32_t payload_begain = METACELL_COUNT;
        const uint32_t payload_end = GetTotalCapacityForThisAPC();

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
            const bool ok = WriteAllRegionsLayoutToHeader_(current_complete_layout);
            ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
            return ok;
        }
        
        std::array<LayoutBoundsOfSingleRelNodeClass*, (CompleteAPCNodeRegionsLayout::CURRENT_TOTAL_APC_REL_NODE_CLASSES - 1)> candidates{};
        size_t count = 0;
        {
            auto all_layouts = current_complete_layout.OrderedViewsFIFO();
            for (auto* one_layout : all_layouts)
            {
                if (!one_layout)
                {
                    continue;
                }
                if (one_layout->PAGE_LAYOUT_CLASS == desired_rel_mask)
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

        auto PriorityScore = [] (const LayoutBoundsOfSingleRelNodeClass* one_layout) noexcept->uint32_t
        {
            if (!one_layout)
            {
                return UNSIGNED_ZERO;
            }
            return one_layout->GetPayloadSpan();
        };

        if (desired_apc_order == ContainerConf::APCSegmentExtendOrder::PRIORITY)
        {
            std::sort(candidates.begin(), candidates.begin() + count, 
                [&](const auto* priority_layout_1, const auto* priority_layout_2) noexcept
                {
                    return PriorityScore(priority_layout_1) > PriorityScore(priority_layout_2);
                }
            );
        }
        else if (desired_apc_order == ContainerConf::APCSegmentExtendOrder::RANDOM)
        {
            const uint32_t seed = ReadMetaCellValue32(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH) ^
                                ReadMetaCellValue32(MetaIndexOfAPCNode::BRANCH_ID);
            for (size_t i = 0; i < count; i++)
            {
                const size_t j = (static_cast<size_t>(seed) + i * (CompleteAPCNodeRegionsLayout::CURRENT_TOTAL_APC_REL_NODE_CLASSES - 1)) % count;
                std::swap(candidates[i], candidates[j]);
            }
            
        }

        for (size_t i = 0; i < count; i++)
        {
            if (!candidates[i])
            {
                continue;
            }
            if (TryFromSpecificNeighbor(*candidates[i]))
            {
                const bool ok = WriteAllRegionsLayoutToHeader_(current_complete_layout);
                ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
                return ok;
            }
        }
        ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
        return false;
        
    }

    clk16_t SegmentIODefinition::ReadLastAcceptedClok16ForThisSegment(APCPagedNodeRelMaskClasses region_kind) noexcept
    {
        switch (region_kind)
        {
            case APCPagedNodeRelMaskClasses::FEEDBACKWARD_MESSAGE :
                return static_cast<clk16_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_BACKWARD_CLOCK16));
        
        default:
            return static_cast<clk16_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_FORWARD_CLOCK16));
        }
    }

    clk16_t SegmentIODefinition::ReadLastEmittedClok16ForThisSegment(APCPagedNodeRelMaskClasses region_kind) noexcept
    {
        switch (region_kind)
        {
            case APCPagedNodeRelMaskClasses::FEEDBACKWARD_MESSAGE :
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

    uint16_t SegmentIODefinition::ReadRegionOccupancyOfALocality(PackedCellLocalityTypes locality_type, APCPagedNodeRelMaskClasses page_class) noexcept
    {
        const packed64_t packed_cell = ReadRegionOccupancyCombinedCell(page_class);

        std::optional<LayoutBoundsOfSingleRelNodeClass> maybe_bounds_of_the_page_class = ReadLayoutBounds(page_class);
        if (!maybe_bounds_of_the_page_class || maybe_bounds_of_the_page_class->IsEmpty())
        {
            return UNSIGNED_ZERO;
        }
        const std::optional<uint16_t> desired_region_occupancy = GetOccuupancyFromPackedCellMode48(
            packed_cell,
            locality_type,
            (static_cast<uint16_t>(maybe_bounds_of_the_page_class->GetPayloadSpan())
        ));
        return desired_region_occupancy && *desired_region_occupancy <= APC_MAX_LENGTH_OR_COUNTER ? *desired_region_occupancy : UNSIGNED_ZERO;
    } 


    bool SegmentIODefinition::CasUpdateOccupancy3x16ThreeSubdivisionCell(
        PackedCellLocalityTypes from_locality,
        PackedCellLocalityTypes to_locality,
        std::optional<APCPagedNodeRelMaskClasses> page_class
    ) noexcept
    {
        if (from_locality == to_locality)
        {
            return true;
        }

        const MetaIndexOfAPCNode meta_idx = page_class.has_value() ? APCAndPagedNodeHelpers::GetOccupancyMetIndexByRegionClass(*page_class) : MetaIndexOfAPCNode::COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48;
        const APCPagedNodeRelMaskClasses desired_page_class = page_class.has_value() ? *page_class : APCPagedNodeRelMaskClasses::CONTROL_SLOT;

        packed64_t observed_cell = ReadFullMetaCell(meta_idx);
        while (true)
        {
            uint16_t published_count = UNSIGNED_ZERO;
            uint16_t claimed_count = UNSIGNED_ZERO;
            uint16_t faulty_count = UNSIGNED_ZERO;
            if (IsThisCellASubdevision_3x16_48t(observed_cell))
            {
                const uint64_t raw48 = PackedCell64_t::ExtractClk48(observed_cell);
                const bool ok = ExtractLowMidHighFromMode48_(raw48, published_count, claimed_count, faulty_count);
                if (!ok)
                {
                    return false;
                }
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
                case PackedCellLocalityTypes::ST_CLAIMED :
                    if (claimed_count > UNSIGNED_ZERO)
                    {
                        --claimed_count;
                        return true;
                    }
                case PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY :
                    if (faulty_count > UNSIGNED_ZERO)
                    {
                        --faulty_count;
                        return true;
                    }
                    
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
                case PackedCellLocalityTypes::ST_CLAIMED :
                    if (claimed_count < APC_MAX_LENGTH_OR_COUNTER)
                    {
                        claimed_count++;
                        return true;
                    }
                case PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY :
                    if (faulty_count < APC_MAX_LENGTH_OR_COUNTER)
                    {
                        faulty_count++;
                        return true;
                    }
                
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
            const packed64_t desired_cell = ComposeAPCOccupancyModel_16x3_48t(published_count, claimed_count, faulty_count, desired_page_class);

            packed64_t expected_cell = observed_cell;

            if (BackingPtr[static_cast<size_t>(meta_idx)].compare_exchange_strong(expected_cell, desired_cell, OnExchangeSuccess, OnExchangeFailure))
            {
                BackingPtr[static_cast<size_t>(meta_idx)].notify_all();
                return true;
            }
            observed_cell = expected_cell;
        }
        
    }

    bool SegmentIODefinition::APPLYCentralAndRegionOccupancyTransitionCell(
        packed64_t old_cell,
        packed64_t new_cell,
        APCPagedNodeRelMaskClasses physical_page_class
    ) noexcept
    {
        const PackedCellLocalityTypes from_locality = PackedCell64_t::ExtractLocalityFromPacked(old_cell);
        const PackedCellLocalityTypes to_locality = PackedCell64_t::ExtractLocalityFromPacked(new_cell);
        if (from_locality == to_locality)
        {
            return true;
        }
        
        const bool total_ok = CasUpdateOccupancy3x16ThreeSubdivisionCell(from_locality, to_locality);
        if (!APCAndPagedNodeHelpers::IsValidAccountingPageClass(physical_page_class))
        {
            return total_ok;
        }

        const bool region_ok = CasUpdateOccupancy3x16ThreeSubdivisionCell(from_locality, to_locality, physical_page_class);

        RefreshReadyBitForRegionFromOccupancy(physical_page_class);
        return total_ok && region_ok;
    }

    bool SegmentIODefinition::RefreshReadyBitForRegionFromOccupancy(APCPagedNodeRelMaskClasses page_class) noexcept
    {
        if (!APCAndPagedNodeHelpers::IsValidAccountingPageClass(page_class))
        {
            return false;
        }
        const uint32_t published = ReadPublishedOccupancyOfAPageClass(page_class);
        if (published > UNSIGNED_ZERO)
        {
            return TurnOnReadyBitForDesiredPagedNode_(page_class);
        }
        return ClearTheDesiredPagedNodeReadyBit_(page_class);
    }
}