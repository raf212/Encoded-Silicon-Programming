#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{
    bool APCHandleDescriptorConstructor::ReadAPCDescriptorTableBeginEndFromRecordBook(
        APCDescriptorRange& return_APC_handle_description_range
    ) noexcept
    {
        SlabFabricTableBoundsCarrietFromRecordBookTable return_bounds{};

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

        if (single_description_index >= SlotCount_)
        {
            return desired_segment_pool_range;
        }

        desired_segment_pool_range.BeginIndex = SegmentPoolBegin_ + static_cast<size_t>(single_description_index);
        desired_segment_pool_range.EndIndex = desired_segment_pool_range.BeginIndex + static_cast<size_t>(PerAPCRuntimeCellCount_) - 1;
        if (
            desired_segment_pool_range.EndIndex > desired_segment_pool_range.BeginIndex &&
            desired_segment_pool_range.BeginIndex > APCDataStructure::METACELL_COUNT &&
            desired_segment_pool_range.EndIndex < SlabCellCount_
        )
        {
            desired_segment_pool_range.IsVAlid = true;
            return desired_segment_pool_range;
        }

        desired_segment_pool_range.IsVAlid = false;
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
        desired_slot_of_apc_descriptor.EndIndex = desired_slot_of_apc_descriptor.BeginIndex + APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX - 1;
        desired_slot_of_apc_descriptor.IsVAlid = true;
        return desired_slot_of_apc_descriptor;
    }

    bool APCHandleDescriptorConstructor::MemCopySingleAPCDescriptionIfValidFromBufferToSlabBasePtr_(
        DescriptionOfAPC::SingleAPCDescriptionCellBuffer& single_unvalidated_apc_description_buffer,
        DescriptionOfAPC::StateOfSingleAPCDescription updated_state,
        OwnershipPolicy updated_true_owner,
        bool check_consumeablity,
        std::optional<uint8_t> vesrion_match
    ) noexcept
    {
        bool buffer_ok = DescriptionOfAPC::ValidateSingleAPCDescriptionBuffer(
            single_unvalidated_apc_description_buffer, check_consumeablity, 
            updated_true_owner, updated_state, vesrion_match
        );

        if (
            !buffer_ok ||
            single_unvalidated_apc_description_buffer[APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX] != DescriptionOfAPC::VALID_BUFFER_MARK ||
            !SlabBasePtr_)
        {
            return false;
        }
        
        const uint64_t apc_description_index = PackedCell64_t::ExtractRaw48FamilyBits(single_unvalidated_apc_description_buffer[
            static_cast<size_t>(APCDescriptorCellType::CURRENT_DESCRIPTOR_INDEX)
        ]);

        const APCDescriptorRange desired_single_description_range =  ReadRangeForASingleAPCSlotFromAPCDescriptor_(apc_description_index);

        if (
            !desired_single_description_range.IsVAlid ||
            desired_single_description_range.EndIndex >= SlabCellCount_
        )
        {
            return false;
        }

        std::memcpy(
            &SlabBasePtr_[desired_single_description_range.BeginIndex],
            &SlabBasePtr_[desired_single_description_range.EndIndex],
            APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX * sizeof(packed64_t)
        );

        return true;

    }


    bool HashTablesConstructor::PublishHashKeyValueAtBucket_(
        size_t bucket_base,
        HashKeyValueDistanceTriplet& a_valid_hash_triplet,
        FabricTableSegmentClasses hash_table
    ) noexcept
    {
        if (
            !CoreOfFabricCoordinator::IsValidHashTable(hash_table) ||
            !a_valid_hash_triplet.IsValid
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
