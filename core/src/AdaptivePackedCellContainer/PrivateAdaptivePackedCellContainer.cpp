#include "AdaptivePackedCellContainer/AdaptivePackedCellContainer.hpp"
#include <iostream>

namespace PredictedAdaptedEncoding
{
    size_t AdaptivePackedCellContainer::GetHashedRendomizedStep_(size_t sequense_number) noexcept
    {
        uint64_t mix_hash = (
            (static_cast<uint64_t>(sequense_number) * ID_HASH_GOLDEN_CONST) ^ (static_cast<uint64_t>(sequense_number >> (VALBITS + 1)))
        );
        size_t step = 1;
        if (PayloadCapacityFromHeader() > 1)
        {
            step = static_cast<size_t>((mix_hash % (PayloadCapacityFromHeader() - 1)) + 1);
        }
        return step;
    }

    void AdaptivePackedCellContainer::InitZeroState_() noexcept
    {
        ResetALLOccupancy16x3ModelToZero_();
        MakeAPCBranchOwned();
        ResetTotalCASFailureForThisBranch();
        UpdateProducerCursorPlacement(static_cast<uint32_t>(PayloadBegin()));
        UpdateConsumerCursorPlacement(static_cast<uint32_t>(PayloadBegin()));
    }

    uint32_t AdaptivePackedCellContainer::ProducerORConsumerCursorSetAndGet_(std::optional<uint32_t> cursor_placement, int32_t increment_or_decrement_of_cursor, 
        bool* did_changed_easy_return, const MetaIndexOfAPCNode cursors_meta_idx
    ) noexcept
    {
        if (PayloadCapacityFromHeader() <= PayloadBegin())
        {
            if (did_changed_easy_return)
            {
                *did_changed_easy_return = false;
            }
            return SegmentIODefinition::BRANCH_SENTINAL;
        }
        
        while (true)
        {
            const uint32_t current_cursor_placement = static_cast<uint32_t>(ReadValuFromAPCMetaIndecies(cursors_meta_idx));
            uint32_t desired_cursor_place = current_cursor_placement;
            if (cursor_placement.has_value())
            {
                desired_cursor_place = cursor_placement.value();
            }

            else if (increment_or_decrement_of_cursor != 0)
            {
                if (current_cursor_placement == SegmentIODefinition::BRANCH_SENTINAL)
                {
                    if (did_changed_easy_return)
                    {
                        *did_changed_easy_return = false;
                    }
                    return current_cursor_placement;
                }
                const size_t payload_end = GetTotalCapacityForThisAPC();
                const size_t payload_capacity = (payload_end > PayloadBegin()) ? (payload_end - PayloadBegin()) : UNSIGNED_ZERO;
                if (payload_capacity == 0)
                {
                    if (did_changed_easy_return)
                    {
                        *did_changed_easy_return = false;
                    }
                    return SegmentIODefinition::BRANCH_SENTINAL;
                }
                
                size_t current_placement = static_cast<size_t>(PayloadBegin());
                if (current_cursor_placement >= PayloadBegin() && current_cursor_placement < payload_end)
                {
                    current_placement = static_cast<size_t>(current_cursor_placement);
                }

                const int64_t current_offset = static_cast<int64_t>(current_placement - PayloadBegin());
                int64_t next_offset = current_offset + static_cast<int64_t>(increment_or_decrement_of_cursor);

                const int64_t modulo = static_cast<int64_t>(payload_capacity);
                next_offset  = next_offset % modulo;
                if (next_offset < 0)
                {
                    next_offset = next_offset + modulo;
                }
                desired_cursor_place = static_cast<uint32_t>(PayloadBegin() + static_cast<uint32_t>(next_offset));
            }
            else
            {
                if (did_changed_easy_return)
                {
                    *did_changed_easy_return = false;
                }
                return current_cursor_placement;
            }
            
            if (desired_cursor_place < PayloadBegin())
            {
                desired_cursor_place = PayloadBegin();
            }
            else if (desired_cursor_place >= GetTotalCapacityForThisAPC())
            {
                desired_cursor_place = static_cast<uint32_t>(GetTotalCapacityForThisAPC() - 1u);
            }

            if (desired_cursor_place == current_cursor_placement)
            {
                if (did_changed_easy_return)
                {
                    *did_changed_easy_return = false;
                }
                return current_cursor_placement;
            }
            if (ReplaceOnlyMetaCellValue(cursors_meta_idx, current_cursor_placement, desired_cursor_place))
            {
                if (did_changed_easy_return)
                {
                    *did_changed_easy_return = true;
                }
                return desired_cursor_place;
            }
        }
    }


    std::optional<packed64_t> AdaptivePackedCellContainer::TryConsumeAndIdleFromRegionLocal_(
        APCPagedNodeSegmentClasses cell_class, size_t& scan_cursor,
        OwnershipPolicy desired_authority_of_updated_cell
    ) noexcept
    {
        (void) desired_authority_of_updated_cell;
        
        if (!IfAPCBranchValid())
        {
            return std::nullopt;
        }

        if (!APCAndPagedNodeHelpers::IsDataConsumablePageClass(cell_class))
        {
            return std::nullopt;
        }

        if (HasThisControlEnumFlag(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT))
        {
            return std::nullopt;
        }
        

        const auto maybe_current_region_bounds = ReadLayoutBoundsAndVersion(cell_class);
        if (!maybe_current_region_bounds.has_value() || maybe_current_region_bounds->IsEmpty())
        {
            return std::nullopt;
        }

        LayoutBoundsOfSingleRelNodeClass current_region_bounds = *maybe_current_region_bounds;
        const size_t region_capacity = current_region_bounds.GetPayloadSpan();

        if (scan_cursor < current_region_bounds.BeginIndex || scan_cursor >= current_region_bounds.EndIndex)
        {
            scan_cursor = current_region_bounds.BeginIndex;
        }

        for (size_t prob = 0; prob < region_capacity; prob++)
        {
            const size_t idx = current_region_bounds.BeginIndex + ((scan_cursor - current_region_bounds.BeginIndex + prob) % region_capacity);
            packed64_t current_cell = BackingPtr[idx].load(MoLoad_);
            const PackedCell64_t::AuthoritiveCellView current_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(current_cell);
            if (!APCAndPagedNodeHelpers::IsThisCellAppropriateAndGenericToConsume(current_cell_view, cell_class))
            {
                continue;
            }

            const packed64_t graceful_idle_cell = PackedCell64_t::MakeDefaultAPCPayloadCellOnMode(
                current_cell_view.CellMode,
                APCAndPagedNodeHelpers::IsTrackedOccupancyPageClass(current_cell_view.PageClass) ? current_cell_view.PageClass : APCPagedNodeSegmentClasses::UNDEFINED,
                current_cell_view.CellValueDataType
            );

            const packed64_t idle_cell = (graceful_idle_cell == PackedCell64_t::PACKED_CELL_SENTINAL)  ? 
                    PackedCell64_t::MakeModeledAPCValidPackedCell(static_cast<ModelFamily>(current_cell_view.CellMode)) : 
                        graceful_idle_cell;

            packed64_t expected_cell = current_cell;
            if (!BackingPtr[idx].compare_exchange_strong(expected_cell, idle_cell, OnExchangeSuccess, OnExchangeFailure))
            {
                TotalCASFailForThisBranchIncreaseAndGet(1);
                continue;
            }

            ApplyCentralAndRegionOccupancyTransitionCell(current_cell, idle_cell, cell_class);
            BackingPtr[idx].notify_all();
            TouchLocalMetaClock48();
            scan_cursor = idx + 1;
            if (scan_cursor >= current_region_bounds.EndIndex)
            {
                scan_cursor = current_region_bounds.BeginIndex;
            }

            return current_cell;
        }
        return std::nullopt;
    }


    PublishResult AdaptivePackedCellContainer::TryPublishToRegionLocal_(
        packed64_t packed_cell_for_publish, 
        APCPagedNodeSegmentClasses cell_class,
        uint16_t max_tries
    ) noexcept
    {
        const PublishResult failed_result{PublishStatus::INVALID, APCDataStructure::APC_SIZE_SENTINAL};
        if (!IfAPCBranchValid())
        {
            return failed_result;
        }

        if (!APCAndPagedNodeHelpers::IsDataConsumablePageClass(cell_class))
        {
            return failed_result;
        }
        if (HasThisControlEnumFlag(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT))
        {
            return {PublishStatus::FULL, APCDataStructure::APC_SIZE_SENTINAL};
        }

        const auto maybe_current_region_bounds = ReadLayoutBoundsAndVersion(cell_class);
        if (!maybe_current_region_bounds|| maybe_current_region_bounds->IsEmpty())
        {
            return failed_result;
        }

        const uint32_t begin_idx = maybe_current_region_bounds->BeginIndex;
        const uint32_t end_idx = maybe_current_region_bounds->EndIndex;
        const size_t span = (end_idx > begin_idx) ? (end_idx - begin_idx) : 0;

        if (span == 0)
        {
            return {PublishStatus::FULL, APCDataStructure::APC_SIZE_SENTINAL};
        }

        const size_t bounded_tries =
            std::min<size_t>(
                span,
                std::max<size_t>(1u, static_cast<size_t>(max_tries))
            );

        for (size_t tries = 0; tries < bounded_tries; tries++)        
        {
            const size_t current_index = begin_idx + ((begin_idx + tries) % span);
            packed64_t observed_cell = BackingPtr[current_index].load(MoLoad_);
            const PackedCell64_t::AuthoritiveCellView observed_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(observed_cell);

            if (observed_cell_view.LocalityOfCell != LocalityPolicy::IDLE)
            {
                continue;
            }
            if (!observed_cell_view.IsCellValid)
            {
                continue;
            }
            
            packed64_t claimd_local_inplace_cell = PackedCell64_t::SetLocalityInPacked(observed_cell, LocalityPolicy::CLAIMED);

            packed64_t expected_cell = observed_cell;
            if (!BackingPtr[current_index].compare_exchange_strong(expected_cell, claimd_local_inplace_cell, OnExchangeSuccess, OnExchangeFailure))
            {
                TotalCASFailForThisBranchIncreaseAndGet(1);
                continue;
            }

            ApplyCentralAndRegionOccupancyTransitionCell(observed_cell, claimd_local_inplace_cell, cell_class);
            BackingPtr[current_index].store(packed_cell_for_publish, MoStoreSeq_);
            BackingPtr[current_index].notify_all();
            ApplyCentralAndRegionOccupancyTransitionCell(claimd_local_inplace_cell, packed_cell_for_publish, cell_class);
            TouchLocalMetaClock48();
            return {PublishStatus::OK, current_index};
        }

        return {PublishStatus::FULL, APCDataStructure::APC_SIZE_SENTINAL};
    }

    uint32_t AdaptivePackedCellContainer::SuggestedInternalAPCExpension_(CompleteAPCNodeRegionsLayout* complete_layout, uint8_t prefared_percentage_of_free) noexcept
    {
        if (!complete_layout || prefared_percentage_of_free == UNSIGNED_ZERO)
        {
            return UNSIGNED_ZERO;
        }

        prefared_percentage_of_free = std::min<uint8_t>(prefared_percentage_of_free, 100u);
        
        complete_layout->NormalizePercentagesIfNeeded();
        LayoutBoundsOfSingleRelNodeClass* free_layout = complete_layout->GetALayoutByRelMask(APCPagedNodeSegmentClasses::FREE_SLOT);
        if (!free_layout || free_layout->IsEmpty())
        {
            return UNSIGNED_ZERO;
        }
        const uint32_t free_span = free_layout->GetPayloadSpan();
        if (free_span == UNSIGNED_ZERO)
        {
            return UNSIGNED_ZERO;
        }
        uint32_t suggested =
            static_cast<uint32_t>(
                (static_cast<uint64_t>(free_span) * prefared_percentage_of_free) / 100u
            );

        suggested = std::max<uint32_t>(suggested, 1u);
        suggested = std::min<uint32_t>(suggested, free_span);

        return suggested;

    }

    size_t AdaptivePackedCellContainer::FindGreatestCommonDivisor_(size_t a, size_t b) noexcept
    {
        while (b != 0)
        {
            const size_t modulo = a % b;
            a = b;
            b = modulo;
        }
        return a;
    }

    size_t AdaptivePackedCellContainer::MakeProbeStepCoPrime_(size_t seed, size_t region_capacity) const noexcept
    {
        if (region_capacity <= 1)
        {
            return 1;
        }
        size_t step = 1u + (seed % (region_capacity - 1u));
        while (FindGreatestCommonDivisor_(step, region_capacity) != 1u)
        {
            ++step;
            if (step >= region_capacity)
            {
                step = 1u;
            }
        }
        return step;
    }

    bool AdaptivePackedCellContainer::RebuildRegionIndexFromPayload_() noexcept
    {
        if (!IfAPCBranchValid())
        {
            return false;
        }
        const size_t region_size = static_cast<size_t>(ReadValuFromAPCMetaIndecies(MetaIndexOfAPCNode::REGION_SIZE));
        if (region_size == 0)
        {
            SOABitmapForAPC_.clear();
            return true;
        }

        const size_t number_of_regions = (PayloadCapacityFromHeader() + region_size - 1u) / region_size;
        const size_t words = (number_of_regions + BIT_LENGTH_OF_A_PACKED_CELL - 1) / BIT_LENGTH_OF_A_PACKED_CELL;
        SOABitmapForAPC_.assign(APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses, std::vector<uint64_t>(words, 0ull));
        uint32_t global_ready_mask = UNSIGNED_ZERO;
        for (size_t region = 0; region < number_of_regions; region++)
        {
            const size_t base = region * region_size;
            const size_t capacity_end = std::min(PayloadCapacityFromHeader(), base + region_size);
            uint32_t region_ready_mask = UNSIGNED_ZERO;
            uint64_t region_epoch = UNSIGNED_ZERO;
            for (size_t i = base; i < capacity_end; i++)
            {
                const size_t absolute_idx = PayloadBegin() + i;
                const packed64_t absolute_packed_cell = BackingPtr[absolute_idx].load(MoLoad_);
                if (PackedCell64_t::ExtractLocalityPolicy(absolute_packed_cell) != LocalityPolicy::PUBLISHED)
                {
                    continue;
                }
                const APCPagedNodeSegmentClasses absolute_cell_relation_mask = PackedCell64_t::ExtractAPCPagedNodeSegmentClasse(absolute_packed_cell);
                region_ready_mask |= APCAndPagedNodeHelpers::MakeOneAPCNodeClassReadyBit(absolute_cell_relation_mask);
                region_epoch = std::max<uint64_t>(region_epoch, PackedCell64_t::ExtractClk16(absolute_packed_cell));

                global_ready_mask |= region_ready_mask;
                if (region_ready_mask != UNSIGNED_ZERO)
                {
                    const size_t word = region / BIT_LENGTH_OF_A_PACKED_CELL;
                    const size_t bit = region % BIT_LENGTH_OF_A_PACKED_CELL;
                    const uint64_t region_mask = (1ull << bit);
                    for (unsigned rel_class = 0; rel_class < APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses; rel_class++)
                    {
                        if (region_ready_mask & (1u << rel_class))
                        {
                            SOABitmapForAPC_[rel_class][word] |= region_mask;
                        }
                    }
                }
            }
        }
        const uint32_t expected_mask = static_cast<uint32_t>(ReadValuFromAPCMetaIndecies(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT));

        ReplaceOnlyMetaCellValue(
            MetaIndexOfAPCNode::PAGED_NODE_READY_BIT,
            expected_mask,
            global_ready_mask
        );
        return true;
    }

    uint16_t AdaptivePackedCellContainer::ComputeAdaptivemaxTreies_(packed64_t packed_cell) noexcept
    {
        const APCPagedNodeSegmentClasses page_class =
            PackedCell64_t::ExtractAPCPagedNodeSegmentClasse(packed_cell);

        if (!APCAndPagedNodeHelpers::IsDataConsumablePageClass(page_class))
        {
            return 1u;
        }

        const AttributePolicy attribute =
            PackedCell64_t::ExtractPriorityPolicy(packed_cell);

        std::optional<LayoutBoundsOfSingleRelNodeClass> layout_bounds =
            ReadLayoutBoundsAndVersion(page_class);

        const uint32_t span =
            (layout_bounds && !layout_bounds->IsEmpty())
                ? layout_bounds->GetPayloadSpan()
                : 1u;

        const uint16_t used_occupancy =
            ReadTotalUsedOccupancyOfARegion(page_class);

        const uint32_t pressure_percentage =
            span > 0u
                ? static_cast<uint32_t>((uint64_t{used_occupancy} * 100u) / span)
                : 100u;

        uint64_t split_threshold =
            ReadValuFromAPCMetaIndecies(MetaIndexOfAPCNode::SPLIT_THRESHOLD_PERCENTAGE);

        if (split_threshold == 0u ||
            split_threshold == BRANCH_SENTINAL ||
            split_threshold > 100u)
        {
            split_threshold = INITIAL_BRANCH_SPLIT_THRESHOLD_PERCENTAGE;
        }

        const uint64_t cas_failure = ReadValuFromAPCMetaIndecies(MetaIndexOfAPCNode::TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH);

        const bool high_contention =
            cas_failure != BRANCH_SENTINAL &&
            cas_failure > static_cast<uint32_t>(used_occupancy + 4u);

        const bool near_full =
            pressure_percentage + 10u >= split_threshold;

        const bool low_pressure =
            pressure_percentage < (split_threshold / 2u);

        const bool at_max_depth =
            MaxDepthRead() > 0u && CurrentBranchDepthRead() >= MaxDepthRead();

        uint32_t budget = std::max<uint32_t>(1u, span / 4u);

        if (low_pressure)
        {
            budget = std::max<uint32_t>(budget, span / 2u);
        }

        if (near_full)
        {
            budget = std::max<uint32_t>(1u, budget / 2u);
        }

        if (high_contention)
        {
            budget = std::max<uint32_t>(1u, budget / 2u);
        }

        const uint32_t priority_u32 =
            static_cast<uint32_t>(attribute);

        if (priority_u32 > static_cast<uint32_t>(AttributePolicy::SELF_CONTAINED_DATA_OR_MODEL))
        {
            budget = std::max<uint32_t>(
                1u,
                budget / std::max<uint32_t>(1u, priority_u32)
            );
        }

        if (at_max_depth)
        {
            budget = std::max<uint32_t>(budget, span);
        }

        budget = std::clamp<uint32_t>(
            budget,
            1u,
            std::min<uint32_t>(span, APC_MAX_LENGTH_OR_COUNTER)
        );

        return static_cast<uint16_t>(budget);
    }


}