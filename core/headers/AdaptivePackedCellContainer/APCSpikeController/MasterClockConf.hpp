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
            return ns_count & MaskLowNBits(FAMILY_48_BIT_LEN);
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

        // packed64_t RefreshPackedCellClockOnly(
        //     packed64_t provided_packed_cell,
        //     std::optional<LocalityPolicy> override_locality = std::nullopt
        // ) noexcept;

        // std::optional<packed64_t> TouchPackedCellClockAndGetCellWithNewClock(
        //     size_t index_of_packed_cell,
        //     std::optional<LocalityPolicy> override_locality = std::nullopt
        // ) noexcept;

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
            return static_cast<clk16_t>((now_ticks48 >> TimerDownShift_) & MaskLowNBits(LOW16_BIT_LEN));
        }

         clk16_t NowClock16() const noexcept
        {
            return GetImmidiateDownShiftedClock16(NowTicks48());
        }

        std::optional<uint64_t> ReconstructCellClock16toFull48BySegmentLocalClock48(size_t index_of_packed_cell) noexcept;

         /// @brief Compose Packed Cell with CLOCK16 for -> ModelFamily::MODEL32 && OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER
         /// @return 
         packed64_t ComposeClockedModel32FroAPC(
            val32_t provided_cell_value32,
            APCPagedNodeSegmentClasses page_class = APCPagedNodeSegmentClasses::UNDEFINED,
            PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST,
            LocalityPolicy locality = LocalityPolicy::PUBLISHED,
            Model32Subclass sub_class = Model32Subclass::SELF_CLASS,
            InternalDataTypePolicy dtype = InternalDataTypePolicy::UnsignedPCellDataType,
            OwnershipPolicy ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER
        )
        {
            const clk16_t now_clock16 = NowClock16();
            const meta16_t desired_meta16 = PackedCell64_t::MakeMeta16ForAnyOwnerAndItsClassModel_32t(
                ownership,
                static_cast<tag8_t>(page_class),
                sub_class, priority, locality, dtype
            );
            return PackedCell64_t::Compose32BitFamilyPackedCell(provided_cell_value32, now_clock16, desired_meta16);
        }

         packed64_t ComposePureClockCell48(
            OwnershipPolicy ownership = OwnershipPolicy::ADAPTIVE_PACKED_CELL_CONTAINER,
            LocalityPolicy locality = LocalityPolicy::PUBLISHED,
            tag8_t cell_class = static_cast<tag8_t>(APCPagedNodeSegmentClasses::FREE_SLOT),
            PriorityPolicy priority = PriorityPolicy::PRESSURE_FIRST
        ) noexcept
        {
            const uint64_t full_clock48 = NowTicks48();

            const meta16_t meta16 = PackedCell64_t::MakeMeta16ForAnyOwnerAndItsClassModel_48t(
                ownership, cell_class, 
                Model48Subclass::PURE_TIMER_48, priority,
                locality, InternalDataTypePolicy::UnsignedPCellDataType
            );

            return PackedCell64_t::Compose48BitFamilyPackedCell(full_clock48, meta16);
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