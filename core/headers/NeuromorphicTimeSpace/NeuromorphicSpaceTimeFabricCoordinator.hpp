#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{

    class NeuromorphicSpaceTimeFabricCoordinator : private APCDataStructure
    {
    private:
        std::atomic<packed64_t>* SlabBasePtr_{nullptr};
        size_t SlabCellCount_{UNSIGNED_ZERO};
        size_t SlotCellCount_{UNSIGNED_ZERO};
        uint16_t SlotCount_{UNSIGNED_ZERO};
        uint16_t SlabId_{UNSIGNED_ZERO};
        size_t SegmentPoolBegin_{METACELL_COUNT};
        size_t SegmentPoolEnd_{METACELL_COUNT};
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

        void ResetScalarsofTheFabric() noexcept;


    public:
        NeuromorphicSpaceTimeFabricCoordinator(/* args */) noexcept = default;

        ~NeuromorphicSpaceTimeFabricCoordinator() noexcept
        {
            ShutDownFabric();
        }

        NeuromorphicSpaceTimeFabricCoordinator(const NeuromorphicSpaceTimeFabricCoordinator&) = delete;
        NeuromorphicSpaceTimeFabricCoordinator& operator = (const NeuromorphicSpaceTimeFabricCoordinator&) = delete;

        void ShutDownFabric() noexcept;

    };

    

}
