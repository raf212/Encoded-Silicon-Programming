#pragma once
#include <functional>
#include "LayoutBoundsOrchestrator.hpp"

namespace PredictedAdaptedEncoding
{
    #define MAXIMUM_CLAIMABLE_COUNT_SEQUENTIALLY 32


    struct HeaderOrchestrator
    {

        using DefaultMemCopyBuffer = std::array<packed64_t, MAXIMUM_CLAIMABLE_COUNT_SEQUENTIALLY>;

        using APCMetaBuffer = std::array<packed64_t, APCDataStructure::METACELL_COUNT + 1>;

        static constexpr void BuildNullMemCopyBuffer(DefaultMemCopyBuffer& a_default_buffer) noexcept
        {
            for (size_t i = 0; i < MAXIMUM_CLAIMABLE_COUNT_SEQUENTIALLY; i++)
            {
                a_default_buffer[i] = PackedCell64_t::PACKED_CELL_SENTINAL;
            }
        }

        static constexpr void ConstructNullHeaderBuffer() noexcept;

        static constexpr void InitializeDefaultHeaderBuffer(APCGroupReserver::APCIdentityDef& required_identity);

        static constexpr bool SetAMetaIndeciesIntheBuffer(MetaIndexOfAPCNode meta_idx, APCMetaBuffer& a_header_buffer);

        static constexpr bool ValidateAHeaderBuffer(APCMetaBuffer& a_header_buffer) noexcept;
    };
    

}