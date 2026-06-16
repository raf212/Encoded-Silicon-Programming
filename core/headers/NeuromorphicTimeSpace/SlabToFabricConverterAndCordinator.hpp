#pragma once 
#include "FabricCellConf.hpp"

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

    class SlabToFabricConverterAndCordinator
    {
    public:

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

        /// @brief ONLY: Use for Initialiazation ONLY
        constexpr void MakeAndStoreFabricMetaValue48_(
            FabricMetaIndicies fabric_meta_idx, uint64_t value, 
            LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED,
            AccessContractOfValue access_contract = AccessContractOfValue::CAS_RMW,
            PriorityPolicy priority = PriorityPolicy::INFLUENCED
        )noexcept;
    
        /// @brief Only Reads Valid FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES -> Cells
        /// @param desired_table OriginOfRecord == FabricTableSegmentClasses -> Used Rename fore Ease of Developement
        /// @return VALID: Index / INVALID: SIZE_MAX
        constexpr size_t ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(OriginOfRecord desired_table) noexcept;
        
        /// @brief Compleatly validates by width and origin -> FabricTableSegmentClasses
        /// @param table_class desired origin table
        /// @return VALID:: 3 -> Packed Cells:: i)Begin, ii)End iii)SaftyAndOriginMeta OR: std::nullopt
        std::optional<FTSC_SlabRangeTripletFrom_RecordBookOfFTSC> GetValidSlabRangeTripletFromRecordBookOfFTSC(FabricTableSegmentClasses table_class) noexcept;

        void IdleAFabricTableClassRangesMemory_(FabricTableSegmentClasses table_class) noexcept;

        constexpr bool WriteARecordBookOfTSCEntry_(
            FabricTableSegmentClasses table_class, 
            size_t begin, size_t end, 
            uint8_t slab_id = UNSIGNED_ZERO
        ) noexcept;

        void InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept;

        APCDescriptorRange ReadRangeForASingleAPCSlotFromAPCDescriptor_(uint64_t apc_slot_index) noexcept;

        bool MemCopySingleAPCDescriptionIfValidFromBufferToSlabBasePtr_(
            DescriptionOfAPC::SingleAPCDescriptionCellBuffer& single_unvalidated_apc_description_buffer,
            DescriptionOfAPC::StateOfSingleAPCDescription updated_state,
            OwnershipPolicy updated_true_owner,
            bool check_consumeablity = false,
            std::optional<uint8_t> vesrion_match = std::nullopt
        ) noexcept;

//checked-----------------------------------------------

        constexpr bool UpdateValidPairedOccupancyApproxAtomically_(
            LocalityPolicy desired_occupancy_of_locality, uint64_t desired_occupancy_value,
            bool force_update = false,
            clk16_t pair_version = APCDataStructure::BRANCH_VERSION
        ) noexcept;

        constexpr void ResetAll4TypesOfOccupancyMetaData_() noexcept;

        /// @brief IN CPP FILE -> FIX:: MakeAndStoreFabricMetaValue48_-> USE ensure proper AccessContractOfValue for each Metaindex
        constexpr void WriteFabricMetaHeader_(size_t table_directory_begin, size_t table_directory_end) noexcept;


        void InitializeAPCDescriptorTable_() noexcept
        {
            DescriptionOfAPC::SingleAPCDescriptionCellBuffer single_apc_description{};
            DescriptionOfAPC::BuildABlankAPCDescriptionBufferwith2CellIdentity(single_apc_description);

            
        }


    public:
        SlabToFabricConverterAndCordinator(/* args */) noexcept = default;

        ~SlabToFabricConverterAndCordinator() noexcept
        {
            ShutDownFabric();
        }

        SlabToFabricConverterAndCordinator(const SlabToFabricConverterAndCordinator&) = delete;
        SlabToFabricConverterAndCordinator& operator = (const SlabToFabricConverterAndCordinator&) = delete;


        /// @brief Uses -> GetValidSlabRangeTripletFromRecordBookOfFTSC to get record and packs into -> APCDescriptorRange
        /// @return VALID::APCDescriptorRange.IsVAlid = true || INVALID:: APCDescriptorRange.IsVAlid = false
        APCDescriptorRange ReadAPCDescriptorTableBeginEndFromRecordBook() noexcept;



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
