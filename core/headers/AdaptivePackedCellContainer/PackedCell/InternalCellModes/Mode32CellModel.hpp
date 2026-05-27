
#pragma once 
#include <array>
#include <utility>
#include "Mode48CellModels.hpp"

namespace PredictedAdaptedEncoding
{
    struct Clock16Subdivision1x8Plus2x4InMode32CellModel
    {
        static constexpr unsigned LOWEST_8BIT_SHIFT = 0u;
        static constexpr unsigned MID_4BIT_SHIFT = 8u;
        static constexpr unsigned HIGHIEST_4bit_SHIFT = 12u;

        static packed64_t ComposeMode32WithThreeClock16Subdivision(
            uint32_t value32,
            uint8_t lowest_8_bit_of_clock16,
            uint8_t mid_4_bit_of_clock16,
            uint8_t highiest_4_bit_of_clock16,
            meta16_t meta16
        )
        {
            const clk16_t combined_subdevision_value = Pack1x8Plus2x4InUnsigned16_(lowest_8_bit_of_clock16, mid_4_bit_of_clock16, highiest_4_bit_of_clock16);
            const packed64_t compressed_packed_cell = PackedCell64_t::ComposeValue32u_64(value32, combined_subdevision_value, meta16);
            if (!IsThisCellA32BitMetaNoClock16Mode32(compressed_packed_cell))
            {
                return PackedCell64_t::PACKED_CELL_SENTINAL;
            }
            return compressed_packed_cell;
        }

        static uint16_t Pack1x8Plus2x4InUnsigned16_(
            uint8_t lowest_8_bit,
            uint8_t mid_4_bit,
            uint8_t highiest_4_bit
        ) noexcept
        {
            return (uint16_t{lowest_8_bit} << LOWEST_8BIT_SHIFT) |
                (uint16_t{mid_4_bit} << MID_4BIT_SHIFT) |
                (uint16_t{highiest_4_bit} << HIGHIEST_4bit_SHIFT);
        }

        static uint8_t ExtractLowest8Bit_(uint16_t raw16) noexcept
        {
            return static_cast<uint8_t>((raw16 >> LOWEST_8BIT_SHIFT) & Subdivision2x16Plus2x8InternalMode48CellModel::MASK_LOW_8);
        }

        static uint8_t ExtractMid4Bit_(uint16_t raw16) noexcept
        {
            return static_cast<uint8_t>((raw16 >>MID_4BIT_SHIFT) & Subdivision2x16Plus2x8InternalMode48CellModel::MASK_LOW_4);
        }

        static uint8_t ExtractHighiest4Bit_(uint16_t raw16) noexcept
        {
            return static_cast<uint8_t>((raw16 >>HIGHIEST_4bit_SHIFT) & Subdivision2x16Plus2x8InternalMode48CellModel::MASK_LOW_4);
        }

        static bool IsThisCellA32BitMetaNoClock16Mode32(packed64_t packed_cell) noexcept
        {
            const PackedCell64_t::AuthoritiveCellView this_cell_auth_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
            return this_cell_auth_view.CellMode == PackedMode::VALUE32 &&
                this_cell_auth_view.RelationOffsetForMode32.has_value() &&
                this_cell_auth_view.RelationOffsetForMode32.value() == RelOffsetMode32::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4 &&
                this_cell_auth_view.CellValueDataType == PackedCellDataType::UnsignedPCellDataType &&
                this_cell_auth_view.LocalityOfCell != PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY;
        }

    };



}
