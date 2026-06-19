#pragma once 
#include "APCHandleDescriptorConstructor.h"

namespace PredictedAdaptedEncoding
{
    class HashTablesConstructor : public APCHandleDescriptorConstructor
    {

        bool PublishHashKeyValueAtBucket_(
            size_t bucket_base,
            HashKeyValueDistanceTriplet& a_valid_hash_triplet,
            FabricTableSegmentClasses hash_table
        )
        {
            if (
                !CoreOfFabricCoordinator::IsValidHashTable(hash_table) ||
                !a_valid_hash_triplet.IsValid
            )
            {
                return false;
            }

            HashTableConf::SingleHashBuffer desired_hash_buffer = HashTableConf::BuildAndValidateAHashBufferFromTriplet(a_valid_hash_triplet);

            if (desired_hash_buffer[HashTableConf::VALIDATION_INDEX_HASH_BUFFER] != HashTableConf::VALIDATION_MARK_OF_HASH_TABLE_BUFFER)
            {
                return false;
            }
            
            return ClaimThenMemCopyFromArray_(bucket_base, HASH_BUCKED_WIDTH_OF_FABRIC, desired_hash_buffer);
            
        }


        std::optional<uint64_t> FindHashValue48_(FabricTableSegmentClasses hash_table, uint64_t key48) noexcept;


    public:
        /// @brief Takes Base Bucket Index -> GATHER: 3 Cells -> CALLS: HashTableConf::ReadKeyValueProbFromValidCells
        /// @param bucked_base_index First Index In Slab For That Hash SIMPLY: HashTableInternalIndexing::KEY_INDEX OF: ANY: Hash Table
        /// @param caller_holds_Claim_guard IF: FALSE: Claimed Cell is Invalid & ONLY: -> SET: -> TRUE: When Caller Is the One Claimed The Cell 
        /// @return VALID: HashKeyValueDistanceTriplet.IsValid -> true || INVALID: HashKeyValueDistanceTriplet.IsValid = false
        HashKeyValueDistanceTriplet ReadValidHashBucketTriplet(size_t bucked_base_index, bool caller_holds_Claim_guard) noexcept;

    };

}