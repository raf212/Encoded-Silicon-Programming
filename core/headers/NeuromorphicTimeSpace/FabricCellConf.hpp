#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{




struct FabricCellConf
{
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


    /// @brief PACKEDMODE:MODE48 -> Model48Subclass::FOUR_SUBDIVISION_2x16_AND_2x8[LOWEST16->Prob Distance | MID16 -> LOWEST:16 Bit From Hash Key | HIGHIEST16 -> LOWEST:16 Bit From Hash VAlue]
    /// @param table_class FabricTableSegmentClasses -> BRANCH_HASH / LOGICAL_HASH / SHARED_HASH
    /// @return 
    static constexpr packed64_t MakeHashProbDistanceCellWithSaftyLock(
        uint64_t key48, 
        uint64_t hash_48,
        uint16_t prob_distance,
        FabricTableSegmentClasses table_class,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {
        const uint16_t mid_Lock_key_low16 = static_cast<uint16_t>(key48 & MaskLowNBits(LOW16_BIT_LEN));
        const uint16_t high_lock_hash_low_16 = static_cast<uint16_t>(hash_48 & MaskLowNBits(LOW16_BIT_LEN));

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



    /// @brief Takes HASH: KEY + VALUE + LOCK :: TRIPLET -> VALIDSTS 
    /// @param key_cell 
    /// @param value_cell 
    /// @param prob_distance_safty MUST MATCH:Raw48BitInCellData -> [LOWEST16->Prob Distance | MID16 -> LOWEST:16 Bit From Hash Key | HIGHIEST16 -> LOWEST:16 Bit From Hash Value] AND Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL
    /// @return CELL INVALID: std::nullopt / HashKeyValueDistanceTriplet with Validity flag
    static constexpr std::optional<HashKeyValueDistanceTriplet> ReadProbDistanceFromValidPackedCell(
        packed64_t key_cell,
        packed64_t value_cell,
        packed64_t prob_distance_safty
    ) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView key_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(key_cell);
        const PackedCell64_t::AuthoritiveCellView value_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(value_cell);
        const PackedCell64_t::AuthoritiveCellView prob_lock_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(prob_distance_safty);

        if (
            !CoreOfFabricCoordinator::IsHashPackedCellRuntimeAccessable(key_cell_auth_view) ||
            !CoreOfFabricCoordinator::IsHashPackedCellRuntimeAccessable(value_cell_auth_view) ||
            !CoreOfFabricCoordinator::IsHashPackedCellRuntimeAccessable(prob_lock_cell_auth_view)
        )
        {
            return std::nullopt;
        }

        uint16_t lock_mid16_key = UNSIGNED_ZERO;
        uint16_t lock_highiest16_value = UNSIGNED_ZERO;
        uint16_t lowest_prob_distance = UNSIGNED_ZERO;

        bool ok = Subdevision16x3InternalMode48CellModel::ExtractLowMidHighFromMode48_(prob_lock_cell_auth_view.Raw48BitInCellData, lowest_prob_distance, lock_mid16_key, lock_highiest16_value);

        if (!ok)
        {
            return std::nullopt;
        }

        const uint16_t key_low16 = static_cast<uint16_t>(key_cell_auth_view.Raw48BitInCellData & MaskLowNBits(LOW16_BIT_LEN));
        const uint16_t value_low16 = static_cast<uint16_t>(value_cell_auth_view.Raw48BitInCellData & MaskLowNBits(LOW16_BIT_LEN));

        if (
            lock_mid16_key == key_low16 &&
            lock_highiest16_value == value_low16
        )
        {
            return HashKeyValueDistanceTriplet{
                value_cell_auth_view.Raw48BitInCellData,
                key_cell_auth_view.Raw48BitInCellData,
                lowest_prob_distance,
                true
            };
        }

        return HashKeyValueDistanceTriplet{
            value_cell_auth_view.Raw48BitInCellData,
            key_cell_auth_view.Raw48BitInCellData,
            lowest_prob_distance,
            false
        };
        

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
            PriorityPolicy::PRESSURE_FIRST,
            masked_width,
            version_origin_slabid
        );
            
    }


};


}