#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{

    /// @brief should be 3
    static constexpr size_t RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC = 3u;

    /// @brief should be 3
    static constexpr size_t HASH_BUCKED_WIDTH_OF_FABRIC = 2u;

    enum class HandleFabricCellSequense : size_t
    {
        BEGIN48 = 0,
        END48 = 1,
        META32 = 2
    };

    struct FTSC_SlabRangeTripletFrom_RecordBookOfFTSC
    {
        packed64_t BeginIdxRawType48Cell = UNSIGNED_ZERO;
        packed64_t EndIdxRawType48Cell = UNSIGNED_ZERO;
        packed64_t WidthVersionOriginSafty = UNSIGNED_ZERO;
    };
    static_assert(sizeof(FTSC_SlabRangeTripletFrom_RecordBookOfFTSC) == RECORD_BOOK_OF_TABLE_SEGMENT_CLASS_WIDTH_OF_FABRIC * sizeof(packed64_t));
    static_assert(alignof(FTSC_SlabRangeTripletFrom_RecordBookOfFTSC) == alignof(packed64_t));



struct FabricCellConf
{
    /// @brief Model32Subclass::UNCLOCKED_1x8_PLUS_2x4-> Value + Version + HandleFabricCellSequense + IDENTITY(Though used FabricTableSegmentClasses::but means identity of directory no cell) + Meta16
    /// @return VALID -> Packed Cell -> OR: UINT64_MAX
    static constexpr packed64_t MakeRecordBookCellOfTSC(
        uint64_t value,
        AccessContractOfValue contract48 = AccessContractOfValue::RAW_PRIVATE,
        LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {
        return PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48,
            contract48,
            FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES,
            cell_locality,
            InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy::ERROR_FIRST,
            value 
        );

    }


    /// @brief Cell DEFAULTS: TypeFamily::VALUE48 + AccessContractOfValue::CLAIMED_GURDED + PriorityPolicy::INFLUENCED
    /// @return VALID -> Packed Cell -> OR: UINT64_MAX:: if FabricTableSegmentClasses dosent belong  BRANCH_HASH, SHARED_HASH, LOGICAL_HASH
    static constexpr packed64_t MakeHashKeyOrValueCell(
        uint64_t hash_key_or_value,
        FabricTableSegmentClasses hash_table_class, 
        LocalityPolicy locality = LocalityPolicy::IDLE
    ) noexcept
    {
        if (
            hash_table_class != FabricTableSegmentClasses::BRANCH_HASH &&
            hash_table_class != FabricTableSegmentClasses::SHARED_HASH &&
            hash_table_class != FabricTableSegmentClasses::LOGICAL_HASH
        )
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }

        return PackedCell64_t::PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48, 
            AccessContractOfValue::CLAIMED_GURDED, 
            hash_table_class, 
            locality,
            InternalDataTypePolicy::UnsignedPCellDataType, 
            PriorityPolicy::INFLUENCED,
            hash_key_or_value
        );
    }
};


}