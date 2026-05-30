#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    class NeuromorphicSpaceTimeFabricCoordinator
    {
    private:
        std::atomic<packed64_t>* SlabBasePtr_{nullptr};
        size_t SlabCellCount_{UNSIGNED_ZERO};
        size_t SlotCellCount_{UNSIGNED_ZERO};
        size_t SlotCount_{UNSIGNED_ZERO};
        uint16_t SlabId_{UNSIGNED_ZERO};
        size_t SegmentPoolBegin_{APCDataStructure::METACELL_COUNT};
        size_t SegmentPoolEnd_{APCDataStructure::METACELL_COUNT};
        uint32_t HashBucketCount_{UNSIGNED_ZERO};
        uint32_t RelationRecordCount_{UNSIGNED_ZERO};
        uint32_t DeviceViewRecordCount_{UNSIGNED_ZERO};
        uint32_t ThreadTableCapacity_{UNSIGNED_ZERO};

        struct CacheEntryOfFabricTable
        {
            size_t BeginIdx{UNSIGNED_ZERO};
            size_t EndIdx{UNSIGNED_ZERO};
            uint32_t VersionCount{UNSIGNED_ZERO};
            uint32_t RecordWidth{UNSIGNED_ZERO};
            bool IsThisEntryValid{false};
        };

        std::array<CacheEntryOfFabricTable, static_cast<size_t>(TableIdOfAPCFabric::COUNT)> TableCache_{};
        std::atomic<bool> FabricInitialized_{false};
        std::atomic<bool> InitializationInProgress_{false};
        AllocatorOfAPCFabricCells AllocatorOfFabric_{};
        AtomicAdaptiveBackoff AdaptiveBackoffCentral_;
        static constexpr uint32_t DEFAULT_MAX_TRIES = 128;

        void FreeAtomicCells_(std::atomic<packed64_t>* packed_cell_memory_ptr) noexcept;

        void ResetScalarsofTheFabric_() noexcept;
        uint32_t NextPowerOf2Unsigned32_(uint32_t given_value) noexcept;

        static constexpr size_t DefaultFabricAlignment16Cell_(size_t value) noexcept
        {
            const uint8_t alignment_value_15 = 16 - 1;
            return (value + alignment_value_15) & ~static_cast<size_t>(alignment_value_15);
        }

        void StorePackedCellUnchecked_(size_t idx, packed64_t packed_cell) noexcept;

        bool CheckAndStoreAPrebuildCellInSlab_(size_t idx, packed64_t packed_cell) noexcept;

        bool MakeCheckAndStoreAFabricControlValidCell_(
            FabricMetaIndicies fabric_meta_idx, uint64_t value32_or_64, 
            PackedMode cell_mode = PackedMode::MODE_48, tag8_t mode_sub_class = static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType,
            PackedCellLocalityTypes locality_of_cell = PackedCellLocalityTypes::IDLE, 
            PriorityPhysics priority = PriorityPhysics::IMPORTANT, clk16_t extended_meta_value = UNSIGNED_ZERO
        ) noexcept;

        void StoreNewDefaultMeta48_(FabricMetaIndicies fabric_meta_idx, uint64_t value) noexcept;


        bool UpdateValidPairedOccupancyApproximation_(
            PackedCellLocalityTypes desired_occupancy_of_locality, uint64_t desired_occupancy_value,
            bool force_update = false,
            clk16_t pair_version = APCDataStructure::BRANCH_VERSION
        ) noexcept;

        void WriceFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept;

        bool DefaultCompareExchangeStrongUncheckedCell_(
            size_t idx,
            const std::pair<packed64_t, packed64_t> expectedF_desiresS,
            bool is_claimed_invalid = true
        ) noexcept
        {
            if (SlabBasePtr_ && idx >= SlabCellCount_)
            {
                return false;
            }

            packed64_t expected_cell = expectedF_desiresS.first;
            for (size_t i = 0; i < DEFAULT_MAX_TRIES; i++)
            {
                if (SlabBasePtr_[idx].compare_exchange_strong(expected_cell, expectedF_desiresS.second, OnExchangeSuccess, OnExchangeFailure))
                {
                    return true;
                }
                
                if (is_claimed_invalid && PackedCell64_t::ExtractLocalityFromPacked(expected_cell) == PackedCellLocalityTypes::CLAIMED)
                {
                    return false;
                }

                //JUST AS A SLOT HOLDER AtomicAdaptiveBackoff has to be build for packed cell
                AdaptiveBackoffCentral_.AdaptiveBackOffPacked(expected_cell);
            }
            
        }

        uint64_t IncrementOrDecrementDeltaFromFabricTrackerMetaIdx() noexcept;



    public:
        NeuromorphicSpaceTimeFabricCoordinator(/* args */) noexcept = default;

        ~NeuromorphicSpaceTimeFabricCoordinator() noexcept
        {
            ShutDownFabric();
        }

        NeuromorphicSpaceTimeFabricCoordinator(const NeuromorphicSpaceTimeFabricCoordinator&) = delete;
        NeuromorphicSpaceTimeFabricCoordinator& operator = (const NeuromorphicSpaceTimeFabricCoordinator&) = delete;

        void ShutDownFabric() noexcept;

        packed64_t GetTotalRawCellUnchackedCell(size_t idx) noexcept;

        bool GetMetaCellView(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept;

        std::optional<uint64_t> ReadOccupancyApproxFromPairedIfValid(
            PackedCellLocalityTypes desired_occupancy_class,
            const PackedCell64_t::AuthoritiveCellView* low_half_view_ptr = nullptr,
            const PackedCell64_t::AuthoritiveCellView* high_half_view_ptr = nullptr
        ) noexcept;

        bool InitializeFabric(
            uint16_t slot_count,
            size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
            uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
            uint32_t fabric_thread_capacity = DEFAULT_THREAD_SLOT_OF_FABRIC
        ) noexcept;

    };

    

}
