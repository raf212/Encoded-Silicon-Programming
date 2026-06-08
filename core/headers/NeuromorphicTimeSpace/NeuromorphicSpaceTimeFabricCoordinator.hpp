#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

    enum class JustifyClaimCas
    {
        SUCCESS = 0,
        OUT_OF_BOUND = 1,
        INVALID_CELL = 2,
        CELL_SENTINAL_STATE = 3,
        CELL_INVALID = 4,
        CAS_LOOP_RANOUT = 5,
        UNDEFINED_CAS_FAILURE = 6,
        INVALID_USE_OF_METHOD = 7

    };

    class NeuromorphicSpaceTimeFabricCoordinator
    {
    public:
        struct alignas(SIZE_OF_A_PAIR) FabricTableRange
        {
            packed64_t BeginIdxRawType48Cell;
            packed64_t EndIdxRawType48Cell;
        };
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
        
        constexpr bool MakeAndStoreDirectlyAFabricOwnedCell_(
            size_t index, 
            uint64_t value32_or_64, 
            FabricTableSegmentClasses fabric_segment_class = FabricTableSegmentClasses::GLOBAL_AND_CONFIG,
            ModelFamily cell_model = ModelFamily::MODEL48, 
            clk16_t extended_meta_value = UNSIGNED_ZERO,
            tag8_t mode_sub_class = static_cast<tag8_t>(Model32Subclass::SELF_CLASS),
            InternalDataTypePolicy cell_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            LocalityPolicy locality_of_cell = LocalityPolicy::IDLE, 
            PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST
        ) noexcept;

        constexpr bool StoreFebricControlMeta48Directly_(
            FabricMetaIndicies fabric_meta_idx, uint64_t value, 
            LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED,
            Model48Subclass sub_class48 = Model48Subclass::SELF_CLASS,
            PriorityPolicy priority = PriorityPolicy::INFLUENCED
        )noexcept;

        constexpr bool UpdateValidPairedOccupancyApproxAtomically_(
            LocalityPolicy desired_occupancy_of_locality, uint64_t desired_occupancy_value,
            bool force_update = false,
            clk16_t pair_version = APCDataStructure::BRANCH_VERSION
        ) noexcept;

        constexpr void ResetAll4TypesOfOccupancyMetaData_() noexcept;
    
        constexpr void WriteFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept;

        /// @return VALID-> INDEX < UINT
        constexpr size_t ReadTableDirectoryBeginIdxOfATableClass_(FabricTableSegmentClasses desired_table) noexcept;
    

        constexpr bool WriteDirectoryEntry_(FabricTableSegmentClasses table_class, size_t begin, size_t end) noexcept;

        constexpr std::optional<FabricTableRange> GetTableDirectoryRangeRaw_(FabricTableSegmentClasses table_class) noexcept
        {
            if (!CoreOfFabricCoordinator::IsValidFabricTable(table_class))
            {
                return std::nullopt;
            }

            const size_t begin_of_desired_table = ReadTableDirectoryBeginIdxOfATableClass_(table_class);
            const size_t end_idx = begin_of_desired_table + 1;
            if (end_idx >= SlabCellCount_ || begin_of_desired_table < APCDataStructure::METACELL_COUNT)
            {
                return std::nullopt;
            }

            FabricTableRange desired_table_range_cells{};

            desired_table_range_cells.BeginIdxRawType48Cell = ReadCompletePackedCellDirectly(begin_of_desired_table);
            desired_table_range_cells.EndIdxRawType48Cell = ReadCompletePackedCellDirectly(end_idx);

            return desired_table_range_cells;
        }
        
//checked-----------------------------------------------


        void InitializeLinearTable_(FabricTableSegmentClasses table_class, uint32_t record_width) noexcept;

        void InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept;
    
        size_t GetSlotCellTypeIdxInFabric_(uint32_t slot, SlotCellTypeOfAPCFabric slot_type) noexcept;

        void InitializeSlotDirectory_() noexcept;

        void MakeAndStoreASlotDirectoryCell_(
            uint32_t slot, 
            SlotCellTypeOfAPCFabric slote_state,
            uint32_t value32, 
            clk16_t extended_meta_value,
            LocalityPolicy locality_of_cell = LocalityPolicy::IDLE
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
        
        constexpr packed64_t ReadCompletePackedCellDirectly(size_t slab_index) noexcept;

        constexpr packed64_t AtomicallyLoadReadCompletePackedCell(size_t slab_index) noexcept;

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



/// checked--------------------------------------------------------
        bool GetMetaCellView(MetaIndexOfAPCNode fabric_meta_idx, PackedCell64_t::AuthoritiveCellView& meta_cell_view_address) noexcept;

        std::optional<uint64_t> ReadOccupancyApproxFromPairedIfValid(
            LocalityPolicy desired_occupancy_class,
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
