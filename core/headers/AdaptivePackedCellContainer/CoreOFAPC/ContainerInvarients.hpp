
#pragma once 
#include <array>
#include <utility>
#include "CoreOfAPC.hpp"

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

    /// @brief VALIDATES THE RAW ID 
    static constexpr bool IsValidAPCId48(uint64_t value) noexcept
    {
        return value != UNSIGNED_ZERO && value < APC_META_CELL_SENTINAL;
    }

    static constexpr uint64_t GetBranchIdFromAPCSlotIdx(uint64_t apc_slot_index) noexcept
    {
        return apc_slot_index + 1u;
    }

    static constexpr uint64_t GetSlotIdxFromBranchId(uint64_t branch_id) noexcept
    {
        return branch_id - 1;
    }


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


struct PublishResult
{
    PublishStatus ResultStatus{PublishStatus::INVALID};
    size_t Index{APCDataStructure::APC_SIZE_SENTINAL};
};

}