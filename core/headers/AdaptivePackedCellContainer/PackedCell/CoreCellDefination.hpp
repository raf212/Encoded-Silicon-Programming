#pragma once

#include "MetaAndLogicDefinationOfPackedCell.h"
namespace PredictedAdaptedEncoding
{

    // when user extracting a cell it can return UINT64_MAX as a symbole of invalid extraction method for that cell.

    struct PackedCell64_t 
    {
        static constexpr uint16_t CLOCK_16_SENTINAL = UINT16_MAX;
        static constexpr uint64_t PACKED_CELL_SENTINAL = UINT64_MAX;
        static constexpr uint64_t MODE_48_MAX_UNSIGNED_LIMIT = 0xFFFFFFFFFFFF;

        static constexpr bool IsThisCellValid(packed64_t packed_cell) noexcept
        {
            const PackedCell64_t::AuthoritiveCellView requested_cell_view = PackedCell64_t::GetAuthoritiveViewsForACell(packed_cell);
            if (!requested_cell_view.IsCellValid)
            {
                return false;
            }
            return true;
        }

        struct AuthoritiveCellView
        {
            packed64_t RawCell{0};
            meta16_t  InCellMeta16{0};
            PriorityPhysics Priority{PriorityPhysics::IDLE};
            PackedCellOwnership CellOwnership{PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER};
            PackedCellLocalityTypes LocalityOfCell{PackedCellLocalityTypes::IDLE};
            PackedMode CellMode{PackedMode::MODE_32};
            APCPagedNodeSegmentClasses PageClass{APCPagedNodeSegmentClasses::NONE};
            FabricTableSegmentClasses FabricTableSegmentClass{FabricTableSegmentClasses::NONE};
            std::optional<SubClassesOfMode32> SubClassOfMode32{std::nullopt};
            std::optional<SubClassesOfMode48> RelationOffsetForMode48{std::nullopt};
            PackedCellDataType CellValueDataType{PackedCellDataType::UnsignedPCellDataType};
            std::optional<clk16_t> InCellClock16{std::nullopt};
            std::optional<uint64_t> CellClock48{std::nullopt};
            std::optional<val32_t> CellValue32{std::nullopt};
            bool IsCellValid{false};
            bool ValidatedView{false};

            bool constexpr IsThisPackedCellValidInRuntime() noexcept
            {
                ValidatedView = true;

                if (LocalityOfCell == PackedCellLocalityTypes::FAULTY)
                {
                    IsCellValid = false;
                    return false;
                }

                if (CellOwnership == PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER && (PageClass == APCPagedNodeSegmentClasses::NONE || PageClass == APCPagedNodeSegmentClasses::FABRIC_SEGMENT_POOL))
                {
                    IsCellValid = false;
                    return false;
                }

                if (CellOwnership == PackedCellOwnership::NEUROMORPHIC_SPACE_TIME_FABRIC && (PageClass == APCPagedNodeSegmentClasses::NONE))
                {
                    IsCellValid = false;
                    return false;
                }


                if (CellMode == PackedMode::MODE_32)
                {
                    if (!CellValue32)
                    {
                        IsCellValid = false;
                        return false;
                    }

                    if (SubClassOfMode32 != SubClassesOfMode32::SELF_CLASS && CellValueDataType != PackedCellDataType::UnsignedPCellDataType)
                    {
                        IsCellValid = false;
                        return false;
                    }

                    if (SubClassOfMode32 == SubClassesOfMode32::LOW_OF_PAIRED_VERSIONED_CELL || SubClassOfMode32 == SubClassesOfMode32::HIGH_OF_PAIRED_VERSIONED_CELL)
                    {
                        if (
                            (PageClass != APCPagedNodeSegmentClasses::PAIRED_POINTER_IN_MEMORY || 
                            PageClass != APCPagedNodeSegmentClasses::CONTROL_SLOT) &&
                            Priority != PriorityPhysics::VERSION_DEPENDENCY
                        )
                        {
                            IsCellValid = false;
                            return false;
                        }
                    }

                    if(
                        SubClassOfMode32 == SubClassesOfMode32::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4 &&
                        CellOwnership == PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER &&
                        PageClass != APCPagedNodeSegmentClasses::CONTROL_SLOT
                    )
                    {
                        IsCellValid = false;
                        return false;
                    }
                    
                }
                else
                {
                    if (RelationOffsetForMode48 == SubClassesOfMode48::PURE_TIMER_48 && CellValueDataType != PackedCellDataType::UnsignedPCellDataType)
                    {
                        IsCellValid = false;
                        return false;
                    }

                    if (!CellClock48)
                    {
                        IsCellValid = false;
                        return false;
                    }
                }

                IsCellValid = true;
                return true;
            }

        };

        static constexpr packed64_t MakeFaultyCell() noexcept
        {
            return MakeAnUncheckedCellBasedOnMode_(
                PackedMode::MODE_32,
                IN_CELL_VALUE_MODE32_SENTINAL,
                UINT16_MAX,
                PriorityPhysics::ERROR_FIRST,
                PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER,
                PackedCellLocalityTypes::FAULTY
            );
        }

        static constexpr packed64_t ComposeValue32u_64(val32_t in_cell_value, clk16_t clock16, meta16_t meta16) noexcept
        {
            if(static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODE_32)
            {
                return MakeFaultyCell();
            }
            packed64_t packed_cell = (packed64_t(in_cell_value) & MaskLowNBits(VALBITS));
            packed_cell = SetCLK16InPacked(packed_cell, clock16);
            packed_cell = SetMETA16InPacked(packed_cell, meta16);
            return packed_cell;
        }

        static constexpr packed64_t ComposeCLK48u_64(uint64_t clockor_value48, meta16_t meta16) noexcept
        {
            if(static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODE_48)
            {
                return MakeFaultyCell();
            }
            packed64_t packed_cell = (packed64_t(clockor_value48) & MaskLowNBits(CLK_B48));
            packed_cell = SetMETA16InPacked(packed_cell, meta16);
            return packed_cell;
        }

        static constexpr packed64_t SetMETA16InPacked(packed64_t packed_cell, meta16_t meta16) noexcept
        {
            constexpr packed64_t top_48_bit_mask = MaskLowNBits(META16_B16) << TOTAL_LOW;
            packed_cell &= ~top_48_bit_mask;
            packed_cell |= (packed64_t(meta16) & MaskLowNBits(META16_B16)) << TOTAL_LOW;
            return packed_cell;
        }


        static constexpr packed64_t MakeInitialAPCValidPackedCell(
            PackedMode cell_mode,
            PackedCellLocalityTypes cell_locality = PackedCellLocalityTypes::IDLE,
            PackedCellOwnership cell_ownership = PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::UNDEFINED,
            PackedCellDataType in_cell_value_data_type = PackedCellDataType::UnsignedPCellDataType,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO,
            PriorityPhysics cell_priority = PriorityPhysics::IDLE,
            SubClassesOfMode32 probable_mode_subclass_type_32 = SubClassesOfMode32::SELF_CLASS,
            SubClassesOfMode48 probable_mode_subclass_type_48 = SubClassesOfMode48::SELF_CLASS
        ) noexcept
        {
            const tag8_t sub_class = (cell_mode == PackedMode::MODE_32) ? 
                static_cast<tag8_t>(probable_mode_subclass_type_32) : static_cast<tag8_t>(probable_mode_subclass_type_48);
            
            return MakeInitialValidBlindPackedCell(
                cell_mode, cell_locality, cell_ownership, static_cast<tag8_t>(page_class),
                in_cell_value_data_type, in_cell_value, in_cell_clk16,
                cell_priority, sub_class
            );
        }

        static constexpr packed64_t MakeInitialFabricValidPackedCell(
            PackedMode cell_mode,
            PackedCellLocalityTypes cell_locality = PackedCellLocalityTypes::IDLE,
            FabricTableSegmentClasses fabric_table_class = FabricTableSegmentClasses::GLOBAL_AND_CONFIG,
            PackedCellDataType in_cell_value_data_type = PackedCellDataType::UnsignedPCellDataType,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO,
            PriorityPhysics cell_priority = PriorityPhysics::IDLE,
            SubClassesOfMode32 probable_mode_subclass_type_32 = SubClassesOfMode32::SELF_CLASS,
            SubClassesOfMode48 probable_mode_subclass_type_48 = SubClassesOfMode48::SELF_CLASS
        ) noexcept
        {
            const tag8_t sub_class = (cell_mode == PackedMode::MODE_32) ? 
                static_cast<tag8_t>(probable_mode_subclass_type_32) : static_cast<tag8_t>(probable_mode_subclass_type_48);
            
            return MakeInitialValidBlindPackedCell(
                cell_mode, cell_locality, 
                PackedCellOwnership::NEUROMORPHIC_SPACE_TIME_FABRIC, 
                static_cast<tag8_t>(fabric_table_class),
                in_cell_value_data_type, in_cell_value, in_cell_clk16,
                cell_priority, sub_class
            );
        }

        static constexpr packed64_t MakeInitialValidBlindPackedCell(
            PackedMode cell_mode,
            PackedCellLocalityTypes cell_locality,
            PackedCellOwnership cell_ownership,
            tag8_t page_class,
            PackedCellDataType in_cell_value_data_type,
            uint64_t in_cell_value,
            clk16_t in_cell_clk16,
            PriorityPhysics cell_priority,
            tag8_t probable_mode_subclass
        ) noexcept
        {
            const packed64_t requested_cell = MakeAnUncheckedCellBasedOnMode_(
                cell_mode, in_cell_value, in_cell_clk16,
                cell_priority, cell_ownership, cell_locality,
                page_class, probable_mode_subclass,
                in_cell_value_data_type
            );

            const AuthoritiveCellView requested_cells_view = GetAuthoritiveViewsForACell(requested_cell);

            return requested_cells_view.IsCellValid ? requested_cell : PACKED_CELL_SENTINAL;
        }

        template <typename PCDT>
        static constexpr  packed64_t ComposeTypedModeValue32Cell(PCDT value32, clk16_t clock16, meta16_t meta16) noexcept
        {
            static_assert(PackedCellTypeBridge<PCDT>::IS_SUPPORTED_TYPE, "Unsupported Cell type");
            static_assert(PackedCellTypeBridge<PCDT>::FITS_MODE_32, "Value type is too large to fite MODE_32");
            if (static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODE_32)
            {
                return MakeFaultyCell();
            }
            if (static_cast<PackedCellDataType>(ExtractValueDataTypeFromMETA16_U_(meta16))!= BridgeOfPackedCellDataType_v<PCDT> )
            {
                return MakeFaultyCell();
            }
            uint64_t value_casted_bit = BitCastMaybe<val32_t>(value32);
            return ComposeValue32u_64(value_casted_bit, clock16, meta16);
            
        }

        template <typename PCDT>
        static constexpr  packed64_t ComposeTypedModeValue48Cell(PCDT value_clock48, meta16_t meta16) noexcept
        {
            static_assert(PackedCellTypeBridge<PCDT>::IS_SUPPORTED_TYPE, "Unsupported Cell type");
            static_assert(PackedCellTypeBridge<PCDT>::FITS_MODE_48, "Value type is too large to fite MODE_32");
            if (static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODE_48)
            {
                return MakeFaultyCell();
            }
            if (static_cast<PackedCellDataType>(ExtractValueDataTypeFromMETA16_U_(meta16)) != BridgeOfPackedCellDataType_v<PCDT> )
            {
                return MakeFaultyCell();
            }
            uint64_t value_casted_bit = BitCastMaybe<uint64_t>(value_clock48);
            return ComposeCLK48u_64(value_casted_bit & MaskLowNBits(CLK_B48), meta16);
            
        }

        static constexpr  packed64_t SetCLK16InPacked(packed64_t packed_cell, clk16_t clk16)
        {
            constexpr packed64_t clk16_mask = (MaskLowNBits(CLK_B16) << VALBITS);
            packed_cell &= ~clk16_mask;
            packed_cell |= (packed64_t(clk16 & MaskLowNBits(CLK_B16)) << VALBITS);
            return packed_cell;
        }

        static constexpr meta16_t ExtractMeta16fromPackedCell(packed64_t packed_cell) noexcept
        {
            return static_cast<meta16_t>((packed_cell >> TOTAL_LOW) & MaskLowNBits(META16_B16));
        }


        static constexpr PackedMode ExtractModeOfPackedCellFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr  bool IsPackedCellVal32(packed64_t packed_cell) noexcept
        {
            if (ExtractModeOfPackedCellFromPacked(packed_cell) == PackedMode::MODE_32)
            {
                return true;
            }
            return false;
        }

        static constexpr val32_t ExtractValue32(packed64_t packed_cell) noexcept
        {
            if (!IsPackedCellVal32(packed_cell))
            {
                return IN_CELL_VALUE_MODE32_SENTINAL;
            }
            return static_cast<val32_t>(packed_cell & MaskLowNBits(VALBITS));
        }

        static constexpr clk16_t ExtractClk16(packed64_t packed_cell) noexcept
        {
            if (!IsPackedCellVal32(packed_cell))
            {
                return CLOCK_16_SENTINAL;
            }
            return static_cast<clk16_t>((packed_cell >> (VALBITS)) & MaskLowNBits(CLK_B16));
        }

        static constexpr uint64_t ExtractClk48(packed64_t packed_cell) noexcept
        {
            if (IsPackedCellVal32(packed_cell))
            {
                return PACKED_CELL_SENTINAL;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
            }
            return static_cast<uint64_t>(packed_cell & MaskLowNBits(CLK_B48));
        }

        static constexpr PriorityPhysics ExtractPriorityFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PriorityPhysics>(ExtractPriorityFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr PackedCellLocalityTypes ExtractLocalityFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PackedCellLocalityTypes>(ExtractLocalityFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr PackedCellOwnership ExtractNodeAuthorityFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PackedCellOwnership>(ExtractCellLocalNodeAuthotityFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr APCPagedNodeSegmentClasses ExtractRelMaskFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<APCPagedNodeSegmentClasses>(ExtractRelMaskFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr SubClassesOfMode32 ExtractRelOffset32FromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<SubClassesOfMode32>(ExtractRelOffsetFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr SubClassesOfMode48 ExtractRelOffset48FromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<SubClassesOfMode48>(ExtractRelOffsetFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr PackedCellDataType ExtractPCellDataTypeFromPacked(packed64_t packed_cell) noexcept
        {
            return static_cast<PackedCellDataType>(ExtractValueDataTypeFromMETA16_U_(ExtractMeta16fromPackedCell(packed_cell)));
        }

        static constexpr bool HasHigherPriorityBetweenCellA_B(packed64_t cell_A, packed64_t cell_B) noexcept
        {
            return ExtractPriorityFromPacked(cell_A) > ExtractPriorityFromPacked(cell_B);
        }


        static constexpr AuthoritiveCellView GetAuthoritiveViewsForACell(packed64_t packed_cell) noexcept
        {
            AuthoritiveCellView out_packed_cell_view{};
            out_packed_cell_view.ValidatedView = true;

            if (packed_cell == PACKED_CELL_SENTINAL)
            {
                out_packed_cell_view.RawCell = packed_cell;
                out_packed_cell_view.IsCellValid = false;
                return out_packed_cell_view;
            }
            
            const meta16_t meta16 = ExtractMeta16fromPackedCell(packed_cell);
            out_packed_cell_view.RawCell = packed_cell;
            out_packed_cell_view.InCellMeta16 = meta16;
            out_packed_cell_view.Priority = static_cast<PriorityPhysics>(ExtractPriorityFromMETA16_U_(meta16));
            out_packed_cell_view.CellOwnership =  static_cast<PackedCellOwnership>(ExtractCellLocalNodeAuthotityFromMETA16_U_(meta16));
            out_packed_cell_view.LocalityOfCell = static_cast<PackedCellLocalityTypes>(ExtractLocalityFromMETA16_U_(meta16));
            out_packed_cell_view.CellMode = static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16));
            if (IsPackedCellVal32(packed_cell))
            {
                out_packed_cell_view.SubClassOfMode32 = static_cast<SubClassesOfMode32>(ExtractRelOffsetFromMETA16_U_(meta16));
                out_packed_cell_view.InCellClock16 = ExtractClk16(packed_cell);
                out_packed_cell_view.CellValue32 = ExtractValue32(packed_cell);
                out_packed_cell_view.PageClass = static_cast<APCPagedNodeSegmentClasses>(ExtractRelMaskFromMETA16_U_(meta16));
            }
            else
            {
                out_packed_cell_view.RelationOffsetForMode48 = static_cast<SubClassesOfMode48>(ExtractRelOffsetFromMETA16_U_(meta16));
                out_packed_cell_view.CellClock48 = ExtractClk48(packed_cell);
                out_packed_cell_view.FabricTableSegmentClass = static_cast<FabricTableSegmentClasses>(ExtractRelMaskFromMETA16_U_(meta16));
            }
            out_packed_cell_view.CellValueDataType = static_cast<PackedCellDataType>(ExtractValueDataTypeFromMETA16_U_(meta16));
            out_packed_cell_view.IsThisPackedCellValidInRuntime();
            return out_packed_cell_view;      
        }

        static  constexpr meta16_t SetPriorityInMETA16(
            meta16_t meta16,
            PriorityPhysics priority
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
            PackedCellOwnership node_authority
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                NODE_AUTH_SHIFT,
                NODE_AUTH_MASK,
                static_cast<tag8_t>(node_authority)
            );
        }

        static  constexpr meta16_t SetLocalityInMETA16(
            meta16_t meta16,
            PackedCellLocalityTypes locality
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
                RELMASK_SHIFT,
                RELMASK_MASK,
                static_cast<tag8_t>(page_class)
            );
        }


        static  constexpr meta16_t SetRelOffsetInMETA16_U(
            meta16_t meta16,
            tag8_t sub_class
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                RELOFFSET_SHIFT,
                RELOFFSET_MASK,
                sub_class
            );
        }

        static  constexpr meta16_t SetRelOffset32InMETA16(
            meta16_t meta16,
            SubClassesOfMode32 rel_offset_32
        ) noexcept
        {
            if (static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODE_32)
            {
                return meta16;
            }
            meta16 = SetCellModeInMETA16(meta16, PackedMode::MODE_32);
            return SetRelOffsetInMETA16_U(
                meta16,
                static_cast<tag8_t>(rel_offset_32)
            );
        }

        static  constexpr meta16_t SetRelOffset48InMETA16(
            meta16_t meta16,
            SubClassesOfMode48 rel_offset_48
        ) noexcept
        {
            if (static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODE_48)
            {
                return meta16;
            }
            meta16 = SetCellModeInMETA16(meta16, PackedMode::MODE_48);
            return SetRelOffsetInMETA16_U(
                meta16,
                static_cast<tag8_t>(rel_offset_48)
            );
        }

        static  constexpr meta16_t SetCellDataTypeInMETA16(
            meta16_t meta16,
            PackedCellDataType cell_data_type
        ) noexcept
        {
            return SetIndicatedMetaInMeta16(
                meta16,
                PCELL_DETATYPE_SHIFT,
                PCELL_DATATYPE_MASK,
                static_cast<tag8_t>(cell_data_type)
            );
        }

        static constexpr packed64_t SetPriorityInPacked(packed64_t packed_cell, PriorityPhysics priority) noexcept
        {
            const meta16_t new_desired_meta = SetPriorityInMETA16(ExtractMeta16fromPackedCell(packed_cell), priority);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetSegmentLayoutInPacked(packed64_t packed_cell, PackedCellOwnership segment_layout) noexcept
        {
            const meta16_t new_desired_meta = SetNodeAuthorityInMETA16(ExtractMeta16fromPackedCell(packed_cell), segment_layout);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetLocalityInPacked(packed64_t packed_cell, PackedCellLocalityTypes local_state) noexcept
        {
            const meta16_t new_desired_meta = SetLocalityInMETA16(ExtractMeta16fromPackedCell(packed_cell), local_state);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }


        static constexpr packed64_t SetPageClassInPacked(packed64_t packed_cell, APCPagedNodeSegmentClasses page_class) noexcept
        {
            const meta16_t new_desired_meta = SetPageClassInMETA16(ExtractMeta16fromPackedCell(packed_cell), page_class);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetRelOffsetForMode32InPacked(packed64_t packed_cell, SubClassesOfMode32 reloffset) noexcept
        {
            const meta16_t new_desired_meta = SetRelOffset32InMETA16(ExtractMeta16fromPackedCell(packed_cell), reloffset);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetRelOffsetForMode48InPacked(packed64_t packed_cell, SubClassesOfMode48 reloffset) noexcept
        {
            const meta16_t new_desired_meta = SetRelOffset48InMETA16(ExtractMeta16fromPackedCell(packed_cell), reloffset);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static constexpr packed64_t SetPCellDataTypeInPacked(packed64_t packed_cell, PackedCellDataType cell_data_type)
        {
            const meta16_t new_desired_meta = SetCellDataTypeInMETA16(ExtractMeta16fromPackedCell(packed_cell), cell_data_type);
            return SetMETA16InPacked(packed_cell, new_desired_meta);
        }

        static  constexpr meta16_t MakeInCellMetaForMode_32t(
            PriorityPhysics priority = PriorityPhysics::IDLE, 
            PackedCellOwnership authority = PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER,
            PackedCellLocalityTypes locality = PackedCellLocalityTypes::IDLE,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::FREE_SLOT,
            SubClassesOfMode32 rel_offset_32 = SubClassesOfMode32::SELF_CLASS,
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType
        ) noexcept
        {
            return MakeInCellMetaFromUnsigned_16t_(
                static_cast<tag8_t>(priority),
                static_cast<tag8_t>(authority),
                static_cast<tag8_t>(locality),
                static_cast<tag8_t>(page_class),
                static_cast<tag8_t>(rel_offset_32),
                static_cast<tag8_t>(PackedMode::MODE_32),
                static_cast<tag8_t>(cell_data_type)
            );
        }

        static  constexpr meta16_t MakeInCellMetaForMode_48t(
            PriorityPhysics priority = PriorityPhysics::IDLE, 
            PackedCellOwnership authority = PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER,
            PackedCellLocalityTypes locality = PackedCellLocalityTypes::IDLE,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::FREE_SLOT,
            SubClassesOfMode48 rel_offset_48 = SubClassesOfMode48::SELF_CLASS,
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType
        ) noexcept
        {
            return MakeInCellMetaFromUnsigned_16t_(
                static_cast<tag8_t>(priority),
                static_cast<tag8_t>(authority),
                static_cast<tag8_t>(locality),
                static_cast<tag8_t>(page_class),
                static_cast<tag8_t>(rel_offset_48),
                static_cast<tag8_t>(PackedMode::MODE_48),
                static_cast<tag8_t>(cell_data_type)
            );
        }

        template <typename PCDT>
        static constexpr std::optional<PCDT> ExtractAnyPackedValueX(packed64_t packed_cell)
        {
            constexpr PackedCellDataType expected_dtype = BridgeOfPackedCellDataType_v<PCDT>;
            if(ExtractPCellDataTypeFromPacked(packed_cell) != expected_dtype)
            {
                return std::nullopt;
            }
            if (IsPackedCellVal32(packed_cell))
            {
                if (sizeof(PCDT) > sizeof(val32_t))
                {
                    return std::nullopt;
                }
                const val32_t value_bits32 = ExtractValue32(packed_cell);
                return BitCastMaybe<PCDT>(value_bits32);
            }

            if (sizeof(PCDT) > SIZE_OF_MODE_48)
            {
                return std::nullopt;
            }
            uint64_t value_bits_48 = ExtractClk48(packed_cell);
            return BitCastMaybe<PCDT>(value_bits_48);
        }
        
    private:

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

        static constexpr  packed64_t MakeAnUncheckedCellBasedOnMode_(
            PackedMode cell_mode,
            uint64_t cell_value = UNSIGNED_ZERO,
            clk16_t clock16 = UNSIGNED_ZERO,
            PriorityPhysics cell_priority = PriorityPhysics::IDLE,
            PackedCellOwnership node_authority = PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER,
            PackedCellLocalityTypes cell_locality = PackedCellLocalityTypes::IDLE, 
            tag8_t page_class = static_cast<tag8_t>(APCPagedNodeSegmentClasses::UNDEFINED),
            tag8_t sub_class = static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType
        ) noexcept
        {
            if (cell_mode == PackedMode::MODE_32)
            {
                const meta16_t meta16_for_mode32 = MakeInCellMetaForMode_32t(
                    cell_priority,
                    node_authority,
                    cell_locality,
                    static_cast<APCPagedNodeSegmentClasses>(page_class),
                    static_cast<SubClassesOfMode32>(sub_class),
                    cell_data_type
                );

                return ComposeValue32u_64(
                    static_cast<val32_t>(cell_value),
                    clock16,
                    meta16_for_mode32
                );
            }
            else
            {
                const meta16_t meta16_for_mode48 = MakeInCellMetaForMode_48t(
                    cell_priority,
                    node_authority,
                    cell_locality,
                    static_cast<APCPagedNodeSegmentClasses>(page_class),
                    static_cast<SubClassesOfMode48>(sub_class),
                    cell_data_type
                );

                return ComposeCLK48u_64(cell_value, meta16_for_mode48);
            }
        }

        static constexpr meta16_t MakeInCellMetaForAny_(
            PackedMode mode_of_cell ,
            PriorityPhysics priority = PriorityPhysics::IDLE, 
            PackedCellOwnership authority = PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER,
            PackedCellLocalityTypes locality = PackedCellLocalityTypes::IDLE,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::FREE_SLOT,
            tag8_t rel_offset_any = static_cast<tag8_t>(SubClassesOfMode32::SELF_CLASS),
            PackedCellDataType cell_data_type = PackedCellDataType::UnsignedPCellDataType
        ) noexcept
        {
            return MakeInCellMetaFromUnsigned_16t_(
                static_cast<tag8_t>(priority),
                static_cast<tag8_t>(authority),
                static_cast<tag8_t>(locality),
                static_cast<tag8_t>(page_class),
                static_cast<tag8_t>(rel_offset_any),
                static_cast<tag8_t>(mode_of_cell),
                static_cast<tag8_t>(cell_data_type)
            );
        }


        static constexpr meta16_t MakeInCellMetaFromUnsigned_16t_(
            tag8_t priority, tag8_t node_authority,
            tag8_t locality, tag8_t rel_mask, 
            tag8_t sub_class, tag8_t pc_type, 
            tag8_t pc_datatype
        ) noexcept
        {

            meta16_t cell_priority = static_cast<meta16_t>(static_cast<tag8_t>(priority) & PRIORITY_MASK);
            meta16_t cell_authority = static_cast<meta16_t>(static_cast<tag8_t>(node_authority) & NODE_AUTH_MASK); 
            meta16_t cell_locality = static_cast<meta16_t>(static_cast<tag8_t>(locality) & LOCALITY_MASK);
            meta16_t cell_mode = static_cast<meta16_t>(static_cast<tag8_t>(pc_type) & CELL_MODE_MASK);
            meta16_t relation_mask = static_cast<meta16_t>(rel_mask & RELMASK_MASK);
            meta16_t relation_offset = static_cast<meta16_t>(static_cast<tag8_t>(sub_class) & RELOFFSET_MASK);
            meta16_t cell_data_type = static_cast<meta16_t>(static_cast<unsigned>(pc_datatype) & PCELL_DATATYPE_MASK);

            meta16_t cell_meta = static_cast<meta16_t>(
                (cell_priority  << (PRIORITY_SHIFT))
                | (cell_authority << (NODE_AUTH_SHIFT))
                | (cell_locality << LOCALITY_SHIFT)
                | (cell_mode << CELL_MODE_SHIFT)
                | (relation_mask << RELMASK_SHIFT)
                | (relation_offset << RELOFFSET_SHIFT)
                | cell_data_type
            );
            return cell_meta;
        }

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
            return static_cast<tag8_t>((meta16 >> RELMASK_SHIFT) & RELMASK_MASK);
        }

        static constexpr tag8_t ExtractRelOffsetFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> RELOFFSET_SHIFT) & RELOFFSET_MASK);
        }

        static constexpr tag8_t ExtractValueDataTypeFromMETA16_U_(meta16_t meta16) noexcept
        {
            return static_cast<tag8_t>((meta16 >> PCELL_DETATYPE_SHIFT) & PCELL_DATATYPE_MASK);
        }

    };


}
