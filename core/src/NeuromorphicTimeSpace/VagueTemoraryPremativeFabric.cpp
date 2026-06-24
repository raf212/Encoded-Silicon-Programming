#pragma once
#include "NeuromorphicTimeSpace/VagueTemoraryPremativeFabric.hpp"
#include "AdaptivePackedCellContainer/AdaptivePackedCellContainer.hpp"
#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.h"

namespace PredictedAdaptedEncoding
{

    std::optional<uint64_t> VagueTemoraryPremativeFabric::ConstructAnAPC_(   
        AdaptivePackedCellContainer& desired_apc,     
        const ContainerConf& container_conf,
        uint64_t shared_id,
        uint64_t logical_id
    ) noexcept
    {
        if (!desired_apc.IfAPCBranchValid())
        {
            return std::nullopt;
        }

        if (
            !SlabBasePtr_ || 
            PerAPCRuntimeCellCount_ < MINIMUM_BRANCH_CAPACITY ||
            CountOfAPC_ == UNSIGNED_ZERO
        )
        {
            return std::nullopt;
        }

        uint64_t desired_apc_slot = PackedCell64_t::PACKED_CELL_SENTINAL;
        bool claimed_desired = false;

        for (uint64_t description_idx = 0; description_idx < CountOfAPC_; description_idx++)
        {
            const DescriptionOfAPC::DescriptorSaftyFiles desired_files = OneShotTryReadingDescriptionState_(description_idx);
            if (
                desired_files.IsValid && 
                desired_files.WidthOfAPC == PerAPCRuntimeCellCount_ &&
                desired_files.LocalityOfTheDescription ==LocalityPolicy::PUBLISHED && 
                desired_files.WhoHoldsTheAcess != OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER &&  
                desired_files.StateOfTheAPC == DescriptionOfAPC::StateOfSingleAPCDescription::RECORD_WITH_SEGMENT_POOL
            )
            {
                claimed_desired = ClaimACompleateAPCDescriptorCells(description_idx);
                if (claimed_desired)
                {
                    desired_apc_slot = description_idx;
                    break;
                }
            }
        }

        if (!claimed_desired && desired_apc_slot >= PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT)
        {
            return std::nullopt;
        }

        DescriptionOfAPC::SingleAPCDescriptionCellBuffer  desired_apc_description_buffer{};

        const bool buffer_ok = ReadACompleateAPCDescriptorBuffer(
            desired_apc_slot, 
            desired_apc_description_buffer, 
            false, 
            OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC, 
            DescriptionOfAPC::StateOfSingleAPCDescription::RECORD_WITH_SEGMENT_POOL
        );
        if (!buffer_ok)
        {
            return std::nullopt;
        }

        const packed64_t updated_safty = DescriptionOfAPC::SwitchStateOrAPCOwnerOfSaftyCell(
            desired_apc_description_buffer[static_cast<size_t>(APCDescriptorCellType::STATE_OWNERSHIP_VESION_SAFTY)],
            DescriptionOfAPC::StateOfSingleAPCDescription::OWNED_BY_APC,
            OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER
        );

        const bool update_ok_safty = DescriptionOfAPC::SetStateSaftyCellInBuffer(desired_apc_description_buffer, updated_safty);
        if (!update_ok_safty)
        {
            return std::nullopt;
        }


        const bool claimed_descripor_for_caller = OneShotUpdateAPCDescriptor(desired_apc_description_buffer, true);
        if (!claimed_descripor_for_caller)
        {
            return std::nullopt;
        }
        
        APCSegmentPoolRange desired_apc_segment_pool_range = GetSegmentPoolBegainEndForSingleAPCDescription_(desired_apc_slot);
        if (!desired_apc_segment_pool_range.IsVAlid)
        {
            return std::nullopt;
        }

        packed64_t* raw_apc_segment_ptr = &SlabBasePtr_[desired_apc_segment_pool_range.BeginIndex];

        if (!desired_apc.BindExternalRawFabricBacking_(
            raw_apc_segment_ptr,
            PerAPCRuntimeCellCount_,
            this,
            desired_apc_slot,
            true
        ))
        {
            return std::nullopt;
        }

        if (!StoreAPCRuntimePtr(desired_apc_slot, &desired_apc))
        {
            return std::nullopt;
        }
        

        desired_apc.InitRootOrChildBranch(
            desired_apc_slot,
            logical_id,
            shared_id,
            PerAPCRuntimeCellCount_,
            container_conf
        );

        return desired_apc_slot;
    }


    AdaptivePackedCellContainer* VagueTemoraryPremativeFabric::GetDeafultInitializedAPCFromFabric(
        const ContainerConf& container_conf,
        uint64_t shared_id,
        uint64_t logical_id
    ) noexcept
    {
        AdaptivePackedCellContainer new_apc{};

        std::optional<uint64_t> maybe_apc_slot_idx = ConstructAnAPC_(
            new_apc,
            container_conf,
            shared_id,
            logical_id
        );

        return maybe_apc_slot_idx.has_value() ? GetAPCRuntimePtr(*maybe_apc_slot_idx) : nullptr;
    
    }

    bool VagueTemoraryPremativeFabric::InitializeFabricWithPtrTable(
        uint16_t slot_count,
        size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
        uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
        uint32_t fabric_thread_capacity = DEFAULT_THREAD_TABLE_CAPACITY
    ) noexcept
    {
        APCRuntimePtrTable_.reset();
        const bool base_ok = InitializeFabric(
            slot_count,
            slot_cell_count,
            slab_id,
            fabric_thread_capacity
        );

        if (!base_ok)
        {
            return false;
        }

        if (!BuildAPCRuntimePtrTable_())
        {
            ShutDownFabric();
            return false;
        }
        
        return true;
    }


}
