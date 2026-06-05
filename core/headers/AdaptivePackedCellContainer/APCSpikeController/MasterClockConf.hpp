#pragma once 
#include "../APCDataStructure/APCGenerics.hpp"


namespace PredictedAdaptedEncoding
{
enum class APCPagedNodeSegmentClasses : tag8_t;

class SegmentIODefinition;
class AdaptivePackedCellContainer;

    struct Timer48
    {
        uint64_t TicksPerSec_ = A_BILLION;

         uint64_t NowTicks() const noexcept
        {
            using  cns = std::chrono::nanoseconds;
            auto d = std::chrono::steady_clock::now().time_since_epoch();
            uint64_t ns_count = static_cast<uint64_t>(std::chrono::duration_cast<cns>(d).count());
            return ns_count & MaskLowNBits(CLK_B48);
        }
    };

    class MasterClockConf final
    {
    private:
        Timer48& MasterTimer48_;
        unsigned TimerDownShift_ = 10u;
        AdaptivePackedCellContainer* APCPtr_ = nullptr;
    public:
        explicit MasterClockConf(AdaptivePackedCellContainer* apc_ptr, Timer48& master_timer) noexcept :
            APCPtr_(apc_ptr), MasterTimer48_(master_timer)
        {}
        MasterClockConf(const MasterClockConf&) = delete;
        MasterClockConf& operator = (const MasterClockConf&) = delete;
        MasterClockConf(MasterClockConf&&) = delete;
        MasterClockConf& operator = (MasterClockConf&&) = delete;
        ~MasterClockConf() noexcept = default;

        packed64_t RefreshPackedCellClockOnly(
            packed64_t provided_packed_cell,
            APCPagedNodeSegmentClasses force_rel_mask = APCPagedNodeSegmentClasses::FABRIC_SEGMENT_POOL,
            std::optional<LocalityPolicy> override_locality = std::nullopt
        ) noexcept;

        std::optional<packed64_t> TouchPackedCellClockAndGetCellWithNewClock(
            size_t index_of_packed_cell,
            APCPagedNodeSegmentClasses force_rel_mask = APCPagedNodeSegmentClasses::FABRIC_SEGMENT_POOL,
            std::optional<LocalityPolicy> override_locality = std::nullopt
        ) noexcept;

        bool TouchSegmentLocalClock48HighPriority() noexcept;

        bool TryAdvanceSegmentsLastAcceptedClock(APCPagedNodeSegmentClasses desired_rel_class) noexcept;


        MasterClockConf* GetMasterClockConfPtr() noexcept
        {
            return this;
        }

         uint64_t NowTicks48() const noexcept
        {
            return MasterTimer48_.NowTicks();
        }

         clk16_t GetImmidiateDownShiftedClock16(uint64_t now_ticks48) const noexcept
        {
            return static_cast<clk16_t>((now_ticks48 >> TimerDownShift_) & MaskLowNBits(CLK_B16));
        }

         clk16_t NowClock16() const noexcept
        {
            return GetImmidiateDownShiftedClock16(NowTicks48());
        }

        std::optional<uint64_t> ReconstructCellClock16toFull48BySegmentLocalClock48(size_t index_of_packed_cell) noexcept;


         packed64_t ComposeValue32WithCurrentThreadStamp16(
            val32_t provided_cell_value32,
            APCPagedNodeSegmentClasses desired_page_class = APCPagedNodeSegmentClasses::NONE,
            PriorityPolicy desired_priority = PriorityPolicy::PRESSURE_FIRST,
            LocalityPolicy desired_locality = LocalityPolicy::PUBLISHED,
            Model32Subclass desired_reloffset = Model32Subclass::SELF_CLASS,
            InternalDataTypePolicy desired_dtype = InternalDataTypePolicy::UnsignedPCellDataType,
            OwnershipPolicy desired_node_authority = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER
        )
        {
            const clk16_t now_clock16 = NowClock16();
            const meta16_t strlfor32 = PackedCell64_t::MakeCellMetaForModel_32t(StructureFamily32::MODEL32, desired_priority, desired_node_authority, desired_locality, desired_page_class, desired_reloffset, desired_dtype);
            return PackedCell64_t::Compose32BitFamilyPackedCell(provided_cell_value32, now_clock16, strlfor32);
        }

         packed64_t ComposePureClockCell48(
            PriorityPolicy desired_priority = PriorityPolicy::PRESSURE_FIRST,
            LocalityPolicy desired_locality = LocalityPolicy::PUBLISHED
        ) noexcept
        {
            const uint64_t full_clock48 = NowTicks48();
            const meta16_t strl_for_pure48_clock = PackedCell64_t::MakeInCellMetaForMode_48t(
                StructureFamily48::MODEL48,
                desired_priority, 
                OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
                desired_locality, 
                APCPagedNodeSegmentClasses::CONTROL_SLOT,
                Model48Subclass::PURE_TIMER_48,
                InternalDataTypePolicy::UnsignedPCellDataType
            );
            return PackedCell64_t::Compose48BitFamilyPackedCell(full_clock48, strl_for_pure48_clock);
        }

         uint8_t SetAndGetTimerDownShift(unsigned down_shift_value = UNSIGNED_ZERO) noexcept
        {
            if (down_shift_value >= MIN_TIMER_DOWNSHIFT && down_shift_value <= MAX_TIMER_DOWNSHIFT)
            {
                TimerDownShift_ = down_shift_value;
            }
            return static_cast<uint8_t>(TimerDownShift_);
        }

    };
    


}