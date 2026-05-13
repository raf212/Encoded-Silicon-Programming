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

        if (!WriteAllRegionsLayoutToHeader_(full_paged_node_layout, static_cast<uint16_t>(BRANCH_VERSION), false))
        {
            ClearOneControlEnumFlagOfAPC(
                ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT
            );
            return;
        }

        WriteBrenchMeta32_(MetaIndexOfAPCNode::REGION_DIR_COUNT, static_cast<val32_t>(APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses));
        WriteBrenchMeta32_(MetaIndexOfAPCNode::EDGE_TABLE_COUNT, UNSIGNED_ZERO);
        WriteBrenchMeta32_(MetaIndexOfAPCNode::WEIGHT_TABLE_COUNT, UNSIGNED_ZERO);
        TurnOnASegmentFlag(ControlEnumOfAPCSegment::HAS_LAYOUT_DIR);
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
        
        auto AssignOne = [&](LayoutBoundsOfSingleRelNodeClass& one) noexcept
        {
            if (!APCAndPagedNodeHelpers::IsValidAccountingPageClass(one.PAGE_LAYOUT_CLASS))
            {
                one.BeginIndex = initial_cursor;
                one.EndIndex = initial_cursor;
                return;
            }

            if (one.PAGE_LAYOUT_CLASS == APCPagedNodeRelMaskClasses::FREE_SLOT)
            {
                return;
            }
            
            one.BeginIndex = initial_cursor;
            uint32_t wanted_span = one.ComputeWantedSpanFromTotal(total_span);
            if (wanted_span == UNSIGNED_ZERO)
            {
                one.EndIndex = initial_cursor;
                return;
            }
            
            wanted_span = std::max<uint32_t>(wanted_span, MIN_REGION_SIZE);
            const uint32_t remaining = payload_end > initial_cursor ? (payload_end - initial_cursor) : UNSIGNED_ZERO;
            wanted_span = std::min<uint32_t>(wanted_span, remaining);
            one.EndIndex = initial_cursor + wanted_span;
            initial_cursor = one.EndIndex;
        };

        auto ordered = full_layout.OrderedViewsFIFO();
        for (auto* one : ordered)
        {
            if (!one)
            {
                return;
            }
            if (one->PAGE_LAYOUT_CLASS == APCPagedNodeRelMaskClasses::FREE_SLOT)
            {
                continue;
            }
            AssignOne(*one);
        }
        
        full_layout.FreeLayout.BeginIndex = initial_cursor;
        full_layout.FreeLayout.EndIndex = payload_end;
        full_layout.FreeLayout.PAGE_LAYOUT_CLASS = APCPagedNodeRelMaskClasses::FREE_SLOT;
    }

    bool SegmentIODefinition::WriteBoundsPairToHeader_(
        const LayoutBoundsOfSingleRelNodeClass layout_bound,
        std::optional<uint16_t> version_number,
        bool caller_holds_the_flag
    ) noexcept
    {
        if (!APCAndPagedNodeHelpers::IsValidAccountingPageClass(layout_bound.PAGE_LAYOUT_CLASS))
        {
            return false;
        }
        
        if (layout_bound.BeginIndex > APC_MAX_LENGTH_OR_COUNTER || layout_bound.EndIndex > APC_MAX_LENGTH_OR_COUNTER)
        {
            return false;
        }


        
        const uint32_t payload_begin = METACELL_COUNT;
        const uint32_t payload_end = GetTotalCapacityForThisAPC();
        if (!layout_bound.IsValid(payload_begin, payload_end))
        {
            return false;
        }

        const uint16_t resolved_version = version_number.has_value() && (*version_number > UNSIGNED_ZERO) ? *version_number : NextGlobalLayoutVersion_().value_or(static_cast<uint16_t>(BRANCH_VERSION));

        if (resolved_version == UNSIGNED_ZERO || resolved_version == APC_INDEX_SENTINAL)
        {
            return false;
        }
        
        return SetLayOutBounds(
            layout_bound.PAGE_LAYOUT_CLASS,
            static_cast<uint16_t>(layout_bound.BeginIndex),
            static_cast<uint16_t>(layout_bound.EndIndex),
            caller_holds_the_flag,
            resolved_version
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

    std::optional<CompleteAPCNodeRegionsLayout> SegmentIODefinition::ReadAndGetFullRegionLayout_(bool allow_read_while_mutating) noexcept
    {  
        if (IsLayoutMutationFlagActive() && !allow_read_while_mutating)
        {
            return std::nullopt;
        }
         
        auto LoadOne = [&](APCPagedNodeRelMaskClasses desired_rel_mask, LayoutBoundsOfSingleRelNodeClass& out_one) noexcept->bool
        {
            auto maybe_one = ReadLayoutBoundsAndVersion(desired_rel_mask);
            if (!maybe_one)
            {
                return false;
            }
            out_one = *maybe_one;
            return true;
        };

        CompleteAPCNodeRegionsLayout out_layout{};
        bool ok = true;
        ok = LoadOne(APCPagedNodeRelMaskClasses::FEEDFORWARD_MESSAGE, out_layout.FeedForwardLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::FEEDBACKWARD_MESSAGE, out_layout.FeedBackwardLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::LATERAL_MESAGE, out_layout.LateralLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::STATE_SLOT, out_layout.StateLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::ERROR_SLOT, out_layout.ErrorLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::EDGE_DESCRIPTOR, out_layout.EdgeDescriptorLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::WEIGHT_SLOT, out_layout.WeightLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::AUX_SLOT, out_layout.AUXLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::HETEROGENOUS_MEMORY_MAYBE_PAIRED_POINTER_OR_RAW_APC_SEGMENT, out_layout.HeterogenousMemoryLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::PAIRED_POINTER_LOCAL_MEMORY, out_layout.LocalPairedPointerLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::PAIRED_POINTER_DISTANCE_MEMORY, out_layout.DistancePairedLayout) && ok;
        ok = LoadOne(APCPagedNodeRelMaskClasses::FREE_SLOT, out_layout.FreeLayout) && ok;
        if (!ok)
        {
            return std::nullopt;
        }

        const std::optional<uint16_t> maybe_global_version = ReadGlobalLayoutVersion_();
        if (!maybe_global_version)
        {
            return std::nullopt;
        }
        
        
        if (!out_layout.DoseAllPhysicalLayoutCarrySameVersionNumberAsGlobal(*maybe_global_version))
        {
            return std::nullopt;
        }
        
        //in complete layout any-one layout can be std::nullopt
        return out_layout;
    }

    bool SegmentIODefinition::WriteAllRegionsLayoutToHeader_(
        const CompleteAPCNodeRegionsLayout& full_layout,
        std::optional<uint16_t> forced_version_number,
        bool caller_holds_the_flag
    ) noexcept
    {
        bool owns_layout_flag = false;

        if (!caller_holds_the_flag)
        {
            if (!TrySetLayoutMutationInFlight())
            {
                return false;
            }
            owns_layout_flag = true;
        }
        else
        {
            if (!IsLayoutMutationFlagActive())
            {
                return false;
            }
        }

        auto ClearIfOwned = [this, owns_layout_flag]() noexcept
        {
            if (owns_layout_flag)
            {
                ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT);
            }
        };

        uint16_t commit_version = 0;
        if (forced_version_number.has_value())
        {
            commit_version = *forced_version_number;
        }
        else
        {
            commit_version = NextGlobalLayoutVersion_().value_or(static_cast<uint16_t>(BRANCH_VERSION));
        }

        if (commit_version == UNSIGNED_ZERO || commit_version == APC_INDEX_SENTINAL)
        {
            ClearIfOwned();
            return false;
        }
        

        
        const bool ok =
            WriteBoundsPairToHeader_(full_layout.FeedForwardLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.FeedBackwardLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.LateralLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.StateLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.ErrorLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.EdgeDescriptorLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.WeightLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.AUXLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.HeterogenousMemoryLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.LocalPairedPointerLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.DistancePairedLayout, commit_version, true) &&
            WriteBoundsPairToHeader_(full_layout.FreeLayout, commit_version, true);

        if (!ok)
        {
            ClearIfOwned();
            return false;
        }

        const bool version_ok = WriteGlobalLayoutVersion_(commit_version);

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