#pragma once 
#include "RecordBookConstructor.h"

namespace PredictedAdaptedEncoding
{
    class APCHandleDescriptorConstructor : public RecordBookConstructor
    {
    protected:
        APCDescriptorRange ReadRangeForASingleAPCSlotFromAPCDescriptor_(uint64_t apc_slot_index) noexcept;

        bool MemCopySingleAPCDescriptionIfValidFromBufferToSlabBasePtr_(
            DescriptionOfAPC::SingleAPCDescriptionCellBuffer& single_unvalidated_apc_description_buffer,
            DescriptionOfAPC::StateOfSingleAPCDescription updated_state,
            OwnershipPolicy updated_true_owner,
            bool check_consumeablity = false,
            std::optional<uint8_t> vesrion_match = std::nullopt
        ) noexcept;

    public:
        /// @brief Uses -> GetValidSlabRangeTripletFromRecordBookOfFTSC to get record and packs into -> APCDescriptorRange
        /// @return VALID::APCDescriptorRange.IsVAlid = true || INVALID:: APCDescriptorRange.IsVAlid = false
        bool ReadAPCDescriptorTableBeginEndFromRecordBook(
            APCDescriptorRange& return_APC_handle_description_range
        ) noexcept;

        /// @brief Directly Gets Segment Pool Range For An APC by using INDEX: Of FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR | Dosent validate handle OR: Initialization 
        /// @param single_description_index Sequential index of desired APC FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR
        /// @return VALID: APCDescriptorRange::IsValid -> true || INVALID: APCDescriptorRange::IsValid -> true-> false
        APCDescriptorRange GetSegmentPoolBegainEndForSingleAPCDescription_(uint64_t single_description_index) noexcept;


    };

}