#pragma once 
#include "SlabToFabricConverterAndCordinator.h"

namespace PredictedAdaptedEncoding
{

class VagueTemoraryPremativeFabric : public SlabToFabricConverterAndCordinator
{

private:

    /// @brief IN FUTURE EITHER GET RID OF THE TABLE OR: STORE INSIDE FABRICE BY USING  AttributePolicy::INSTRUCTION_RAW64_NEXT
    std::unique_ptr<std::atomic<AdaptivePackedCellContainer*>[]> APCRuntimePtrTable_{nullptr};
    std::vector<std::unique_ptr<AdaptivePackedCellContainer>> FabricOwnedAPCViews_{};

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

    std::optional<uint64_t> ConstructAnAPC_(   
        AdaptivePackedCellContainer& desired_apc,     
        const ContainerConf& container_conf,
        uint64_t shared_id = UNSIGNED_ZERO,
        uint64_t logical_id = UNSIGNED_ZERO
    ) noexcept;

    bool InitializeFabricWithPtrTable(
        uint16_t slot_count,
        size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
        uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
        uint32_t fabric_thread_capacity = DEFAULT_THREAD_TABLE_CAPACITY
    ) noexcept;

    void ShutDownFabricWithPtrTable() noexcept
    {
        FabricOwnedAPCViews_.clear();
        ClearAPCRuntimePtrTable_();
        APCRuntimePtrTable_.reset();
        ShutDownFabric();
    }

    AdaptivePackedCellContainer* GetAPCRuntimePtrByBranchId(uint64_t branch_id) noexcept
    {
        if (branch_id == UNSIGNED_ZERO || branch_id >= PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT)
        {
            return nullptr;
        }
        
        return GetAPCRuntimePtr(CoreOfFabricCoordinator::GetSlotIdxFromBranchId(branch_id));
    }

};


}