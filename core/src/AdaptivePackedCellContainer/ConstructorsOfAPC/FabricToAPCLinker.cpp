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

    bool FabricToAPCLinker::IsThisAPCValidRange_(size_t width,  APCSegmentPoolRange* return_range) noexcept
    {
        if (!FabricOwnerPtr_)
        {
            return false;
        }
        
        const APCSegmentPoolRange range_of_this_apc = FabricOwnerPtr_->GetSegmentPoolBegainEndForSingleAPCDescription_(IdxOfThisAPCInFabric_);
        if (!range_of_this_apc.IsVAlid)
        {
            return false;
        }

        if (return_range)
        {
            *return_range = range_of_this_apc;
        }
        
        const size_t start_idx_of_memcopy_in_slab = range_of_this_apc.BeginIndex + width;

        if (start_idx_of_memcopy_in_slab + width > range_of_this_apc.EndIndex)
        {
            return false;
        }
        return true;
    }


    template<size_t NUMBER_OF_CELLS>
    bool FabricToAPCLinker::ClaimAndCopyToAPCFromArray(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept
    {
        APCSegmentPoolRange range_of_this_apc{};
        if (!IsThisAPCValidRange_(sequential_number_of_cells, &range_of_this_apc))
        {
            return false;
        }

        return FabricOwnerPtr_->ClaimThenMemCopyFromArray_(
            (range_of_this_apc.BeginIndex + starting_idx_in_apc), 
            sequential_number_of_cells, 
            source_cells
        );
    }

    template<size_t NUMBER_OF_CELLS>
    bool FabricToAPCLinker::ForceCopyToAPCFromArray(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& source_cells
    ) noexcept
    {
        APCSegmentPoolRange range_of_this_apc{};
        if (!IsThisAPCValidRange_(sequential_number_of_cells, &range_of_this_apc))
        {
            return false;
        }
        
        return FabricOwnerPtr_->ForceMemCopyFromArray_(
            (range_of_this_apc.BeginIndex + starting_idx_in_apc), 
            sequential_number_of_cells, 
            source_cells
        );
    }

    template<size_t NUMBER_OF_CELLS>
    bool FabricToAPCLinker::CopyFromAPCToANArrayBuffer(
        size_t starting_idx_in_apc,
        size_t sequential_number_of_cells,
        const std::array<packed64_t, NUMBER_OF_CELLS>& return_buffer
    ) noexcept
    {
        APCSegmentPoolRange range_of_this_apc{};
        if (!IsThisAPCValidRange_(sequential_number_of_cells, &range_of_this_apc))
        {
            return false;
        }
        
        return FabricOwnerPtr_->ReadASnapShotFromSlab(
            (range_of_this_apc.BeginIndex + starting_idx_in_apc), 
            sequential_number_of_cells, 
            return_buffer
        );
    }
}