
#pragma once 
#include <array>
#include <utility>
#include "APCDataStructure.hpp"

namespace PredictedAdaptedEncoding
{
    struct ContainerConf
    {
        PackedMode InitialMode = PackedMode::MODE_32;
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

    struct APCAndPagedNodeHelpers
    {
        static constexpr uint8_t HIGH_FOUR_NIBBLE = 0x0Fu;
        static constexpr uint8_t HIGH_ALL_EIGHT_NIBBLE = 0xFFu;
        static constexpr size_t SIZE_OF_APCPagedNodeRelMaskClasses = 16u;

        static  bool INewerClock16(clk16_t candidate, clk16_t baseline) noexcept
        {
            if (candidate == baseline)
            {
                return false;
            }
            return static_cast<uint16_t>(candidate - baseline) < HALF16Bit_THRESHOLD_WRAP;
            
        }
        static APCPagedNodeSegmentClasses ExtractPagedRelMaskFromPacked (packed64_t packed_cell) noexcept
        {
            return static_cast<APCPagedNodeSegmentClasses>(PackedCell64_t::ExtractRelMaskFromPacked(packed_cell));
        }


        static constexpr uint32_t MakeOneAPCNodeClassReadyBit(APCPagedNodeSegmentClasses desired_rel_class) noexcept
        {
            const uint32_t rel_class = static_cast<uint8_t>(desired_rel_class) & HIGH_FOUR_NIBBLE;
            if (rel_class == static_cast<uint8_t>(APCPagedNodeSegmentClasses::NONE) || rel_class == static_cast<uint8_t>(APCPagedNodeSegmentClasses::NANNULL))
            {
                return UNSIGNED_ZERO;
            }
            return (1u << rel_class);
        }


        static bool CanCellBeConsumedForThisRegion(packed64_t packed_cell, APCPagedNodeSegmentClasses region_kind) noexcept
        {
            return PackedCell64_t::ExtractLocalityFromPacked(packed_cell) == PackedCellLocalityTypes::PUBLISHED &&
                ExtractPagedRelMaskFromPacked(packed_cell) == region_kind &&
                PackedCell64_t::ExtractRelOffset32FromPacked(packed_cell) == SubClassesOfMode32::SELF_CLASS;
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

        static  bool IsEmbededControlCell(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {
            if (a_cell_view.PageClass == APCPagedNodeSegmentClasses::CONTROL_SLOT)
            {
                return true;
            }
            
            return false;
        }

        static  bool IsEmbededTimerCell(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {
            return a_cell_view.CellMode == PackedMode::MODE_48 && 
                a_cell_view.RelationOffsetForMode48.has_value() &&
                *a_cell_view.RelationOffsetForMode48 == SubClassesOfMode48::PURE_TIMER_48;
        }

        static  bool IsThisCellAppropriateAndGenericToConsume(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {


            if (!IsGenericPayloadOffset(a_cell_view))
            {
                return false;
            }

            if (a_cell_view.LocalityOfCell != PackedCellLocalityTypes::PUBLISHED)
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

        static  bool IsCellAppropriatelyPagedAndPublishedAsGeneric(
            const PackedCell64_t::AuthoritiveCellView& view,
            APCPagedNodeSegmentClasses region_kind
        ) noexcept
        {
            return IsThisCellAppropriateAndGenericToConsume(view) &&
                view.PageClass == region_kind;
        }    

        static  bool IsTrackedOccupancyPageClass(APCPagedNodeSegmentClasses page_class) noexcept
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
                page_class != APCPagedNodeSegmentClasses::NANNULL;
        }

        static  bool IsDataConsumablePageClass(APCPagedNodeSegmentClasses page_class) noexcept
        {
            /*
                These regions may contain normal user/runtime data.
                CONTROL_SLOT is not data-consumable.
                FREE_SLOT is not data-consumable.
                NONE/NANNULL are invalid.
            */
            return page_class != APCPagedNodeSegmentClasses::NONE &&
                page_class != APCPagedNodeSegmentClasses::NANNULL &&
                page_class != APCPagedNodeSegmentClasses::CONTROL_SLOT &&
                page_class != APCPagedNodeSegmentClasses::FREE_SLOT;
        }

        static  bool IsGenericPayloadOffset(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
        {
            if (!a_cell_view.IsCellValid)
            {
                return false;
            }
            
            if (a_cell_view.CellMode == PackedMode::MODE_32)
            {
                return a_cell_view.SubClassOfMode32.has_value() && *a_cell_view.SubClassOfMode32 == SubClassesOfMode32::SELF_CLASS;
            }
            if (a_cell_view.CellMode == PackedMode::MODE_48)
            {
                return a_cell_view.RelationOffsetForMode48.has_value() && *a_cell_view.RelationOffsetForMode48 == SubClassesOfMode48::SELF_CLASS;
            }
            return false;
        }

        static  bool DoseThisCellUpdateableAsOccupancy16x3(
            const PackedCell64_t::AuthoritiveCellView& occupancy_cell_view,
            PackedCellLocalityTypes desired_cell_locality = PackedCellLocalityTypes::PUBLISHED
        ) noexcept
        {
            if (
                !occupancy_cell_view.IsCellValid || occupancy_cell_view.PageClass != APCPagedNodeSegmentClasses::CONTROL_SLOT ||
                occupancy_cell_view.CellMode != PackedMode::MODE_48 ||
                occupancy_cell_view.LocalityOfCell != desired_cell_locality ||
                !occupancy_cell_view.RelationOffsetForMode48.has_value() ||
                *occupancy_cell_view.RelationOffsetForMode48 != SubClassesOfMode48::SUBDIVISION16x3_INTERNAL_CELL_MODEL
            )
            {
                return false;
            }
            return true;
        }

};
    
    struct LayoutBoundsOfSingleRelNodeClass
    {
        uint32_t BeginIndex = APCDataStructure::BRANCH_SENTINAL;
        uint32_t EndIndex = APCDataStructure::BRANCH_SENTINAL;
        APCPagedNodeSegmentClasses PAGE_LAYOUT_CLASS = APCPagedNodeSegmentClasses::NANNULL;
        float InitialOrCurrentPercentage = 0u;
        uint16_t VersionNumber = 0u;

        static  MetaIndexOfAPCNode GetLayoutCellMetaIndexForPageClass(
            APCPagedNodeSegmentClasses page_class
        ) noexcept
        {
            switch (page_class)
            {
                case APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE:
                    return MetaIndexOfAPCNode::MESSAGE_FEEDFORWARD_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE:
                    return MetaIndexOfAPCNode::MESSAGE_FEEDBACKWARD_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::LATERAL_MESAGE:
                    return MetaIndexOfAPCNode::LATERAL_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::STATE_SLOT:
                    return MetaIndexOfAPCNode::STATE_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::ERROR_SLOT:
                    return MetaIndexOfAPCNode::ERROR_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::EDGE_DESCRIPTOR:
                    return MetaIndexOfAPCNode::EDGE_DESCRIPTIOR_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::WEIGHT_SLOT:
                    return MetaIndexOfAPCNode::WEIGHT_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::AUX_SLOT:
                    return MetaIndexOfAPCNode::AUX_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::HETEROGENOUS_RAW_MEMORY:
                    return MetaIndexOfAPCNode::HETEROGENOUS_RAW_MEMORY_BOUNDS_VERSION;
                    
                case APCPagedNodeSegmentClasses::SLOT_TABLE_DESCRIPTOR:
                    return MetaIndexOfAPCNode::SLOT_TABLE_DESCRIPTOR_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::PAIRED_POINTER_IN_MEMORY:
                    return MetaIndexOfAPCNode::PAIRED_POINTER_IN_MEMORY_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::UNDEFINED:
                    return MetaIndexOfAPCNode::UNDEFINED_BOUNDS_VERSION;

                case APCPagedNodeSegmentClasses::FREE_SLOT:
                    return MetaIndexOfAPCNode::FREE_BOUNDS_VERSION;

                default:
                    return MetaIndexOfAPCNode::EOF_APC_HEADER;
            }
        }


        void SetOrResetPercentage(uint32_t total_capacity_of_apc) noexcept
        {
            InitialOrCurrentPercentage = static_cast<float>((static_cast<float>(GetPayloadSpan()) / static_cast<float>(total_capacity_of_apc)) * 100.00);
        }

        constexpr bool IsValid(uint32_t payload_begain, uint32_t payload_end) const noexcept
        {
            return BeginIndex >= payload_begain && EndIndex >= BeginIndex && EndIndex <= payload_end && PAGE_LAYOUT_CLASS!= APCPagedNodeSegmentClasses::NANNULL;
        }

        bool IsEmpty() const noexcept
        {
            return EndIndex <= BeginIndex || PAGE_LAYOUT_CLASS == APCPagedNodeSegmentClasses::NANNULL;
        }

        uint32_t GetPayloadSpan() const noexcept
        {
            return (EndIndex > BeginIndex) ? (EndIndex - BeginIndex) : 0u;
        }

        constexpr bool CanBorrowRightFrom(const LayoutBoundsOfSingleRelNodeClass& right) const noexcept
        {
            return EndIndex == right.BeginIndex && right.GetPayloadSpan() > 0u && right.PAGE_LAYOUT_CLASS != APCPagedNodeSegmentClasses::NANNULL;
        }

        constexpr bool CanBorrowLeftFrom(const LayoutBoundsOfSingleRelNodeClass& left) const noexcept
        {
            return BeginIndex == left.EndIndex && left.GetPayloadSpan() > 0u && left.PAGE_LAYOUT_CLASS != APCPagedNodeSegmentClasses::NANNULL;
        }

        bool TryGrowRight(uint32_t amount, LayoutBoundsOfSingleRelNodeClass& right) noexcept
        {
            if (!CanBorrowRightFrom(right) || amount == 0u || right.GetPayloadSpan() < amount)
            {
                return false;
            }
            EndIndex +=amount;
            right.BeginIndex +=amount;
            return true;            
        }

        bool TryGrowLeft(uint32_t amount, LayoutBoundsOfSingleRelNodeClass& left) noexcept
        {
            if (!CanBorrowLeftFrom(left) || amount == 0u || left.GetPayloadSpan() < amount)
            {
                return false;
            }
            BeginIndex -= amount;
            left.EndIndex -= amount;
            return true;
        }

        uint32_t ClampOrNormalize(uint32_t idx) const noexcept
        {
            if (IsEmpty())
            {
                return BeginIndex;
            }
            if (idx < BeginIndex || idx >= EndIndex)
            {
                return BeginIndex;
            }
            return BeginIndex + ((idx - BeginIndex) % GetPayloadSpan());
        }

        constexpr uint32_t ComputeWantedSpanFromTotal(uint32_t total_payload_span) const noexcept
        {
            return (static_cast<uint32_t>(InitialOrCurrentPercentage) * total_payload_span) / 100u;
        }

         bool DoseThisIndexPhysicallyExistInThisRegion(size_t index) const noexcept
        {
            return index >= BeginIndex && index < EndIndex;
        }
        
         bool CanCellBEConsumedForThisPhysicalRegion(
            packed64_t packed_cell,
            size_t idx
        ) noexcept
        {
            if (!DoseThisIndexPhysicallyExistInThisRegion(idx))
            {
                return false;
            }

            const PackedCell64_t::AuthoritiveCellView a_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);

            if (!a_cell_view.IsCellValid)
            {
                return false;
            }

            if (a_cell_view.PageClass != PAGE_LAYOUT_CLASS)
            {
                return false;
            }
            
            if (APCAndPagedNodeHelpers::IsEmbededControlCell(a_cell_view) || APCAndPagedNodeHelpers::IsEmbededTimerCell(a_cell_view))
            {
                return false;
            }
            
            return APCAndPagedNodeHelpers::IsCellAppropriatelyPagedAndPublishedAsGeneric(a_cell_view, PAGE_LAYOUT_CLASS);
        }

    };

    struct CompleteAPCNodeRegionsLayout
    {
        static constexpr LayoutBoundsOfSingleRelNodeClass MakeDefaultDesiredLayout(
            APCPagedNodeSegmentClasses desired_layout_class,
            uint8_t initial_percentage
        ) noexcept
        {
            return LayoutBoundsOfSingleRelNodeClass{
                APCDataStructure::BRANCH_SENTINAL,
                APCDataStructure::BRANCH_SENTINAL,
                desired_layout_class,
                static_cast<float>(initial_percentage)
            };
        }

        LayoutBoundsOfSingleRelNodeClass FeedForwardLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, FEEDFOEWARD_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass FeedBackwardLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE, FEEDBACKWARD_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass LateralLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::LATERAL_MESAGE, UNSIGNED_ZERO)};
        LayoutBoundsOfSingleRelNodeClass StateLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::STATE_SLOT, STATESLOT_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass ErrorLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::ERROR_SLOT, ERRORSLOT_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass EdgeDescriptorLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::EDGE_DESCRIPTOR, EDGEDESCRIPTOR_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass WeightLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::WEIGHT_SLOT, WEIGHTSLOT_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass AUXLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::AUX_SLOT, AUXSLOT_PERCENTAGE)};
        LayoutBoundsOfSingleRelNodeClass HeterogenousMemoryLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::HETEROGENOUS_RAW_MEMORY, UNSIGNED_ZERO)};
        LayoutBoundsOfSingleRelNodeClass LocalPairedPointerLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::SLOT_TABLE_DESCRIPTOR, UNSIGNED_ZERO)};
        LayoutBoundsOfSingleRelNodeClass DistancePairedLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::PAIRED_POINTER_IN_MEMORY, UNSIGNED_ZERO)};
        LayoutBoundsOfSingleRelNodeClass UndefinedLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::UNDEFINED, UNSIGNED_ZERO)};
        LayoutBoundsOfSingleRelNodeClass FreeLayout{MakeDefaultDesiredLayout(APCPagedNodeSegmentClasses::FREE_SLOT, FREE_PERCENTAGE)};
        //we can add 8 more threrritically rel_mask = 4 bit ->16 classes 
        static constexpr uint8_t CURRENT_TOTAL_APC_REL_NODE_CLASSES = 13u;

        constexpr float SumOfPercentage() const noexcept
        {
            return FeedForwardLayout.InitialOrCurrentPercentage + FeedBackwardLayout.InitialOrCurrentPercentage + 
                    LateralLayout.InitialOrCurrentPercentage + StateLayout.InitialOrCurrentPercentage +
                    ErrorLayout.InitialOrCurrentPercentage + EdgeDescriptorLayout.InitialOrCurrentPercentage + 
                    WeightLayout.InitialOrCurrentPercentage + AUXLayout.InitialOrCurrentPercentage + 
                    HeterogenousMemoryLayout.InitialOrCurrentPercentage + LocalPairedPointerLayout.InitialOrCurrentPercentage +
                    DistancePairedLayout.InitialOrCurrentPercentage + UndefinedLayout.InitialOrCurrentPercentage +
                    FreeLayout.InitialOrCurrentPercentage;
        }

        bool DoseAllPhysicalLayoutCarrySameVersionNumberAsGlobal(
            uint16_t global_version_number
        ) noexcept
        {
            if (global_version_number == UNSIGNED_ZERO ||
                global_version_number == APCDataStructure::APC_INDEX_SENTINAL)
            {
                return false;
            }

            return
                FeedForwardLayout.VersionNumber == global_version_number &&
                FeedBackwardLayout.VersionNumber == global_version_number &&
                LateralLayout.VersionNumber == global_version_number &&
                StateLayout.VersionNumber == global_version_number &&
                ErrorLayout.VersionNumber == global_version_number &&
                EdgeDescriptorLayout.VersionNumber == global_version_number &&
                WeightLayout.VersionNumber == global_version_number &&
                AUXLayout.VersionNumber == global_version_number &&
                HeterogenousMemoryLayout.VersionNumber == global_version_number &&
                LocalPairedPointerLayout.VersionNumber == global_version_number &&
                DistancePairedLayout.VersionNumber == global_version_number &&
                UndefinedLayout.VersionNumber == global_version_number &&
                FreeLayout.VersionNumber == global_version_number;
        }
        
        bool NormalizePercentagesIfNeeded() noexcept
        {
            const float sum_of_default = SumOfPercentage();
            if (sum_of_default == 100.00)
            {
                return true;
            }
            if (sum_of_default == 0.00)
            {
                FreeLayout.InitialOrCurrentPercentage = 100.00;
                return true;
            }
            auto NormalizeOne = [sum_of_default](LayoutBoundsOfSingleRelNodeClass& one) noexcept
            {
                one.InitialOrCurrentPercentage = (one.InitialOrCurrentPercentage * 100) / sum_of_default;
            };
            
            NormalizeOne(FeedForwardLayout);
            NormalizeOne(FeedBackwardLayout);
            NormalizeOne(LateralLayout);
            NormalizeOne(StateLayout);
            NormalizeOne(ErrorLayout);
            NormalizeOne(EdgeDescriptorLayout);
            NormalizeOne(WeightLayout);
            NormalizeOne(AUXLayout);
            NormalizeOne(HeterogenousMemoryLayout);
            NormalizeOne(LocalPairedPointerLayout);
            NormalizeOne(DistancePairedLayout);
            NormalizeOne(UndefinedLayout);
            NormalizeOne(FreeLayout);

            float repaired_sum = SumOfPercentage();
            if (repaired_sum < 100)
            {
                FreeLayout.InitialOrCurrentPercentage = FreeLayout.InitialOrCurrentPercentage + (100 - repaired_sum);
            }
            return true;
        }

        LayoutBoundsOfSingleRelNodeClass* GetALayoutByRelMask(APCPagedNodeSegmentClasses desired_rel_mask) noexcept
        {
            switch (desired_rel_mask)
            {
                case APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE:  return &FeedForwardLayout;
                case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE: return &FeedBackwardLayout;
                case APCPagedNodeSegmentClasses::LATERAL_MESAGE     :  return &LateralLayout;
                case APCPagedNodeSegmentClasses::STATE_SLOT:           return &StateLayout;
                case APCPagedNodeSegmentClasses::ERROR_SLOT:           return &ErrorLayout;
                case APCPagedNodeSegmentClasses::EDGE_DESCRIPTOR:      return &EdgeDescriptorLayout;
                case APCPagedNodeSegmentClasses::WEIGHT_SLOT:          return &WeightLayout;
                case APCPagedNodeSegmentClasses::AUX_SLOT:             return &AUXLayout;
                case APCPagedNodeSegmentClasses::HETEROGENOUS_RAW_MEMORY:
                    return &HeterogenousMemoryLayout;
                case APCPagedNodeSegmentClasses::SLOT_TABLE_DESCRIPTOR: 
                    return &LocalPairedPointerLayout;
                case APCPagedNodeSegmentClasses::PAIRED_POINTER_IN_MEMORY:
                    return &DistancePairedLayout;
                case APCPagedNodeSegmentClasses::UNDEFINED:            return &UndefinedLayout;
                case APCPagedNodeSegmentClasses::FREE_SLOT:            return &FreeLayout;
                default:                                               return nullptr;
            }
        }
        const LayoutBoundsOfSingleRelNodeClass* GetALayoutByRelMask(APCPagedNodeSegmentClasses desired_rel_mask) const noexcept
        {
            switch (desired_rel_mask)
            {
                case APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE:  return &FeedForwardLayout;
                case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE: return &FeedBackwardLayout;
                case APCPagedNodeSegmentClasses::LATERAL_MESAGE     :  return &LateralLayout;
                case APCPagedNodeSegmentClasses::STATE_SLOT:           return &StateLayout;
                case APCPagedNodeSegmentClasses::ERROR_SLOT:           return &ErrorLayout;
                case APCPagedNodeSegmentClasses::EDGE_DESCRIPTOR:      return &EdgeDescriptorLayout;
                case APCPagedNodeSegmentClasses::WEIGHT_SLOT:          return &WeightLayout;
                case APCPagedNodeSegmentClasses::AUX_SLOT:             return &AUXLayout;
                case APCPagedNodeSegmentClasses::HETEROGENOUS_RAW_MEMORY:
                    return &HeterogenousMemoryLayout;
                case APCPagedNodeSegmentClasses::SLOT_TABLE_DESCRIPTOR: 
                    return &LocalPairedPointerLayout;
                case APCPagedNodeSegmentClasses::PAIRED_POINTER_IN_MEMORY:
                    return &DistancePairedLayout;
                case APCPagedNodeSegmentClasses::UNDEFINED:            return &UndefinedLayout;
                case APCPagedNodeSegmentClasses::FREE_SLOT:            return &FreeLayout;
                default:                                               return nullptr;
            }
        }

        std::array<LayoutBoundsOfSingleRelNodeClass*, CURRENT_TOTAL_APC_REL_NODE_CLASSES> OrderedViewsFIFO() noexcept
        {
            return {
                &FeedForwardLayout, &FeedBackwardLayout, 
                &LateralLayout, &StateLayout,  
                &ErrorLayout, &EdgeDescriptorLayout,
                &WeightLayout, &AUXLayout, 
                &HeterogenousMemoryLayout, &LocalPairedPointerLayout,
                &DistancePairedLayout, &UndefinedLayout,
                &FreeLayout
            };
        }
    };



    struct AcquirePairedPointerStruct
    {
        uint64_t AssembeledPtr = 0;
        size_t HeadIdx = APCDataStructure::APC_SIZE_SENTINAL;
        size_t TailIdx = APCDataStructure::APC_SIZE_SENTINAL;
        packed64_t HeadScreenshot = 0;
        packed64_t TailScreenshot = 0;
        SubClassesOfMode32 Position = SubClassesOfMode32::SELF_CLASS;
        bool Ownership = false;
    };

    enum class PublishStatus : uint8_t
    {
        OK = 0,
        FULL = 1,
        INVALID = 2
    };

    struct PublishResult
    {
        PublishStatus ResultStatus{PublishStatus::INVALID};
        size_t Index{APCDataStructure::APC_SIZE_SENTINAL};
    };
    
}
