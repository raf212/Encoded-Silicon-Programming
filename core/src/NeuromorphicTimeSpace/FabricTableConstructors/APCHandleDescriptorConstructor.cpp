#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.h"

namespace PredictedAdaptedEncoding
{
    bool APCHandleDescriptorConstructor::ReadAPCDescriptorTableBeginEndFromRecordBook(
        APCDescriptorRange& return_APC_handle_description_range
    ) noexcept
    {
        RecordBookTablesBoundsCarrier return_bounds{};

        bool bounds_ok = GetValidSlabRangeTripletFromRecordBookOfFTSC(FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR, return_bounds);

        if (!bounds_ok)
        {
            return_APC_handle_description_range.IsVAlid = false;
            return false;
        }

        return_APC_handle_description_range.BeginIndex = return_bounds.BeginIndex;
        return_APC_handle_description_range.EndIndex = return_bounds.EndIndex;
        return_APC_handle_description_range.IsVAlid = return_bounds.IsValid;

        return return_bounds.IsValid;
    }


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

    APCDescriptorRange APCHandleDescriptorConstructor::ReadARangeOfAPCDescriptorFromRecordBook_(uint64_t apc_slot_index) noexcept
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
        std::optional<OwnershipPolicy> validate_observer,
        std::optional<DescriptionOfAPC::StateOfSingleAPCDescription> desired_state,
        std::optional<uint8_t> version_match
    ) noexcept
    {
        if (apc_description_index >= CountOfAPC_)
        {
            return false;
        }

        DescriptionOfAPC::BuildABlankAPCDescriptionBufferwith2CellIdentity(return_buffer);

        const APCDescriptorRange this_apc_descriptor_range = ReadARangeOfAPCDescriptorFromRecordBook_(apc_description_index);

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
            version_match
        );
    }


    bool APCHandleDescriptorConstructor::ClaimACompleateAPCDescriptorCells(uint64_t apc_description_idx) noexcept
    {
        if (apc_description_idx >= CountOfAPC_)
        {
            return false;
        }

        const APCDescriptorRange this_apc_descriptor_range = ReadARangeOfAPCDescriptorFromRecordBook_(apc_description_idx);
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

        const APCDescriptorRange desired_descriptor_range = ReadARangeOfAPCDescriptorFromRecordBook_(current_descriptor_idx.value());
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
    


}
