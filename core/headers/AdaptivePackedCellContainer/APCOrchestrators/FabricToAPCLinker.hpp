#pragma once
#include <functional>
#include "LayoutBoundsOrchestrator.hpp"

namespace PredictedAdaptedEncoding
{
    class SlabToFabricConverterAndCordinator;

    class PackedCellContainerManager;

    struct APCBackingCellAtomicRefViewTemp
    {
        packed64_t* CellPtr{nullptr};

        APCBackingCellAtomicRefViewTemp() noexcept = default;
        explicit APCBackingCellAtomicRefViewTemp(packed64_t* apc_ptr) noexcept :
            CellPtr(apc_ptr)
        {}

        bool IsValid() const noexcept
        {
            return CellPtr != nullptr;
        }

        packed64_t load(std::memory_order order = MoLoad_) const noexcept
        {
            if (!CellPtr)
            {
                return PackedCell64_t::PACKED_CELL_SENTINAL;
            }
            std::atomic_ref<packed64_t> cell_ptr_ref(*CellPtr);
            return cell_ptr_ref.load(order);
        }

        void store(packed64_t desired, std::memory_order order = MoStoreSeq_) const noexcept
        {
            if (!CellPtr)
            {
                return;
            }
            std::atomic_ref<packed64_t> cell_ptr_ref(*CellPtr);
            cell_ptr_ref.store(desired, order);
        }

        bool compare_exchange_strong(
            packed64_t& expected,
            packed64_t desired,
            std::memory_order success = OnExchangeSuccess,
            std::memory_order failure = OnExchangeFailure
        ) const noexcept
        {
            if (!CellPtr)
            {
                return false;
            }
            std::atomic_ref<packed64_t> cell_ptr_ref(*CellPtr);
            return cell_ptr_ref.compare_exchange_strong(expected, desired, success, failure);
        }

        bool compare_exchange_weak(
            packed64_t& expected,
            packed64_t desired,
            std::memory_order success = OnExchangeSuccess,
            std::memory_order failure = OnExchangeFailure
        ) const noexcept
        {
            if (!CellPtr)
            {
                return false;
            }
            std::atomic_ref<packed64_t> cell_ptr_ref(*CellPtr);
            return cell_ptr_ref.compare_exchange_weak(expected, desired, success, failure);
        }

        void notify_all() const noexcept
        {
            if (!CellPtr)
            {
                return;
            }
    #if defined(__cpp_lib_atomic_wait)
            std::atomic_ref<packed64_t> cell_ptr_ref(*CellPtr);
            cell_ptr_ref.notify_all();
    #endif
        }

        void notify_one() const noexcept
        {
            if (!CellPtr)
            {
                return;
            }
    #if defined(__cpp_lib_atomic_wait)
            std::atomic_ref<packed64_t> cell_ptr_ref(*CellPtr);
            cell_ptr_ref.notify_one();
    #endif
        }

        static constexpr APCBackingCellAtomicRefViewTemp* BuildBackingViewOverCells_(packed64_t* raw_ptr, size_t count) noexcept
        {
            if (!raw_ptr || count == UNSIGNED_ZERO)
            {
                return nullptr;
            };

            void* raw_view = nullptr;
            try
            {
                raw_view = ::operator new[](sizeof(APCBackingCellAtomicRefViewTemp) * count, std::align_val_t{alignof(APCBackingCellAtomicRefViewTemp)});
            }
            catch(...)
            {
                return nullptr;
            }
            
            auto* view = static_cast<APCBackingCellAtomicRefViewTemp*>(raw_view);
            for (size_t i = 0; i < count; i++)
            {
                new(&view[i]) APCBackingCellAtomicRefViewTemp(&raw_ptr[i]);
            }
            return view;
        }

        static constexpr void FreeBackingView_(APCBackingCellAtomicRefViewTemp* view, size_t count) noexcept
        {
            if (!view)
            {
                return;
            }

            for (size_t i = 0; i < count; i++)
            {
                view[i].~APCBackingCellAtomicRefViewTemp();
            }
            ::operator delete[](
                static_cast<void*>(view), std::align_val_t{alignof(APCBackingCellAtomicRefViewTemp)}
            );
        }

    };

static_assert(sizeof(APCBackingCellAtomicRefViewTemp) == sizeof(packed64_t));
static_assert(alignof(APCBackingCellAtomicRefViewTemp) == alignof(packed64_t));


class FabricToAPCLinker : public APCDataStructure
{

protected:

    packed64_t* OwnedRawBackingCells_{nullptr};
    APCBackingCellAtomicRefViewTemp* OwnedBackingView_{nullptr};
    SlabToFabricConverterAndCordinator* FabricOwnerPtr_{nullptr};
    uint64_t FabricSlotIndex_{APCDataStructure::APC_SIZE_SENTINAL};
    bool FabricBackend_{false};
    bool FabricObjectOwnedByFabric_{false};

/// UPDATE Candidates
    size_t CapacityOfThisAPC_{UNSIGNED_ZERO};
    Timer48 LocalTimer48_;
    std::function<void(const char*, const char*)> APCLogger_;
    std::vector<std::vector<uint64_t>> SOABitmapForAPC_;
    static inline std::atomic<uint32_t> GlobalBranchIdAlloc_{1};
///

    bool BindExternalRawFabricBacking_(
        packed64_t* raw_cells_ptr,
        size_t cell_count,
        SlabToFabricConverterAndCordinator* fabric_owner,
        uint64_t fabric_slot_idx,
        bool object_owned_by_fabric
    ) noexcept;

    void ReleseFabricBindingOnly_() noexcept;

public:
    APCBackingCellAtomicRefViewTemp* BackingPtr{nullptr};

    void FreeAll() noexcept;

    void SetFabricOwnerForGlobalAPC(SlabToFabricConverterAndCordinator* fabric_owner) noexcept;


    bool IsFabricBackend() const noexcept
    {
        return FabricBackend_;
    }

    uint64_t GetFabricSlotIndex() const noexcept
    {
        return FabricSlotIndex_;
    }

    SlabToFabricConverterAndCordinator* GetFabricOwner() noexcept
    {
        return FabricOwnerPtr_;
    }

    bool IsBound() const noexcept
    {
        return BackingPtr != nullptr && CapacityOfThisAPC_ >= METACELL_COUNT;
    }

    size_t PayloadCapacity() const noexcept
    {
        return CapacityOfThisAPC_ > METACELL_COUNT ? (CapacityOfThisAPC_ - METACELL_COUNT) : 0u;
    }

    bool ValidMetaIdx(MetaIndexOfAPCNode idx) noexcept
    {
        return BackingPtr && static_cast<size_t>(idx) < CapacityOfThisAPC_ && static_cast<size_t>(idx) < METACELL_COUNT;
    }

    packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept
    {
        if (ValidMetaIdx(idx))
        {
            return BackingPtr[static_cast<size_t>(idx)].load(MoLoad_);
        }
        return PACKED_CELL_SENTENAL;
    }


    /// REMOVE CANDIDATES
    packed64_t* GetAPCBackinghPtr() noexcept
    {
        if (BackingPtr && BackingPtr->CellPtr)
        {
            return BackingPtr->CellPtr;
        }
        return nullptr;
    }


    void BindExternalStorage_(packed64_t* packed_ptr, size_t cell_count) noexcept
    {
        BackingPtr->CellPtr = packed_ptr;
        CapacityOfThisAPC_ = cell_count;
    }

    void UnbindExternalStorage_() noexcept
    {
        BackingPtr = nullptr;
        CapacityOfThisAPC_ = UNSIGNED_ZERO;
    }

    static constexpr uint32_t PayloadBegin() noexcept
    {
        return METACELL_COUNT;
    }

    // AtomicAdaptiveBackoff* GetAtomicAdaptiveBackoffPtr() noexcept
    // {
    //     return AdaptiveBackoffOfAPCPtr_;
    // }


    // PackedCellContainerManager* GetAPCManager() noexcept
    // {
    //     if (!APCManagerPtr_)
    //     {
    //         return nullptr;
    //     }
    //     return APCManagerPtr_;
    // }
    /// END: REMOVE CANDIDATES

};
    
    
    
}