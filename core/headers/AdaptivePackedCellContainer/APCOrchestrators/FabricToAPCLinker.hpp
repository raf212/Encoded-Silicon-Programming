#pragma once
#include <functional>
#include "LayoutBoundsOrchestrator.hpp"

namespace PredictedAdaptedEncoding
{
    
class VagueTemoraryPremativeFabric;
class AdaptivePackedCellContainer;

class FabricToAPCLinker : public APCDataStructure
{

protected:

    packed64_t* OwnedRawBackingCells_{nullptr};
    APCBackingCellAtomicRefViewTemp* OwnedBackingView_{nullptr};
    VagueTemoraryPremativeFabric* FabricOwnerPtr_{nullptr};
    uint64_t IdxOfThisAPCInFabric_{APCDataStructure::APC_SIZE_SENTINAL};
    bool FabricBackend_{false};
    bool FabricObjectOwnedByFabric_{false};

/// UPDATE Candidates
    size_t CapacityOfThisAPC_{UNSIGNED_ZERO};
    Timer48 LocalTimer48_;
    std::function<void(const char*, const char*)> APCLogger_;
    std::vector<std::vector<uint64_t>> SOABitmapForAPC_;
///

    void ReleseFabricBindingOnly_() noexcept;

public:
    APCBackingCellAtomicRefViewTemp* BackingPtr{nullptr};

    void FreeAll() noexcept;

    void SetFabricOwnerForGlobalAPC(VagueTemoraryPremativeFabric* fabric_owner) noexcept;

    template<size_t NUMBER_OF_CELLS>
    bool ClaimAndCopyToAPCFromArray(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept;

    template<size_t NUMBER_OF_CELLS>
    bool ForceCopyToAPCFromArray(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept;


    template<size_t NUMBER_OF_CELLS>
    bool CopyFromAPCToANArrayBuffer(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept;


    bool BindExternalRawFabricBacking_(
        packed64_t* raw_cells_ptr,
        size_t cell_count,
        VagueTemoraryPremativeFabric* fabric_owner,
        uint64_t fabric_slot_idx,
        bool object_owned_by_fabric
    ) noexcept;


    bool IsFabricBackend() const noexcept
    {
        return FabricBackend_;
    }

    uint64_t GetFabricSlotIndex() const noexcept
    {
        return IdxOfThisAPCInFabric_;
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

    static constexpr uint32_t PayloadBegin() noexcept
    {
        return METACELL_COUNT;
    }


};
    
    
    
}