#pragma once

#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{

    void SlabToFabricConverterAndCordinator::ShutDownFabric() noexcept
    {
        if (InitializationInProgress_.load(MoLoad_))
        {

        }
        FabricInitialized_.store(false, MoStoreSeq_);
        packed64_t* old_ptr = SlabBasePtr_;
        const size_t old_count = SlabCellCount_;
        SlabBasePtr_ = nullptr;
        SlabCellCount_ = UNSIGNED_ZERO;
        if (old_ptr)
        {
            FreeRawPackedCells_(old_ptr, old_count);
        }
        ResetScalarsofTheFabric_();
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


    bool APCHandleDescriptorConstructor::ReadAPCDescriptorTableBeginEndFromRecordBook(
        APCDescriptorRange& return_APC_handle_description_range
    ) noexcept
    {
        SlabFabricTableBoundsCarrietFromRecordBookTable return_bounds{};

        bool bounds_ok = GetValidSlabRangeTripletFromRecordBookOfFTSC(FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR, return_bounds);

        if (!bounds_ok)
        {
            return_APC_handle_description_range.IsVAlid = false;
            return false;
        }

        return_APC_handle_description_range.BeginIndex = return_bounds.BeginIndex;
        return_APC_handle_description_range.EndIndex = return_bounds.EndIndex;
        return_APC_handle_description_range.IsVAlid = return_bounds.IsValid;

        return return_bounds.IsValid;
    }


    APCDescriptorRange APCHandleDescriptorConstructor::GetSegmentPoolBegainEndForSingleAPCDescription_(uint64_t single_description_index) noexcept
    {
        
        APCDescriptorRange desired_segment_pool_range{};

        if (single_description_index >= SlotCount_)
        {
            return desired_segment_pool_range;
        }

        desired_segment_pool_range.BeginIndex = SegmentPoolBegin_ + static_cast<size_t>(single_description_index);
        desired_segment_pool_range.EndIndex = desired_segment_pool_range.BeginIndex + static_cast<size_t>(PerAPCRuntimeCellCount_) - 1;
        if (
            desired_segment_pool_range.EndIndex > desired_segment_pool_range.BeginIndex &&
            desired_segment_pool_range.BeginIndex > APCDataStructure::METACELL_COUNT &&
            desired_segment_pool_range.EndIndex < SlabCellCount_
        )
        {
            desired_segment_pool_range.IsVAlid = true;
            return desired_segment_pool_range;
        }

        desired_segment_pool_range.IsVAlid = false;
        return desired_segment_pool_range;
        
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



    HashKeyValueDistanceTriplet HashTablesConstructor::ReadValidHashBucketTriplet(size_t bucked_base_index) noexcept
    {
        if (!SlabBasePtr_ || bucked_base_index + HASH_BUCKED_WIDTH_OF_FABRIC >= SlabCellCount_)
        {
            const HashKeyValueDistanceTriplet invalid_value{};
            return invalid_value;
        }

        const packed64_t key_cell = AtomicallyLoadReadCompletePackedCell(bucked_base_index + static_cast<size_t>(HashTableInternalIndexing::KEY_INDEX));
        const packed64_t value_cell = AtomicallyLoadReadCompletePackedCell(bucked_base_index + static_cast<size_t>(HashTableInternalIndexing::VALUE_INDEX));
        const packed64_t prob_safty_cell = AtomicallyLoadReadCompletePackedCell(bucked_base_index + static_cast<size_t>(HashTableInternalIndexing::PROB_DISTANCE_LOCK));
        
        return HashTableConf::ReadKeyValueProbFromValidCells(
            key_cell,
            value_cell,
            prob_safty_cell
        );
    }

}
