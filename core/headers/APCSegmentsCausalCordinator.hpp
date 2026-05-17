#pragma once
#include <array>
#include <utility>
#include "AdaptivePackedCellContainer/AdaptivePackedCellContainer.hpp"
#include "PackedCellContainerManager.hpp"

namespace PredictedAdaptedEncoding
{
class APCSegmentsCausalCordinator : public AdaptivePackedCellContainer
{
private:

     static MetaIndexOfAPCNode AcceptedClockIndex_(APCPagedNodeSegmentClasses region) noexcept
     {
         switch (region)
         {
         case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE:
             return MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_BACKWARD_CLOCK16;
    

         case APCPagedNodeSegmentClasses::STATE_SLOT:
         case APCPagedNodeSegmentClasses::ERROR_SLOT:
         case APCPagedNodeSegmentClasses::AUX_SLOT:
         case APCPagedNodeSegmentClasses::LATERAL_MESAGE:
         case APCPagedNodeSegmentClasses::EDGE_DESCRIPTOR:
         case APCPagedNodeSegmentClasses::WEIGHT_SLOT:
         case APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE:
         default:
             return MetaIndexOfAPCNode::LAST_ACCEPTED_FEED_FORWARD_CLOCK16;
         }
     }

    static MetaIndexOfAPCNode EmittedClockIndex_(APCPagedNodeSegmentClasses region) noexcept
    {
        return region == APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE
            ? MetaIndexOfAPCNode::LAST_EMITTED_FEED_BACKWARD_CLOCK16
            : MetaIndexOfAPCNode::LAST_EMITTED_FEED_FORWARD_CLOCK16;
    }

    bool TryAdvanceClock_(MetaIndexOfAPCNode idx, clk16_t candidate) noexcept
    {

        while (true)
        {
            const uint32_t current32 = ReadMetaCellValue32(idx);
            const clk16_t current = static_cast<clk16_t>(current32);

            if (current32 != UNSIGNED_ZERO &&
                !APCAndPagedNodeHelpers::INewerClock16(candidate, current))
            {
                return false;
            }

            if (JustUpdateValueOfMeta32(idx, current32, static_cast<uint32_t>(candidate), false))
            {
                return true;
            }
        }
    }

public:
    APCSegmentsCausalCordinator() noexcept = default;
    ~APCSegmentsCausalCordinator() = default;

    bool AcceptCausalCell(APCPagedNodeSegmentClasses region, packed64_t cell) noexcept
    {
        const clk16_t incoming = PackedCell64_t::ExtractClk16(cell);
        return TryAdvanceClock_(AcceptedClockIndex_(region), incoming);
    }

    bool MarkEmittedCausalCell(APCPagedNodeSegmentClasses region, packed64_t cell) noexcept
    {
        const clk16_t emitted = PackedCell64_t::ExtractClk16(cell);
        return TryAdvanceClock_(EmittedClockIndex_(region), emitted);
    }

    bool PublishCausal(
        APCPagedNodeSegmentClasses region,
        packed64_t cell,
        std::atomic<uint64_t>* growth_counter = nullptr
    ) noexcept
    {
        cell = PackedCell64_t::SetPageClassInPacked(cell, region);

        if (TryPublishRegionalSharedGrowthOnce(region, cell, growth_counter))
        {
            MarkEmittedCausalCell(region, cell);
            return true;
        }
        return false;
    }

    std::optional<packed64_t> ConsumeCausal(
        APCPagedNodeSegmentClasses region,
        size_t& scan_cursor,
        std::atomic<uint64_t>* older_counter = nullptr,
        bool drop_older = false
    ) noexcept
    {
        const size_t max_drops = drop_older
            ? (static_cast<size_t>(PayloadCapacityFromHeader()) * 2u + 16u)
            : 1u;
        size_t drops = 0;

        while (drops < max_drops)
        {
            auto maybe = ConsumeCellByRegionMaskTraverseStartFromThisAPC(region, scan_cursor);
            if (!maybe) return std::nullopt;

            if (AcceptCausalCell(region, *maybe))
                return maybe;

            if (older_counter)
                older_counter->fetch_add(1, std::memory_order_relaxed);

            if (!drop_older)
                return maybe;

            ++drops;
        }
        return std::nullopt;  // drop budget exhausted — caller should back off
    }

    uint32_t CountPublishedInRegion(APCPagedNodeSegmentClasses region) noexcept
    {
        uint32_t count = 0;
        for (size_t i = PayloadBegin(); i < GetTotalCapacityForThisAPC(); ++i)
        {
            packed64_t cell = BackingPtr[i].load(MoLoad_);
            if (APCAndPagedNodeHelpers::CanCellBeConsumedForThisRegion(cell, region))
            {
                ++count;
            }
        }
        return count;
    }

    bool HasAnyPublishedInChain(APCPagedNodeSegmentClasses region) noexcept
    {
        AdaptivePackedCellContainer* current = FindSharedRootOrThis();
        while (current)
        {
            auto* node = static_cast<APCSegmentsCausalCordinator*>(current);
            if (node->CountPublishedInRegion(region) > 0)
            {
                return true;
            }
            current = current->GetNextSharedSegment();
        }
        return false;
    }
};
}