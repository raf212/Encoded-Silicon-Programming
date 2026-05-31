#pragma once

#include <type_traits>
#include <cstring>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <optional>
#include <vector>
#include <algorithm>
#include <limits>
#include <cassert>
#include <cstddef>
#include <array>
#include <thread>
#include <stdexcept>
#include <memory>
#include <bit>
#include "Defination.h"

#if defined(_MSC_VER)
    #include <intrin.h>
#endif
// META16 / PNLTCOD:
// [ priority:3 | node_authority:2 | locality:2 | cell_mode:1 | relmask:4 | reloffset:2 | dtype:2 ]
// shifts:
// priority=13, node_authority=11, locality=9, cell_mode=8, relmask=4, reloffset=2, dtype=0


namespace PredictedAdaptedEncoding {
    using packed64_t = uint64_t;
    using val32_t    = uint32_t;
    using clk16_t    = uint16_t;
    using tag8_t     = uint8_t;
    using meta16_t   = uint16_t;



    static constexpr ::std::memory_order MoLoad_      = ::std::memory_order_acquire;
    static constexpr ::std::memory_order MoStoreSeq_  = ::std::memory_order_release;
    static constexpr ::std::memory_order MoStoreUnSeq_= ::std::memory_order_relaxed;
    static constexpr ::std::memory_order OnExchangeSuccess   = ::std::memory_order_acq_rel;
    static constexpr ::std::memory_order OnExchangeFailure   = ::std::memory_order_relaxed;

    static constexpr unsigned CLK_B48 = 48u;
    static constexpr unsigned VALBITS  = 32u;
    static constexpr unsigned CLK_B16  = 16u;
    static constexpr unsigned META16_B16  = 16u;
    static constexpr unsigned STBITS   = 8u;
    static constexpr unsigned TOTAL_LOW = 48u;

    static constexpr unsigned PRIO_LEN = 3u;
    static constexpr unsigned NODE_AUTH_LEN = 2u;
    static constexpr unsigned LOCALITY_LEN = 2u;// will be 2u
    static constexpr unsigned PCTYPE_LEN = 1u;
    static constexpr unsigned RELMASK_LEN = 4u;
    static constexpr unsigned RELOFFSET_LEN = 2u;
    static constexpr unsigned PCELL_DATATYPE_LEN = 2u;

    //shifts 
    static constexpr unsigned PCELL_DETATYPE_SHIFT = 0u;
    static constexpr unsigned RELOFFSET_SHIFT = PCELL_DETATYPE_SHIFT + PCELL_DATATYPE_LEN;
    static constexpr unsigned RELMASK_SHIFT = RELOFFSET_SHIFT + RELOFFSET_LEN;
    static constexpr unsigned CELL_MODE_SHIFT = RELMASK_SHIFT + RELMASK_LEN;
    static constexpr unsigned LOCALITY_SHIFT = CELL_MODE_SHIFT + PCTYPE_LEN;
    static constexpr unsigned NODE_AUTH_SHIFT = LOCALITY_SHIFT + LOCALITY_LEN;
    static constexpr unsigned PRIORITY_SHIFT = NODE_AUTH_SHIFT + NODE_AUTH_LEN;
    static_assert(PRIORITY_SHIFT + PRIO_LEN == META16_B16, "PNLTCOD must be 16 bits");
    //mask
    static constexpr tag8_t PCELL_DATATYPE_MASK = static_cast<tag8_t>((1u << PCELL_DATATYPE_LEN) - 1u);
    static constexpr tag8_t RELOFFSET_MASK = static_cast<tag8_t>((1u << RELOFFSET_LEN) - 1u);
    static constexpr tag8_t RELMASK_MASK = static_cast<tag8_t>((1u << RELMASK_LEN) - 1u);
    static constexpr tag8_t CELL_MODE_MASK = static_cast<tag8_t>((1u << PCTYPE_LEN) - 1u);
    static constexpr tag8_t LOCALITY_MASK = static_cast<tag8_t>((1u << LOCALITY_LEN) - 1u);
    static constexpr tag8_t NODE_AUTH_MASK = static_cast<tag8_t>((1u << NODE_AUTH_LEN) - 1u);
    static constexpr tag8_t PRIORITY_MASK = static_cast<tag8_t>((1u << PRIO_LEN) - 1u);
    
    static constexpr uint8_t MAX_PRIORITY   = static_cast<tag8_t>(PRIORITY_MASK);

    enum class PackedCellLocalityTypes : tag8_t
    {
        IDLE = 0,
        PUBLISHED = 1,
        CLAIMED = 2,
        FAULTY = 3
    };

    enum class PackedCellOwnership : tag8_t
    {
        ADAPTIVE_PACKED_CELL_CONTAINER = 0,
        NEUROMORPHIC_SPACE_TIME_FABRIC = 1,
        RESERVED_2 = 2,
        RESERVED_3 = 3
    };

    enum class PackedCellDataType : tag8_t
    {
        CharPCellDataType = 0,
        IntPCellDataType = 1,
        FloatPCellDataType = 2,
        UnsignedPCellDataType = 3
    };

    enum class PackedMode : tag8_t
    {
        MODE_32 = 0,
        MODE_48 = 1
    };

    enum class SubClassesOfMode32 : tag8_t
    {
        SELF_CLASS = 0,
        LOW_OF_PAIRED_VERSIONED_CELL = 1,
        HIGH_OF_PAIRED_VERSIONED_CELL = 2,
        SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4 = 3
    };

    enum class SubClassesOfMode48 : tag8_t
    {
        SELF_CLASS = 0,
        PURE_TIMER_48 = 1,
        SUBDIVISION16x3_INTERNAL_CELL_MODEL  = 2,
        FOUR_SUBDIVISION_2x16_AND_2x8 = 3
    };

    enum class PriorityPhysics : tag8_t
    {
        IDLE = 0,
        IMPORTANT = 1,
        VERSION_DEPENDENCY = 2,
        INHERIT_SOURCE_PRIORITY = 3,
        ERROR_FIRST = 4,
        OLDEST_CLOCK_FIRST = 5,
        PRESSURE_FIRST = 6,
        MAX_OF_SOURCE_AND_TARGET = 7
    };

    enum class APCPagedNodeSegmentClasses : tag8_t
    {
        NONE = 0x0,
        FEEDFORWARD_MESSAGE  = 0x1,
        FEEDBACKWARD_MESSAGE = 0x2,
        LATERAL_MESAGE = 0x3,
        STATE_SLOT = 0x4,
        ERROR_SLOT = 0x5,
        EDGE_DESCRIPTOR = 0x6,
        WEIGHT_SLOT = 0x7,
        CONTROL_SLOT = 0x8,
        AUX_SLOT = 0x9,
        HETEROGENOUS_RAW_MEMORY = 0xA,
        SLOT_TABLE_DESCRIPTOR = 0xB,
        //paired pinter should be valid only in case of SubClassesOfMode32->Paired Subclass
        PAIRED_POINTER_IN_MEMORY = 0xC,
        FREE_SLOT     = 0xD,
        UNDEFINED = 0xE,
        FABRIC_SEGMENT_POOL     = 0xF
    };

    enum class FabricTableSegmentClasses : uint16_t //14
    {
        NONE = 0,
        GLOBAL_CONFIG = 1,
        TABLE_DIRECTORY = 2,
        SLOT_DIRECTORY = 3,
        BRANCH_HASH = 4,
        LOGICAL_HASH = 5,
        SHARED_HASH = 6,
        RELATION_TABLE = 7,
        FREE_RETIRE_TABLE = 8,
        READY_QUEUE = 9,
        WORK_QUEUE = 10,
        DEVICE_VIEW_TABLE = 11,
        THREAD_TABLE  = 12,
        SEGMENT_POOL = 13,
        COUNT = 14,
        GENERIC_CONTROL = 15
    };

    static  constexpr packed64_t MaskLowNBits(unsigned n) noexcept
    {
        if (n == UNSIGNED_ZERO) return packed64_t(0);
        if (n >= BIT_LENGTH_OF_A_PACKED_CELL) return ~packed64_t(0);
        // produce low-n ones without shifting by >= width
        return ((packed64_t(1) << n) - 1u);                  
    }

    template<typename PCDT>
    struct PackedCellTypeBridge
    {
        static_assert(std::is_trivially_copyable_v<PCDT>, "Packed Cell allowes Trivially copyable 32/48 bit unsigned, int, float, char");
        using Decayed = std::remove_cv_t<std::remove_reference_t<PCDT>>;
        static constexpr bool IS_CHAR_LIKE = std::is_same_v<Decayed, char> || std::is_same_v<Decayed, signed char> || std::is_same_v<Decayed, unsigned char>;
        static constexpr bool IS_FLOAT_LIKE = std::is_floating_point_v<Decayed>;
        static constexpr bool IS_SIGNED_LIKE = std::is_integral_v<Decayed> && std::is_signed_v<Decayed> && !IS_CHAR_LIKE;
        static constexpr bool IS_UNSIGNED_LIKE = std::is_integral_v<Decayed> && std::is_unsigned_v<Decayed> && !IS_CHAR_LIKE;


        static constexpr PackedCellDataType DType  = 
            IS_FLOAT_LIKE           ? PackedCellDataType::FloatPCellDataType    :
            IS_SIGNED_LIKE          ? PackedCellDataType::IntPCellDataType      :
            IS_UNSIGNED_LIKE        ? PackedCellDataType::UnsignedPCellDataType :
            IS_CHAR_LIKE            ? PackedCellDataType::CharPCellDataType     :
                                    PackedCellDataType::UnsignedPCellDataType   ;
        static constexpr bool FITS_MODE_32 = (sizeof(Decayed) <= sizeof(val32_t));
        static constexpr bool FITS_MODE_48 = (sizeof(Decayed) <= SIZE_OF_MODE_48);
        static constexpr bool IS_SUPPORTED_TYPE = IS_FLOAT_LIKE || IS_SIGNED_LIKE || IS_UNSIGNED_LIKE || IS_CHAR_LIKE;

    };


    template<typename PCDT>
     constexpr PackedCellDataType BridgeOfPackedCellDataType_v = PackedCellTypeBridge<PCDT>::DType;


    template <typename To, typename From>
     To BitCastMaybe(const From& from_address)
    {
        To out;
        if constexpr (sizeof(To) == sizeof(From))
        {
            return std::bit_cast<To>(from_address);
        }
        else if constexpr (sizeof(To) < sizeof(From))
        {
            std::memcpy(&out, &from_address, sizeof(To));
            return out;
        }
        else
        {
            std::memset(&out, 0, sizeof(To));
            std::memcpy(&out, &from_address, sizeof(From));
            return out;
        }
    }


}