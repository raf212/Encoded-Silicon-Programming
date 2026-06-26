
#pragma once 
#include <array>
#include <utility>
#include "CoreOfAPC.hpp"

namespace PredictedAdaptedEncoding
{

struct ContainerInvarients
{
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


};

}