#pragma once 
#include "APCSegmentsCausalCordinator.hpp"
#include "NeuromorphicAPCFabricCordinator.hpp"


namespace PredictedAdaptedEncoding
{

    bool NeuromorphicAPCFabricCordinator::UsePreAllocatedNodePoolOfAPC(size_t requested_apc_size) noexcept
    {
        const size_t slot_capacity = (
            requested_apc_size > UNSIGNED_ZERO && requested_apc_size <= APCDataStructure::APC_MAX_LENGTH_OR_COUNTER
        ) ? std::max<size_t>(requested_apc_size, MINIMUM_BRANCH_CAPACITY) : MINIMUM_BRANCH_CAPACITY;

        const size_t slab_bytes = (requested_apc_size > APCDataStructure::APC_MAX_LENGTH_OR_COUNTER) ?
            std::max<size_t>(requested_apc_size, DEFAULT_FABRIC_SLBM_BYTES) :  DEFAULT_FABRIC_SLBM_BYTES;

        return InitializeStorageFabric(slot_capacity, slab_bytes);
    }

    bool NeuromorphicAPCFabricCordinator::BindExternalRootAPCToFabric(AdaptivePackedCellContainer* apc_ptr, size_t wanted_capacity) noexcept
    {
        if (!apc_ptr || !IsFabricInitialized() || wanted_capacity > FabricSlotCapacityCells_)
        {
            return false;
        }

        const size_t a_free_slot_idx = PopFreeSlotRecordHeadOfAPCFabric_();
        if (a_free_slot_idx == APCDataStructure::APC_INDEX_SENTINAL)
        {
            return false;
        }

        const uint32_t current_slot_generation_after_add = SlotTablePtrs_[a_free_slot_idx].Generation.fetch_add(1u, std::memory_order_acq_rel) + 1u;

        HandleOfAPC buffer_handle{};
        buffer_handle.SlabId = UNSIGNED_ZERO;
        buffer_handle.SlotIndex = static_cast<uint16_t>(a_free_slot_idx);
        buffer_handle.Genaration = static_cast<uint16_t>(std::max<uint32_t>(current_slot_generation_after_add, APCDataStructure::BRANCH_VERSION));

        std::atomic<packed64_t>* backing_ptr = &FabricCellsPtrs_[a_free_slot_idx * FabricSlotCapacityCells_];
        ///
        if (!apc_ptr->BindExternalAPCBackingPtr(backing_ptr, FabricSlotCapacityCells_, buffer_handle, false))
        {
            PushFreeSlotRecordOfAPCFabric_(a_free_slot_idx);
            return false;
        }

        SlotTablePtrs_[a_free_slot_idx].ObjectProbableAPCPtr.store(reinterpret_cast<uintptr_t>(apc_ptr), MoStoreSeq_);
        return true;
        
    }

}


