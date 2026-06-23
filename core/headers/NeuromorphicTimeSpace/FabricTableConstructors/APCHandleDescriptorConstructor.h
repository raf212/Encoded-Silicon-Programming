#pragma once 
#include "RecordBookConstructor.h"

namespace PredictedAdaptedEncoding
{
    class APCHandleDescriptorConstructor : public RecordBookConstructor
    {
    protected:
        /// @brief Read Description OF:: Single APC SLOT ||(WARNING: IT DOSENT READ THE MEMORY RANGE OF THE APC)
        /// @param apc_slot_index 
        /// @return 
        APCDescriptorRange ReadARangeOfAPCDescriptorFromRecordBook_(uint64_t apc_slot_index) noexcept;

    public:

        /// @brief Directly Gets Segment Pool Range For An APC by using INDEX: Of FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR | Dosent validate handle OR: Initialization 
        /// @param single_description_index Sequential index of desired APC FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR
        /// @return VALID: APCDescriptorRange::IsValid -> true || INVALID: APCDescriptorRange::IsValid -> true-> false
        APCSegmentPoolRange GetSegmentPoolBegainEndForSingleAPCDescription_(uint64_t single_description_index) noexcept;

        /// @brief Returns A return_buffer bbased obn set rules
        /// @param claimed_is_invalid MEANS: IF true -> LocalityPolicy::CLAIMED ->INVALID
        /// @param validate_observer Fabric can observe when ownership to APC but APC cant observe when ownership to fabric 
        /// @param desired_state  APC shouldnt observe StateOfSingleAPCDescription::EMPTY_RECORD / StateOfSingleAPCDescription::LOGICALY_RETIRED
        /// @param version_match VERSION:: Probably optional for now 
        /// @return 
        bool ReadACompleateAPCDescriptorBuffer(
            uint64_t apc_description_index, 
            DescriptionOfAPC::SingleAPCDescriptionCellBuffer& return_buffer,
            bool claimed_is_invalid = true,
            std::optional<OwnershipPolicy> validate_observer = std::nullopt,
            std::optional<DescriptionOfAPC::StateOfSingleAPCDescription> desired_state = std::nullopt,
            std::optional<uint8_t> version_match = std::nullopt
        ) noexcept;

        /// @brief CLAIMS: LocalityPolicy::CLAIMED Set TO: ALL: OF the descriptor Cells 
        /// @param apc_description_idx INDEX of the DEscription
        /// @return If Claimed -> true / false
        bool ClaimACompleateAPCDescriptorCells(uint64_t apc_description_idx) noexcept;

        /// @brief UPDATES: A Description In ONE SHOT
        bool OneShotUpdateAPCDescriptor(
            DescriptionOfAPC::SingleAPCDescriptionCellBuffer& a_valid_description_buffer,
            bool caller_holds_claim_guard = false
        ) noexcept;

        // std::optional<DescriptionOfAPC::StateOfSingleAPCDescription> OneShotTryReadingDescriptionState(uint64_t apc_description_index) noexcept
        // {
            
        // }
        
    };

}