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
        
    // }
};


}