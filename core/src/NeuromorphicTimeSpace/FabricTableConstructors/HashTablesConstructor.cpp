#pragma once

#include "NeuromorphicTimeSpace/SlabToFabricConverterAndCordinator.hpp"

namespace PredictedAdaptedEncoding
{

    HashKeyValueDistanceTriplet HashTablesConstructor::ReadValidHashBucketTriplet(size_t bucked_base_index, bool caller_holds_Claim_guard) noexcept
    {
        if (!SlabBasePtr_ || bucked_base_index + HASH_BUCKED_WIDTH_OF_FABRIC >= SlabCellCount_)
        {
            const HashKeyValueDistanceTriplet invalid_value{};
            return invalid_value;
        }

        const packed64_t key_cell = AtomicallyLoadReadCompletePackedCell(bucked_base_index + static_cast<size_t>(HashTableInternalIndexing::KEY_INDEX));
        const packed64_t value_cell = AtomicallyLoadReadCompletePackedCell(bucked_base_index + static_cast<size_t>(HashTableInternalIndexing::VALUE_INDEX));
        const packed64_t prob_safty_cell = AtomicallyLoadReadCompletePackedCell(bucked_base_index + static_cast<size_t>(HashTableInternalIndexing::PROB_DISTANCE_LOCK));
        
        return HashTableConf::ReadKeyValueProbFromValidCells(
            key_cell,
            value_cell,
            prob_safty_cell,
            caller_holds_Claim_guard
        );
    }
}
