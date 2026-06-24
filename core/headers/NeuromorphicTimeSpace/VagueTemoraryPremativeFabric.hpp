#pragma once 
#include "SlabToFabricConverterAndCordinator.h"

namespace PredictedAdaptedEncoding
{

class VagueTemoraryPremativeFabric : public SlabToFabricConverterAndCordinator
{

private:

    /// @brief IN FUTURE EITHER GET RID OF THE TABLE OR: STORE INSIDE FABRICE BY USING  AttributePolicy::INSTRUCTION_RAW64_NEXT
    std::unique_ptr<std::atomic<AdaptivePackedCellContainer*>[]> APCRuntimePtrTable_{nullptr};

    bool BuildAPCRuntimePtrTable_() noexcept
    {
        if (CountOfAPC_ == UNSIGNED_ZERO)
        {
            return false;
        }
        APCRuntimePtrTable_.reset(new (std::nothrow) std::atomic<AdaptivePackedCellContainer*>[static_cast<size_t>(CountOfAPC_)]);

        if (!APCRuntimePtrTable_)
        {
            return false;
        }

        for (size_t i = 0; i < static_cast<size_t>(CountOfAPC_); i++)
        {
            APCRuntimePtrTable_[i].store(nullptr, MoStoreSeq_);
        }
        
        return true;
    }

    void ClearAPCRuntimePtrTable_() noexcept
    {
        if (!APCRuntimePtrTable_)
        {
            return;
        }
        for (size_t i = 0; i < static_cast<size_t>(CountOfAPC_); i++)
        {
            APCRuntimePtrTable_[i].store(nullptr, MoStoreSeq_);
        }
    }

    bool StoreAPCRuntimePtr(size_t apc_idx, AdaptivePackedCellContainer* apc_ptr) noexcept
    {
        if (!APCRuntimePtrTable_ || apc_idx >= CountOfAPC_)
        {
            return false;
        }

        APCRuntimePtrTable_[apc_idx].store(apc_ptr, MoStoreSeq_);
        return true;
    }

    AdaptivePackedCellContainer* GetAPCRuntimePtr(size_t apc_idx) noexcept
    {
        if (!APCRuntimePtrTable_ || apc_idx >= CountOfAPC_)
        {
            return nullptr;
        }

        return APCRuntimePtrTable_[apc_idx].load(MoLoad_);
    }

public:

    // AdaptivePackedCellContainer* GetDeafultInitializedAPCFromFabric(
    //     const ContainerConf& container_conf,
    //     std::optional<uint64_t> shared_id = std::nullopt,
    //     std::optional<uint64_t>logical_id = std::nullopt
    // )
    // {
    //     if (
    //         !SlabBasePtr_ || 
    //         PerAPCRuntimeCellCount_ < MINIMUM_BRANCH_CAPACITY ||
    //         CountOfAPC_ == UNSIGNED_ZERO
    //     )
    //     {
    //         return nullptr;
    //     }

    //     uint64_t desired_apc_slot = PackedCell64_t::PACKED_CELL_SENTINAL;
    //     bool claimed_desired = false;


    //     for (uint64_t description_idx = 0; description_idx < CountOfAPC_; description_idx++)
    //     {
    //         const DescriptionOfAPC::DescriptorSaftyFiles desired_files = OneShotTryReadingDescriptionState_(description_idx);
    //         if (
    //             desired_files.IsValid && 
    //             desired_files.WidthOfAPC == PerAPCRuntimeCellCount_ &&
    //             desired_files.LocalityOfTheDescription ==LocalityPolicy::PUBLISHED && 
    //             desired_files.WhoHoldsTheAcess != OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER &&  
    //             desired_files.StateOfTheAPC == DescriptionOfAPC::StateOfSingleAPCDescription::RECORD_WITH_SEGMENT_POOL
    //         )
    //         {
    //             claimed_desired = ClaimACompleateAPCDescriptorCells(description_idx);
    //             if (claimed_desired)
    //             {
    //                 desired_apc_slot = description_idx;
    //                 break;
    //             }
    //         }
    //     }

    //     if (!claimed_desired && desired_apc_slot >= PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT)
    //     {
    //         return nullptr;
    //     }

    //     DescriptionOfAPC::SingleAPCDescriptionCellBuffer  desired_apc_description_buffer{};

    //     const bool buffer_ok = ReadACompleateAPCDescriptorBuffer(
    //         desired_apc_slot, 
    //         desired_apc_description_buffer, 
    //         false, 
    //         OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC, 
    //         DescriptionOfAPC::StateOfSingleAPCDescription::RECORD_WITH_SEGMENT_POOL
    //     );

    //     if (!buffer_ok)
    //     {
    //         return nullptr;
    //     }


        
        


        
        
    // }
};


}