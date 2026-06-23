
#pragma once 
#include <array>
#include <utility>
#include "APCEnums.h"

namespace PredictedAdaptedEncoding
{

    struct APCDataStructure 
    {
        static constexpr size_t METACELL_COUNT = 96;
        static constexpr uint32_t BRANCH_MAGIC = 0x41504342u;//big-endian
        static constexpr uint32_t EOF_HEADER = 0x72616600;//big-endian
        static constexpr uint16_t BRANCH_VERSION = 1u;
        static constexpr packed64_t PACKED_CELL_SENTENAL = UINT64_MAX;
        static constexpr uint32_t APC_MAX_LENGTH_OR_COUNTER = UINT16_MAX - 1;
        static constexpr uint32_t APC_INDEX_SENTINAL = UINT16_MAX;
        static constexpr uint32_t BRANCH_SENTINAL = IN_CELL_VALUE_MODE32_SENTINAL;
        static constexpr size_t APC_CACHELINE_SIZE = 64u;
        static constexpr size_t APC_SIZE_SENTINAL = SIZE_MAX;

        static constexpr uint32_t FABRIC_MAGIC = 0x41504643u;
        static constexpr uint32_t FABRIC_META_EOF = 0x41474946u;

protected:
        static constexpr bool DoseU32FitsInU16_(uint32_t value) noexcept
        {
            return value <= APC_MAX_LENGTH_OR_COUNTER;
        }

        static constexpr packed64_t* AllocateAlignedRawPackedCells_(size_t count)
        {
            if (count == UNSIGNED_ZERO)
            {
                return nullptr;
            }

            void* raw_ptr = nullptr;

            try
            {
                raw_ptr = ::operator new[](sizeof(packed64_t) * count, std::align_val_t{APC_CACHELINE_SIZE});
            }
            catch(...)
            {
                return nullptr;
            }
            
            auto* packed_cells = static_cast<packed64_t*>(raw_ptr);
            for (size_t i = 0; i < count; i++)
            {
                packed_cells[i] = PackedCell64_t::PACKED_CELL_SENTINAL;
            }
            return packed_cells;
        }

        static constexpr void FreeAlignedRawPackedCells_(packed64_t* backing_ptr) noexcept
        {
            if (!backing_ptr)
            {
                return;
            }
            ::operator delete[](static_cast<void*>(backing_ptr), std::align_val_t{APC_CACHELINE_SIZE});
        }
    };

    struct Timer48
    {
        static constexpr uint64_t TicksPerSec_ = A_BILLION;

        static constexpr uint64_t NowTicks() noexcept
        {
            using  cns = std::chrono::nanoseconds;
            auto d = std::chrono::steady_clock::now().time_since_epoch();
            uint64_t ns_count = static_cast<uint64_t>(std::chrono::duration_cast<cns>(d).count());
            return ns_count & MaskLowNBits(FAMILY_48_BIT_LEN);
        }

        static constexpr uint16_t NowClock16() noexcept
        {
            return static_cast<uint16_t>(NowTicks() & MaskLowNBits(LOW16_BIT_LEN));
        }
    };

    struct APCAndPagedNodeHelpers
    {
        static constexpr uint8_t HIGH_FOUR_NIBBLE = 0x0Fu;
        static constexpr uint8_t HIGH_ALL_EIGHT_NIBBLE = 0xFFu;
        static constexpr size_t SIZE_OF_APCPagedNodeRelMaskClasses = 16u;

        static constexpr bool INewerClock16(clk16_t candidate, clk16_t baseline) noexcept
        {
            if (candidate == baseline)
            {
                return false;
            }
            return static_cast<uint16_t>(candidate - baseline) < HALF16Bit_THRESHOLD_WRAP;
            
        }
        static constexpr APCPagedNodeSegmentClasses ExtractPagedRelMaskFromPacked (packed64_t packed_cell) noexcept
        {
            return static_cast<APCPagedNodeSegmentClasses>(PackedCell64_t::ExtractAPCPagedNodeSegmentClasse(packed_cell));
        }


        static constexpr uint32_t MakeOneAPCNodeClassReadyBit(APCPagedNodeSegmentClasses desired_rel_class) noexcept
        {
            const uint32_t rel_class = static_cast<uint8_t>(desired_rel_class) & HIGH_FOUR_NIBBLE;
            if (rel_class == static_cast<uint8_t>(APCPagedNodeSegmentClasses::NONE) || rel_class == static_cast<uint8_t>(APCPagedNodeSegmentClasses::NULLNAN))
            {
                return UNSIGNED_ZERO;
            }
            return (1u << rel_class);
        }


        static constexpr bool CanCellBeConsumedForThisRegion(packed64_t packed_cell, APCPagedNodeSegmentClasses region_kind) noexcept
        {
            return PackedCell64_t::ExtractLocalityPolicy(packed_cell) == LocalityPolicy::PUBLISHED &&
                ExtractPagedRelMaskFromPacked(packed_cell) == region_kind;        
            }

        static constexpr MetaIndexOfAPCNode GetOccupancyMetIndexByRegionClass(
            APCPagedNodeSegmentClasses desired_region_class
        )noexcept
        {
            return static_cast<MetaIndexOfAPCNode>(
                static_cast<size_t>(MetaIndexOfAPCNode::REGION_OCCUPANCY_NONE) +
                (static_cast<uint8_t>(desired_region_class) & HIGH_FOUR_NIBBLE)
                );
        }

        static constexpr bool IsEmbededControlCell(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {
            if (a_cell_view.PageClass == APCPagedNodeSegmentClasses::CONTROL_SLOT)
            {
                return true;
            }
            
            return false;
        }

        static constexpr bool IsEmbededTimerCell(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {
            return a_cell_view.CellMode == PackedMode::MODEL48 && 
                a_cell_view.SubClassOfModel48 == Model48Subclass::PURE_TIMER_48;
        }

        static constexpr bool IsThisCellAppropriateAndGenericToConsume(const PackedCell64_t::AuthoritiveCellView& a_cell_view, APCPagedNodeSegmentClasses page_class) noexcept
        {

            if (!a_cell_view.IsCellValid)
            {
                return false;
            }
            
            if (a_cell_view.LocalityOfCell != LocalityPolicy::PUBLISHED)
            {
                return false;
            }

            if (page_class != a_cell_view.PageClass)
            {
                return false;
            }
            
            if (!IsDataConsumablePageClass(a_cell_view.PageClass))
            {
                return false;
            }

            if (IsEmbededControlCell(a_cell_view) || IsEmbededTimerCell(a_cell_view))
            {
                return false;
            }

            return true;
        }


        static constexpr bool IsTrackedOccupancyPageClass(APCPagedNodeSegmentClasses page_class) noexcept
        {
            /*
                Occupancy-tracked means:
                - it has a REGION_OCCUPANCY_* counter cell
                - it can contribute PUBLISHED / CLAIMED / FAULTY to central occupancy

                This includes CONTROL_SLOT because metacells are real packed cells.
                This includes UNDEFINED because it is the quarantine/emergence lane.
                This includes FREE_SLOT only for non-idle abnormal transitions.
                Normal idle free capacity is still derived, not counted.
            */
            return page_class != APCPagedNodeSegmentClasses::NONE &&
                page_class != APCPagedNodeSegmentClasses::NULLNAN;
        }

        static constexpr bool IsDataConsumablePageClass(APCPagedNodeSegmentClasses page_class) noexcept
        {
            /*
                These regions may contain normal user/runtime data.
                CONTROL_SLOT is not data-consumable.
                FREE_SLOT is not data-consumable.
                NONE/NULLNAN are invalid.
            */
            return page_class != APCPagedNodeSegmentClasses::NONE &&
                page_class != APCPagedNodeSegmentClasses::NULLNAN &&
                page_class != APCPagedNodeSegmentClasses::CONTROL_SLOT &&
                page_class != APCPagedNodeSegmentClasses::FREE_SLOT;
        }

        static constexpr bool DoseThisCellUpdateableAsOccupancy16x3(
            const PackedCell64_t::AuthoritiveCellView& occupancy_cell_view,
            LocalityPolicy desired_cell_locality = LocalityPolicy::PUBLISHED
        ) noexcept
        {
            if (
                !occupancy_cell_view.IsCellValid || occupancy_cell_view.PageClass != APCPagedNodeSegmentClasses::CONTROL_SLOT ||
                occupancy_cell_view.CellMode != PackedMode::MODEL48 ||
                occupancy_cell_view.LocalityOfCell != desired_cell_locality ||
                occupancy_cell_view.SubClassOfModel48 != Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL
            )
            {
                return false;
            }
            return true;
        }

    static constexpr packed64_t TryToSwitchPageClassRuntime(
        packed64_t packed_cell,
        APCPagedNodeSegmentClasses page_class
    ) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView desired_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
        if (!desired_cell_view.IsCellValid)
        {
            return PackedCell64_t::MakeFaultyCell();
        }

        if (desired_cell_view.CellOwnership != OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER)
        {
            return packed_cell;
        }

        if (desired_cell_view.ContractOfValue == AccessContractOfValue::CAS_RMW)
        {
            return packed_cell;
        }

        if (desired_cell_view.LocalityOfCell == LocalityPolicy::CLAIMED)
        {
            switch (desired_cell_view.ContractOfValue)
            {

            case AccessContractOfValue::ATOMIC_SLNAPSHOT:
                return packed_cell;

            case AccessContractOfValue::CLAIMED_GURDED:
                return packed_cell;
            
            default:
                break;
            }
        }
        
        if (desired_cell_view.PageClass == APCPagedNodeSegmentClasses::CONTROL_SLOT)
        {
            return packed_cell;
        }

        packed_cell = PackedCell64_t::SetPageClassInPacked(packed_cell, page_class);
        
        return packed_cell;
        
    }

};

    struct ContainerConf
    {
        PackedMode InitialMode = PackedMode::MODEL32;
        size_t ProducerBlockSize = MIN_PRODUCER_BLOCK_SIZE;
        size_t RegionSize = MIN_REGION_SIZE;
        uint32_t RetireBatchThreshold = MIN_RETIRE_BATCH_THRESHOLD;
        uint32_t BackgroundEpochAdvanceMS = MIN_BACKGROUND_EPOCH_MS;
        bool EnableBranching = true;
        uint32_t BranchSplitThresholdPercentage = INITIAL_BRANCH_SPLIT_THRESHOLD_PERCENTAGE;
        uint32_t BranchMaxDepth = MAX_BRANCH_DEPTH;
        size_t BranchMinChildCapacity = MINIMUM_BRANCH_CAPACITY;
        uint32_t NodeGroupSize = 1u;

        enum class APCSegmentExtendOrder : uint8_t
        {
            FIFO = 0,
            PRIORITY = 1,
            RANDOM = 2
        };
    };

    struct PublishResult
    {
        PublishStatus ResultStatus{PublishStatus::INVALID};
        size_t Index{APCDataStructure::APC_SIZE_SENTINAL};
    };

}