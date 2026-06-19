#pragma once 
#include "APCHandleDescriptorConstructor.h"

namespace PredictedAdaptedEncoding
{
    class HashTablesConstructor : public APCHandleDescriptorConstructor
    {

        // bool PublishHashKeyValueAtBucket_(
        //     size_t bucket_base,
        //     HashKeyValueDistanceTriplet& a_valid_hash_triplet,
        //     FabricTableSegmentClasses hash_table
        // )
        // {
        //     if (
        //         !CoreOfFabricCoordinator::IsValidHashTable(hash_table) ||
        //         !a_valid_hash_triplet.IsValid
        //     )
        //     {
        //         return false;
        //     }

        //     HashTableConf::SingleHashBuffer desired_hash_buffer = HashTableConf::BuildAndValidateAHashBufferFromTriplet(a_valid_hash_triplet);

        //     if (desired_hash_buffer[HashTableConf::VALIDATION_INDEX_HASH_BUFFER] != HashTableConf::VALIDATION_MARK_OF_HASH_TABLE_BUFFER)
        //     {
        //         return false;
        //     }
            
            // bool ok = ClaimThenMemCopyFromArray_(bucket_base, HASH_BUCKED_WIDTH_OF_FABRIC, desired_hash_buffer, false);
            
        // }



        // bool InsertOrUpdateRobinHoodHash48_(FabricTableSegmentClasses hash_table, uint64_t key48, uint64_t value48) noexcept
        // {
        //     if (key48 == UNSIGNED_ZERO || key48 ==  HashTableConf::HASH_TOMBSTONE_KEY)
        //     {
        //         return false;
        //     }

        //     SlabFabricTableBoundsCarrietFromRecordBookTable desired_hash_table_bounds {};

        //     bool is_valid_bounds = GetValidSlabRangeTripletFromRecordBookOfFTSC(hash_table, desired_hash_table_bounds);
        //     if (!is_valid_bounds)
        //     {
        //         return false;
        //     }
            
        //     const uint64_t bucket_count = static_cast<uint64_t>(
        //         (desired_hash_table_bounds.EndIndex - desired_hash_table_bounds.BeginIndex) / HASH_BUCKED_WIDTH_OF_FABRIC
        //     );

        //     if (bucket_count == UNSIGNED_ZERO || (bucket_count & (bucket_count -1) != UNSIGNED_ZERO))
        //     {
        //         return false;
        //     }

        //     uint64_t incoming_key = key48;
        //     uint64_t incoming_value = value48;
        //     uint64_t incoming_hash = HashTableConf::HashUnsigned48_(incoming_key);
        //     uint16_t incoming_prob = UNSIGNED_ZERO;
        //     uint64_t incoming_bucket = incoming_hash & (bucket_count - 1u);

        //     for (
        //         uint64_t steps = 0;
        //         steps < bucket_count && incoming_prob != HashTableConf::PROB_DISTANCE_SENTINAL; 
        //         steps++
        //     )
        //     {
        //         const size_t base_idx = desired_hash_table_bounds.BeginIndex + static_cast<size_t>(incoming_bucket) * HASH_BUCKED_WIDTH_OF_FABRIC;
        //         HashKeyValueDistanceTriplet current_hash_data = ReadValidHashBucketTriplet(base_idx);
        //         if (
        //             !current_hash_data.IsValid ||
        //             current_hash_data.HashKey == UNSIGNED_ZERO
        //         )
        //         {
        //             return 
        //         }
                

        //     }
            
            

            
            
        // }


        std::optional<uint64_t> FindHashValue48_(FabricTableSegmentClasses hash_table, uint64_t key48) noexcept;


    public:
        /// @brief Takes Base Bucket Index -> GATHER: 3 Cells -> CALLS: HashTableConf::ReadKeyValueProbFromValidCells
        /// @param bucked_base_index First Index In Slab For That Hash SIMPLY: HashTableInternalIndexing::KEY_INDEX OF: ANY: Hash Table
        /// @param caller_holds_Claim_guard IF: FALSE: Claimed Cell is Invalid & ONLY: -> SET: -> TRUE: When Caller Is the One Claimed The Cell 
        /// @return VALID: HashKeyValueDistanceTriplet.IsValid -> true || INVALID: HashKeyValueDistanceTriplet.IsValid = false
        HashKeyValueDistanceTriplet ReadValidHashBucketTriplet(size_t bucked_base_index, bool caller_holds_Claim_guard) noexcept;

    };

}