#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{

    void RecordBookConstructor::IdleAFabricTableClassRangesMemory_(FabricTableSegmentClasses table_class) noexcept
    {

        RecordBookTablesBoundsCarrier return_bounds{};
        bool bounds_ok = GetValidSlabRangeTripletFromRecordBookOfFTSC(table_class, return_bounds);

        if (!bounds_ok)
        {
            return;
        }

        const packed64_t idle_table_cell = PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48, AccessContractOfValue::ATOMIC_SLNAPSHOT,
            table_class, LocalityPolicy::IDLE,
            InternalDataTypePolicy::UnsignedPCellDataType
        );

        for (size_t idx = return_bounds.BeginIndex; idx < return_bounds.EndIndex; idx++)
        {
            StorePackedCellUncheckedDirectly(idx, idle_table_cell);
        }
        
    }

    bool RecordBookConstructor::GetValidSlabRangeTripletFromRecordBookOfFTSC(
        FabricTableSegmentClasses table_class,
        RecordBookTablesBoundsCarrier& return_bounds
    ) noexcept
    {
        if (!PackedCell64_t::IsValidFabricTable(table_class))
        {
            return false;
        }

        const size_t begin_of_desired_table = ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(table_class) + static_cast<size_t>(RecordBookInternalIndexing::BEGIN48);
        const size_t end_idx = begin_of_desired_table + static_cast<size_t>(RecordBookInternalIndexing::END48);
        const size_t safty_lock_meta_cell = begin_of_desired_table + static_cast<size_t>(RecordBookInternalIndexing::META32);
        if (end_idx >= SlabCellCount_ || begin_of_desired_table < APCDataStructure::METACELL_COUNT)
        {
            return false;
        }

        const RecordBookCellTripletGroup triplet{ 
            ReadCompletePackedCellDirectly(begin_of_desired_table),
            ReadCompletePackedCellDirectly(end_idx),
            ReadCompletePackedCellDirectly(safty_lock_meta_cell)
        };


        return_bounds  = RecordBookConf::ValidateAFabricTableRangeStruct(triplet, table_class);
        if (
            return_bounds.IsValid &&
            return_bounds.BeginIndex >= APCDataStructure::METACELL_COUNT &&
            return_bounds.EndIndex > return_bounds.BeginIndex
        )
        {
            return true;
        }

        return_bounds.IsValid = false;
        return false;
    }


    constexpr void RecordBookConstructor::WriteARecordBookOfTSCEntry_(
        OriginOfRecord table_class, 
        size_t begin, size_t end, 
        uint8_t slab_id
    ) noexcept
    {
        if (!PackedCell64_t::IsValidFabricTable(table_class))
        {
            return;
        }

        if (!SlabBasePtr_)
        {
            return;
        }

        const size_t base_idx = ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(table_class);
        if (base_idx == APCDataStructure::APC_SIZE_SENTINAL || (base_idx + RECORD_BOOK_WIDTH > SlabCellCount_))
        {
            return;
        }

        RecordBookCellTripletGroup desired_record_triplet {};

        desired_record_triplet.BeginIdxRawType48Cell = RecordBookConf::MakeRecordBookCellOfTSC(
            static_cast<uint64_t>(begin)
        );

        desired_record_triplet.EndIdxRawType48Cell =  RecordBookConf::MakeRecordBookCellOfTSC(
            static_cast<uint64_t>(end)
        );

        desired_record_triplet.WidthVersionOriginSafty = RecordBookConf::MakeRecordBookSaftyLock(
            begin, end, table_class, 
            LocalityPolicy::PUBLISHED, slab_id
        );


        const RecordBookTablesBoundsCarrier validated_bounds = RecordBookConf::ValidateAFabricTableRangeStruct(desired_record_triplet, table_class);
        if (
            validated_bounds.IsValid &&
            validated_bounds.BeginIndex >= APCDataStructure::METACELL_COUNT &&
            validated_bounds.EndIndex > validated_bounds.BeginIndex
        )
        {
            AtomicallyStorePackedCellUnchecked(
                base_idx + static_cast<size_t>(RecordBookInternalIndexing::BEGIN48), 
                desired_record_triplet.BeginIdxRawType48Cell
            );
            
            AtomicallyStorePackedCellUnchecked(
                base_idx + static_cast<size_t>(RecordBookInternalIndexing::END48), 
                desired_record_triplet.EndIdxRawType48Cell
            );                
            
            AtomicallyStorePackedCellUnchecked(
                base_idx + static_cast<size_t>(RecordBookInternalIndexing::META32), 
                desired_record_triplet.WidthVersionOriginSafty
            );

            return;
        }
        
        return;
        
    }


    constexpr size_t RecordBookConstructor::ReadOriginIndexBeginOfRecordBookOfFabricTableSegmentClasses_(
        OriginOfRecord table_class
    ) noexcept
    {
        if (!PackedCell64_t::IsValidFabricTable(table_class))
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }

        /// ALways same derives from -> FabricMetaIndicies
        const packed64_t directory_begin_cell = ReadCompletePackedCellDirectly(static_cast<size_t>(FabricMetaIndicies::RECORD_BOOK_OF_TSC_BEGIN));

        if (RecordBookConf::IsThisValidRecordBookPackedCell(directory_begin_cell))
        {
            const size_t base_origin_table_idx = static_cast<size_t>(PackedCell64_t::ExtractRaw48FamilyBits(directory_begin_cell));

            return base_origin_table_idx + (static_cast<size_t>(table_class) * RECORD_BOOK_WIDTH);        
        }
        
        return APCDataStructure::APC_SIZE_SENTINAL;

    }
        
}
