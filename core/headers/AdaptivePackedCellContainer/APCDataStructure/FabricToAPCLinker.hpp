#pragma once
#include "APCSpikeController/AtomicAdaptiveBackoff.hpp"

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
    bool FabricObjectOwnedByFabric{false};

/// remove candidate 
    PackedCellContainerManager* APCManagerPtr_{nullptr};
    AtomicAdaptiveBackoff* AdaptiveBackoffOfAPCPtr_{nullptr};




public:

    static APCBackingCellAtomicRefViewTemp* BuildBackingViewOverCells_(packed64_t* raw_ptr, size_t count) noexcept
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

    static void FreeBackingView_(APCBackingCellAtomicRefViewTemp* view, size_t count) noexcept
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


    void SetFabricOwnerForGlobalAPC(SlabToFabricConverterAndCordinator* fabric_owner) noexcept
    {
        FabricOwnerPtr_ = fabric_owner;
        if (fabric_owner)
        {
            APCManagerPtr_ = nullptr;
            AdaptiveBackoffOfAPCPtr_ = nullptr;
        }
    }



    

};
    
    
    
}