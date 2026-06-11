#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{




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
        if (!CoreOfFabricCoordinator::IsValidHashTable(hash_table_class))
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }

        if (locality == LocalityPolicy::PUBLISHED && 
            (hash_key_or_value == UNSIGNED_ZERO || hash_key_or_value >= PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT)
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

    static constexpr packed64_t MakeHashProbDistanceCellWithSaftyLock(
        uint64_t key48, 
        uint64_t hash_48,
        uint16_t prob_distance,
        FabricTableSegmentClasses table_class,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {
        const uint16_t mid_Lock_key_low16 = static_cast<uint16_t>(key48 & MaskLowNBits(LOW16_BIT_MASK));
        const uint16_t high_lock_hash_low_16 = static_cast<uint16_t>(hash_48 & MaskLowNBits(LOW16_BIT_MASK));

        const uint64_t desired_prob_distance_lock = Subdevision16x3InternalMode48CellModel::PackUnsigned16x3ToMode48_(
            prob_distance,
            mid_Lock_key_low16,
            high_lock_hash_low_16
        );

        return PackedCell64_t::MakeModeledFabricValidPackedCell(
            ModelFamily::MODEL48,
            static_cast<tag8_t>(Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL),
            table_class,
            locality,
            InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy::INFLUENCED,
            desired_prob_distance_lock
        );
    }


    // static constexpr bool IsValidHashPackedCell(PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
    // {
    //     if (
    //         !a_cell_view.IsCellValid || 
    //         a_cell_view.CellOwnership != OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC ||
    //         a_cell_view.CellValueDataType != InternalDataTypePolicy::UNASSIGNED_UNUSED_NANNULL
    //     )
    //     {
    //         return false;
    //     }
        
        
    // }


    // static constexpr std::optional<HashKeyValueDistanceTriplet> ReadProbDistanceFromValidPackedCell(
    //     packed64_t key_cell,
    //     packed64_t value_cell,
    //     packed64_t prob_distance_safty
    // ) noexcept
    // {
    //     const PackedCell64_t::AuthoritiveCellView key_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(key_cell);
    //     const PackedCell64_t::AuthoritiveCellView value_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(key_cell);
    //     const PackedCell64_t::AuthoritiveCellView prob_lock_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(key_cell);



    // }


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
            PriorityPolicy::PRESSURE_FIRST,
            masked_width,
            version_origin_slabid
        );
            
    }


};


}