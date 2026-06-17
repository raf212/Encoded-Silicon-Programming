#pragma once 
#include "FabricCellConf.hpp"

namespace PredictedAdaptedEncoding
{

    
    class SlabToFabricConverterAndCordinator
    {
    public:

    private:
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

        //FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES

        /// @brief Only Reads Valid FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES -> Cells
        /// @param desired_table OriginOfRecord == FabricTableSegmentClasses -> Used Rename fore Ease of Developement
        /// @return VALID: Index / INVALID: SIZE_MAX
        constexpr size_t ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(OriginOfRecord desired_table) noexcept;
        
        /// @brief Compleatly validates by width and origin -> FabricTableSegmentClasses
        /// @param table_class desired origin table
        /// @return VALID:: 3 -> Packed Cells:: i)Begin, ii)End iii)SaftyAndOriginMeta OR: false & Maybe Inspactable data
        bool GetValidSlabRangeTripletFromRecordBookOfFTSC(
            FabricTableSegmentClasses table_class,
            SlabFabricTableBoundsCarrietFromRecordBookTable& return_bounds
        ) noexcept;

        /// @brief FILL: DESIRED: FabricTableSegmentClasses with Idle Fabric Cell -> CALLS: GetValidSlabRangeTripletFromRecordBookOfFTSC TO: Get Range In SLab
        /// @param table_class Desired FabricTableSegmentClasses You want Idle
        void IdleAFabricTableClassRangesMemory_(FabricTableSegmentClasses table_class) noexcept;

        /// @brief WRITES: A Single Entry OF: FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES == (2xPackedMode::VALUE48 + 1xPackedMode::Model32)
        /// @param table_class Desired FabricTableSegmentClasses == OriginOfRecord
        /// @param begin Begin Index OF: FabricTableSegmentClasses -> Record
        /// @param end End Index OF: FabricTableSegmentClasses -> Record
        constexpr void WriteARecordBookOfTSCEntry_(
            OriginOfRecord table_class, 
            size_t begin, size_t end, 
            uint8_t slab_id = UNSIGNED_ZERO
        ) noexcept;
        // END::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES

        //FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR

        APCDescriptorRange ReadRangeForASingleAPCSlotFromAPCDescriptor_(uint64_t apc_slot_index) noexcept;

        bool MemCopySingleAPCDescriptionIfValidFromBufferToSlabBasePtr_(
            DescriptionOfAPC::SingleAPCDescriptionCellBuffer& single_unvalidated_apc_description_buffer,
            DescriptionOfAPC::StateOfSingleAPCDescription updated_state,
            OwnershipPolicy updated_true_owner,
            bool check_consumeablity = false,
            std::optional<uint8_t> vesrion_match = std::nullopt
        ) noexcept;

        /// @brief BUILD: & INITIALIZED: All The APC Handle Descriptor With Segment Pool <-  CONSISTING: Packed CEll -> PacvkedMode::VALUE32
        void InitializeAPCDescriptorTable_() noexcept;
        //END:FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR

        //FabricTableSegmentClasses::BRANCH_HASH / LOGICAL_HASH / SHARED_HASH

        void InitializeHashTable_(FabricTableSegmentClasses table_class) noexcept;


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

        /// @brief CALLES: 4xUpdateValidPairedOccupancyApproxAtomically_ ON: Each LocalityPolicy
        constexpr void Zero4LocalityBasedOccupancyOfFabric_() noexcept;

        /// @brief INITIALIZES: All FabricMetaIndicies
        /// @param table_directory_begin 
        /// @param table_directory_end 
        constexpr void InitializeCompleateFabricMetaIndices_(size_t table_directory_begin, size_t table_directory_end) noexcept;



//checked----------------------------------------------










        // bool InsertOrUpdateRobinHoodHash48_(FabricTableSegmentClasses hash_table, uint64_t key48, uint64_t value48) noexcept
        // {
        //     if (key48 == UNSIGNED_ZERO || key48 ==  HashTableConf::HASH_TOMBSTONE_KEY)
        //     {
        //         return false;
        //     }

        //     const HashKeyValueDistanceTriplet desired_triplet = ReadValidHashBucketTriplet()
            
        // }


        std::optional<uint64_t> FindHashValue48_(FabricTableSegmentClasses hash_table, uint64_t key48) noexcept;

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
        bool ReadAPCDescriptorTableBeginEndFromRecordBook(
            APCDescriptorRange& return_APC_handle_description_range
        ) noexcept;

        void ShutDownFabric() noexcept;
        
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

        /// @brief Directly Gets Segment Pool Range For An APC by using INDEX: Of FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR | Dosent validate handle OR: Initialization 
        /// @param single_description_index Sequential index of desired APC FabricTableSegmentClasses::APC_HANDLE_DESCRIPTOR
        /// @return VALID: APCDescriptorRange::IsValid -> true || INVALID: APCDescriptorRange::IsValid -> true-> false
        APCDescriptorRange GetSegmentPoolBegainEndForSingleAPCDescription_(uint64_t single_description_index) noexcept;

        /// @brief Takes Base Bucket Index -> GATHER: 3 Cells -> CALLS: HashTableConf::ReadKeyValueProbFromValidCells
        /// @param bucked_base_index First Index In Slab For That Hash SIMPLY: HashTableInternalIndexing::KEY_INDEX OF: ANY: Hash Table
        /// @return VALID: HashKeyValueDistanceTriplet.IsValid -> true || INVALID: HashKeyValueDistanceTriplet.IsValid = false
        HashKeyValueDistanceTriplet ReadValidHashBucketTriplet(size_t bucked_base_index) noexcept;


/// checked--------------------------------------------------------

        bool InitializeFabric(
            uint16_t slot_count,
            size_t slot_cell_count = MINIMUM_BRANCH_CAPACITY,
            uint8_t slab_id = APCDataStructure::BRANCH_VERSION,
            uint32_t fabric_thread_capacity = DEFAULT_THREAD_SLOT_OF_FABRIC
        ) noexcept;

    };

    

}
