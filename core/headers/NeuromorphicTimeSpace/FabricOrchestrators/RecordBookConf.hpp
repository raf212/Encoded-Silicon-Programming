#pragma once 
#include "../CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

struct FTSC_SlabRangeTripletFrom_RecordBookOfFTSC
{
    packed64_t BeginIdxRawType48Cell = UNSIGNED_ZERO;
    packed64_t EndIdxRawType48Cell = UNSIGNED_ZERO;
    packed64_t WidthVersionOriginSafty = UNSIGNED_ZERO;
    
};
static_assert(sizeof(FTSC_SlabRangeTripletFrom_RecordBookOfFTSC) == RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC * sizeof(packed64_t));
static_assert(alignof(FTSC_SlabRangeTripletFrom_RecordBookOfFTSC) == alignof(packed64_t));

struct SlabFabricTableBoundsCarrietFromRecordBookTable
{
    uint64_t BeginIndex = UNSIGNED_ZERO;
    uint64_t EndeIndex = UNSIGNED_ZERO;
    OriginOfRecord OwnerTableOfTheBounds = OriginOfRecord::NULLNAN;
    uint8_t Width = UNSIGNED_ZERO;
    bool IsValid = false;  
};
static_assert(sizeof(SlabFabricTableBoundsCarrietFromRecordBookTable) == RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC * sizeof(packed64_t));
static_assert(alignof(SlabFabricTableBoundsCarrietFromRecordBookTable) == alignof(packed64_t));


struct RecordBookConf
{
    static constexpr bool IsTheCellConsumeableAsRecordBookCellOfTSC(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
    {

        if (!CoreOfFabricCoordinator::CommonValidityCheckOfFabricCellsTableSegmentClasses(a_cell_view))
        {
            return false;
        }
        
        if (a_cell_view.FabricTableSegmentClass != FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES)
        {
            return false;
        }
        
        
        switch (a_cell_view.CellMode)
        {
        case PackedMode::VALUE48:
            return a_cell_view.ContractOfValue != AccessContractOfValue::UNASSIGNED_UNUSED_NANNULL &&
                a_cell_view.Raw48BitInCellData < PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT;
        case PackedMode::MODEL32:

            return a_cell_view.SubClassOfModel32 == Model32Subclass::UNCLOCKED_1x8_PLUS_2x4 &&
                a_cell_view.Raw32BitInCellData < IN_CELL_VALUE_MODE32_SENTINAL;

        default:
            return false;
        }            
    }

    static constexpr bool IsThisValidRecordBookPackedCell(
        packed64_t packed_cell,
        PackedCell64_t::AuthoritiveCellView* return_cell_view = nullptr
    ) noexcept
    {
        PackedCell64_t::AuthoritiveCellView desired_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
        const bool ok = IsTheCellConsumeableAsRecordBookCellOfTSC(desired_cell_view);
        if (return_cell_view)
        {
            *return_cell_view = desired_cell_view;
        }
        return ok;
    }


    static constexpr std::optional<uint64_t> ValidateAFabricTableRangeStruct(const FTSC_SlabRangeTripletFrom_RecordBookOfFTSC& provided_range_triplet, OriginOfRecord origin_table_segment_class) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView auth_view_of_begin_idx = PackedCell64_t::GetAuthoritiveViewsForACell(provided_range_triplet.BeginIdxRawType48Cell);
        const PackedCell64_t::AuthoritiveCellView auth_view_of_end_idx = PackedCell64_t::GetAuthoritiveViewsForACell(provided_range_triplet.EndIdxRawType48Cell);
        const PackedCell64_t::AuthoritiveCellView auth_view_of_safty_meta = PackedCell64_t::GetAuthoritiveViewsForACell(provided_range_triplet.WidthVersionOriginSafty);


        if (!auth_view_of_begin_idx.IsCellValid || !auth_view_of_begin_idx.IsCellValid || !auth_view_of_safty_meta.IsCellValid)
        {
            return std::nullopt;
        }

        if (
            !IsTheCellConsumeableAsRecordBookCellOfTSC(auth_view_of_begin_idx) || 
            !IsTheCellConsumeableAsRecordBookCellOfTSC(auth_view_of_end_idx) ||
            !IsTheCellConsumeableAsRecordBookCellOfTSC(auth_view_of_safty_meta)
        )
        {
            return std::nullopt;
        }

        if (auth_view_of_begin_idx.Raw48BitInCellData < APCDataStructure::METACELL_COUNT || 
            auth_view_of_begin_idx.Raw48BitInCellData >= auth_view_of_end_idx.Raw48BitInCellData ||
            auth_view_of_begin_idx.CellMode != PackedMode::MODEL32 ||
            auth_view_of_safty_meta.InCellClock16 == UNSIGNED_ZERO
        )
        {
            return std::nullopt;
        }

        const uint8_t record_width = Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractLowest8Bit_(auth_view_of_safty_meta.InCellClock16);
        const OriginOfRecord origin_table = static_cast<OriginOfRecord>(Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractMid4Bit_(auth_view_of_safty_meta.InCellClock16));

        if (origin_table != origin_table_segment_class)
        {
            return std::nullopt;
        }
        

        const uint64_t full_width = (auth_view_of_end_idx.Raw48BitInCellData) - (auth_view_of_begin_idx.Raw48BitInCellData);
        if ((static_cast<uint32_t>(full_width) !=  auth_view_of_safty_meta.Raw32BitInCellData))
        {
            return std::nullopt;
        }
        
        if (origin_table != FabricTableSegmentClasses::SEGMENT_POOL)
        {
            if (
                record_width != CoreOfFabricCoordinator::GetWidthOfValidFabricTable(origin_table_segment_class) ||
                (full_width % record_width) != UNSIGNED_ZERO
            )
            {
                return std::nullopt;
            }
        }
        

        return full_width;
    }

    /// @return VALID -> Packed Cell -> OR: UINT64_MAX
    static constexpr packed64_t MakeRecordBookCellOfTSC(
        uint64_t value,
        LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {
        return PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48,
            AccessContractOfValue::RAW_PRIVATE,
            FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES,
            cell_locality,
            InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy::INFLUENCED,
            value 
        );

    }

    /// @brief Creats a Decriptive cell for Record Book Table Class :: with external 16bit meta indicating 
    ///[LOW8-> PER RECORD WIDTH OF ORIGIN(EXCEPT:SEGMENT_POOL) | MID4-> OriginOfRecord(ORIGIN:FabricTableSegmentClasses) | HIGH4-> VERSION(SlabId_)]
    /// @param begin_idx 
    /// @param end_idx 
    /// @param origin_table_class 
    /// @param locality 
    /// @param version 
    /// @return 
    static constexpr packed64_t MakeRecordBookSaftyLock(
        size_t begin_idx, size_t end_idx, 
        OriginOfRecord origin_table_class,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED, 
        uint8_t slab_id = UNSIGNED_ZERO
    ) noexcept
    {
        const uint32_t masked_width = static_cast<uint32_t>(end_idx - begin_idx);

        const uint8_t origin_per_record_width = CoreOfFabricCoordinator::GetWidthOfValidFabricTable(origin_table_class);

        if (origin_per_record_width == CoreOfFabricCoordinator::EACH_TABLE_RECORD_SENTINAL)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        
        const uint16_t version_origin_slabid = Clock16Subdivision1x8Plus2x4InMode32CellModel::Pack1x8Plus2x4InUnsigned16_(origin_per_record_width, static_cast<uint8_t>(origin_table_class), slab_id);

        return PackedCell64_t::MakeModeledFabricValidPackedCell(ModelFamily::MODEL32,
            static_cast<tag8_t>(Model32Subclass::UNCLOCKED_1x8_PLUS_2x4),
            FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES,
            locality, InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy::INFLUENCED,
            masked_width,
            version_origin_slabid
        );
            
    }


};


}