#pragma once

#include "MetaAndLogicDefinationOfPackedCell.h"
namespace PredictedAdaptedEncoding
{

    // when user extracting a cell it can return UINT64_MAX as a symbole of invalid extraction method for that cell.

    struct PackedCellExtractors
    {
        static constexpr uint16_t CLOCK_16_SENTINAL = UINT16_MAX;
        static constexpr uint64_t PACKED_CELL_SENTINAL = UINT64_MAX;

        /// @brief Make meta for ANY: OwnershipPolicy of ModelFamily::MODEL48
        /// @param page_class uint8_t :: For safer use Use like static_cast<uint8_t>(Param->enum::value)
        /// @return 
        static constexpr meta16_t MakeMeta16ForAnyOwnerAndItsClassModel_48t(
            OwnershipPolicy ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            tag8_t cell_class = static_cast<tag8_t>(APCPagedNodeSegmentClasses::FREE_SLOT),
            Model48Subclass sub_class = Model48Subclass::SELF_CLASS,
            PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST, 
            LocalityPolicy locality = LocalityPolicy::IDLE,
            InternalDataTypePolicy cell_data_type = InternalDataTypePolicy::UnsignedPCellDataType
        ) noexcept
        {
            return MakeInCellMeta_16t(
                PackedMode::MODEL48,
                locality,
                ownership,
                cell_data_type,
                static_cast<tag8_t>(cell_class),
                static_cast<tag8_t>(sub_class),
                static_cast<tag8_t>(priority)
            );
        }

        /// @brief Make meta for ANY: OwnershipPolicy of ModelFamily::MODEL32
        /// @param page_class uint8_t :: For safer use Use like static_cast<uint8_t>(Param->enum::value)
        /// @return 
        static constexpr meta16_t MakeMeta16ForAnyOwnerAndItsClassModel_32t(
            OwnershipPolicy ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            tag8_t cell_class = static_cast<tag8_t>(APCPagedNodeSegmentClasses::FREE_SLOT),
            Model32Subclass sub_class = Model32Subclass::SELF_CLASS,
            PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST, 
            LocalityPolicy locality = LocalityPolicy::IDLE,
            InternalDataTypePolicy cell_data_type = InternalDataTypePolicy::UnsignedPCellDataType
        ) noexcept
        {
            return MakeInCellMeta_16t(
                PackedMode::MODEL32,
                locality,
                ownership,
                cell_data_type,
                static_cast<tag8_t>(cell_class),
                static_cast<tag8_t>(sub_class),
                static_cast<tag8_t>(priority)
            );
        }

        static constexpr meta16_t ExtractMeta16fromPackedCell(packed64_t packed_cell) noexcept
        {
            return static_cast<meta16_t>((packed_cell >> TOTAL_LOW) & MaskLowNBits(META16_B16));
        }

        static constexpr PackedMode ExtractModeOfPackedCellFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr  bool IsPackedCellFrom32BitFamily(packed64_t packed_cell) noexcept
        {
            const PackedMode packed_mode = ExtractModeOfPackedCellFromPacked(packed_cell);

            if (packed_mode == PackedMode::MODEL32 || packed_mode == PackedMode::VALUE32)
            {
                return true;
            }
            
            return false;
        }


        /// @return uint32_t or UINT32_MAX::In wrong function call && Packed Cell == UINT64_MAX
        static constexpr val32_t ExtractModel32(packed64_t packed_cell) noexcept
        {
            if (ExtractModeOfPackedCellFromPacked(packed_cell) != PackedMode::MODEL32)
            {
                return IN_CELL_VALUE_MODE32_SENTINAL;
            }
            
            return static_cast<val32_t>(packed_cell & MaskLowNBits(VALBITS));
        }

        static constexpr clk16_t ExtractClk16(packed64_t packed_cell) noexcept
        {
            if (!IsPackedCellFrom32BitFamily(packed_cell))
            {
                return CLOCK_16_SENTINAL;
            }
            return static_cast<clk16_t>((packed_cell >> (VALBITS)) & MaskLowNBits(CLK_B16));
        }

        static constexpr uint64_t ExtractModel48(packed64_t packed_cell) noexcept
        {
            if (ExtractModeOfPackedCellFromPacked(packed_cell) != PackedMode::MODEL48)
            {
                return PACKED_CELL_SENTINAL;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
            }
            return static_cast<uint64_t>(packed_cell & MaskLowNBits(FAMILY_48_BIT_LEN));
        }

        static constexpr PriorityPolicy ExtractPriorityFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PriorityPolicy>(ExtractPriorityFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr LocalityPolicy ExtractLocalityFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<LocalityPolicy>(ExtractLocalityFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr OwnershipPolicy ExtractNodeAuthorityFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<OwnershipPolicy>(ExtractCellLocalNodeAuthotityFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr APCPagedNodeSegmentClasses ExtractRelMaskFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<APCPagedNodeSegmentClasses>(ExtractRelMaskFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr Model32Subclass ExtractRelOffset32FromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<Model32Subclass>(ExtractSubClassOrContractFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr Model48Subclass ExtractRelOffset48FromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<Model48Subclass>(ExtractSubClassOrContractFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr InternalDataTypePolicy ExtractPCellDataTypeFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<InternalDataTypePolicy>(ExtractValueDataTypeFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        template <typename PCDT>
        static constexpr std::optional<PCDT> ExtractAnyPackedValueX(packed64_t packed_cell)
        {
            constexpr InternalDataTypePolicy expected_dtype = BridgeOfPackedCellDataType_v<PCDT>;
            if(ExtractPCellDataTypeFromPacked(packed_cell) != expected_dtype)
            {
                return std::nullopt;
            }
            if (IsPackedCellFrom32BitFamily(packed_cell))
            {
                if (sizeof(PCDT) > sizeof(val32_t))
                {
                    return std::nullopt;
                }
                const val32_t value_bits32 = ExtractModel32(packed_cell);
                return BitCastMaybe<PCDT>(value_bits32);
            }

            if (sizeof(PCDT) > SIZE_OF_MODE_48)
            {
                return std::nullopt;
            }
            uint64_t value_bits_48 = ExtractModel48(packed_cell);
            return BitCastMaybe<PCDT>(value_bits_48);
        }

        /// @brief Make meta Using only HIGHEST_TRUTH: 
        /// @param class_of_cell TYPE: uint8_t :: For safer use Use like static_cast<uint8_t>(Param->enum::value)
        /// @param sub_class TYPE: uint8_t :: For safer use Use like static_cast<uint8_t>(Param->enum::value)
        /// @param priority TYPE: uint8_t :: For safer use Use like static_cast<uint8_t>(Param->enum::value)
        /// @return 
        static constexpr meta16_t MakeInCellMeta_16t(
            PackedMode mode, 
            LocalityPolicy locality, 
            OwnershipPolicy cell_ownership,
            InternalDataTypePolicy data_type,
            tag8_t class_of_cell, 
            tag8_t sub_class, 
            tag8_t priority
        ) noexcept
        {

            meta16_t cell_priority = static_cast<meta16_t>(static_cast<tag8_t>(priority) & PRIORITY_MASK);
            meta16_t cell_authority = static_cast<meta16_t>(static_cast<tag8_t>(cell_ownership) & NODE_AUTH_MASK); 
            meta16_t cell_locality = static_cast<meta16_t>(static_cast<tag8_t>(locality) & LOCALITY_MASK);
            meta16_t cell_mode = static_cast<meta16_t>(static_cast<tag8_t>(mode) & CELL_MODE_MASK);
            meta16_t cell_class = static_cast<meta16_t>(class_of_cell & CELL_CLASS_MASK);
            meta16_t cell_sub_class = static_cast<meta16_t>(static_cast<tag8_t>(sub_class) & SUBCLASS_MASK);
            meta16_t cell_data_type = static_cast<meta16_t>(static_cast<unsigned>(data_type) & CELL_INTERNAL_DATA_TYPE_MASK);

            meta16_t cell_meta = static_cast<meta16_t>(
                (cell_priority  << (PRIORITY_SHIFT))
                | (cell_authority << (NODE_AUTH_SHIFT))
                | (cell_locality << LOCALITY_SHIFT)
                | (cell_mode << CELL_MODE_SHIFT)
                | (cell_class << CELL_CLASS_SHIFT)
                | (cell_sub_class << SUBCLASS_SHIFT)
                | cell_data_type
            );
            return cell_meta;
        }


protected:

        static constexpr tag8_t ExtractPriorityFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> PRIORITY_SHIFT) & PRIORITY_MASK);
        }

        static constexpr tag8_t ExtractCellLocalNodeAuthotityFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> NODE_AUTH_SHIFT ) & NODE_AUTH_MASK);
        }
        
        static constexpr tag8_t ExtractLocalityFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> LOCALITY_SHIFT) & LOCALITY_MASK);
        }

        static constexpr tag8_t ExtractCellModeFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> CELL_MODE_SHIFT) & CELL_MODE_MASK);
        }

        static constexpr tag8_t ExtractRelMaskFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> CELL_CLASS_SHIFT) & CELL_CLASS_MASK);
        }

        static constexpr tag8_t ExtractSubClassOrContractFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> SUBCLASS_SHIFT) & SUBCLASS_MASK);
        }

        static constexpr tag8_t ExtractValueDataTypeFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> PCELL_DETATYPE_SHIFT) & CELL_INTERNAL_DATA_TYPE_MASK);
        }
    };
    
    struct PackedCellSetters : public PackedCellExtractors
    {

        static constexpr  packed64_t SetCLK16InPacked(packed64_t packed_cell, clk16_t clk16)
        {
            constexpr packed64_t clk16_mask = (MaskLowNBits(CLK_B16) << VALBITS);
            packed_cell &= ~clk16_mask;
            packed_cell |= (packed64_t(clk16 & MaskLowNBits(CLK_B16)) << VALBITS);
            return packed_cell;
        }

        static constexpr packed64_t SetPriorityInPacked(packed64_t packed_cell, PriorityPolicy priority) noexcept
        {
            const meta16_t new_desired_meta = SetPriorityInMETA16(ExtractMeta16fromPackedCell(packed_cell), priority);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetSegmentLayoutInPacked(packed64_t packed_cell, OwnershipPolicy segment_layout) noexcept
        {
            const meta16_t new_desired_meta = SetNodeAuthorityInMETA16(ExtractMeta16fromPackedCell(packed_cell), segment_layout);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetLocalityInPacked(packed64_t packed_cell, LocalityPolicy local_state) noexcept
        {
            const meta16_t new_desired_meta = SetLocalityInMETA16(ExtractMeta16fromPackedCell(packed_cell), local_state);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }


        static constexpr packed64_t SetPageClassInPacked(packed64_t packed_cell, APCPagedNodeSegmentClasses page_class) noexcept
        {
            const meta16_t new_desired_meta = SetPageClassInMETA16(ExtractMeta16fromPackedCell(packed_cell), page_class);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetSubClassForModel32InPacked(packed64_t packed_cell, Model32Subclass sub_class) noexcept
        {
            const meta16_t new_desired_meta = SetSubClassOfModeInMETA16(ExtractMeta16fromPackedCell(packed_cell), static_cast<tag8_t>(sub_class));
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetSubClassForModel48InPacked(packed64_t packed_cell, Model48Subclass sub_class) noexcept
        {
            const meta16_t new_desired_meta = SetSubClassOfModeInMETA16(ExtractMeta16fromPackedCell(packed_cell), static_cast<tag8_t>(sub_class));
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetPCellDataTypeInPacked(packed64_t packed_cell, InternalDataTypePolicy cell_data_type)
        {
            const meta16_t new_desired_meta = SetCellDataTypeInMETA16(ExtractMeta16fromPackedCell(packed_cell), cell_data_type);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

    protected:

        static constexpr packed64_t SetMETA16InPacked(packed64_t packed_cell, meta16_t meta16) noexcept
        {
            constexpr packed64_t top_48_bit_mask = MaskLowNBits(META16_B16) << TOTAL_LOW;
            packed_cell &= ~top_48_bit_mask;
            packed_cell |= (packed64_t(meta16) & MaskLowNBits(META16_B16)) << TOTAL_LOW;
            return packed_cell;
        }

        static  constexpr meta16_t SetPriorityInMETA16(
            meta16_t meta16,
            PriorityPolicy priority
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                PRIORITY_SHIFT,
                PRIORITY_MASK,
                static_cast<tag8_t>(priority)
            );
        }

        static  constexpr meta16_t SetNodeAuthorityInMETA16(
            meta16_t meta16,
            OwnershipPolicy cell_ownership
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                NODE_AUTH_SHIFT,
                NODE_AUTH_MASK,
                static_cast<tag8_t>(cell_ownership)
            );
        }

        static  constexpr meta16_t SetLocalityInMETA16(
            meta16_t meta16,
            LocalityPolicy locality
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                LOCALITY_SHIFT,
                LOCALITY_MASK,
                static_cast<tag8_t>(locality)
            );
        }

        static  constexpr meta16_t SetCellModeInMETA16(
            meta16_t meta16,
            PackedMode cell_mode
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                CELL_MODE_SHIFT,
                CELL_MODE_MASK,
                static_cast<tag8_t>(cell_mode)
            );
        }

        static  constexpr meta16_t SetPageClassInMETA16(
            meta16_t meta16,
            APCPagedNodeSegmentClasses page_class
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                CELL_CLASS_SHIFT,
                CELL_CLASS_MASK,
                static_cast<tag8_t>(page_class)
            );
        }


        static  constexpr meta16_t SetSubClassOfModeInMETA16(
            meta16_t meta16,
            tag8_t sub_class
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                SUBCLASS_SHIFT,
                SUBCLASS_MASK,
                sub_class
            );
        }

        static  constexpr meta16_t SetCellDataTypeInMETA16(
            meta16_t meta16,
            InternalDataTypePolicy cell_data_type
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                PCELL_DETATYPE_SHIFT,
                CELL_INTERNAL_DATA_TYPE_MASK,
                static_cast<tag8_t>(cell_data_type)
            );
        }
    
        static  constexpr meta16_t ClearIndicatedMeta16Field_(
            meta16_t meta16,
            unsigned shift,
            tag8_t mask
        ) noexcept
        {
            return static_cast<meta16_t>(
                meta16 & ~static_cast<meta16_t>(
                    static_cast<meta16_t>(mask) << shift
                )
            );
        }

        static  constexpr meta16_t SetIndicatedMetaInMeta16(
            meta16_t meta16,
            unsigned shift,
            tag8_t mask,
            tag8_t value
        ) noexcept
        {
            const meta16_t cleared_indicated = ClearIndicatedMeta16Field_(
                meta16, shift, mask
            );
            const meta16_t only_inserted_meta16 = static_cast<meta16_t>(
                static_cast<meta16_t>(value & mask) << shift
            );
            return static_cast<meta16_t>(cleared_indicated | only_inserted_meta16);
        }
    };
    

}
