#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{

struct HashKeyValueDistanceTriplet
{
    uint64_t HashValue = UNSIGNED_ZERO;
    uint64_t HashKey = UNSIGNED_ZERO;
    uint16_t ProbDistance = UNSIGNED_ZERO;
    bool IsValid = false;
};
static_assert(sizeof(HashKeyValueDistanceTriplet) == HASH_BUCKED_WIDTH_OF_FABRIC * sizeof(uint64_t));
static_assert(alignof(HashKeyValueDistanceTriplet) == alignof(uint64_t));

struct HashHelpers
{
    static constexpr uint8_t HASH_SHIFT_1 = 30u;
    static constexpr uint8_t HASH_SHIFT_2 = 27u;
    static constexpr uint8_t HASH_SHIFT_3 = 31u;
    static constexpr uint64_t DEFAULT_HAS_CONST_1 = 0xbf58476d1ce4e5b9ull;
    static constexpr uint64_t DEFAULT_HAS_CONST_2 = 0x94d049bb133111ebull;
    static constexpr uint64_t HASH_TOMBSTONE_KEY = PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT;
    static constexpr uint8_t MIN_HASH_BUCKET_COUNT = 16u;
    static constexpr uint16_t PROB_DISTANCE_SENTINAL = UINT16_MAX;
    
    static constexpr uint64_t NextPowerOf2Unsigned48_(uint64_t given_value) noexcept
    {
        if (given_value <= 2u)
        {
            return 2u;
        }
        --given_value;
        given_value |= given_value >> 1u;
        given_value |= given_value >> 2u;
        given_value |= given_value >> 4u;
        given_value |= given_value >> 8u;
        given_value |= given_value >> 16u;
        given_value |= given_value >> 32u;

        return given_value + 1u;
    }

    /// @brief Convert A Value to Hash
    /// @param given_value Must be 
    /// @return 
    static constexpr uint64_t HashUnsigned48_(uint64_t given_value) noexcept
    {
        given_value = given_value & MaskLowNBits(FAMILY_48_BIT_LEN);
        given_value ^= given_value >> HASH_SHIFT_1;
        given_value *= DEFAULT_HAS_CONST_1;
        given_value ^= given_value >> HASH_SHIFT_2;
        given_value *= DEFAULT_HAS_CONST_2;
        given_value ^=  given_value >> HASH_SHIFT_3;

        given_value = given_value & MaskLowNBits(FAMILY_48_BIT_LEN);

        return given_value == UNSIGNED_ZERO ? 1u : given_value;
    }

    static constexpr uint64_t BucketCountForExpectedEntries(uint64_t count_of_entries) noexcept
    {
        if (
            count_of_entries == UNSIGNED_ZERO ||
            count_of_entries >= APC_FABRIC_INDEX_SENTINAL
        )
        {
            return UNSIGNED_ZERO;
        }

        const uint64_t wanted_bucket_count = std::max<uint64_t>(MIN_HASH_BUCKET_COUNT, count_of_entries * 2);

        return NextPowerOf2Unsigned48_(wanted_bucket_count);
        
    }
};


struct HashTableConf : public HashHelpers
{

    static constexpr bool IsHashPackedCellRuntimeAccessable(const PackedCell64_t::AuthoritiveCellView& a_cell_view) noexcept
    {

        if (!CoreOfFabricCoordinator::CommonValidityCheckOfFabricCellsTableSegmentClasses(a_cell_view))
        {
            return false;
        }
        

        if (
            !CoreOfFabricCoordinator::IsValidHashTable(a_cell_view.FabricTableSegmentClass) ||
            a_cell_view.LocalityOfCell == LocalityPolicy::CLAIMED ||
            a_cell_view.Raw48BitInCellData == PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT
        )
        {
            return false;
        }

        switch (a_cell_view.CellMode)
        {
        case PackedMode::VALUE48:
            return a_cell_view.ContractOfValue == AccessContractOfValue::CLAIMED_GURDED;

        case PackedMode::MODEL48:
            return a_cell_view.SubClassOfModel48 == Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL;
            
        default:
            return false;
        }
        
    }


    /// @brief Cell DEFAULTS: TypeFamily::VALUE48 + AccessContractOfValue::CLAIMED_GURDED + AttributePolicy::INSTRUCTION_CELL
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
            AttributePolicy::SELF_CONTAINED_DATA_OR_MODEL,
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
            AttributePolicy::INSTRUCTION_CELL,
            desired_prob_distance_lock
        );
    }



    /// @brief Takes HASH: KEY + VALUE + LOCK :: TRIPLET -> VALIDSTS 
    /// @param key_cell 
    /// @param value_cell 
    /// @param prob_distance_safty MUST MATCH:Raw48BitInCellData -> [LOWEST16->Prob Distance | MID16 -> LOWEST:16 Bit From Hash Key | HIGHIEST16 -> LOWEST:16 Bit From Hash Value] AND Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL
    /// @return CELL INVALID: std::nullopt / HashKeyValueDistanceTriplet with Validity flag
    static constexpr HashKeyValueDistanceTriplet ReadKeyValueProbFromValidCells(
        packed64_t key_cell,
        packed64_t value_cell,
        packed64_t prob_distance_safty
    ) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView key_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(key_cell);
        const PackedCell64_t::AuthoritiveCellView value_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(value_cell);
        const PackedCell64_t::AuthoritiveCellView prob_lock_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(prob_distance_safty);

        const HashKeyValueDistanceTriplet invalid_value{};
        if (
            !IsHashPackedCellRuntimeAccessable(key_cell_auth_view) ||
            !IsHashPackedCellRuntimeAccessable(value_cell_auth_view) ||
            !IsHashPackedCellRuntimeAccessable(prob_lock_cell_auth_view)
        )
        {
            return invalid_value;
        }

        uint16_t lock_mid16_key = UNSIGNED_ZERO;
        uint16_t lock_highiest16_value = UNSIGNED_ZERO;
        uint16_t lowest_prob_distance = UNSIGNED_ZERO;

        bool ok = Subdevision16x3InternalMode48CellModel::ExtractLowMidHighFromMode48_(prob_lock_cell_auth_view.Raw48BitInCellData, lowest_prob_distance, lock_mid16_key, lock_highiest16_value);

        if (!ok)
        {
            return invalid_value;
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


};




}