
#pragma once 
#include <array>
#include <utility>
#include "CoreCellDefination.hpp"

namespace PredictedAdaptedEncoding
{

    struct PackedModel16x3_MODE_CLKVAL48
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
            return PackedCell64_t::ExtractModeOfPackedCellFromPacked(packed_cell) == PackedMode::MODE_CLKVAL48 &&
                PackedCell64_t::ExtractRelOffset48FromPacked(packed_cell) == RelOffsetMode48::THREE_16_BIT_SUB_DIVISION &&
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


}
