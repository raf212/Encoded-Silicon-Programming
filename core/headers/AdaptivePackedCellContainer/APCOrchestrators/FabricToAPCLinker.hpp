#pragma once
#include <functional>
#include "LayoutBoundsOrchestrator.hpp"

namespace PredictedAdaptedEncoding
{
    
class VagueTemoraryPremativeFabric;

class FabricToAPCLinker : public APCDataStructure
{

protected:

    packed64_t* OwnedRawBackingCells_{nullptr};
    APCBackingCellAtomicRefViewTemp* OwnedBackingView_{nullptr};
    VagueTemoraryPremativeFabric* FabricOwnerPtr_{nullptr};
    uint64_t FabricSlotIndex_{APCDataStructure::APC_SIZE_SENTINAL};
    bool FabricBackend_{false};
    bool FabricObjectOwnedByFabric_{false};

/// UPDATE Candidates
    size_t CapacityOfThisAPC_{UNSIGNED_ZERO};
    Timer48 LocalTimer48_;
    std::function<void(const char*, const char*)> APCLogger_;
    std::vector<std::vector<uint64_t>> SOABitmapForAPC_;
    static inline std::atomic<uint32_t> GlobalBranchIdAlloc_{1};
///

    bool BindExternalRawFabricBacking_(
        packed64_t* raw_cells_ptr,
        size_t cell_count,
        VagueTemoraryPremativeFabric* fabric_owner,
        uint64_t fabric_slot_idx,
        bool object_owned_by_fabric
    ) noexcept;

    void ReleseFabricBindingOnly_() noexcept;

public:
    APCBackingCellAtomicRefViewTemp* BackingPtr{nullptr};

    void FreeAll() noexcept;

    void SetFabricOwnerForGlobalAPC(VagueTemoraryPremativeFabric* fabric_owner) noexcept;


    bool IsFabricBackend() const noexcept
    {
        return FabricBackend_;
    }

    uint64_t GetFabricSlotIndex() const noexcept
    {
        return FabricSlotIndex_;
    }

    VagueTemoraryPremativeFabric* GetFabricOwner() noexcept
    {
        return FabricOwnerPtr_;
    }

    bool IsBound() const noexcept
    {
        return BackingPtr != nullptr && CapacityOfThisAPC_ >= METACELL_COUNT;
    }

    size_t PayloadCapacity() const noexcept
    {
        return CapacityOfThisAPC_ > METACELL_COUNT ? (CapacityOfThisAPC_ - METACELL_COUNT) : 0u;
    }

    bool ValidMetaIdx(MetaIndexOfAPCNode idx) noexcept
    {
        return BackingPtr && static_cast<size_t>(idx) < CapacityOfThisAPC_ && static_cast<size_t>(idx) < METACELL_COUNT;
    }

    packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept
    {
        if (ValidMetaIdx(idx))
        {
            return BackingPtr[static_cast<size_t>(idx)].load(MoLoad_);
        }
        return PACKED_CELL_SENTENAL;
    }


    /// REMOVE CANDIDATES
    packed64_t* GetAPCBackinghPtr() noexcept
    {
        if (BackingPtr && BackingPtr->CellPtr)
        {
            return BackingPtr->CellPtr;
        }
        return nullptr;
    }


    void BindExternalStorage_(packed64_t* packed_ptr, size_t cell_count) noexcept
    {
        BackingPtr->CellPtr = packed_ptr;
        CapacityOfThisAPC_ = cell_count;
    }

    void UnbindExternalStorage_() noexcept
    {
        BackingPtr = nullptr;
        CapacityOfThisAPC_ = UNSIGNED_ZERO;
    }

    static constexpr uint32_t PayloadBegin() noexcept
    {
        return METACELL_COUNT;
    }


};
    
    
    
}