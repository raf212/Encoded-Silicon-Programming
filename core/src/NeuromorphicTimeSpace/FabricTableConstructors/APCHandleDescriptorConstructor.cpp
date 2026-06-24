#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.h"

namespace PredictedAdaptedEncoding
{

    APCSegmentPoolRange APCHandleDescriptorConstructor::GetSegmentPoolBegainEndForSingleAPCDescription_(uint64_t single_description_index) noexcept
    {
        
        APCSegmentPoolRange desired_segment_pool_range{};

        if (
            single_description_index >= CountOfAPC_ ||
            PerAPCRuntimeCellCount_ == UNSIGNED_ZERO
        )
        {
            return desired_segment_pool_range;
        }


        const uint64_t apc_count_offset = single_description_index * PerAPCRuntimeCellCount_;
        desired_segment_pool_range.BeginIndex = SegmentPoolBegin_ + static_cast<size_t>(apc_count_offset);
        desired_segment_pool_range.EndIndex = desired_segment_pool_range.BeginIndex + static_cast<size_t>(PerAPCRuntimeCellCount_);
        if (
            desired_segment_pool_range.BeginIndex >= SegmentPoolBegin_ &&
            desired_segment_pool_range.BeginIndex < desired_segment_pool_range.EndIndex &&
            desired_segment_pool_range.EndIndex <= SegmentPoolEnd_ &&
            desired_segment_pool_range.EndIndex <= SlabCellCount_
        )
        {
            desired_segment_pool_range.IsVAlid = true;
        }

        return desired_segment_pool_range;
        
    }

    APCDescriptorRange APCHandleDescriptorConstructor::ReadARangeOfAPCDescription_(uint64_t apc_slot_index) noexcept
    {
        APCDescriptorRange probable_full_range_of_apc_descriptor{};
        const bool ok = ReadAPCDescriptorTableBeginEndFromRecordBook(probable_full_range_of_apc_descriptor);

        APCDescriptorRange desired_slot_of_apc_descriptor{};

        if (!ok)
        {
            return desired_slot_of_apc_descriptor;
        }

        desired_slot_of_apc_descriptor.BeginIndex = probable_full_range_of_apc_descriptor.BeginIndex + static_cast<size_t>(apc_slot_index) * APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX;
        desired_slot_of_apc_descriptor.EndIndex = desired_slot_of_apc_descriptor.BeginIndex + APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX;
        desired_slot_of_apc_descriptor.IsVAlid = true;
        return desired_slot_of_apc_descriptor;
    }


    bool APCHandleDescriptorConstructor::ReadACompleateAPCDescriptorBuffer(
        uint64_t apc_description_index, 
        DescriptionOfAPC::SingleAPCDescriptionCellBuffer& return_buffer,
        bool claimed_is_invalid,
        OwnershipPolicy validate_observer,
        DescriptionOfAPC::StateOfSingleAPCDescription desired_state,
        std::optional<uint8_t> version_match
    ) noexcept
    {
        if (apc_description_index >= CountOfAPC_)
        {
            return false;
        }

        DescriptionOfAPC::BuildABlankAPCDescriptionBufferwith2CellIdentity(return_buffer);

        const APCDescriptorRange this_apc_descriptor_range = ReadARangeOfAPCDescription_(apc_description_index);

        if (!this_apc_descriptor_range.IsVAlid)
        {
            return false;
        }

        std::memcpy(
            &SlabBasePtr_[this_apc_descriptor_range.BeginIndex],
            &return_buffer,
            APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX * sizeof(packed64_t)
        );

        return DescriptionOfAPC::ValidateSingleAPCDescriptionBuffer(
            return_buffer,
            claimed_is_invalid,
            validate_observer,
            desired_state,
            apc_description_index,
            version_match
        );
    }


    bool APCHandleDescriptorConstructor::ClaimACompleateAPCDescriptorCells(uint64_t apc_description_idx) noexcept
    {
        if (apc_description_idx >= CountOfAPC_)
        {
            return false;
        }

        const APCDescriptorRange this_apc_descriptor_range = ReadARangeOfAPCDescription_(apc_description_idx);
        if (!this_apc_descriptor_range.IsVAlid)
        {
            return false;
        }
        
        return ClaimNxSequentialPackedCellStrong_(this_apc_descriptor_range.BeginIndex, APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX);
    }



    bool APCHandleDescriptorConstructor::OneShotUpdateAPCDescriptor(
        DescriptionOfAPC::SingleAPCDescriptionCellBuffer& a_valid_description_buffer,
        bool caller_holds_claim_guard
    ) noexcept
    {
        std::optional<uint64_t> current_descriptor_idx = DescriptionOfAPC::ReadADataValueFromADescriptionBuffer(a_valid_description_buffer, APCDescriptorCellType::CURRENT_DESCRIPTOR_INDEX);
        if (!current_descriptor_idx.has_value())
        {
            return false;
        }

        const APCDescriptorRange desired_descriptor_range = ReadARangeOfAPCDescription_(current_descriptor_idx.value());
        if (!desired_descriptor_range.IsVAlid)
        {
            return false;
        }

        if (!caller_holds_claim_guard)
        {
            return ClaimThenMemCopyFromArray_(
                desired_descriptor_range.BeginIndex,
                APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX,
                a_valid_description_buffer
            );
        }
        else
        {
            return ForceMemCopyFromArray_(
                desired_descriptor_range.BeginIndex,
                APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX,
                a_valid_description_buffer
            );
        }
    }
    
    DescriptionOfAPC::DescriptorSaftyFiles APCHandleDescriptorConstructor::OneShotTryReadingDescriptionState_(uint64_t apc_description_index) noexcept
    {
        DescriptionOfAPC::DescriptorSaftyFiles return_files{};

        if (!SlabBasePtr_ || apc_description_index >= CountOfAPC_)
        {
            return return_files;
        }
        
        const APCDescriptorRange desired_description_range = ReadARangeOfAPCDescription_(apc_description_index);
        if (!desired_description_range.IsVAlid)
        {
            return return_files;
        }
        const size_t state_cell_idx = desired_description_range.BeginIndex + static_cast<size_t>(APCDescriptorCellType::STATE_OWNERSHIP_VESION_SAFTY);
        const packed64_t state_of_apc_cell = SlabBasePtr_[state_cell_idx];

        return_files = DescriptionOfAPC::ReadFilesFromStateSaftyofADescriptor(state_of_apc_cell);
        return return_files;
    }


}
