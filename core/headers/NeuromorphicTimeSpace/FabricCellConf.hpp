#pragma once 
#include "FabricOrchestrators/RecordBookConf.hpp"
#include "FabricOrchestrators/DescriptionOfAPC.hpp"
#include "FabricOrchestrators/HashTableConf.hpp"

namespace PredictedAdaptedEncoding
{

    class FabricConstructor
    {
    protected:
        packed64_t* SlabBasePtr_{nullptr};

        size_t SlabCellCount_{UNSIGNED_ZERO};
        uint64_t PerAPCRuntimeCellCount_{UNSIGNED_ZERO};
        uint64_t SlotCount_{UNSIGNED_ZERO};
        uint8_t SlabId_{UNSIGNED_ZERO};

        size_t SegmentPoolBegin_{APCDataStructure::METACELL_COUNT};
        size_t SegmentPoolEnd_{APCDataStructure::METACELL_COUNT};
        
        uint64_t HashBucketCount_{UNSIGNED_ZERO};
        uint64_t RelationRecordCount_{UNSIGNED_ZERO};
        uint64_t DeviceViewRecordCount_{UNSIGNED_ZERO};
        uint64_t ThreadTableCapacity_{UNSIGNED_ZERO};


        std::atomic<bool> FabricInitialized_{false};
        std::atomic<bool> InitializationInProgress_{false};
        RawPackedCellAllocator AllocatorOfFabric_{};
        AtomicAdaptiveBackoff AdaptiveBackoffCentral_;
        static constexpr uint32_t DEFAULT_MAX_TRIES = 128;

        static constexpr size_t DefaultFabricAlignment16Cell_(size_t value) noexcept
        {
            const uint8_t alignment_value_15 = 16 - 1;
            return (value + alignment_value_15) & ~static_cast<size_t>(alignment_value_15);
        }

        /// @brief ONLY: Use for Initialiazation ONLY
        constexpr void MakeAndStoreFabricMetaValue48_(
            FabricMetaIndicies fabric_meta_idx, uint64_t value, 
            LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED,
            AccessContractOfValue access_contract = AccessContractOfValue::CAS_RMW,
            AttributePolicy attribute = AttributePolicy::SELF_CONTAINED_DATA_OR_MODEL
        )noexcept;


    public:

        constexpr packed64_t ReadCompletePackedCellDirectly(size_t slab_index) noexcept;

        constexpr packed64_t AtomicallyLoadReadCompletePackedCell(size_t slab_index) noexcept;

        bool ReadFabricMetaCellViewAtomically(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept;

        constexpr void StorePackedCellUncheckedDirectly(size_t slab_index, packed64_t packed_cell) noexcept;

        constexpr void AtomicallyStorePackedCellUnchecked(size_t slab_index, packed64_t packed_cell, std::memory_order mem_order = MoStoreSeq_) noexcept;

        /// @brief Do not change default memory order unless have total idea
        /// @param expected_packed_cell ->ADDRESS
        /// @return bool
        constexpr bool CompareExchangeStrongFromFabric(
            size_t slab_index, 
            packed64_t& expected_packed_cell, 
            packed64_t desired_packed_cell,
            std::memory_order mem_order_success = MoClaimSuccess,
            std::memory_order mem_order_failure = MoClaimFailure
        ) noexcept;

        /// @brief Do not change default memory order unless have total idea
        /// @param expected_packed_cell ->ADDRESS
        /// @return bool
        constexpr bool CompareExchangeWeakInSlab(
            size_t slab_index, 
            packed64_t& expected_packed_cell, 
            packed64_t desired_packed_cell,
            std::memory_order mem_order_success = MoClaimSuccess,
            std::memory_order mem_order_failure = MoLoad_
        ) noexcept;

        JustifyClaimCas TryClaimACellInSlab(PackedCell64_t::AuthoritiveCellView& expected_cell_auth_view, packed64_t* desired_packed_cell = nullptr) noexcept;

        std::optional<uint64_t> ReadOccupancyApproxFromPairedIfValid(
            LocalityPolicy desired_occupancy_class,
            const PackedCell64_t::AuthoritiveCellView* low_half_view_ptr = nullptr,
            const PackedCell64_t::AuthoritiveCellView* high_half_view_ptr = nullptr
        ) noexcept;

        /// @brief UPDATES OR: Initializes PAIRED: Occupancy | Why PAIRED ? To Potentially Justify by Version OR: Internal CLOCK16 How Much Accumulatiom Diffarence Between Total and the DISTANCE: By Version or CLOCK16 
        /// @param candidate_to_update DESIRED: LocalityPolicy -> Count Want TO Be Updated | GETS TRANSLETED: To -> FabricMetaIndicies BY: CoreOfFabricCoordinator::GetDesiredLowIdxOfOccupancyPairFromLocality
        /// @param desired_occupancy_value IF: desired_occupancy_value <= UINT32_MAX ONLY -> USED: FabricMetaIndicies::FABRIC_OCCUPANCY_APPROXIMATION_LOCALITY_LOW32 || BOTH: LOW32 + HIGH32
        /// @param force_update DO NOT CHANGE TO: true untill Understand USE: IF: false -> CAS: Update || true -> ATOMIC STORE: 
        /// @return 
        constexpr bool UpdateValidPairedOccupancyApproxAtomically_(
            LocalityPolicy candidate_to_update, uint64_t desired_occupancy_value,
            bool force_update = false,
            clk16_t pair_version = UNSIGNED_ZERO
        ) noexcept;

    };



}