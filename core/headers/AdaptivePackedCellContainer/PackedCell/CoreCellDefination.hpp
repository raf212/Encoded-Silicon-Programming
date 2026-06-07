#pragma once

#include "CellExtractorAndSetter.hpp"
namespace PredictedAdaptedEncoding
{

    struct PackedCell64_t  : public PackedCellSetters
    {
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

            std::optional<size_t> SlabIndexOfPackeCell{std::nullopt};

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
                            Priority != PriorityPolicy::INFLUENCED
                        )
                        {
                            IsCellValid = false;
                            return false;                            
                        }
                    }

                    if(
                        SubClassOfModel32 == Model32Subclass::UNCLOCKED_1x8_PLUS_2x4 &&
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
            const PackedMode packed_mode = static_cast<PackedMode>(ExtractCellModeFromMETA16_U_(meta16));
            if(packed_mode != PackedMode::MODEL48 && packed_mode != PackedMode::VALUE48)
            {
                return MakeFaultyCell();
            }
            packed64_t packed_cell = (packed64_t(clockor_value48) & MaskLowNBits(CLK_B48));
            packed_cell = SetMETA16InPacked(packed_cell, meta16);
            return packed_cell;
        }

        /// @brief Can Be used to create a new valid Packed CEll ->FOR: OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER
        /// @param cell_mode If Passing PackedMode enum just Cast static_cast<ModelFamily>(PackedMode existing_mode)
        /// @param sub_class If Passing PackedMode enum just Cast static_cast<tag8_t>(Model32Subclass/Model48Subclass desired SUB CLASS)
        /// @return VALID -> Packed Cell -> OR: UINT64_MAX
        static constexpr packed64_t MakeModeledAPCValidPackedCell(
            ModelFamily cell_model,
            tag8_t sub_class = static_cast<tag8_t>(Model32Subclass::SELF_CLASS),
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::UNDEFINED,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            InternalDataTypePolicy in_cell_value_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO

        ) noexcept
        {
            return MakeInitialValidGeneralPackedCell(
                static_cast<PackedMode>(cell_model), 
                cell_locality, 
                OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER, 
                static_cast<tag8_t>(page_class),
                in_cell_value_data_type, in_cell_value, in_cell_clk16,
                cell_priority, sub_class
            );
        }

        /// @brief Can Be used to create a new valid Packed CEll ->FOR: OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC
        /// @param cell_mode If Passing PackedMode enum just Cast static_cast<ModelFamily>(PackedMode desired ModelFamily)
        /// @param sub_class If Passing PackedMode enum just Cast static_cast<tag8_t>(Model32Subclass/Model48Subclass desired SUB CLASS)
        /// @return VALID -> Packed Cell -> OR: UINT64_MAX
        static constexpr packed64_t MakeModeledFabricValidPackedCell(
            ModelFamily cell_model,
            tag8_t sub_class = static_cast<tag8_t>(Model32Subclass::SELF_CLASS),
            FabricTableSegmentClasses table_class = FabricTableSegmentClasses::GENERIC_CONTROL,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            InternalDataTypePolicy in_cell_value_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO

        ) noexcept
        {   
            return MakeInitialValidGeneralPackedCell(
                static_cast<PackedMode>(cell_model), 
                cell_locality, 
                OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC, 
                static_cast<tag8_t>(table_class),
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


        /// @brief Can Be used to create a new valid Packed CEll ->FOR: OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC
        /// @return VALID -> Packed Cell -> OR: UINT64_MAX
        static constexpr packed64_t MakeTypedFabricValidPackedCell(
            TypeFamily cell_model,
            AccessContractOfValue sub_class = AccessContractOfValue::ATOMIC_SLNAPSHOT,
            FabricTableSegmentClasses table_class = FabricTableSegmentClasses::GENERIC_CONTROL,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            InternalDataTypePolicy in_cell_value_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO
        ) noexcept
        {
            return MakeInitialValidGeneralPackedCell(
                static_cast<PackedMode>(cell_model), 
                cell_locality, 
                OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC, 
                static_cast<tag8_t>(table_class),
                in_cell_value_data_type, in_cell_value, 
                in_cell_clk16, cell_priority, 
                static_cast<tag8_t>(sub_class)
            );
        }

        /// @brief Can Be used to create a new valid Packed CEll ->FOR: OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC
        /// @return VALID -> Packed Cell -> OR: UINT64_MAX
        static constexpr packed64_t MakeTypedAPCValidPackedCell(
            TypeFamily cell_model,
            AccessContractOfValue sub_class = AccessContractOfValue::ATOMIC_SLNAPSHOT,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::UNDEFINED,
            LocalityPolicy cell_locality = LocalityPolicy::IDLE,
            InternalDataTypePolicy in_cell_value_data_type = InternalDataTypePolicy::UnsignedPCellDataType,
            PriorityPolicy cell_priority = PriorityPolicy::PRESSURE_FIRST,
            uint64_t in_cell_value = UNSIGNED_ZERO,
            clk16_t in_cell_clk16 = UNSIGNED_ZERO
        ) noexcept
        {
            return MakeInitialValidGeneralPackedCell(
                static_cast<PackedMode>(cell_model), 
                cell_locality, 
                OwnershipPolicy::NEUROMORPHIC_SPACE_TIME_FABRIC, 
                static_cast<tag8_t>(page_class),
                in_cell_value_data_type, in_cell_value, 
                in_cell_clk16, cell_priority, 
                static_cast<tag8_t>(sub_class)
            );
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



        
    private:

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


    };


}
