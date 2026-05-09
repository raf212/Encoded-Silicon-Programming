
#pragma once 
#include <array>
#include <utility>
#include "../PackedCell/CoreCellDefination.hpp"

namespace PredictedAdaptedEncoding
{
    enum class MetaIndexOfAPCNode : size_t
    {
        // identity
        MAGIC_ID = 0,
        MANAGER_CONTROL_FLAGS = 1,
        SEGMENT_CONF_FLAGS = 2,

        // logical-node identity
        BRANCH_ID = 3,
        LOGICAL_NODE_ID = 4,
        SHARED_ID = 5,
        SHARED_PREVIOUS_ID = 6,
        SHARED_NEXT_ID = 7,

        // runtime-control
        BRANCH_DEPTH = 8,
        MAX_DEPTH = 9,
        BRANCH_PRIORITY = 10,
        CURRENT_ACTIVE_THREADS = 11,
        SPLIT_THRESHOLD_PERCENTAGE = 12,
        SEGMENT_KIND = 13,

        // payload / capacity
        TOTAL_CAPACITY_OF_THIS_SEGEMENT = 14,

        // timing
        LOCAL_CLOCK48 = 15,
        LAST_SPLIT_EPOCH = 16,

        // region summary
        REGION_DIR_COUNT = 17,
        REGION_SIZE = 18,
        REGION_COUNT = 19,
        PAGED_NODE_READY_BIT = 20,
        PRODUCER_BLOCK_SIZE = 21,
        BACKGROUND_EPOCH_ADVANCE_MS = 22,
        DEFINED_MODE_OF_CURRENT_APC = 23,
        RETIRE_BRANCH_THRASHOLD = 24,
        PRODUCER_CURSOR_PLACEMENT = 25,
        CONSUMER_CURSORE_PLACEMENT = 26,
        CURRENTLY_OWNED = 27,
        TOTAL_CAS_FAILURE_FOR_THIS_APC_BRANCH = 28,
        NODE_GROUP_SIZE = 29,
        NODE_AUX_PARAM_U32 = 30,

        // graph ports
        FEEDFORWARD_IN_TARGET_ID = 31,
        FEEDFORWARD_OUT_TARGET_ID = 32,
        FEEDBACKWARD_IN_TARGET_ID = 33,
        FEEDBACKWARD_OUT_TARGET_ID = 34,
        LATERAL_0_TARGET_ID = 35,
        LATERAL_1_TARGET_ID = 36,
        NODE_ROLE_FLAGS_RESERVED = 37,
        LAST_ACCEPTED_FEED_FORWARD_CLOCK16 = 38,
        LAST_EMITTED_FEED_FORWARD_CLOCK16 = 39,
        LAST_ACCEPTED_FEED_BACKWARD_CLOCK16 = 40,
        LAST_EMITTED_FEED_BACKWARD_CLOCK16 = 41,
        NODE_COMPUTE_KIND = 42,

        // payload bounds versions
        MESSAGE_FEEDFORWARD_BOUNDS_VERSION = 43,
        MESSAGE_FEEDBACKWARD_BOUNDS_VERSION = 44,
        LATERAL_BOUNDS_VERSION = 45,
        STATE_BOUNDS_VERSION = 46,
        ERROR_BOUNDS_VERSION = 47,
        EDGE_DESCRIPTIOR_BOUNDS_VERSION = 48,
        WEIGHT_BOUNDS_VERSION = 49,
        AUX_BOUNDS_VERSION = 50,
        HETEROGENOUS_MEMORY_MAYBE_PAIRED_POINTER_OR_RAW_APC_SEGMENT_BOUNDS_VERSION = 51,
        PAIRED_POINTER_LOCAL_MEMORY_BOUNDS_VERSION = 52,
        PAIRED_POINTER_DISTANCE_MEMORY_BOUNDS_VERSION = 53,
        FREE_BOUNDS_VERSION = 54,
        UNDEFINED_BOUNDS_VERSION = 55,
        RESERVED_56 = 56,

        // region occupancy
        REGION_OCCUPANCY_NONE = 57,
        REGION_OCCUPANCY_FF = 58,
        REGION_OCCUPANCY_FB = 59,
        REGION_OCCUPANCY_LATERAL = 60,
        REGION_OCCUPANCY_STATE = 61,
        REGION_OCCUPANCY_ERROR = 62,
        REGION_OCCUPANCY_EDGE = 63,
        REGION_OCCUPANCY_WEIGHT = 64,
        REGION_OCCUPANCY_CONTROL = 65,
        REGION_OCCUPANCY_AUX = 66,
        REGION_OCCUPANCY_HETEROGENOUS_MEMORY_MAYBE_PAIRED_POINTER_OR_RAW_APC_SEGMENT = 67,
        REGION_OCCUPANCY_PAIRED_POINTER_LOCAL_MEMORY = 68,
        REGION_OCCUPANCY_PAIRED_POINTER_DISTANCE_MEMORY = 69,
        REGION_OCCUPANCY_FREE = 70,
        REGION_OCCUPANCY_UNDEFINED = 71,
        REGION_OCCUPANCY_NANNULL = 72,

        // retire epoch
        RETIRE_EPOCH_LOW32 = 73,
        RETIRE_EPOCH_HIGH32 = 74,

        // combined occupancy
        COMBINED_OCCUPANCY_PUBLISHED_CLAIMED_FAULTY_3x16_48 = 75,

        // edge / weight tables
        EDGE_TABLE_COUNT = 76,
        WEIGHT_TABLE_COUNT = 77,

        // amount / end marker
        EOF_APC_HEADER = 95
    };

    class APCDataStructure
    {
    public:
        static constexpr size_t METACELL_COUNT = 96;
        static constexpr uint32_t BRANCH_MAGIC = 0x41504342u;//big-endian
        static constexpr uint32_t EOF_HEADER = 0x72616600;//big-endian
        static constexpr uint32_t BRANCH_VERSION = 1u;
        static constexpr packed64_t PACKED_CELL_SENTENAL = UINT64_MAX;
        static constexpr uint32_t APC_MAX_LENGTH_OR_COUNTER = UINT16_MAX - 1;
        static constexpr uint32_t APC_INDEX_SENTINAL = UINT16_MAX;
        static constexpr uint32_t BRANCH_SENTINAL = UINT32_MAX;
        static constexpr size_t APC_CACHELINE_SIZE = 64u;

        static constexpr uint64_t MASK_LOW_16 = MaskLowNBits(16);


        static inline packed64_t Compose3Unsigned16bitIndependentInMode48(
            uint16_t low16_bits,
            uint16_t mid_16_bits,
            uint16_t high_16_bits,
            meta16_t meta16
        ) noexcept
        {
            const uint64_t raw48 = (PackUnsigned16x3ToMode48_(low16_bits, mid_16_bits, high_16_bits) & MaskLowNBits(CLK_B48));
            return PackedCell64_t::ComposeCLK48u_64(raw48, meta16);
        }

        static inline packed64_t ComposeAPCOccupancyModel_16x3_48t(
            uint16_t published_count,
            uint16_t claimed_count,
            uint16_t faulty_count,
            APCPagedNodeRelMaskClasses page_class,
            PackedCellLocalityTypes locality = PackedCellLocalityTypes::ST_PUBLISHED,
            PriorityPhysics priority = PriorityPhysics::DEFAULT_PRIORITY,
            PackedCellNodeAuthority authority = PackedCellNodeAuthority::BIDIRECTIONAL_NEUROMORPHIC_SYSTEM
        ) noexcept
        {
            const meta16_t meta16 = PackedCell64_t::MakeInCellMetaForMode_48t(
                priority, 
                authority,
                locality,
                page_class,
                RelOffsetMode48::THREE_16_BIT_SUB_DIVISION,
                PackedCellDataType::UnsignedPCellDataType
            );
            return Compose3Unsigned16bitIndependentInMode48(
                published_count,
                claimed_count,
                faulty_count,
                meta16
            );
        }

        static inline std::optional<uint16_t>GetOccuupancyFromPackedCellMode48(
            packed64_t packed_cell,
            PackedCellLocalityTypes desired_occupancy_bucket,
            uint16_t physical_capacity
        ) noexcept
        {
            if (!IsThisCellASubdevision_3x16_48t(packed_cell))
            {
                return std::nullopt;
            }
            const uint64_t raw48 = PackedCell64_t::ExtractClk48(packed_cell);
            if (raw48 == PackedCell64_t::PACKED_CELL_SENTINAL)
            {
                return std::nullopt;
            }
            
            switch (desired_occupancy_bucket)
            {
            case PackedCellLocalityTypes::ST_PUBLISHED :
                return ExtractLow16FromUnsigned48_(raw48);
            case PackedCellLocalityTypes::ST_CLAIMED :
                return ExtractMid16FromUnsigned48_(raw48);
            case PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY :
                return ExtractHigh16FromUnsigned48_(raw48);
            case PackedCellLocalityTypes::ST_IDLE :
                return DerivedIdleFromPackedCell48(packed_cell, physical_capacity);
            default:
                return std::nullopt;
            }
        }

        static inline uint16_t GetTootalOccupancyFromPackedCell(packed64_t packed_cell) noexcept
        {
            if (!IsThisCellASubdevision_3x16_48t(packed_cell))
            {
                return UNSIGNED_ZERO;
            }
            const uint64_t raw48 = PackedCell64_t::ExtractClk48(packed_cell);
            uint16_t published, claimed, faulty = UNSIGNED_ZERO;
            ExtractLowMidHighFromMode48_(raw48, published, claimed, faulty);
            return published + claimed + faulty;
        }

        static inline packed64_t ComposeLayoutModelof16x3(
            uint16_t begin_low,
            uint16_t end_mid,
            uint16_t version_high,
            APCPagedNodeRelMaskClasses page_class,
            PackedCellLocalityTypes locality = PackedCellLocalityTypes::ST_PUBLISHED,
            PriorityPhysics priority = PriorityPhysics::DEFAULT_PRIORITY,
            PackedCellNodeAuthority authority = PackedCellNodeAuthority::BIDIRECTIONAL_NEUROMORPHIC_SYSTEM
        ) noexcept
        {
            const meta16_t meta16 = PackedCell64_t::MakeInCellMetaForMode_48t(priority, authority, locality, page_class, RelOffsetMode48::THREE_16_BIT_SUB_DIVISION, PackedCellDataType::UnsignedPCellDataType);
            return Compose3Unsigned16bitIndependentInMode48(begin_low, end_mid, version_high, meta16);
        }

        static inline bool ExtractLayoutModel_BegainL_EndM_VersionH(packed64_t packed_cell, uint16_t& begin_index, uint16_t& end_index, uint16_t& version_count) noexcept
        {
            if (!IsThisCellASubdevision_3x16_48t(packed_cell))
            {
                return false;
            }

            const uint64_t raw48 = PackedCell64_t::ExtractClk48(packed_cell);
            
            return ExtractLowMidHighFromMode48_(raw48, begin_index, end_index, version_count);
        }


    protected:
        static constexpr unsigned PACK3XU16TOMODE48_SHIFT_LOW = 0u;
        static constexpr unsigned PACK3XU16TOMODE48_SHIFT_MID = 16u;
        static constexpr unsigned PACK3XU16TOMODE48_SHIFT_HIGH = 32u;
        
        static inline bool DoseU32FitsInU16_(uint32_t value) noexcept
        {
            return value <= APC_MAX_LENGTH_OR_COUNTER;
        }

        static inline uint64_t PackUnsigned16x3ToMode48_(
            uint16_t low16_bits,
            uint16_t mid_16_bits,
            uint16_t high_16_bits
        ) noexcept
        {
            return (uint64_t{low16_bits} << PACK3XU16TOMODE48_SHIFT_LOW)    |
                (uint64_t{mid_16_bits} << PACK3XU16TOMODE48_SHIFT_MID)      |
                (uint64_t(high_16_bits) << PACK3XU16TOMODE48_SHIFT_HIGH);
        }

        static inline uint16_t ExtractLow16FromUnsigned48_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> PACK3XU16TOMODE48_SHIFT_LOW) & MASK_LOW_16);
        }

        static inline uint16_t ExtractMid16FromUnsigned48_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> PACK3XU16TOMODE48_SHIFT_MID) & MASK_LOW_16);
        }

        static inline uint16_t ExtractHigh16FromUnsigned48_(uint64_t raw48) noexcept
        {
            return static_cast<uint16_t>((raw48 >> PACK3XU16TOMODE48_SHIFT_HIGH) & MASK_LOW_16);
        }

        static inline uint32_t SumOf3PartOccupancyOf48Bit_(uint64_t raw48) noexcept
        {
            return ExtractLow16FromUnsigned48_(raw48) + ExtractMid16FromUnsigned48_(raw48) + ExtractHigh16FromUnsigned48_(raw48);
        }

        static inline uint16_t DeriveIdleCoundtFromRaw48General_(uint64_t raw48, uint16_t physical_capacity) noexcept
        {
            const  uint32_t in_use_potion = SumOf3PartOccupancyOf48Bit_(raw48);
            return in_use_potion > physical_capacity ? UNSIGNED_ZERO : static_cast<uint16_t>(physical_capacity - in_use_potion);
        }

        static inline uint16_t DerivedIdleFromPackedCell48(packed64_t packed_cell, uint16_t physical_capacity) noexcept
        {
            const uint64_t raw48 = PackedCell64_t::ExtractClk48(packed_cell);
            if (raw48 == PackedCell64_t::PACKED_CELL_SENTINAL)
            {
                return UNSIGNED_ZERO;
            }
            return DeriveIdleCoundtFromRaw48General_(raw48, physical_capacity);
        }

        static inline bool IsThisCellASubdevision_3x16_48t(packed64_t packed_cell) noexcept
        {
            return PackedCell64_t::ExtractModeOfPackedCellFromPacked(packed_cell) == PackedMode::MODE_CLKVAL48 &&
                PackedCell64_t::ExtractRelOffset48FromPacked(packed_cell) == RelOffsetMode48::THREE_16_BIT_SUB_DIVISION &&
                PackedCell64_t::ExtractLocalityFromPacked(packed_cell) != PackedCellLocalityTypes::ST_EXCEPTION_BIT_FAULTY;
        }

        static inline bool ExtractLowMidHighFromMode48_(uint64_t raw48, uint16_t& low, uint16_t& mid, uint16_t& high)
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

        static std::atomic<packed64_t>* AllocateAlignedAtomicCells_(size_t count)
        {
            const size_t bytes = sizeof(std::atomic<packed64_t>) * count;
            void* raw_ptr = ::operator new[](
                bytes,
                std::align_val_t{APC_CACHELINE_SIZE}
            );
            auto* cells_ptr = static_cast<std::atomic<packed64_t>*>(raw_ptr);
            for (size_t i = 0; i < count; i++)
            {
                new(&cells_ptr[i]) std::atomic<packed64_t>{};
            }
            return cells_ptr;
        }

        static void FreeAlignedAtomicCells_(
            std::atomic<packed64_t>* backing_ptr,
            size_t count
        ) noexcept
        {
            if (!backing_ptr)
            {
                return;
            }
            for (size_t i = 0; i < count; i++)
            {
                backing_ptr[i].~atomic<packed64_t>();
            }
            ::operator delete[](
                static_cast<void*>(backing_ptr),
                std ::align_val_t{APC_CACHELINE_SIZE}
            );
        }


    };
    
}
