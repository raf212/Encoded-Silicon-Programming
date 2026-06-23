#pragma once 
#include <array>
#include <utility>
#include "OccupancyOrchestrator.hpp"

namespace PredictedAdaptedEncoding
{

struct LayoutBoundsOfSingleRelNodeClass
{
    uint32_t BeginIndex = APCDataStructure::BRANCH_SENTINAL;
    uint32_t EndIndex = APCDataStructure::BRANCH_SENTINAL;
    APCPagedNodeSegmentClasses PAGE_LAYOUT_CLASS = APCPagedNodeSegmentClasses::NULLNAN;
    float InitialOrCurrentPercentage = 0u;
    uint16_t VersionNumber = 0u;

    static constexpr MetaIndexOfAPCNode GetLayoutCellMetaIndexForPageClass(
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
        return BeginIndex >= payload_begain && EndIndex >= BeginIndex && EndIndex <= payload_end && PAGE_LAYOUT_CLASS!= APCPagedNodeSegmentClasses::NULLNAN;
    }

    constexpr bool IsEmpty() const noexcept
    {
        return EndIndex <= BeginIndex || PAGE_LAYOUT_CLASS == APCPagedNodeSegmentClasses::NULLNAN;
    }

    constexpr uint32_t GetPayloadSpan() const noexcept
    {
        return (EndIndex > BeginIndex) ? (EndIndex - BeginIndex) : 0u;
    }

    constexpr bool CanBorrowRightFrom(const LayoutBoundsOfSingleRelNodeClass& right) const noexcept
    {
        return EndIndex == right.BeginIndex && right.GetPayloadSpan() > 0u && right.PAGE_LAYOUT_CLASS != APCPagedNodeSegmentClasses::NULLNAN;
    }

    constexpr bool CanBorrowLeftFrom(const LayoutBoundsOfSingleRelNodeClass& left) const noexcept
    {
        return BeginIndex == left.EndIndex && left.GetPayloadSpan() > 0u && left.PAGE_LAYOUT_CLASS != APCPagedNodeSegmentClasses::NULLNAN;
    }

    constexpr bool TryGrowRight(uint32_t amount, LayoutBoundsOfSingleRelNodeClass& right) noexcept
    {
        if (!CanBorrowRightFrom(right) || amount == 0u || right.GetPayloadSpan() < amount)
        {
            return false;
        }
        EndIndex +=amount;
        right.BeginIndex +=amount;
        return true;            
    }

    constexpr bool TryGrowLeft(uint32_t amount, LayoutBoundsOfSingleRelNodeClass& left) noexcept
    {
        if (!CanBorrowLeftFrom(left) || amount == 0u || left.GetPayloadSpan() < amount)
        {
            return false;
        }
        BeginIndex -= amount;
        left.EndIndex -= amount;
        return true;
    }

    constexpr uint32_t ClampOrNormalize(uint32_t idx) const noexcept
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

        if (!APCAndPagedNodeHelpers::IsDataConsumablePageClass(a_cell_view.PageClass))
        {
            return false;
        }
        
        return true;
    }

    static constexpr bool ExtractLayoutModel_BegainL_EndM_VersionH(packed64_t packed_cell, uint16_t& begin_index, uint16_t& end_index, uint16_t& version_count) noexcept
    {
        if (!Subdevision16x3InternalMode48CellModel::IsThisCellASubdevision_3x16_48t(packed_cell))
        {
            return false;
        }

        const uint64_t raw48 = PackedCell64_t::ExtractRaw48FamilyBits(packed_cell);
        
        return Subdevision16x3InternalMode48CellModel::ExtractLowMidHighFromMode48_(raw48, begin_index, end_index, version_count);
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

    constexpr bool DoseAllPhysicalLayoutCarrySameVersionNumberAsGlobal(
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
    
    constexpr bool NormalizePercentagesIfNeeded() noexcept
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

    constexpr LayoutBoundsOfSingleRelNodeClass* GetALayoutByRelMask(APCPagedNodeSegmentClasses desired_rel_mask) noexcept
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
}