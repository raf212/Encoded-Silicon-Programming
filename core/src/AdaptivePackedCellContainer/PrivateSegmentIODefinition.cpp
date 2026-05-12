#include "APCSegmentsCausalCordinator.hpp"
#include "PackedCellContainerManager.hpp"
#include <iostream>

namespace PredictedAdaptedEncoding
{

    void SegmentIODefinition::InitDefaultAPCSegmentedNodeLayout_() noexcept
    {
        const uint32_t payload_begain = METACELL_COUNT;
        const uint32_t payload_end = GetTotalCapacityForThisAPC();
        if (payload_end <= payload_begain)
        {
            return;
        }
        CompleteAPCNodeRegionsLayout full_paged_node_layout{};        
        

        BuidDefaultLayoutPlan_(full_paged_node_layout);

        if (!WriteAllRegionsLayoutToHeader_(full_paged_node_layout))
        {
            return;
        }

        WriteBrenchMeta32_(MetaIndexOfAPCNode::REGION_DIR_COUNT, static_cast<val32_t>(APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses));
        WriteBrenchMeta32_(MetaIndexOfAPCNode::EDGE_TABLE_COUNT, UNSIGNED_ZERO);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::WEIGHT_TABLE_COUNT, UNSIGNED_ZERO);
        TurnOnASegmentFlag(ControlEnumOfAPCSegment::HAS_LAYOUT_DIR);
        
    }

    void SegmentIODefinition::BuidDefaultLayoutPlan_(CompleteAPCNodeRegionsLayout& full_layout) noexcept
    {
        const uint32_t payload_begain = METACELL_COUNT;
        const uint32_t payload_end = GetTotalCapacityForThisAPC();
        if (payload_end <= payload_begain)
        {
            return;
        }
        full_layout.NormalizePercentagesIfNeeded();

        const uint32_t total_span = payload_end - payload_begain;
        uint32_t initial_cursor = payload_begain;

        const std::optional<uint16_t> maybe_current_global_version = ReadGlobalLayoutVersion_();
        const uint16_t current_or_start_version = maybe_current_global_version.has_value() ? *maybe_current_global_version : 1u;

        if (!WriteGlobalLayoutVersion_(current_or_start_version))
        {
            return;
        }
        
        auto AssignOne = [&](LayoutBoundsOfSingleRelNodeClass& one, bool keep_tail = false) noexcept
        {
            if (one.PAGE_LAYOUT_CLASS == APCPagedNodeRelMaskClasses::NANNULL)
            {
                one.BeginIndex = initial_cursor;
                one.EndIndex = initial_cursor;
                return;
            }
            one.BeginIndex = initial_cursor;
            one.VersionNumber = current_or_start_version;
            uint32_t wanted_span = one.ComputeWantedSpanFromTotal(total_span);
            if (one.PAGE_LAYOUT_CLASS != APCPagedNodeRelMaskClasses::FREE_SLOT)
            {
                wanted_span = std::max<uint32_t>(wanted_span, 2u);
            }
            if (keep_tail)
            {
                one.EndIndex = payload_end;
                initial_cursor = payload_end;
                return;
            }
            const uint32_t remaining_span = (payload_end > initial_cursor) ? (payload_end - initial_cursor) : UNSIGNED_ZERO;
            wanted_span = std::min<uint32_t>(wanted_span, remaining_span);
            one.EndIndex = initial_cursor + wanted_span;
            initial_cursor = one.EndIndex;
        };
        AssignOne(full_layout.FeedForwardLayout);
        AssignOne(full_layout.FeedBackwardLayout);
        AssignOne(full_layout.LateralLayout);
        AssignOne(full_layout.StateLayout);
        AssignOne(full_layout.ErrorLayout);
        AssignOne(full_layout.EdgeDescriptorLayout);
        AssignOne(full_layout.WeightLayout);
        AssignOne(full_layout.AUXLayout);
        AssignOne(full_layout.HeterogenousMemoryLayout);
        AssignOne(full_layout.LocalPairedPointerLayout);
        AssignOne(full_layout.DistancePairedLayout);

        full_layout.FreeLayout.BeginIndex = initial_cursor;
        full_layout.FreeLayout.EndIndex = payload_end;
    }

    bool SegmentIODefinition::WriteBoundsPairToHeader_(const LayoutBoundsOfSingleRelNodeClass layout_bound) noexcept
    {
        if (!APCAndPagedNodeHelpers::IsValidAccountingPageClass(layout_bound.PAGE_LAYOUT_CLASS))
        {
            return false;
        }
        
        if (layout_bound.BeginIndex > APC_MAX_LENGTH_OR_COUNTER || layout_bound.EndIndex > APC_MAX_LENGTH_OR_COUNTER)
        {
            return false;
        }
        
        const std::optional<uint16_t> maybe_global_version = ReadGlobalLayoutVersion_();
        if (maybe_global_version.has_value())
        {
            if (layout_bound.VersionNumber != *maybe_global_version)
            {
                return false;
            }
        }
        
        return SetLayOutBounds(
            layout_bound.PAGE_LAYOUT_CLASS,
            static_cast<uint16_t>(layout_bound.BeginIndex),
            static_cast<uint16_t>(layout_bound.EndIndex),
            layout_bound.VersionNumber
        );
    }



    bool SegmentIODefinition::UpdateAPCModeFlagsInHeader_(uint32_t flags_to_turn_on, uint32_t flags_to_turn_off, MetaIndexOfAPCNode desired_flag_idx) noexcept
    {
        if (desired_flag_idx != MetaIndexOfAPCNode::SEGMENT_CONF_FLAGS && desired_flag_idx != MetaIndexOfAPCNode::MANAGER_CONTROL_FLAGS)
        {
            return false;
        }
        
        while (true)
        {
            const uint32_t current_flags = ReadMetaCellValue32(desired_flag_idx);
            if (current_flags == BRANCH_SENTINAL)
            {
                return false;
            }
            
            uint32_t next_flags = current_flags;
            next_flags |= flags_to_turn_on;
            next_flags &= ~flags_to_turn_off;
            if (next_flags == current_flags)
            {
                return true;
            }
            if (JustUpdateValueOfMeta32(desired_flag_idx, current_flags, next_flags))
            {
                return true;
            }
        }
    }

    std::optional<CompleteAPCNodeRegionsLayout> SegmentIODefinition::ReadAndGetFullRegionLayout_() noexcept
    {   
        auto LoadOne = [&](APCPagedNodeRelMaskClasses desired_rel_mask, LayoutBoundsOfSingleRelNodeClass& out_one) noexcept->bool
        {
            if (!IsLayoutMutationFlagActive())
            {
                auto maybe_one = ReadLayoutBoundsAndVersion(desired_rel_mask);
                if (!maybe_one)
                {
                    return false;
                }
                out_one = *maybe_one;
                return true;
            }
            return false;
        };

        CompleteAPCNodeRegionsLayout out_layout{};
        LoadOne(APCPagedNodeRelMaskClasses::FEEDFORWARD_MESSAGE, out_layout.FeedForwardLayout);
        LoadOne(APCPagedNodeRelMaskClasses::FEEDBACKWARD_MESSAGE, out_layout.FeedBackwardLayout);
        LoadOne(APCPagedNodeRelMaskClasses::LATERAL_MESAGE, out_layout.LateralLayout);
        LoadOne(APCPagedNodeRelMaskClasses::STATE_SLOT, out_layout.StateLayout);
        LoadOne(APCPagedNodeRelMaskClasses::ERROR_SLOT, out_layout.ErrorLayout);
        LoadOne(APCPagedNodeRelMaskClasses::EDGE_DESCRIPTOR, out_layout.EdgeDescriptorLayout);
        LoadOne(APCPagedNodeRelMaskClasses::WEIGHT_SLOT, out_layout.WeightLayout);
        LoadOne(APCPagedNodeRelMaskClasses::AUX_SLOT, out_layout.AUXLayout);
        LoadOne(APCPagedNodeRelMaskClasses::HETEROGENOUS_MEMORY_MAYBE_PAIRED_POINTER_OR_RAW_APC_SEGMENT, out_layout.HeterogenousMemoryLayout);
        LoadOne(APCPagedNodeRelMaskClasses::PAIRED_POINTER_LOCAL_MEMORY, out_layout.LocalPairedPointerLayout);
        LoadOne(APCPagedNodeRelMaskClasses::PAIRED_POINTER_DISTANCE_MEMORY, out_layout.DistancePairedLayout);
        LoadOne(APCPagedNodeRelMaskClasses::FREE_SLOT, out_layout.FreeLayout);
        //in complete layout any-one layout can be std::nullopt
        return out_layout;
    }

    bool SegmentIODefinition::WriteAllRegionsLayoutToHeader_(const CompleteAPCNodeRegionsLayout& full_layout) noexcept
    {
        if (!IsLayoutMutationFlagActive())
        {
            if (!TrySetLayoutMutationInFlight())
            {
                return false;
            }
        }

        auto ClearIfOwned = [this]() noexcept
        {
            if (IsLayoutMutationFlagActive())
            {
                ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
            }
        };
        
        const bool ok =
            WriteBoundsPairToHeader_(full_layout.FeedForwardLayout) &&
            WriteBoundsPairToHeader_(full_layout.FeedBackwardLayout) &&
            WriteBoundsPairToHeader_(full_layout.LateralLayout) &&
            WriteBoundsPairToHeader_(full_layout.StateLayout) &&
            WriteBoundsPairToHeader_(full_layout.ErrorLayout) &&
            WriteBoundsPairToHeader_(full_layout.EdgeDescriptorLayout) &&
            WriteBoundsPairToHeader_(full_layout.WeightLayout) &&
            WriteBoundsPairToHeader_(full_layout.AUXLayout) &&
            WriteBoundsPairToHeader_(full_layout.HeterogenousMemoryLayout) &&
            WriteBoundsPairToHeader_(full_layout.LocalPairedPointerLayout) &&
            WriteBoundsPairToHeader_(full_layout.DistancePairedLayout) &&
            WriteBoundsPairToHeader_(full_layout.FreeLayout);

        if (!ok)
        {
            ClearIfOwned();
            return false;
        }

        const bool version_ok = WriteGlobalLayoutVersion_(full_layout.FeedBackwardLayout.VersionNumber + 1u);

        ClearIfOwned();
        return version_ok;
    }

    bool SegmentIODefinition::TurnOnReadyBitForDesiredPagedNode_(APCPagedNodeRelMaskClasses desired_region_class) noexcept
    {
        const uint32_t anew_readybit = APCAndPagedNodeHelpers::MakeOneAPCNodeClassReadyBit(desired_region_class);
        if (anew_readybit == 0)
        {
            return false;
        }
        while (true)
        {
            const uint32_t compleate_current_paged_node_ready_bit = ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT);
            const uint32_t updated_current_ready_bit = compleate_current_paged_node_ready_bit | anew_readybit;
            if (updated_current_ready_bit == compleate_current_paged_node_ready_bit)
            {
                return true;
            }
            if (JustUpdateValueOfMeta32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT, compleate_current_paged_node_ready_bit, updated_current_ready_bit))
            {
                return true;
            }
        }
        return false;
    }

    bool SegmentIODefinition::ClearTheDesiredPagedNodeReadyBit_(APCPagedNodeRelMaskClasses desired_region_class) noexcept
    {
        const uint32_t anew_readybit = APCAndPagedNodeHelpers::MakeOneAPCNodeClassReadyBit(desired_region_class);
        if (anew_readybit == 0)
        {
            return false;
        }
        while (true)
        {
            const uint32_t compleate_current_paged_node_ready_bit = ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT);
            const uint32_t updated_current_ready_bit = compleate_current_paged_node_ready_bit & ~anew_readybit;
            if (updated_current_ready_bit == compleate_current_paged_node_ready_bit)
            {
                return true;
            }
            if (JustUpdateValueOfMeta32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT, compleate_current_paged_node_ready_bit, updated_current_ready_bit))
            {
                return true;
            }
        }
        return false;
    }

    bool SegmentIODefinition::ResetALLOccupancy16x3ModelToZero_() noexcept
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



}