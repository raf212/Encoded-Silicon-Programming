
#pragma once 
#include <array>
#include <utility>
#include "ConstructorsAndCarriersOfAPC.hpp"

namespace PredictedAdaptedEncoding
{

struct APCDataStructure
{

    static constexpr size_t METACELL_COUNT = 96;
    static constexpr uint32_t BRANCH_MAGIC = 0x41504342u;//big-endian
    static constexpr uint32_t EOF_HEADER = 0x72616600;//big-endian
    static constexpr uint16_t BRANCH_VERSION = 1u;
    static constexpr packed64_t PACKED_CELL_SENTENAL = UINT64_MAX;
    static constexpr packed64_t APC_META_CELL_SENTINAL = PackedCell64_t::BIT_FAMILY_48_SENTINAL;
    static constexpr uint32_t APC_ALL_INDEX_LIMIT = UINT16_MAX - 1;
    static constexpr uint32_t APC_INDEX_SENTINAL = UINT16_MAX;
    static constexpr size_t APC_CACHELINE_SIZE = 64u;
    static constexpr size_t APC_SIZE_SENTINAL = SIZE_MAX;


    static constexpr uint64_t AutoExtractDataOfAValidAPCCell(
        packed64_t packed_cell, 
        bool is_claimed_cell_valid = false,
        PackedCell64_t::AuthoritiveCellView* return_auth_view_ptr = nullptr
    ) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView desired_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
        if (!desired_auth_view.IsCellValid && desired_auth_view.CellOwnership != OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }

        if (return_auth_view_ptr)
        {
            *return_auth_view_ptr = desired_auth_view;
        }

        if (!is_claimed_cell_valid && desired_auth_view.LocalityOfCell == LocalityPolicy::CLAIMED)
        {
            return false;
        }

        switch (desired_auth_view.CellMode)
        {
        case PackedMode::VALUE32:
            return desired_auth_view.Raw32BitInCellData;

        case PackedMode::VALUE48:
            return desired_auth_view.Raw48BitInCellData;

        case PackedMode::MODEL32:
            if (desired_auth_view.SubClassOfModel32 == Model32Subclass::SELF_CLASS && desired_auth_view.Attribute == AttributePolicy::SELF_CONTAINED_DATA_OR_MODEL)
            {
                return desired_auth_view.Raw32BitInCellData;
            }
            return PackedCell64_t::PACKED_CELL_SENTINAL;

        case PackedMode::MODEL48:
                return desired_auth_view.Raw48BitInCellData;
        default:
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }
        
    }


    static constexpr packed64_t ReplaceValueInAPCTypeFamilyCell(packed64_t packed_cell, uint64_t desired_value, bool is_claimed_cell_valid = false) noexcept
    {
        const PackedCell64_t::AuthoritiveCellView desired_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
        if (!desired_cell_auth_view.IsCellValid)
        {
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }

        if (!is_claimed_cell_valid && desired_cell_auth_view.LocalityOfCell == LocalityPolicy::CLAIMED)
        {
            return false;
        }

        switch (desired_cell_auth_view.CellMode)
        {
        case PackedMode::VALUE32:
            return PackedCell64_t::Compose32BitFamilyPackedCell(static_cast<uint32_t>(desired_value) & MaskLowNBits(VALBITS), desired_cell_auth_view.InCellClock16, desired_cell_auth_view.InCellMeta16);

        case PackedMode::VALUE48:
            return PackedCell64_t::Compose48BitFamilyPackedCell(desired_cell_auth_view.Raw48BitInCellData, desired_cell_auth_view.InCellMeta16);
        default:
            return PackedCell64_t::PACKED_CELL_SENTINAL;
        }

    }

protected:
        static constexpr void FreeAlignedRawPackedCells_(packed64_t* backing_ptr) noexcept
        {
            if (!backing_ptr)
            {
                return;
            }
            ::operator delete[](static_cast<void*>(backing_ptr), std::align_val_t{APC_CACHELINE_SIZE});
        }


};


struct HashIdConstructror
{
    static constexpr uint64_t GROUP_IDX_BIT_BOUNDRY = 16u;
    static constexpr uint64_t GROUP_SEQUENTIAL_INDEX_MASK = UINT16_MAX;
    static constexpr uint64_t GROUP_PREFIX_MASK = UINT32_MAX;

    /// @brief VALIDATES THE RAW ID 
    static constexpr bool IsValidAPCId48(uint64_t value) noexcept
    {
        return value != UNSIGNED_ZERO && value < APCDataStructure::APC_META_CELL_SENTINAL;
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
        return (static_cast<uint64_t>(prefix32) << GROUP_IDX_BIT_BOUNDRY | static_cast<uint64_t>(index16));
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



struct PublishResult
{
    PublishStatus ResultStatus{PublishStatus::INVALID};
    size_t Index{APCDataStructure::APC_SIZE_SENTINAL};
};


struct Timer48
{
    static constexpr uint64_t TicksPerSec_ = A_BILLION;

    static constexpr uint64_t NowTicks() noexcept
    {
        using  cns = std::chrono::nanoseconds;
        auto d = std::chrono::steady_clock::now().time_since_epoch();
        uint64_t ns_count = static_cast<uint64_t>(std::chrono::duration_cast<cns>(d).count());
        return ns_count & MaskLowNBits(FAMILY_48_BIT_LEN);
    }

    static constexpr uint16_t NowClock16() noexcept
    {
        return static_cast<uint16_t>(NowTicks() & MaskLowNBits(LOW16_BIT_LEN));
    }
};

}