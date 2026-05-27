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
        uint16_t SlotCount_{UNSIGNED_ZERO};
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

        void FreeAtomicCells_(std::atomic<packed64_t>* packed_cell_memory_ptr) noexcept;

        void ResetScalarsofTheFabric_() noexcept;
        uint32_t NextPowerOf2Unsigned32_(uint32_t given_value) noexcept;

        static constexpr size_t DefaultFabricAlignment16Cell_(size_t value) noexcept
        {
            const uint8_t alignment_value_15 = 16 - 1;
            return (value + alignment_value_15) & ~static_cast<size_t>(alignment_value_15);
        }

        void StorePackedCellUnchecked_(size_t idx, packed64_t packed_cell) noexcept;

        bool StoreAValidPackedCell_(size_t idx, packed64_t packed_cell) noexcept;

        bool StoreAValidFabricMetaCellOnly_(
            FabricMetaIndicies fabric_meta_idx, uint64_t value32_or_64, 
            PackedMode cell_mode = PackedMode::MODE_32, tag8_t mode_sub_class = static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType,
            PackedCellLocalityTypes locality_of_cell = PackedCellLocalityTypes::IDLE, 
            PriorityPhysics priority = PriorityPhysics::IMPORTANT, clk16_t extended_meta_value = UNSIGNED_ZERO
        ) noexcept;

    public:
        NeuromorphicSpaceTimeFabricCoordinator(/* args */) noexcept = default;

        ~NeuromorphicSpaceTimeFabricCoordinator() noexcept
        {
            ShutDownFabric();
        }

        NeuromorphicSpaceTimeFabricCoordinator(const NeuromorphicSpaceTimeFabricCoordinator&) = delete;
        NeuromorphicSpaceTimeFabricCoordinator& operator = (const NeuromorphicSpaceTimeFabricCoordinator&) = delete;

        void ShutDownFabric() noexcept;

        bool InitializeFabric(
            uint16_t slot_count,
            size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
            uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
            uint32_t fabric_thread_capacity = DEFAULT_THREAD_SLOT_OF_FABRIC
        ) noexcept;

    };

    

}
