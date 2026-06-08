
#pragma once 
namespace PredictedAdaptedEncoding
{
    //packedCell
    #define ID_HASH_GOLDEN_CONST 0x9E3779B97F4A7C15ull 
    #define DEFAULT_PAIRED_HEAD_HALF_PRIORITY 10u
    #define SIZE_OF_MODE_48 6u // 6 * 8 = 48
    //runtime
    #define MIN_PRODUCER_BLOCK_SIZE 96
    #define MIN_REGION_SIZE 4
    #define MIN_RETIRE_BATCH_THRESHOLD 16
    #define MIN_BACKGROUND_EPOCH_MS 50
    #define INITIAL_BRANCH_SPLIT_THRESHOLD_PERCENTAGE 70
    #define MAX_BRANCH_DEPTH 10
    //default Rel Class percentage
    #define FEEDFOEWARD_PERCENTAGE 8u
    #define FEEDBACKWARD_PERCENTAGE 6u
    #define STATESLOT_PERCENTAGE 8u
    #define ERRORSLOT_PERCENTAGE 6u
    #define EDGEDESCRIPTOR_PERCENTAGE 7u
    #define WEIGHTSLOT_PERCENTAGE 7u
    #define AUXSLOT_PERCENTAGE 3u
    #define FREE_PERCENTAGE 55u
    //masterclock
    #define HALF16Bit_THRESHOLD_WRAP 0x8000u
    #define MIN_TIMER_DOWNSHIFT 6
    #define MAX_TIMER_DOWNSHIFT 14
    #define A_BILLION 1000000000ull
    #define THRESHHOLD_64BIT 1e-12
    //AtomicAdaptiveBackoff
    #define BURNCYCLE_THRESHOLD 4
    #define PAUSE_THRESHOLD 16
    #define YIELD_THRESHOLD 48
    #define MINIMUM_SLEEP_THRASHOLD_US 8
    //manager
    #define BIT_PATTERN_THREAD_TOKEN_GENERATOR 0xA5A5A5A5u

    static constexpr size_t SIZE_OF_A_PAIR =( 2 * sizeof(uint64_t));
    static constexpr size_t BIT_LENGTH_OF_A_PACKED_CELL = 64;
    static constexpr unsigned UNSIGNED_ZERO = 0u;
    static constexpr unsigned MINIMUM_BRANCH_CAPACITY = 256u;
    static constexpr uint32_t IN_CELL_VALUE_MODE32_SENTINAL = UINT32_MAX;
    
    
}