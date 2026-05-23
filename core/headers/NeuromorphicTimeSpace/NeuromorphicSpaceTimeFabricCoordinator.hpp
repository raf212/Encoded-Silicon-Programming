#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{

    class AdaptivePackedCellContainer;
    static constexpr uint32_t APC_FABRIC_HASH_TOMBSTONE_KEY = UINT32_MAX;
    static constexpr uint32_t DEFAULT_HAS_CONST_1 = 0x7feb352du;
    static constexpr uint32_t DEFAULT_HAS_CONST_2 = 0x846ca68bu;

    static constexpr size_t FABRIC_CONTROLIO_LENGTH = 4096u;
    static constexpr size_t TABLE_ENTRY_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t HASH_BUCKED_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t SLOT_RECORD_WIDTH_OF_FABRIC = 12u;
    static constexpr size_t RELATION_WIDTH_OF_FABRIC = 8u;
    static constexpr size_t QUEUE_RECORD_WIDTH_OF_FABRIC = 2u;
    static constexpr size_t WORK_RECORD_WIDTH_OF_FABRIC = 4u;
    static constexpr size_t DEFAULT_THREAD_SLOT_OF_FABRIC = 256u;


    enum class SegmentClassOfAPCFabric : uint16_t
    {
        NONE = 0,
        META_DATA = 1,
        DIRECTORY = 2,
        SLOT_TABLE = 3,
        HASH_TABLE = 4,
        RELATION_TABLE = 5,
        QUEUE_TABLE = 6,
        DEVICE_VIEW = 7,
        SEGMENT_POOL = 8,
        RETIRED_SEGMENT_OF_FABRIC = 9,
        UNDEFINED_SEGMENT_OF_FABRIC = 10
    };

    

    
    
}
