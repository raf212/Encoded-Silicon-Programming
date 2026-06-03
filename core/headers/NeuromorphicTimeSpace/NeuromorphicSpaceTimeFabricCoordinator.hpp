#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    class NeuromorphicSpaceTimeFabricCoordinator
    {
    private:
        packed64_t* SlabBasePtr_{nullptr};

        size_t SlabCellCount_{UNSIGNED_ZERO};
        uint64_t SlotCellCount_{UNSIGNED_ZERO};
        uint64_t SlotCount_{UNSIGNED_ZERO};
        uint8_t SlabId_{UNSIGNED_ZERO};

        size_t SegmentPoolBegin_{APCDataStructure::METACELL_COUNT};
        size_t SegmentPoolEnd_{APCDataStructure::METACELL_COUNT};
        
        uint64_t HashBucketCount_{UNSIGNED_ZERO};
        uint64_t RelationRecordCount_{UNSIGNED_ZERO};
        uint64_t DeviceViewRecordCount_{UNSIGNED_ZERO};
        uint64_t ThreadTableCapacity_{UNSIGNED_ZERO};

        //--remove
        struct CacheEntryOfFabricTable
        {
            size_t BeginIdx{UNSIGNED_ZERO};
            size_t EndIdx{UNSIGNED_ZERO};
            uint32_t RecordWidth{UNSIGNED_ZERO};
            uint16_t VersionCount{UNSIGNED_ZERO};
            bool IsThisEntryValid{false};
        };
        std::array<CacheEntryOfFabricTable, static_cast<size_t>(FabricTableSegmentClasses::COUNT)> TableCache_{};
        //--remove


        std::atomic<bool> FabricInitialized_{false};
        std::atomic<bool> InitializationInProgress_{false};
        RawPackedCellAllocator AllocatorOfFabric_{};
        AtomicAdaptiveBackoff AdaptiveBackoffCentral_;
        static constexpr uint32_t DEFAULT_MAX_TRIES = 128;

        //METHODS--------------------------------------------------------------------------------
        packed64_t* AllocatePackedCellRaw_(size_t count_of_cells) noexcept;
        
        void FreeRawPackedCells_(packed64_t*packed_cell_memory_ptr, size_t packed_cell_count) noexcept;

        void ResetScalarsofTheFabric_() noexcept;

        static constexpr size_t DefaultFabricAlignment16Cell_(size_t value) noexcept
        {
            const uint8_t alignment_value_15 = 16 - 1;
            return (value + alignment_value_15) & ~static_cast<size_t>(alignment_value_15);
        }
        
        bool MakeAndStoreDirectlyAFabricOwnedCell_(
            size_t index, 
            uint64_t value32_or_64, 
            FabricTableSegmentClasses fabric_segment_class = FabricTableSegmentClasses::GLOBAL_AND_CONFIG,
            PackedMode cell_mode = PackedMode::MODE_48, 
            clk16_t extended_meta_value = UNSIGNED_ZERO,
            tag8_t mode_sub_class = static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType,
            PackedCellLocalityTypes locality_of_cell = PackedCellLocalityTypes::IDLE, 
            PriorityPhysics priority = PriorityPhysics::IMPORTANT
        ) noexcept;

        bool StoreFebricControlMeta48Directly_(
            FabricMetaIndicies fabric_meta_idx, uint64_t value, 
            PackedCellLocalityTypes cell_locality = PackedCellLocalityTypes::PUBLISHED,
            SubClassesOfMode48 sub_class48 = SubClassesOfMode48::SELF_CLASS,
            PriorityPhysics priority = PriorityPhysics::VERSION_DEPENDENCY
        )noexcept;

        bool UpdateValidPairedOccupancyApproxAtomically_(
            PackedCellLocalityTypes desired_occupancy_of_locality, uint64_t desired_occupancy_value,
            bool force_update = false,
            clk16_t pair_version = APCDataStructure::BRANCH_VERSION
        ) noexcept;

//checked-----------------------------------------------

        void ResetAll4TypesOfOccupancyMetaData() noexcept;

        void WriteFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept;

        bool DefaultCompareExchangeStrongUncheckedCell_(
            size_t idx,
            const std::pair<packed64_t, packed64_t> expectedF_desiresS,
            bool is_claimed_invalid = true
        ) noexcept;


        size_t GetTableDirectoryBeginIdx_(FabricTableSegmentClasses desired_table, uint8_t part = UNSIGNED_ZERO) noexcept;

        void WriteDirectoryEntry_(FabricTableSegmentClasses table_id, size_t begin, size_t end, uint16_t version) noexcept;

        void InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept;
    
        size_t GetSlotCellTypeIdxInFabric_(uint32_t slot, SlotCellTypeOfAPCFabric slot_type) noexcept;

        void InitializeSlotDirectory_() noexcept;

        void MakeAndStoreASlotDirectoryCell_(
            uint32_t slot, 
            SlotCellTypeOfAPCFabric slote_state,
            uint32_t value32, 
            clk16_t extended_meta_value,
            PackedCellLocalityTypes locality_of_cell = PackedCellLocalityTypes::IDLE
        ) noexcept;

        uint64_t IncrementOrDecrementDeltaFromFabricTrackerMetaIdx_(FabricMetaIndicies meta_idx) noexcept;

    public:
        NeuromorphicSpaceTimeFabricCoordinator(/* args */) noexcept = default;

        ~NeuromorphicSpaceTimeFabricCoordinator() noexcept
        {
            ShutDownFabric();
        }

        NeuromorphicSpaceTimeFabricCoordinator(const NeuromorphicSpaceTimeFabricCoordinator&) = delete;
        NeuromorphicSpaceTimeFabricCoordinator& operator = (const NeuromorphicSpaceTimeFabricCoordinator&) = delete;

        void ShutDownFabric() noexcept;
        
        packed64_t ReadCompletePackedCellDirectly(size_t slab_index) noexcept;

        packed64_t AtomicallyLoadReadCompletePackedCell(size_t slab_index) noexcept;

        void StorePackedCellUncheckedDirectly(size_t slab_index, packed64_t packed_cell) noexcept;

        void AtomicallyStorePackedCellUnchecked(size_t slab_index, packed64_t packed_cell, std::memory_order mem_order = MoStoreSeq_) noexcept;

        /// @brief Do not change default memory order unless have total idea
        /// @param expected_packed_cell ->ADDRESS
        /// @return bool
        bool AtomicallyCompareAndExchangeStrongPackedCell(
            size_t slab_index, 
            packed64_t& expected_packed_cell, 
            packed64_t desired_packed_cell,
            std::memory_order mem_order_success = MoClaimSuccess,
            std::memory_order mem_order_failure = MoClaimFailure
        ) noexcept;

/// checked--------------------------------------------------------

        bool GetMetaCellView(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept;

        std::optional<uint64_t> ReadOccupancyApproxFromPairedIfValid(
            PackedCellLocalityTypes desired_occupancy_class,
            const PackedCell64_t::AuthoritiveCellView* low_half_view_ptr = nullptr,
            const PackedCell64_t::AuthoritiveCellView* high_half_view_ptr = nullptr
        ) noexcept;

        bool GetFabricTableCache(FabricTableSegmentClasses febric_class, CacheEntryOfFabricTable& cache_entry) noexcept;


        bool IsThisAValidFabricTableCacheEntry(CacheEntryOfFabricTable& cache_entry) noexcept
        {
            const bool ok = cache_entry.VersionCount != UNSIGNED_ZERO && cache_entry.BeginIdx <= cache_entry.EndIdx &&
                cache_entry.EndIdx <= SlabCellCount_;
            cache_entry.IsThisEntryValid = ok;
            return ok;
        }

        bool InitializeFabric(
            uint16_t slot_count,
            size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
            uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
            uint32_t fabric_thread_capacity = DEFAULT_THREAD_SLOT_OF_FABRIC
        ) noexcept;

    };

    

}
