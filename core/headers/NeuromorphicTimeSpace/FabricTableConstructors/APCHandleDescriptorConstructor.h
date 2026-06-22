#pragma once 
#include "RecordBookConstructor.h"

namespace PredictedAdaptedEncoding
{
    class APCHandleDescriptorConstructor : public RecordBookConstructor
    {
    protected:
        APCDescriptorRange ReadRangeForASingleAPCSlotFromAPCDescriptor_(uint64_t apc_slot_index) noexcept;

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