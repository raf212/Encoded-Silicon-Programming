#pragma once

#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{
    constexpr void FabricConstructor::MakeAndStoreFabricMetaValue48_(
        FabricMetaIndicies fabric_meta_idx, 
        uint64_t value, 
        LocalityPolicy cell_locality,
        AccessContractOfValue access_contract,
        AttributePolicy attribute
    )noexcept
    {
        const size_t slab_index = static_cast<size_t>(fabric_meta_idx);
        if (slab_index >= APCDataStructure::METACELL_COUNT)
        {
            return;
        }

        const packed64_t desired_packed_cell = PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48, access_contract,
            FabricTableSegmentClasses::GENERIC_CONTROL,
            cell_locality,
            InternalDataTypePolicy ::UnsignedPCellDataType,
            attribute,
            value
        );

        StorePackedCellUncheckedDirectly(slab_index, desired_packed_cell);
        
    }

    //Integrate AtomicAdaptiveBackoff
    // add CAS_FAILURE_COUNT
    constexpr bool FabricConstructor::UpdateValidPairedOccupancyApproxAtomically_(
        LocalityPolicy candidate_to_update, uint64_t desired_occupancy_value,
        bool force_update, clk16_t pair_version
    ) noexcept
    {
        const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(candidate_to_update);

        if (desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
        {
            return false;
        }

        const size_t low_idx = static_cast<size_t>(desired_occupancy_low_idx);
        const size_t high_idx = low_idx + 1;

        const std::pair<packed64_t, packed64_t> low32_and_probable_high32 = PairedVersionedCellModelOfMode32::GetPairOfLow32FAndHigh32SFromUnsigned64ForFabric(
            desired_occupancy_value, pair_version,
            LocalityPolicy::PUBLISHED,
            FabricTableSegmentClasses::GENERIC_CONTROL,
            AttributePolicy::SELF_CONTAINED_DATA_OR_MODEL
        );

        auto ForceUpdate = [&](){
            AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
            AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
            return true;
        };
        
        if (force_update)
        {
            return ForceUpdate();
        }

        const PackedCell64_t::AuthoritiveCellView low32_half_view{};
        const PackedCell64_t::AuthoritiveCellView high32_half_view{};
        const std::optional<uint64_t> maybe_desired_candidate_occupancy = ReadOccupancyApproxFromPairedIfValid(candidate_to_update, &low32_half_view, &high32_half_view);

        if (!maybe_desired_candidate_occupancy|| *maybe_desired_candidate_occupancy == PackedCell64_t::PACKED_CELL_SENTINAL)
        {
            return ForceUpdate();
        }
    
        if (low32_half_view.LocalityOfCell == LocalityPolicy::CLAIMED || high32_half_view.LocalityOfCell == LocalityPolicy::CLAIMED)
        {
            return false;
        }


        //just cmpx  low
        if (
            *maybe_desired_candidate_occupancy <= IN_CELL_VALUE_MODE32_SENTINAL && 
            low32_and_probable_high32.second == PackedCell64_t::PACKED_CELL_SENTINAL
        )
        {
            packed64_t expected = low32_half_view.RawCell;
            const packed64_t desired = low32_and_probable_high32.first;
            for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
            {
                if (CompareExchangeStrongFromFabric(low_idx, expected, desired))
                {
                    AtomicallyStorePackedCellUnchecked(high_idx, low32_and_probable_high32.second);
                    return true;
                }

                if (PackedCell64_t::ExtractLocalityPolicy(expected) == LocalityPolicy::CLAIMED)
                {
                    return false;
                }
                
            }
            //intehrate failure count and AtomicAdaptiveBackoff
            return false;
        }

        //double cas 
        if ((low32_half_view.IsCellValid && high32_half_view.IsCellValid) || *maybe_desired_candidate_occupancy > IN_CELL_VALUE_MODE32_SENTINAL)
        {
            packed64_t expected_low = low32_half_view.RawCell;
            const packed64_t desired_claimed_low = PackedCell64_t::SetLocalityInPacked(low32_and_probable_high32.first, LocalityPolicy::CLAIMED);

            auto RestoreLow = [&]()
            {
                AtomicallyStorePackedCellUnchecked(low_idx, low32_half_view.RawCell);
                return false;
            };

            for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
            {
                if (CompareExchangeStrongFromFabric(low_idx, expected_low, desired_claimed_low))
                {
                    packed64_t expected_high = high32_half_view.RawCell;

                    for (size_t j = 0; j < DEFAULT_MAX_TRIES; j++)
                    {
                        if (CompareExchangeStrongFromFabric(high_idx, expected_high, low32_and_probable_high32.second))
                        {
                            AtomicallyStorePackedCellUnchecked(low_idx, low32_and_probable_high32.first);
                            return true;
                        }

                        if (PackedCell64_t::ExtractLocalityPolicy(expected_high) == LocalityPolicy::CLAIMED)
                        {
                            return RestoreLow();
                        }
                    }

                    return RestoreLow();
                }

                if (PackedCell64_t::ExtractLocalityPolicy(expected_low) == LocalityPolicy::CLAIMED)
                {
                    return false;
                }
            }
        }

        return ForceUpdate(); 
    }



    constexpr packed64_t FabricConstructor::ReadCompletePackedCellDirectly(size_t slab_index) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        const packed64_t desired_cell_raw = SlabBasePtr_[slab_index];

        return desired_cell_raw;
    } 

    constexpr packed64_t FabricConstructor::AtomicallyLoadReadCompletePackedCell(size_t slab_index) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        std::atomic_ref<const packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        const packed64_t desired_cell_raw = packed_cell_ref.load(MoLoad_);

        return desired_cell_raw;
    }

    constexpr void FabricConstructor::StorePackedCellUncheckedDirectly(size_t slab_index, packed64_t packed_cell) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return;
        }
        SlabBasePtr_[slab_index] = packed_cell;
    }

    constexpr void FabricConstructor::AtomicallyStorePackedCellUnchecked(
        size_t slab_index, packed64_t packed_cell,
        std::memory_order mem_order
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return;
        }
        std::atomic_ref<packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        packed_cell_ref.store(packed_cell, mem_order);
        packed_cell_ref.notify_all();
    }

    constexpr bool FabricConstructor::CompareExchangeStrongFromFabric(
        size_t slab_index, 
        packed64_t& expected_packed_cell, 
        packed64_t desired_packed_cell,
        std::memory_order mem_order_success,
        std::memory_order mem_order_failure
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return false;
        }
        std::atomic_ref<packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        return packed_cell_ref.compare_exchange_strong(expected_packed_cell, desired_packed_cell, mem_order_success, mem_order_failure);
    }

    constexpr bool FabricConstructor::CompareExchangeWeakInSlab(
        size_t slab_index, 
        packed64_t& expected_packed_cell, 
        packed64_t desired_packed_cell,
        std::memory_order mem_order_success,
        std::memory_order mem_order_failure
    ) noexcept
    {
        if (!SlabBasePtr_ || slab_index >= SlabCellCount_)
        {
            return false;
        }
        std::atomic_ref<packed64_t> packed_cell_ref(SlabBasePtr_[slab_index]);
        return packed_cell_ref.compare_exchange_weak(expected_packed_cell, desired_packed_cell, mem_order_success, mem_order_failure);
    }

    /// @brief An Invalid Cell Is not Claimable - Reinitate as valid cell
    /// @param slab_index 
    /// @param expected_cell 
    /// @return 
    JustifyClaimCas FabricConstructor::TryClaimACellInSlab(PackedCell64_t::AuthoritiveCellView& expected_cell_auth_view, packed64_t* desired_packed_cell) noexcept
    {
        if (!expected_cell_auth_view.IsCellValid)
        {
            return JustifyClaimCas::INVALID_CELL;
        }

        if (expected_cell_auth_view.SlabIndexOfPackeCell == APCDataStructure::APC_SIZE_SENTINAL)
        {
            return JustifyClaimCas::OUT_OF_BOUND;
        }
        
        for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
        {
            packed64_t currennt_expected_cell = AtomicallyLoadReadCompletePackedCell(expected_cell_auth_view.SlabIndexOfPackeCell);
            if (currennt_expected_cell == PackedCell64_t::PACKED_CELL_SENTINAL)
            {
                return JustifyClaimCas::CELL_SENTINAL_STATE;
            }
            if (currennt_expected_cell != expected_cell_auth_view.RawCell)
            {
                return JustifyClaimCas::INVALID_USE_OF_METHOD;
            }
            
            const packed64_t desired_cell = PackedCell64_t::SetLocalityInPacked(expected_cell_auth_view.RawCell, LocalityPolicy::CLAIMED);
            if (CompareExchangeWeakInSlab(expected_cell_auth_view.SlabIndexOfPackeCell, currennt_expected_cell, desired_cell))
            {
                if (desired_packed_cell)
                {
                    *desired_packed_cell = desired_cell;
                }

                return JustifyClaimCas::SUCCESS;
            }
            AdaptiveBackoffCentral_.AdaptiveBackOffPacked(currennt_expected_cell);
        }

        return JustifyClaimCas::CAS_LOOP_RANOUT;
        
    }


    bool FabricConstructor::ReadFabricMetaCellViewAtomically(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept
    {
        const size_t meta_index_in_slab = static_cast<size_t>(fabric_meta_idx);
        if (meta_index_in_slab >= APCDataStructure::METACELL_COUNT || !SlabBasePtr_)
        {
            return false;
        }

        std::atomic_ref<const packed64_t> packed_cell_ref(SlabBasePtr_[meta_index_in_slab]);
        const packed64_t packed_meta_cell = packed_cell_ref.load(MoLoad_);
        meta_cell_view_address = PackedCell64_t::GetAuthoritiveViewsForACell(packed_meta_cell);

        return true;        
    }



    std::optional<uint64_t> FabricConstructor::ReadOccupancyApproxFromPairedIfValid(
        LocalityPolicy desired_occupancy_class,
        const PackedCell64_t::AuthoritiveCellView* low_half_view_ptr,
        const PackedCell64_t::AuthoritiveCellView* high_half_view_ptr
    ) noexcept
    {
        const FabricMetaIndicies desired_occupancy_low_idx = CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality(desired_occupancy_class);
        if (desired_occupancy_low_idx == FabricMetaIndicies::EOF_FABRIC_HEADER)
        {
            return std::nullopt;
        }

        const packed64_t raw_occ_low = AtomicallyLoadReadCompletePackedCell(static_cast<size_t>(desired_occupancy_low_idx));
        const packed64_t raw_occ_high = AtomicallyLoadReadCompletePackedCell(static_cast<size_t>(desired_occupancy_low_idx) + 1);

        return PairedVersionedCellModelOfMode32::GetFullUnsigned64FromPairedVersionedCell(raw_occ_low, raw_occ_high, low_half_view_ptr, high_half_view_ptr);
    }

}