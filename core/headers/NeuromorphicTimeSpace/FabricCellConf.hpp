#pragma once 
#include "CoreOfFabricCoordinator.hpp"

namespace PredictedAdaptedEncoding
{




struct FabricCellConf
{
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


};

struct SingleAPCDescriptionStruct
{


    using SingleAPCDescriptionCellBuffer = std::array<uint64_t, APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC + 1>;

    /// @brief Assignes UINT64_MAX  UPTO:INDEX: APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC - 1 and Next 2 INDEX: UNSIGNED_ZERO
    /// @param default_array 
    static constexpr void BuildABlankAPCDescriptionBufferwith2CellIdentity(SingleAPCDescriptionCellBuffer& default_array)
    {
        for (size_t i = 0; i < default_array.size(); i++)
        {
            if (i < APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC)
            {
                default_array[i] = PackedCell64_t::PACKED_CELL_SENTINAL;
            }
            else
            {
                default_array[i] = UNSIGNED_ZERO;
            }
        }
    }

    enum class StateOfSingleAPCDescription : uint8_t
    {
        EMPTY_RECORD = 0,
        RECORD_WITH_SEGMENT_POOL = 1,
        OWNED_BY_APC = 2,
        RETIREMENT_REQUEST = 3,
        LOGICALY_RETIRED = 4
    };

    static constexpr bool IsValidValue48APCDescription(APCDescriptorCellType descriptor_cell_type) noexcept
    {
        if (descriptor_cell_type < APCDescriptorCellType::OCCUPANCY_CELL16x3)
        {
            return true;
        }
        return false;
    }

    static constexpr bool IsValidAPCDescriptionCell(
        packed64_t packed_cell, 
        std::optional<PackedMode> mode_check = std::nullopt,
        bool check_consumeablity = true
    ) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView desired_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
        if (!CoreOfFabricCoordinator::CommonValidityCheckOfFabricCellsTableSegmentClasses(desired_auth_view) ||
            desired_auth_view.FabricTableSegmentClass != FabricTableSegmentClasses::APC_DESCRIPTOR
        )
        {
            return false;
        }

        if ( 
            check_consumeablity && 
            desired_auth_view.LocalityOfCell == LocalityPolicy::CLAIMED
        )
        {
            return false;
        }

        if (mode_check.has_value() && mode_check != desired_auth_view.CellMode)
        {
            return false;
        }
        

        switch (desired_auth_view.CellMode)
        {

        case PackedMode::VALUE48:
            return desired_auth_view.ContractOfValue == AccessContractOfValue::CLAIMED_GURDED;
        
        case PackedMode::MODEL48:
            return desired_auth_view.SubClassOfModel48 == Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL;

        case PackedMode::MODEL32:
            return desired_auth_view.SubClassOfModel32 == Model32Subclass::UNCLOCKED_1x8_PLUS_2x4;
        
        default:
            return false;
        }
        
    }


    static constexpr packed64_t MakeAPCDescriptionValue48Cell(
        uint64_t cell_value, 
        LocalityPolicy locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {
        return PackedCell64_t::MakeTypedFabricValidPackedCell(
            TypeFamily::VALUE48,
            AccessContractOfValue::CLAIMED_GURDED,
            FabricTableSegmentClasses::APC_DESCRIPTOR,
            locality,
            InternalDataTypePolicy ::UnsignedPCellDataType,
            PriorityPolicy::INFLUENCED,
            cell_value
        );
        
    }


    /// @brief USES: Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL & CREATS: Occupancy cell OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC  [LocalityPolicy::PUBLISHED | LocalityPolicy::CLAIMED | LocalityPolicy::FAULTY]
    /// @param published_count LOWEST: uint16_t in PackUnsigned16x3ToMode48_
    /// @param claimed_count MID: uint16_t in PackUnsigned16x3ToMode48_
    /// @param faulty_count HIGHIEST: uint16_t in PackUnsigned16x3ToMode48_
    /// @return 
    static constexpr packed64_t MakeOccupancyDescriptionForAPCDescriptionTable(
        uint16_t published_count,
        uint16_t claimed_count,
        uint16_t faulty_count,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {   
        const uint64_t raw_48 =  Subdevision16x3InternalMode48CellModel::PackUnsigned16x3ToMode48_(
            published_count,
            claimed_count,
            faulty_count
        );

        return PackedCell64_t::MakeModeledFabricValidPackedCell(
            ModelFamily::MODEL48,
            static_cast<tag8_t>(Model48Subclass::SUBDIVISION16x3_INTERNAL_CELL_MODEL),
            FabricTableSegmentClasses::APC_DESCRIPTOR,
            locality,
            InternalDataTypePolicy ::UnsignedPCellDataType,
            PriorityPolicy::INFLUENCED,
            raw_48
        );
    }

    /// @brief 
    /// @param state_of_apc MUST PACKEDMODE:MODEL32 -> AND Model32Subclass::UNCLOCKED_1x8_PLUS_2x4 -> [LOWEST16-> VERSION | MID16 -> State Of APC | HIGHIEST16 -> Ownership Of APC] 
    /// @return CELL INVALID: std::nullopt / HashKeyValueDistanceTriplet with Validity flag
    static constexpr packed64_t MakeStateandSaftyCellOfSingleAPCDescriptor(
        size_t segment_pool_begin, 
        size_t segment_pool_end,
        StateOfSingleAPCDescription state_of_apc,
        uint8_t version,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED,
        OwnershipPolicy origin_ownership = OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC
    )
    {

        const uint32_t apc_width = static_cast<uint32_t>(segment_pool_end - segment_pool_begin);

        uint16_t state_version_ownership = UNSIGNED_ZERO;
        if (apc_width == UNSIGNED_ZERO || apc_width > APCDataStructure::APC_MAX_LENGTH_OR_COUNTER)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        
        state_version_ownership = Clock16Subdivision1x8Plus2x4InMode32CellModel::Pack1x8Plus2x4InUnsigned16_(version, static_cast<uint8_t>(state_of_apc), static_cast<uint8_t>(origin_ownership));

        return PackedCell64_t::MakeModeledFabricValidPackedCell(
            ModelFamily::MODEL32,
            static_cast<tag8_t>(Model32Subclass::UNCLOCKED_1x8_PLUS_2x4),
            FabricTableSegmentClasses::APC_DESCRIPTOR,
            locality, InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy::INFLUENCED,
            apc_width,
            state_version_ownership
        );

    }


    static constexpr bool IsValidStateSaftyCell(
        SingleAPCDescriptionCellBuffer& single_apc_description,
        std::optional<OwnershipPolicy> validate_observer = std::nullopt,
        std::optional<StateOfSingleAPCDescription> desired_state = std::nullopt,
        std::optional<uint8_t> version_match = std::nullopt
    ) noexcept
    {
        const packed64_t segment_pool_begin_cell =  single_apc_description[static_cast<size_t>(APCDescriptorCellType::APC_SEGMENTPOOL_BEGAIN_SLAB)];
        const packed64_t segment_pool_end_cell = single_apc_description[static_cast<size_t>(APCDescriptorCellType::APC_SEGMENTPOOL_END_SLAB)];
        const packed64_t state_version_ownership_cell = single_apc_description[static_cast<size_t>(APCDescriptorCellType::STATE_OWNERSHIP_VESION_SAFTY)];

        if (
            !IsValidAPCDescriptionCell(segment_pool_begin_cell, PackedMode::VALUE48, true) ||
            !IsValidAPCDescriptionCell(segment_pool_end_cell, PackedMode::VALUE48, true) ||
            !IsValidAPCDescriptionCell(state_version_ownership_cell, PackedMode::MODEL32, true)
        )
        {
            return false;
        }
        
        const PackedCell64_t::AuthoritiveCellView auth_view_of_state_version_ownership = PackedCell64_t::GetAuthoritiveViewsForACell(state_version_ownership_cell);

        const uint32_t apc_width = static_cast<uint32_t>(
            PackedCell64_t::ExtractRaw48FamilyBits(segment_pool_begin_cell) - PackedCell64_t::ExtractRaw48FamilyBits(segment_pool_end_cell)
        );


        if (auth_view_of_state_version_ownership.Raw32BitInCellData != apc_width)
        {
            return false;
        }

        const uint8_t version = Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractLowest8Bit_(auth_view_of_state_version_ownership.InCellClock16);
        const StateOfSingleAPCDescription state_of_description = static_cast<StateOfSingleAPCDescription>(Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractMid4Bit_(auth_view_of_state_version_ownership.InCellClock16));
        const OwnershipPolicy ownershipe_of_description = static_cast<OwnershipPolicy>(Clock16Subdivision1x8Plus2x4InMode32CellModel::ExtractHighiest4Bit_(auth_view_of_state_version_ownership.InCellClock16));
        
        if (validate_observer.has_value() && *validate_observer != ownershipe_of_description)
        {
            return false;
        }

        if (desired_state.has_value() && *desired_state != state_of_description)
        {
            return false;
        }

        if (version_match.has_value() && *version_match != version)
        {
            return false;
        }
        
        return true;

    }

    static constexpr void SetADescriptionInCellBuffer(
        SingleAPCDescriptionCellBuffer& single_apc_description_buffer,
        APCDescriptorCellType cell_type, 
        uint64_t cell_value,
        LocalityPolicy cell_locality = LocalityPolicy::PUBLISHED
    ) noexcept
    {
        if (!IsValidValue48APCDescription(cell_type))
        {
            return;
        }
        
        const packed64_t desired_packed_cell = MakeAPCDescriptionValue48Cell(cell_value, cell_locality);

        single_apc_description_buffer[static_cast<size_t>(cell_type)] = desired_packed_cell;
    }  
    
    static constexpr void SetOccupancyInBuffer(
        SingleAPCDescriptionCellBuffer& single_apc_description_buffer,
        uint16_t published_count,
        uint16_t claimed_count,
        uint16_t faulty_count,
        LocalityPolicy locality = LocalityPolicy::PUBLISHED
    )
    {
        const packed64_t desired_occupancy_cell = MakeOccupancyDescriptionForAPCDescriptionTable(
            published_count,
            claimed_count,
            faulty_count,
            locality
        );

        single_apc_description_buffer[static_cast<size_t>(APCDescriptorCellType::OCCUPANCY_CELL16x3)] = desired_occupancy_cell;
    }

    /// @brief VALIDATES: INDEX 0 - APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC - 1 if valid sets 1 to index APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC == 12:: Means valid
    /// @param single_apc_description_buffer 
    /// @param check_consumeablity 
    /// @param validate_observer Fabric can observe when ownership to APC but APC cant observe when ownership to fabric 
    /// @param desired_state  APC shouldnt observe StateOfSingleAPCDescription::EMPTY_RECORD / StateOfSingleAPCDescription::LOGICALY_RETIRED
    /// @param version_match VERSION:: Probably optional for now 
    /// @return 
    static constexpr bool ValidateSingleAPCDescriptionBuffer(
        SingleAPCDescriptionCellBuffer& single_apc_description_buffer,
        bool check_consumeablity = true,
        std::optional<OwnershipPolicy> validate_observer = std::nullopt,
        std::optional<StateOfSingleAPCDescription> desired_state = std::nullopt,
        std::optional<uint8_t> version_match = std::nullopt
    ) noexcept
    {
        for (size_t i = 0; i < static_cast<size_t>(APCDescriptorCellType::OCCUPANCY_CELL16x3); i++)
        {
            if (!IsValidAPCDescriptionCell(single_apc_description_buffer[i], PackedMode::VALUE48, check_consumeablity))
            {
                return false;
            }            
        }

        if (
            !IsValidAPCDescriptionCell(
                single_apc_description_buffer[static_cast<size_t>(APCDescriptorCellType::OCCUPANCY_CELL16x3)], 
                PackedMode::MODEL48, 
                check_consumeablity
            ) ||
            !IsValidStateSaftyCell(single_apc_description_buffer, validate_observer, desired_state, version_match) 
        )
        {
            return false;
        }

        single_apc_description_buffer[APC_DESCRIPTOR_RECORD_WIDTH_IN_FABRIC] = 1ull;
        
        return true;
    }


};


}