#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

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


    APCDescriptorRange APCHandleDescriptorConstructor::GetSegmentPoolBegainEndForSingleAPCDescription_(uint64_t single_description_index) noexcept
    {
        
        APCDescriptorRange desired_segment_pool_range{};

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

    APCDescriptorRange APCHandleDescriptorConstructor::ReadRangeForASingleAPCSlotFromAPCDescriptor_(uint64_t apc_slot_index) noexcept
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

    bool HashTablesConstructor::PublishHashKeyValueAtBucket_(
        size_t bucket_base,
        HashFilesCarrier& a_valid_hash_triplet,
        FabricTableSegmentClasses hash_table
    ) noexcept
    {
        if (
            !CoreOfFabricCoordinator::IsValidHashTable(hash_table) ||
            !a_valid_hash_triplet.IsValid ||
            bucket_base + HASH_BUCKED_WIDTH_OF_FABRIC > SlabCellCount_
        )
        {
            return false;
        }

        HashTableConf::SingleHashBuffer desired_hash_buffer = HashTableConf::BuildAndValidateAHashBufferFromTriplet(a_valid_hash_triplet);

        if (desired_hash_buffer[HashTableConf::VALIDATION_INDEX_HASH_BUFFER] != HashTableConf::VALIDATION_MARK_OF_HASH_TABLE_BUFFER)
        {
            return false;
        }
        
        return ClaimThenMemCopyFromArray_(bucket_base, HASH_BUCKED_WIDTH_OF_FABRIC, desired_hash_buffer);
        
    }

}
