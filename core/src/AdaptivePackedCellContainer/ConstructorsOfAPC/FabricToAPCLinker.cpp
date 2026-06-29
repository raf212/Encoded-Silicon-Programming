#include "AdaptivePackedCellContainer/AdaptivePackedCellContainer.hpp"
#include "NeuromorphicTimeSpace/VagueTemoraryPremativeFabric.hpp"

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
        IdxOfThisAPCInFabric_ = fabric_slot_idx;
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
        IdxOfThisAPCInFabric_ = APCDataStructure::APC_SIZE_SENTINAL;
        FabricBackend_ = false;
        FabricObjectOwnedByFabric_ = false;
    }

    void FabricToAPCLinker::SetFabricOwnerForGlobalAPC(VagueTemoraryPremativeFabric* fabric_owner) noexcept
    {
        FabricOwnerPtr_ = fabric_owner;
    }

    template<size_t NUMBER_OF_CELLS>
    bool FabricToAPCLinker::ClaimAndCopyToAPCFromArray(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept
    {
        if (!FabricOwnerPtr_)
        {
            return false;
        }
        
        APCSegmentPoolRange range_of_this_apc = FabricOwnerPtr_->GetSegmentPoolBegainEndForSingleAPCDescription_(IdxOfThisAPCInFabric_);
        if (!range_of_this_apc.IsVAlid)
        {
            return false;
        }

        const size_t start_idx_of_memcopy_in_slab = range_of_this_apc.BeginIndex + starting_idx_in_apc;

        if (start_idx_of_memcopy_in_slab + sequential_number_of_cells > range_of_this_apc.EndIndex)
        {
            return false;
        }
        
        return FabricOwnerPtr_->ClaimThenMemCopyFromArray_(start_idx_of_memcopy_in_slab, sequential_number_of_cells, source_cells);
    }

    template<size_t NUMBER_OF_CELLS>
    bool FabricToAPCLinker::ForceCopyToAPCFromArray(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept
    {
        if (!FabricOwnerPtr_)
        {
            return false;
        }
        
        APCSegmentPoolRange range_of_this_apc = FabricOwnerPtr_->GetSegmentPoolBegainEndForSingleAPCDescription_(IdxOfThisAPCInFabric_);
        if (!range_of_this_apc.IsVAlid)
        {
            return false;
        }

        const size_t start_idx_of_memcopy_in_slab = range_of_this_apc.BeginIndex + starting_idx_in_apc;

        if (start_idx_of_memcopy_in_slab + sequential_number_of_cells > range_of_this_apc.EndIndex)
        {
            return false;
        }
        
        return FabricOwnerPtr_->ForceMemCopyFromArray_(start_idx_of_memcopy_in_slab, sequential_number_of_cells, source_cells);

    }

    template<size_t NUMBER_OF_CELLS>
    bool FabricToAPCLinker::CopyFromAPCToANArrayBuffer(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& return_buffer
    ) noexcept
    {
        if (!FabricOwnerPtr_)
        {
            return false;
        }
        
        APCSegmentPoolRange range_of_this_apc = FabricOwnerPtr_->GetSegmentPoolBegainEndForSingleAPCDescription_(IdxOfThisAPCInFabric_);
        if (!range_of_this_apc.IsVAlid)
        {
            return false;
        }

        const size_t start_idx_of_memcopy_in_slab = range_of_this_apc.BeginIndex + starting_idx_in_apc;

        if (start_idx_of_memcopy_in_slab + sequential_number_of_cells > range_of_this_apc.EndIndex)
        {
            return false;
        }
        
        return FabricOwnerPtr_->ReadASnapShotFromSlab(start_idx_of_memcopy_in_slab, sequential_number_of_cells, return_buffer);

    }
}