
#pragma once 
#include <array>
#include <utility>
#include "CoreCellDefination.hpp"

namespace PredictedAdaptedEncoding
{

    struct Subdevision16x3InternalMode48CellModel
    {
        static constexpr uint64_t MASK_LOW_16 = MaskLowNBits(16);
        static constexpr unsigned PACK3XU16TOMODE48_SHIFT_LOW = 0u;
        static constexpr unsigned PACK3XU16TOMODE48_SHIFT_MID = 16u;
        static constexpr unsigned PACK3XU16TOMODE48_SHIFT_HIGH = 32u;

        static packed64_t Compose3Unsigned16bitIndependentInMode48(
            uint16_t low16_bits,
            uint16_t mid_16_bits,
            uint16_t high_16_bits,
            meta16_t meta16
        ) noexcept
        {
            const uint64_t raw48 = (PackUnsigned16x3ToMode48_(low16_bits, mid_16_bits, high_16_bits) & MaskLowNBits(CLK_B48));
            return PackedCell64_t::ComposeCLK48u_64(raw48, meta16);
        }

        static uint64_t PackUnsigned16x3ToMode48_(
            uint16_t low16_bits,
            uint16_t mid_16_bits,
            uint16_t high_16_bits
        ) noexcept
        {
            return (uint64_t{low16_bits} << PACK3XU16TOMODE48_SHIFT_LOW)    |
                (uint64_t{mid_16_bits} << PACK3XU16TOMODE48_SHIFT_MID)      |
                (uint64_t(high_16_bits) << PACK3XU16TOMODE48_SHIFT_HIGH);
        }

        static uint16_t ExtractLow16FromUnsigned48_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> PACK3XU16TOMODE48_SHIFT_LOW) & MASK_LOW_16);
        }

        static uint16_t ExtractMid16FromUnsigned48_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> PACK3XU16TOMODE48_SHIFT_MID) & MASK_LOW_16);
        }

        static uint16_t ExtractHigh16FromUnsigned48_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> PACK3XU16TOMODE48_SHIFT_HIGH) & MASK_LOW_16);
        }

        static bool IsThisCellASubdevision_3x16_48t(packed64_t packed_cell) noexcept
        {
            return PackedCell64_t::ExtractModeOfPackedCellFromPacked(packed_cell) == PackedMode::CLOCK_OR_VALUE_48 &&
                PackedCell64_t::ExtractRelOffset48FromPacked(packed_cell) == RelOffsetMode48::SUBDIVISION16x3_INTERNAL_CELL_MODEL &&
                PackedCell64_t::ExtractLocalityFromPacked(packed_cell) != PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY;
        }

        static bool ExtractLowMidHighFromMode48_(uint64_t raw48, uint16_t& low, uint16_t& mid, uint16_t& high)
        {
            if (raw48 == PackedCell64_t::PACKED_CELL_SENTINAL)
            {
                return false;
            }
            low = ExtractLow16FromUnsigned48_(raw48);
            mid = ExtractMid16FromUnsigned48_(raw48);
            high = ExtractHigh16FromUnsigned48_(raw48);
            return true;
        }
    };


    struct Subdivision2x16Plus2x8InternalMode48CellModel
    {
        static constexpr unsigned FIRST_16BIT_SHIFT_0 = 0u;
        static constexpr unsigned SECOND_16BIT_SHIFT_1 = 16u;
        static constexpr unsigned FIRST_8BIT_SHIFT_2 = 24u;
        static constexpr unsigned SECOND_8BIT_SHIFT_3 = 32u;

        static constexpr uint64_t MASK_LOW_8 = MaskLowNBits(8);

        static packed64_t Compose4Unsigned16x2Plus8x2IndependendentInMode48(
            uint16_t lowest_16bit_0,
            uint16_t low_16bit_1,
            uint8_t high_8bit_2,
            uint8_t highest_8bit_3,
            meta16_t meta16
        ) noexcept
        {
            const uint64_t raw48 = (
                Pack2x16Plus2x8UnsignedSubdivision_(
                    lowest_16bit_0, low_16bit_1,
                    high_8bit_2, highest_8bit_3
                ) & MaskLowNBits(CLK_B48)
            );
            return PackedCell64_t::ComposeCLK48u_64(raw48, meta16);
        }

        static uint64_t Pack2x16Plus2x8UnsignedSubdivision_(
            uint16_t low16_0,
            uint16_t low16_1,
            uint8_t high8_2,
            uint8_t high8_3
        ) noexcept
        {
            return (uint64_t{low16_0} << FIRST_16BIT_SHIFT_0) |
                (uint64_t{low16_1} << SECOND_16BIT_SHIFT_1) |
                (uint64_t{high8_2} << FIRST_8BIT_SHIFT_2) |
                (uint64_t{high8_3} << SECOND_8BIT_SHIFT_3);
        }

        static uint16_t ExtractLowestFirstLow16Bit0_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> FIRST_16BIT_SHIFT_0) & Subdevision16x3InternalMode48CellModel::MASK_LOW_16);
        }

        static uint16_t ExtractSecondLow16Bit1_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> SECOND_16BIT_SHIFT_1) & Subdevision16x3InternalMode48CellModel::MASK_LOW_16);
        }

        static uint8_t ExtractHigh8Bit2_(uint64_t raw48) noexcept
        {
            return static_cast<uint8_t>((raw48 >> FIRST_8BIT_SHIFT_2) & MASK_LOW_8);
        }

        static uint8_t ExtractHighestHigh8Bit3_(uint64_t raw48) noexcept
        {
            return static_cast<uint8_t>((raw48 >> SECOND_8BIT_SHIFT_3) & MASK_LOW_8);
        }

        static bool IsThisCellAFourSubdevision_48t(packed64_t packed_cell) noexcept
        {
            return PackedCell64_t::ExtractModeOfPackedCellFromPacked(packed_cell) == PackedMode::CLOCK_OR_VALUE_48 &&
                PackedCell64_t::ExtractRelOffset48FromPacked(packed_cell) == RelOffsetMode48::FOUR_SUBDIVISION_2x16_AND_2x8 &&
                PackedCell64_t::ExtractLocalityFromPacked(packed_cell) != PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY;
        }

        static bool ExtractAll4SubdivisionInOneGoIfValid_(
            uint64_t raw48, uint16_t& lowest_16bit_0, uint16_t& low_16bit_1,
            uint8_t& high_8bit_2, uint8_t& highest_8bit_3
        ) noexcept
        {
            if (raw48 == PackedCell64_t::PACKED_CELL_SENTINAL)
            {
                return false;
            }
            lowest_16bit_0 = ExtractLowestFirstLow16Bit0_(raw48);
            low_16bit_1 = ExtractSecondLow16Bit1_(raw48);
            high_8bit_2 = ExtractHigh8Bit2_(raw48);
            highest_8bit_3 = ExtractHighestHigh8Bit3_(raw48);
            return true;
        }

    };

    struct Clock16Subdivision1x8Plus2x4InMode32CellModel
    {
        static constexpr unsigned LOWEST_8BIT_SHIFT = 0u;
        static constexpr unsigned MID_4BIT_SHIFT = 8u;
        static constexpr unsigned HIGHIEST_4bit_SHIFT = 12u;

        static constexpr uint16_t MASK_LOW_4 = 0xF;

        static packed64_t ComposeMode32WithThreeClock16Subdivision_(
            uint32_t value32,
            uint8_t lowest_8_bit_of_clock16,
            uint8_t mid_4_bit_of_clock16,
            uint8_t highiest_4_bit_of_clock16,
            meta16_t meta16
        )
        {
            const clk16_t combined_subdevision_value = Pack1x8Plus2x4InUnsigned16_(lowest_8_bit_of_clock16, mid_4_bit_of_clock16, highiest_4_bit_of_clock16);
            const packed64_t compressed_packed_cell = PackedCell64_t::ComposeValue32u_64(value32, combined_subdevision_value, meta16);
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
            return static_cast<uint8_t>((raw16 >>MID_4BIT_SHIFT) & MASK_LOW_4);
        }

        static uint8_t ExtractHighiest4Bit_(uint16_t raw16) noexcept
        {
            return static_cast<uint8_t>((raw16 >>HIGHIEST_4bit_SHIFT) & MASK_LOW_4);
        }

        static bool IsThisCellA32BitMetaNoClock16Mode32(packed64_t packed_cell) noexcept
        {
            return PackedCell64_t::ExtractModeOfPackedCellFromPacked(packed_cell) == PackedMode::VALUE32 &&
                PackedCell64_t::ExtractRelOffset32FromPacked(packed_cell) == RelOffsetMode32::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4 &&
                PackedCell64_t::ExtractLocalityFromPacked(packed_cell) != PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY;
        }

    };



}
