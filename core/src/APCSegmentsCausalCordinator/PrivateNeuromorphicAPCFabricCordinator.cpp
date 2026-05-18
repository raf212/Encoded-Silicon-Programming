#pragma once 
#include "APCSegmentsCausalCordinator.hpp"
#include "NeuromorphicAPCFabricCordinator.hpp"

namespace PredictedAdaptedEncoding
{

    uint32_t NeuromorphicAPCFabricCordinator::HashUnsigned32_(uint32_t given_value) noexcept
    {
        given_value ^= given_value >> CLK_B16;
        given_value *= DEFAULT_HAS_CONST_1;
        given_value ^= given_value >> (CLK_B16 - 1);
        given_value *= DEFAULT_HAS_CONST_2;
        given_value ^=  given_value >> CLK_B16;
        return given_value;
    }

    bool NeuromorphicAPCFabricCordinator::InitializeStorageFabric(
        size_t slot_cell_capacity,
        size_t slab_bytes,
        uint32_t branch_table_capacity,
        uint32_t logical_table_capacity,
        uint32_t shared_table_capacity,
        uint32_t page_class_capacity,
        uint32_t work_ring_capacity,
        uint32_t view_capacity,
        uint32_t view_cell_capacity
    ) noexcept
    {
        bool expected = false;
        if (!FabricInitialized_.compare_exchange_strong(expected, true, OnExchangeSuccess))
        {
            return true;
        }

        if (slot_cell_capacity < AdaptivePackedCellContainer::PayloadBegin())
        {
            slot_cell_capacity = MINIMUM_BRANCH_CAPACITY;
        }

        const size_t slab_cells = std::max<size_t>(
            slab_bytes / sizeof(std::atomic<packed64_t>), slot_cell_capacity
        );

        const size_t slots_of_slab_count = slab_cells / slot_cell_capacity;

        if (slots_of_slab_count == UNSIGNED_ZERO || slots_of_slab_count  > APCDataStructure::APC_INDEX_SENTINAL)
        {
            FabricInitialized_.store(false, MoStoreSeq_);
            return false;
        }

        try
        {
            FabricSlotCapacityCells_ = slot_cell_capacity;
            FabricSlotCount_ = slots_of_slab_count;
            FabricSlabCells_ = slots_of_slab_count * slot_cell_capacity;
            BranchTableCapacity_ = NextPowerOf2Unsigned32_(branch_table_capacity);
            LogicalTableCapacity_ = NextPowerOf2Unsigned32_(logical_table_capacity);
            SharedTableCapacity_ = NextPowerOf2Unsigned32_(shared_table_capacity);
            PageClassesTableCapacity_ = std::max<uint32_t>(page_class_capacity, MAX_VAL);
            WorkingTableCapacity_ = NextPowerOf2Unsigned32_(std::max<uint32_t>(work_ring_capacity, MAX_VAL));
            ActiveViewCapacity_ = std::max<uint32_t>(view_capacity, CLK_B16);
            CellInActiveViewCapacity_ = std::max<uint32_t>(view_cell_capacity , DEFAULT_ACTIVE_VIEW_CAPACITY);

            FabricCellsPtrs_.reset(new std::atomic<packed64_t>[FabricSlabCells_]);
            FabricObjectPoolPtrs_.reset(new AdaptivePackedCellContainer[FabricSlotCount_]);
            SlotTablePtrs_.reset(new SlotRecordOfAPCFabric[FabricSlotCount_]);

            BranchTablePtrs_.reset(new HashEntryOfAPC[BranchTableCapacity_]);
            LogicalTablePtrs_.reset(new HashEntryOfAPC[LogicalTableCapacity_]);
            SharedTablePtrs_.reset(new SharedChainRecordOfAPCFabric[SharedTableCapacity_]);
            PageClassesTablePtrs_.reset(new PageClassesRecordOfAPCFabric[PageClassesTableCapacity_]);
            WorkRingPtrs_.reset(new WorkRecordOfAPCFabric[WorkingTableCapacity_]);
            FabricActiveViewPtrs_.reset(new FabricViewOfAPC[CellInActiveViewCapacity_]);

            CellIndicesViewPtrs_.reset(new uint32_t[CellInActiveViewCapacity_]);
            Float32CellViewPtrs_.reset(new float[CellInActiveViewCapacity_]);
            OriginalPacked64ViewPtrs_.reset(new packed64_t[CellInActiveViewCapacity_]);
            Clock16ViewPtrs_.reset(new clk16_t[CellInActiveViewCapacity_]);
            Meta16ViewPtrs_.reset(new meta16_t[CellInActiveViewCapacity_]);

            const packed64_t idle_packed_cell32 = PackedCell64_t::MakeInitialPacked(PackedMode::MODE_VALUE32);

            for (size_t i = 0; i < FabricSlabCells_; i++)
            {
                FabricCellsPtrs_[i].store(idle_packed_cell32, MoStoreUnSeq_);
            }

            for (size_t i = 0; i < FabricSlotCount_; i++)
            {
                SlotTablePtrs_[i].ResetSlotRecord();
                SlotTablePtrs_[i].Generation.store(APCDataStructure::BRANCH_VERSION, MoStoreSeq_);
                SlotTablePtrs_[i].ObjectProbableAPCPtr.store(reinterpret_cast<uintptr_t>(&FabricObjectPoolPtrs_[i]), MoStoreSeq_);
                SlotTablePtrs_[i].NextFree.store(
                    (i + 1u < FabricSlotCount_) ? i + 1u : APCDataStructure::APC_SIZE_SENTINAL,
                    MoStoreSeq_
                );
            }

            FreeSlotHeadOfFabric_.store(UNSIGNED_ZERO, MoStoreSeq_);

            for (uint32_t pc = 0; pc < PageClassesTableCapacity_; pc++)
            {
                PageClassesTablePtrs_[pc].ResetPageClassRecord();
                PageClassesTablePtrs_[pc].FreeNextRelation.store(
                    (pc + 1u < PageClassesTableCapacity_) ? pc + 1u : APCDataStructure::BRANCH_SENTINAL,
                    MoStoreSeq_
                );
            }

            PageClassesFreeHead_.store(PageClassesTableCapacity_ ? UNSIGNED_ZERO : APCDataStructure::BRANCH_SENTINAL, MoStoreSeq_);

            for (uint32_t i = 0; i < WorkingTableCapacity_; i++)
            {
                WorkRingPtrs_[i].BranchId.store(UNSIGNED_ZERO, MoStoreSeq_);
                WorkRingPtrs_[i].WorkKind.store(UNSIGNED_ZERO, MoStoreSeq_);
                WorkRingPtrs_[i].PageClass.store(UNSIGNED_ZERO, MoStoreSeq_);
                WorkRingPtrs_[i].PackedHandle.store(UNSIGNED_ZERO, MoStoreSeq_);
            }
            
            ViewCellCursor_.store(UNSIGNED_ZERO, MoStoreSeq_);
            NextValidViewId_.store(APCDataStructure::BRANCH_VERSION, MoStoreSeq_);

        }
        catch(...)
        {
            FabricCellsPtrs_.reset();
            FabricObjectPoolPtrs_.reset();
            SlotTablePtrs_.reset();
            BranchTablePtrs_.reset();
            LogicalTablePtrs_.reset();
            SharedTablePtrs_.reset();
            PageClassesTablePtrs_.reset();
            WorkRingPtrs_.reset();
            FabricActiveViewPtrs_.reset();
            CellIndicesViewPtrs_.reset();
            Float32CellViewPtrs_.reset();
            OriginalPacked64ViewPtrs_.reset();
            Clock16ViewPtrs_.reset();
            FabricInitialized_.store(false, MoStoreSeq_);
            return false;
        }
        
        return true;
        
    }


    // size_t NeuromorphicAPCFabricCordinator::PopFreeSlot_() noexcept
    // {
    //     if (!)
    //     {
    //         /* code */
    //     }
        
    // }

}


