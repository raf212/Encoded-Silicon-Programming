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

            PriorityPolicy Priority{PriorityPolicy::PRESSURE_FIRST};

            OwnershipPolicy CellOwnership{OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER};

            LocalityPolicy LocalityOfCell{LocalityPolicy::IDLE};

            PackedMode CellMode{PackedMode::MODEL32};

            APCPagedNodeSegmentClasses PageClass{APCPagedNodeSegmentClasses::NONE};

            FabricTableSegmentClasses FabricTableSegmentClass{FabricTableSegmentClasses::NONE};

            std::optional<AccessContractOfValue> AccessContractOfValue{std::nullopt};

            std::optional<Model32Subclass> SubClassOfModel32{std::nullopt};

            std::optional<Model48Subclass> SubClassOfModel48{std::nullopt};

            InternalDataTypePolicy CellValueDataType{InternalDataTypePolicy::UnsignedPCellDataType};

            std::optional<clk16_t> InCellClock16{std::nullopt};

            std::optional<uint64_t> CellClock48{std::nullopt};

            std::optional<val32_t> CellValue32{std::nullopt};

            bool IsCellValid{false};
            bool ValidatedView{false};

            bool constexpr IsThisPackedCellValidInRuntime() noexcept
            {
                ValidatedView = true;

                if (LocalityOfCell == LocalityPolicy::FAULTY)
                {
                    IsCellValid = false;
                    return false;
                }

                if (
                    CellOwnership == OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER && 
                    (
                        PageClass == APCPagedNodeSegmentClasses::NONE || 
                        PageClass == APCPagedNodeSegmentClasses::NULLNAN &&
                        FabricTableSegmentClass != FabricTableSegmentClasses::NONE
                    )
                )
                {
                    IsCellValid = false;
                    return false;
                }

                if ((CellMode == PackedMode::VALUE32 || CellMode == PackedMode::VALUE48) && !AccessContractOfValue.has_value())
                {
                    IsCellValid = false;
                    return false;
                }
                

                if (
                    CellOwnership == OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC && 
                    (FabricTableSegmentClass == FabricTableSegmentClasses::NONE || PageClass != APCPagedNodeSegmentClasses::NONE)
                )
                {
                    IsCellValid = false;
                    return false;
                }

                if (CellMode == PackedMode::MODEL32)
                {
                    if (!CellValue32)
                    {
                        IsCellValid = false;
                        return false;
                    }

                    if (SubClassOfModel32 != Model32Subclass::SELF_CLASS && CellValueDataType != InternalDataTypePolicy::UnsignedPCellDataType)
                    {
                        IsCellValid = false;
                        return false;
                    }

                    if (SubClassOfModel32 == Model32Subclass::LOW_OF_PAIRED_VERSIONED_CELL || SubClassOfModel32 == Model32Subclass::HIGH_OF_PAIRED_VERSIONED_CELL)
                    {
                        if (
                            FabricTableSegmentClass == FabricTableSegmentClasses::NONE  && 
                            (   
                                PageClass != APCPagedNodeSegmentClasses::PAIRED_POINTER_IN_MEMORY || 
                                PageClass != APCPagedNodeSegmentClasses::CONTROL_SLOT
                            ) &&
                            Priority != PriorityPolicy::VERSIONED
                        )
                        {
                            IsCellValid = false;
                            return false;                            
                        }
                    }

                    if(
                        SubClassOfModel32 == Model32Subclass::SUBDEVISION_NO_CLOCK16_32BIT_META_1x8PLUS2x4 &&
                        CellOwnership == OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER &&
                        PageClass != APCPagedNodeSegmentClasses::CONTROL_SLOT
                    )
                    {
                        IsCellValid = false;
                        return false;
                    }
                    
                }
                else
                {
                    if (SubClassOfModel48 == Model48Subclass::PURE_TIMER_48 && CellValueDataType != InternalDataTypePolicy::UnsignedPCellDataType)
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

        /// @brief Should be improved
        static constexpr packed64_t MakeFaultyCell() noexcept
        {
            return MakeAnUncheckedCell_(
                PackedMode::MODEL32,
                IN_CELL_VALUE_MODE32_SENTINAL,
                UINT16_MAX,
                PriorityPolicy::ERROR_FIRST,
                OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
                LocalityPolicy::FAULTY
            );
        }

        static constexpr packed64_t Compose32BitFamilyPackedCell(val32_t in_cell_value, clk16_t clock16, meta16_t meta16) noexcept
        {
            const PackedMode packed_mode = static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16));
            if(
                packed_mode != PackedMode::MODEL32 && packed_mode != PackedMode::VALUE32 
            )
            {
                return MakeFaultyCell();
            }
            packed64_t packed_cell = (packed64_t(in_cell_value) & MaskLowNBits(VALBITS));
            packed_cell = SetCLK16InPacked(packed_cell, clock16);
            packed_cell = SetMETA16InPacked(packed_cell, meta16);
            return packed_cell;
        }

        static constexpr packed64_t Compose48BitFamilyPackedCell(uint64_t clockor_value48, meta16_t meta16) noexcept
        {
            if(static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16)) != PackedMode::MODEL48)
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
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            OwnershipPolicy cell_ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::UNDEFINED,
            InternalDataTypePolicy in_cell_value_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            Model32Subclass probable_mode_subclass_type_32 = Model32Subclass::SELF_CLASS,
            Model48Subclass probable_mode_subclass_type_48 = Model48Subclass::SELF_CLASS
        ) noexcept
        {
            const tag8_t sub_class = (cell_mode == PackedMode::MODEL32) ? 
                static_cast<tag8_t>(probable_mode_subclass_type_32) : static_cast<tag8_t>(probable_mode_subclass_type_48);
            
            return MakeInitialValidGeneralPackedCell(
                cell_mode, cell_locality, cell_ownership, static_cast<tag8_t>(page_class),
                in_cell_value_data_type, in_cell_value, in_cell_clk16,
                cell_priority, sub_class
            );
        }

        /// @brief Can be used to create Packed Cell for any MODEL family OF: HIGHEST_TRUTH -> OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC
        /// @param cell_mode 
        /// @param cell_locality 
        /// @param fabric_table_class 
        /// @param in_cell_value_data_type 
        /// @param in_cell_value 
        /// @param in_cell_clk16 
        /// @param cell_priority 
        /// @param probable_mode_subclass_type_32 
        /// @param probable_mode_subclass_type_48 
        /// @return 
        static constexpr packed64_t MakeInitialFabricValidPackedCellModel(
            PackedMode cell_mode,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            FabricTableSegmentClasses fabric_table_class = FabricTableSegmentClasses::GLOBAL_AND_CONFIG,
            InternalDataTypePolicy in_cell_value_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            Model32Subclass probable_mode_subclass_type_32 = Model32Subclass::SELF_CLASS,
            Model48Subclass probable_mode_subclass_type_48 = Model48Subclass::SELF_CLASS
        ) noexcept
        {
            const tag8_t sub_class = (cell_mode == PackedMode::MODEL32) ? 
                static_cast<tag8_t>(probable_mode_subclass_type_32) : static_cast<tag8_t>(probable_mode_subclass_type_48);
            
            return MakeInitialValidGeneralPackedCell(
                cell_mode, cell_locality, 
                OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC, 
                static_cast<tag8_t>(fabric_table_class),
                in_cell_value_data_type, in_cell_value, in_cell_clk16,
                cell_priority, sub_class
            );
        }

        /// @brief Can be used to create Packed Cell of ANY: CLASS-> APCPagedNodeSegmentClasses / FabricTableSegmentClasses && SUB-CLASS-> Model32Subclass / Model48Subclass / AccessContractOfValue
        /// @param cell_class Should derive from -> APCPagedNodeSegmentClasses / FabricTableSegmentClasses
        /// @param sub_class Should derive from -> Model32Subclass / Model48Subclass / AccessContractOfValue
        /// @return VALID -> Packed Cell -> OR: UINT64_MAX
        static constexpr packed64_t MakeInitialValidGeneralPackedCell(
            PackedMode cell_mode,
            LocalityPolicy cell_locality,
            OwnershipPolicy cell_ownership,
            tag8_t cell_class,
            InternalDataTypePolicy in_cell_value_data_type,
            uint64_t in_cell_value,
            clk16_t in_cell_clk16,
            PriorityPolicy cell_priority,
            tag8_t sub_class
        ) noexcept
        {
            const packed64_t requested_cell = MakeAnUncheckedCell_(
                cell_mode, in_cell_value, in_cell_clk16,
                cell_priority, cell_ownership, cell_locality,
                cell_class, sub_class,
                in_cell_value_data_type
            );

            const AuthoritiveCellView requested_cells_view = GetAuthoritiveViewsForACell(requested_cell);

            return requested_cells_view.IsCellValid ? requested_cell : PACKED_CELL_SENTINAL;
        }

        /// @brief Should i type restrict just for VALUE32
        template <typename PCDT>
        static constexpr  packed64_t ComposeTypedModeValue32Cell(PCDT value32, clk16_t clock16, meta16_t meta16) noexcept
        {
            static_assert(PackedCellTypeBridge<PCDT>::IS_SUPPORTED_TYPE, "Unsupported Cell type");
            static_assert(PackedCellTypeBridge<PCDT>::FITS_MODE_32, "Value type is too large to fite MODEL32");

            const PackedMode packed_mode = static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16));
            if(
                packed_mode != PackedMode::MODEL32 && packed_mode != PackedMode::VALUE32 
            )
            {
                return MakeFaultyCell();
            }

            if (static_cast<InternalDataTypePolicy>(ExtractValueDataTypeFromMETA16_U_(meta16))!= BridgeOfPackedCellDataType_v<PCDT> )
            {
                return MakeFaultyCell();
            }
            uint64_t value_casted_bit = BitCastMaybe<val32_t>(value32);
            return Compose32BitFamilyPackedCell(value_casted_bit, clock16, meta16);
            
        }

        /// @brief Should i type restrict just for VALUE48
        template <typename PCDT>
        static constexpr  packed64_t ComposeTypedModeValue48Cell(PCDT value_clock48, meta16_t meta16) noexcept
        {
            static_assert(PackedCellTypeBridge<PCDT>::IS_SUPPORTED_TYPE, "Unsupported Cell type");
            static_assert(PackedCellTypeBridge<PCDT>::FITS_MODE_48, "Value type is too large to fite MODEL48");

            const PackedMode packed_mode = static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16));
            if(
                packed_mode != PackedMode::MODEL48 && packed_mode != PackedMode::VALUE48
            )
            {
                return MakeFaultyCell();
            }

            if (static_cast<InternalDataTypePolicy>(ExtractValueDataTypeFromMETA16_U_(meta16)) != BridgeOfPackedCellDataType_v<PCDT> )
            {
                return MakeFaultyCell();
            }
            uint64_t value_casted_bit = BitCastMaybe<uint64_t>(value_clock48);
            return Compose48BitFamilyPackedCell(value_casted_bit & MaskLowNBits(CLK_B48), meta16);
            
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

        static constexpr  bool IsPackedCellFrom32BitFamily(packed64_t packed_cell) noexcept
        {
            const PackedMode packed_mode = ExtractModeOfPackedCellFromPacked(packed_cell);

            if (packed_mode == PackedMode::MODEL32 || packed_mode == PackedMode::VALUE32)
            {
                return true;
            }
            
            return false;
        }

        static constexpr val32_t ExtractValue32(packed64_t packed_cell) noexcept
        {
            if (!IsPackedCellFrom32BitFamily(packed_cell))
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

        static constexpr uint64_t ExtractClk48(packed64_t packed_cell) noexcept
        {
            if (IsPackedCellFrom32BitFamily(packed_cell))
            {
                return PACKED_CELL_SENTINAL;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
            }
            return static_cast<uint64_t>(packed_cell & MaskLowNBits(CLK_B48));
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
            out_packed_cell_view.Priority = static_cast<PriorityPolicy>(ExtractPriorityFromMETA16_U_(meta16));
            out_packed_cell_view.CellOwnership =  static_cast<OwnershipPolicy>(ExtractCellLocalNodeAuthotityFromMETA16_U_(meta16));
            out_packed_cell_view.LocalityOfCell = static_cast<LocalityPolicy>(ExtractLocalityFromMETA16_U_(meta16));
            out_packed_cell_view.CellMode = static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16));

            if (out_packed_cell_view.CellOwnership == OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER)
            {
                out_packed_cell_view.PageClass = static_cast<APCPagedNodeSegmentClasses>(ExtractRelMaskFromMETA16_U_(meta16));
            }
            else
            {
                out_packed_cell_view.FabricTableSegmentClass = static_cast<FabricTableSegmentClasses>(ExtractRelMaskFromMETA16_U_(meta16));
            }

            if (out_packed_cell_view.CellMode == PackedMode::MODEL32 || 
                out_packed_cell_view.CellMode == PackedMode::VALUE32
            )
            {
                if (out_packed_cell_view.CellMode == PackedMode::MODEL32)
                {
                    out_packed_cell_view.SubClassOfModel32 = static_cast<Model32Subclass>(ExtractSubClassOrContractFromMETA16_U_(meta16));
                }
                else
                {
                    out_packed_cell_view.AccessContractOfValue = static_cast<AccessContractOfValue>(ExtractSubClassOrContractFromMETA16_U_(meta16));
                }
                out_packed_cell_view.InCellClock16 = ExtractClk16(packed_cell);
                out_packed_cell_view.CellValue32 = ExtractValue32(packed_cell);
            }
            else
            {
                if (out_packed_cell_view.CellMode == PackedMode::MODEL48)
                {
                    out_packed_cell_view.SubClassOfModel48 = static_cast<Model48Subclass>(ExtractSubClassOrContractFromMETA16_U_(meta16));
                }
                else
                {
                    out_packed_cell_view.AccessContractOfValue = static_cast<AccessContractOfValue>(ExtractSubClassOrContractFromMETA16_U_(meta16));
                }

                out_packed_cell_view.CellClock48 = ExtractClk48(packed_cell);
            }

            out_packed_cell_view.CellValueDataType = static_cast<InternalDataTypePolicy>(ExtractValueDataTypeFromMETA16_U_(meta16));
            out_packed_cell_view.IsThisPackedCellValidInRuntime();
            return out_packed_cell_view;      
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

        /// @brief Can be used to create every type of Packed Cell
        static constexpr packed64_t MakeAnUncheckedCell_(
            PackedMode cell_mode,
            uint64_t cell_value = UNSIGNED_ZERO,
            clk16_t clock16 = UNSIGNED_ZERO,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            OwnershipPolicy cell_ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE, 
            tag8_t cell_class = UNSIGNED_ZERO,
            tag8_t sub_class = UNSIGNED_ZERO,
            InternalDataTypePolicy cell_data_type = InternalDataTypePolicy::UnsignedPCellDataType
        ) noexcept
        {
            const meta16_t desired_meta16 =  MakeInCellMeta_16t(
                cell_mode,
                cell_locality,
                cell_ownership,
                cell_data_type,
                cell_class,
                sub_class,
                static_cast<tag8_t>(cell_priority)
            );

            if (cell_mode == PackedMode::MODEL32 || cell_mode == PackedMode::VALUE32)
            {
                return Compose32BitFamilyPackedCell(
                    static_cast<val32_t>(cell_value),
                    clock16,
                    desired_meta16
                );
            }
            else
            {
                return Compose48BitFamilyPackedCell(cell_value, desired_meta16);
            }
        }
public:
        static  constexpr meta16_t MakeInCellMetaForMode_48t(
            StructureFamily48 cell_behavior = StructureFamily48::VALUE48,
            PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST, 
            OwnershipPolicy ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            LocalityPolicy locality = LocalityPolicy::IDLE,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::FREE_SLOT,
            Model48Subclass sub_class = Model48Subclass::SELF_CLASS,
            InternalDataTypePolicy cell_data_type = InternalDataTypePolicy::UnsignedPCellDataType
        ) noexcept
        {
            return MakeInCellMeta_16t(
                static_cast<PackedMode>(cell_behavior),
                locality,
                ownership,
                cell_data_type,
                static_cast<tag8_t>(page_class),
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

private:

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


}
