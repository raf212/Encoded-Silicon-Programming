
#pragma once 
#include <array>
#include <utility>
#include "ConstructorsAndCarriersOfAPC.hpp"

namespace PredictedAdaptedEncoding
{


struct HashIdConstructror
{
    static constexpr uint64_t GROUP_IDX_BIT_BOUNDRY = 16u;
    static constexpr uint64_t GROUP_SEQUENTIAL_INDEX_MASK = UINT16_MAX;
    static constexpr uint64_t GROUP_PREFIX_MASK = UINT32_MAX;

    /// @brief VALIDATES THE RAW ID 
    static constexpr bool IsValidAPCId48(uint64_t value) noexcept
    {
        return value != UNSIGNED_ZERO && value < PackedCell64_t::BIT_FAMILY_48_SENTINAL;
    }

    static constexpr bool IsValidAPCSlotIdx(uint64_t slot_idx) noexcept
    {
        return slot_idx < PackedCell64_t::APC_FABRIC_SLOT_LIMIT;
    }

    static constexpr bool IsValidHashHandle(uint64_t handle) noexcept
    {
        return handle > UNSIGNED_ZERO && handle < PackedCell64_t::BIT_FAMILY_48_SENTINAL;
    }

    static constexpr uint64_t APCSlotIdxToHashTableHandler(uint64_t apc_slot_idx) noexcept
    {
        if (IsValidAPCSlotIdx(apc_slot_idx))
        {
            return apc_slot_idx + 1;
        }
        return PackedCell64_t::PACKED_CELL_SENTINAL;
    }

    static constexpr uint64_t HashTableHandlerToAPCSlotIdx(uint64_t handler) noexcept
    {
        if (IsValidHashHandle(handler))
        {
            return handler - 1;
        }
        return PackedCell64_t::PACKED_CELL_SENTINAL;
    }

    /// @brief CREATS: HASH KEY: Based On a Desired SHARED / LOGICAL Group ID
    /// @param sequential_idx_of_desired_id SEQUENTIAL IDX < UINT16_MAX - 1
    /// @return IF INVALID: UINT64_MAX
    static constexpr uint64_t MakeGroupAccessKey48(uint64_t group_id, uint64_t sequential_idx_of_desired_id) noexcept
    {
        if (
            group_id == UNSIGNED_ZERO ||
            group_id > GROUP_SEQUENTIAL_INDEX_MASK ||
            sequential_idx_of_desired_id > APCDataStructure::APC_ALL_INDEX_LIMIT
        )
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }

        const uint64_t key = ((group_id & GROUP_PREFIX_MASK) << GROUP_IDX_BIT_BOUNDRY) | (sequential_idx_of_desired_id & GROUP_SEQUENTIAL_INDEX_MASK);

        return IsValidAPCId48(key) ? key : PackedCell64_t::PACKED_CELL_SENTINAL;
    }

    /// @brief Get 32 Bit Prefix of A Group Key Can be used to set a Different sequential idx to find linked APC's
    /// @param group_key48 Raw key
    /// @return If Key VALID: -> 32 BIT PREFIX / std::nullopt
    static constexpr std::optional<uint32_t> GroupPreFix32FromKey48(uint64_t group_key48) noexcept
    {
        if (!IsValidAPCId48(group_key48))
        {
            return std::nullopt;
        }
        
        return static_cast<uint32_t>((group_key48 >> GROUP_IDX_BIT_BOUNDRY) & GROUP_PREFIX_MASK);
    }

    /// @brief Get 16 Bit Sequential Linked Idx From Key
    /// @param group_key48 Raw Key
    /// @return if Key VALID: -> 16 BIT SEQUENTIAL IDX / std::nullopt
    static constexpr std::optional<uint16_t> GetSeqIndexOfAHashKey(uint64_t group_key48) noexcept
    {
        if (!IsValidAPCId48(group_key48))
        {
            return std::nullopt;
        }

        return static_cast<uint16_t>(group_key48 & GROUP_SEQUENTIAL_INDEX_MASK);
    }

    static constexpr uint64_t RebuildOriginalKey(uint32_t prefix32, uint16_t index16) noexcept
    {
        const uint64_t original_key = (static_cast<uint64_t>(prefix32) << GROUP_IDX_BIT_BOUNDRY | static_cast<uint64_t>(index16));

        return IsValidAPCId48(original_key) ? original_key : PackedCell64_t::PACKED_CELL_SENTINAL;
    }

    static uint64_t MakeARandom48bitValue() noexcept
    {
        static std::atomic<uint64_t> global_counter{1u};

        auto SplitMix64 = [](uint64_t x) noexcept -> uint64_t
        {
            x += 0x9E3779B97F4A7C15ull;
            x = (x ^ (x >> 30u)) * 0xBF58476D1CE4E5B9ull;
            x = (x ^ (x >> 27u)) * 0x94D049BB133111EBull;
            x = x ^ (x >> 31u);
            return x;
        };

        uint64_t random_seed = global_counter.fetch_add(1, std::memory_order_acq_rel);

        random_seed ^= static_cast<uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );

        random_seed ^= reinterpret_cast<uint64_t>(&random_seed);

        try
        {
            std::random_device random_device;
            random_seed ^= (static_cast<uint64_t>(random_device()) << VALBITS);
            random_seed ^= static_cast<uint64_t>(random_device());
        }
        catch(...)
        {
            random_seed ^= 0xD6E8FEB86659FD93ull;
        }

        for (uint32_t attempt = 0; attempt < 8u; attempt++)
        {
            random_seed = SplitMix64(random_seed);
            const uint64_t seed48 = random_seed & MaskLowNBits(TOTAL_LOW);

            if (seed48 != UNSIGNED_ZERO && seed48 < PackedCell64_t::BIT_FAMILY_48_SENTINAL)
            {
                return seed48;
            }
        }
        
        const uint64_t fallback = SplitMix64(global_counter.fetch_add(1u, std::memory_order_acq_rel)) & MaskLowNBits(TOTAL_LOW);

        return fallback!= UNSIGNED_ZERO && fallback < PackedCell64_t::BIT_FAMILY_48_SENTINAL ? fallback : PackedCell64_t::PACKED_CELL_SENTINAL;

    }

};

struct APCGroupReserver
{
    enum class APCIdentityDef : uint8_t
    {
        INVALID = 0,
        ROOT = 1,
        CHILD = 2,
        DEFAULT_ASSIGNMENT = 3,
        NULL_USER_INSTRUCTION = 4
    };

    struct APCInitialIdentityStruct
    {
        PackedMode InitialMode = PackedMode::UNASSIGNED_UNUSED_NANNULL;

        uint64_t APCSlotIndex = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t BranchID = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t SharedID = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t LogicalId = PackedCell64_t::PACKED_CELL_SENTINAL;

        uint64_t AcessPassCodeOfAPC = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t SharedHashKey = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t LogicalHashKey = PackedCell64_t::PACKED_CELL_SENTINAL;

        uint64_t SharedPreviousId = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t SharedNextId = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t LogicalPreviousId = PackedCell64_t::PACKED_CELL_SENTINAL;
        uint64_t LogicalNextId = PackedCell64_t::PACKED_CELL_SENTINAL;

        uint16_t SharedSequentialCount = APCDataStructure::APC_INDEX_SENTINAL;
        uint16_t LogicalSequentalCount = APCDataStructure::APC_INDEX_SENTINAL;

        bool IsAssignable = false;
        APCIdentityDef HorizontalSharedState = APCIdentityDef::INVALID;
        APCIdentityDef VarticalLogicState  = APCIdentityDef::INVALID;
        bool IsAPCRootOfThisLocic = false;


        enum class APCSegmentExtendOrder : uint8_t
        {
            FIFO = 0,
            PRIORITY = 1,
            RANDOM = 2
        };
    };

    static constexpr bool IsMinimalValidAxis(const APCInitialIdentityStruct& a_initial_apc_conf) noexcept
    {
        return a_initial_apc_conf.HorizontalSharedState != APCIdentityDef::INVALID &&
            a_initial_apc_conf.VarticalLogicState != APCIdentityDef::INVALID;
    }

    static constexpr bool IsMinimalValidIdentity(APCInitialIdentityStruct& a_initial_apc_conf) noexcept
    {
        a_initial_apc_conf.IsAssignable = a_initial_apc_conf.InitialMode != PackedMode::UNASSIGNED_UNUSED_NANNULL &&
            HashIdConstructror::IsValidAPCSlotIdx(a_initial_apc_conf.APCSlotIndex) &&
            HashIdConstructror::IsValidAPCId48(a_initial_apc_conf.BranchID) &&
            HashIdConstructror::IsValidAPCId48(a_initial_apc_conf.AcessPassCodeOfAPC) &&
            IsMinimalValidAxis(a_initial_apc_conf);
        
        return a_initial_apc_conf.IsAssignable;
    }

    static constexpr APCInitialIdentityStruct MakeDefaultIdentityForAPC(PackedMode initial_mode) noexcept
    {
        APCInitialIdentityStruct requested_identity{};
        requested_identity.InitialMode = initial_mode;
        requested_identity.HorizontalSharedState = APCIdentityDef::NULL_USER_INSTRUCTION;
        requested_identity.VarticalLogicState = APCIdentityDef::NULL_USER_INSTRUCTION;
        return requested_identity;
    }

    static constexpr APCInitialIdentityStruct MakeAGroupedIdentityForAPC(
        PackedMode initial_mode,
        uint64_t shared_id,
        uint64_t logical_id
    ) noexcept
    {
        APCInitialIdentityStruct requested_identgity{};

        requested_identgity.InitialMode = initial_mode;

        if (HashIdConstructror::IsValidAPCId48(shared_id))
        {
            requested_identgity.SharedID = shared_id;
            requested_identgity.HorizontalSharedState = APCIdentityDef::DEFAULT_ASSIGNMENT;
        }
        else
        {
            requested_identgity.HorizontalSharedState = APCIdentityDef::NULL_USER_INSTRUCTION;
        }

        if (HashIdConstructror::IsValidAPCId48(logical_id))
        {
            requested_identgity.LogicalId = logical_id;
            requested_identgity.VarticalLogicState = APCIdentityDef::DEFAULT_ASSIGNMENT;
        }
        else
        {
            requested_identgity.VarticalLogicState = APCIdentityDef::NULL_USER_INSTRUCTION;
        }
        
        return requested_identgity;
        
    }

    // static constexpr bool ValidateKeyIdPairs(APCInitialIdentityStruct& identinty_struct) noexcept;
    // static constexpr bool IsIdentityRoot(APCInitialIdentityStruct& idintity_struct, FabricTableSegmentClasses hash_table) noexcept;
    
};





struct PublishResult
{
    PublishStatus ResultStatus{PublishStatus::INVALID};
    size_t Index{APCDataStructure::APC_SIZE_SENTINAL};
};




}