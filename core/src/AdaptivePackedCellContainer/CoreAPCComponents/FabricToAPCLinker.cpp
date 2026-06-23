#include "AdaptivePackedCellContainer/APCSegmentsCausalCordinator.hpp"

namespace PredictedAdaptedEncoding
{


    bool FabricToAPCLinker::BindExternalRawFabricBacking_(
        packed64_t* raw_cells_ptr,
        size_t cell_count,
        VagueTemoraryPremativeFabric* fabric_owner,
        uint64_t fabric_slot_idx,
        bool object_owned_by_fabric
    ) noexcept
    {
        if (!raw_cells_ptr || cell_count < MINIMUM_BRANCH_CAPACITY || !fabric_owner)
        {
            return false;
        }
        
        APCBackingCellAtomicRefViewTemp* backing_mem_view = APCBackingCellAtomicRefViewTemp::BuildBackingViewOverCells_(raw_cells_ptr, cell_count);
        if (!backing_mem_view)
        {
            return false;
        }

        BackingPtr = backing_mem_view;
        OwnedBackingView_ = backing_mem_view;
        OwnedRawBackingCells_ = nullptr;
        CapacityOfThisAPC_ = cell_count;
        FabricOwnerPtr_ = fabric_owner;
        FabricSlotIndex_ = fabric_slot_idx;
        FabricBackend_ = true;
        FabricObjectOwnedByFabric_ = object_owned_by_fabric;
        return true;
    }


    void FabricToAPCLinker::ReleseFabricBindingOnly_() noexcept
    {
        if (OwnedBackingView_)
        {
            APCBackingCellAtomicRefViewTemp::FreeBackingView_(OwnedBackingView_, CapacityOfThisAPC_);
        }

        OwnedBackingView_ = nullptr;
        BackingPtr = nullptr;
        OwnedRawBackingCells_ = nullptr;
        CapacityOfThisAPC_ = UNSIGNED_ZERO;
        FabricOwnerPtr_ = nullptr;
        FabricSlotIndex_ = APCDataStructure::APC_SIZE_SENTINAL;
        FabricBackend_ = false;
        FabricObjectOwnedByFabric_ = false;
    }

    void FabricToAPCLinker::SetFabricOwnerForGlobalAPC(VagueTemoraryPremativeFabric* fabric_owner) noexcept
    {
        FabricOwnerPtr_ = fabric_owner;
    }


}