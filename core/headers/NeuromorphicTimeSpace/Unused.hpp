#pragma once 
#include "../AdaptivePackedCellContainer/SegmentIODefinition.hpp"

namespace PredictedAdaptedEncoding
{
    enum class ReadContractResultOfPackedCell : uint8_t
    {
        UNDEFINED_ERROR = 0,

        SUCCESSFUL_SINGLE_CELL_NON_ATOMIC_READ = 1,
        SUCCESSFUL_SINGLE_CELL_ATOMIC_READ = 2,
        SUCCESSFUL_PAIRED_VERSION_MATCHED_NON_ATOMIC_READ = 3,
        SUCCESSFUL_PAIRED_VERSION_MATCHED_ATOMIC_READ = 4,

        CLAIMED_CONTRACT_BREACHED = 5,
        FAULTY_CONTRACT_BREACHED = 6,
        VERSION_CONTRACT_BREACHED = 7,
        MODE_CONTRACT_BREACHED = 8,
        SUB_CLASS_CONTRACT_BREACHED = 9,
        DTATA_TYPE_CONTRACT_BREACHED = 10,
        MULTIPLE_CONTRACT_BREACHED = 11,

        SEGMENT_IO_NODE_SEGMENT_CLASS_CONTRACT_BREACHED = 12,
        FABRIC_TABLE_CLASS_CONTRACT_BREACHED = 13,

        CELL_READ_CONTAINS_PACKED_CELL_SENTINAL = 14

    };

    enum class RelationCellOfAPCFabric : size_t
    {
        SOURCE_BRANCH = 0,
        TARGET_BRANCH = 1,
        KIND_OF_BRANCH = 2,
        REGION_POLICY = 3,
        NEXT_OUT = 4,
        NEXT_IN = 5,
        WEIGHTOR_VIEW = 6,
        CLOCK_PRIORITY_DEVICE_VIEW = 7
    };

    enum class RelationKindOfFabric : size_t
    {
        NONE = 0,
        SHARED_NEXT,
        SHARED_PREVIOUS,
        FEED_FORWAED_IN,
        FEED_FORWARD_OUT,
        FEED_BACKWARD_IN,
        FEED_BACKWARD_OUT,
        LATERAL_0,
        LATERAL_1,
        STATE_BIND,
        ERROR_BIND,
        AUX_BIND,
        DEVICE_VIEW_LINK,
        RETIRE_LINK
    };

    enum class DeviceHintOfNeuromorphicFabric : uint16_t
    {
        HOST_FAST_CPU = 0,
        SYCL_SHARED_USM = 1,
        SYCL_DEVICE_USM = 2,
        ONEDPL_PARALLEL_SCAN_SORT_REDUCE = 3,
        ONEDNN_PRIMITIVE = 4,
        ONEMKL_DENSE = 5,
        ONEMKL_SPARSE = 6,
        PYTORCH_CPP_TENSOR_VIEW = 7
    };

    enum class WorkKindOfAPCFabric : uint16_t
    {
        NONE = 0,
        CREATE_SHARED_SEGMENT = 1,
        REBUILD_READY_MASK = 2,
        REBUILD_CHAIN_METADATA = 3,
        REBUILD_FLOAT32_VIEW = 4,
        RETIRE_HANDLE = 5,
        RECYCLE_RETIRED = 6,
        COMPACT_REGION_FOR_DEVICE = 7,
        COMPACT_HASH_TABLE = 8,
        COMPACT_ALL_HASH_TABLES = 9,
        RELATION_GC = 10,
        COMPUTE_SAFE_EPOCH = 11,
        MIGRATE_OR_GROW_FABRIC = 12
    };

    enum class ThreadCellTypeOfAPCFabric : size_t
    {
        EPOCH48 = 0,
        WAIT_TOKEN = 2,
        THREAD_STATE = 3,
        THREAD_FLAGS = 4
    };




    // bool SlabToFabricConverterAndCordinator::InitializeFabric(
    //     uint16_t slot_count,
    //     size_t slot_cell_count,
    //     uint8_t slab_id,
    //     uint32_t fabric_thread_capacity
    // ) noexcept
    // {
    //     bool expected_init = false;
    //     if (!InitializationInProgress_.compare_exchange_strong(
    //         expected_init,
    //         true,
    //         std::memory_order_acq_rel,
    //         std::memory_order_acquire
    //     ))
    //     {
    //         return false;
    //     }

    //     auto ReleseInitGuard = [this]() noexcept
    //     {
    //         InitializationInProgress_.store(false, std::memory_order_release);
    //     };

    //     ShutDownFabric();

    //     if (slot_count == UNSIGNED_ZERO || slot_count == APCDataStructure::APC_INDEX_SENTINAL)
    //     {
    //         ReleseInitGuard();
    //         return false;
    //     }

    //     if ((slab_id & Clock16Subdivision1x8Plus2x4InMode32CellModel::MASK_LOW_4) == UNSIGNED_ZERO)
    //     {
    //         slab_id = APCDataStructure::BRANCH_VERSION;
    //     }
        
    //     SlabId_ = static_cast<uint8_t>(slab_id & Clock16Subdivision1x8Plus2x4InMode32CellModel::MASK_LOW_4);
    //     SlotCount_ = slot_count,
    //     SlotCellCount_ = slot_cell_count,
    //     ThreadTableCapacity_ = std::max<uint32_t>(1u, fabric_thread_capacity);

    //     const size_t global_config_begin = UNSIGNED_ZERO;
    //     const size_t global_config_end = APCDataStructure::METACELL_COUNT;

    //     size_t cursor = DefaultFabricAlignment16Cell_(global_config_end);
    //     const size_t table_directory_begin = cursor;
    //     const size_t table_directory_end = table_directory_begin + static_cast<size_t>(FabricTableSegmentClasses::COUNT) * APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX;

    //     cursor = DefaultFabricAlignment16Cell_(table_directory_end);
    //     const size_t slot_directory_begin = cursor;
    //     const size_t slot_directory_end =  slot_directory_begin + (static_cast<size_t>(slot_count) * APC_DESCRIPTOR_WIDTH_OR_VALIDATION_INDEX);

    //     HashBucketCount_ = CoreOfFabricCoordinator::NextPowerOf2Unsigned32_(
    //         std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, static_cast<uint32_t>(slot_count) * HASH_BUCKED_WIDTH_OF_FABRIC)
    //     );
    //     cursor = DefaultFabricAlignment16Cell_(slot_directory_end);
    //     const size_t hash_span = static_cast<size_t>(HashBucketCount_) * HASH_BUCKED_WIDTH_OF_FABRIC;
    //     const size_t branch_hash_begin = cursor;
    //     const size_t branch_hash_end = branch_hash_begin + hash_span;

    //     cursor = DefaultFabricAlignment16Cell_(branch_hash_end);
    //     const size_t logical_hash_begin = cursor;
    //     const size_t logical_hash_end = logical_hash_begin + hash_span;

    //     cursor = DefaultFabricAlignment16Cell_(logical_hash_end);
    //     const size_t shared_hash_begin = cursor;
    //     const size_t shared_hash_end = shared_hash_begin + hash_span;

    //     RelationRecordCount_ = CoreOfFabricCoordinator::NextPowerOf2Unsigned32_(std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, static_cast<uint32_t>(slot_count) * 4u));
    //     const size_t edge_of_fabric_table_begin = cursor;
    //     const size_t edge_of_fabric_table_end = edge_of_fabric_table_begin + static_cast<size_t>(RelationRecordCount_) * RELATION_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(edge_of_fabric_table_end);
    //     const size_t free_retire_begin = cursor;
    //     const size_t free_retire_end = free_retire_begin + static_cast<size_t>(slot_count) * QUEUE_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(free_retire_end);
    //     const size_t ready_begin = cursor;
    //     const size_t ready_end = ready_begin + static_cast<size_t>(slot_count) * QUEUE_RECORD_WIDTH_OF_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(ready_end);
    //     const size_t work_begin = cursor;
    //     const size_t work_end = work_begin + static_cast<size_t>(slot_count) * WORK_RECORD_WIDTH_OF_FABRIC;

    //     DeviceViewRecordCount_ = CoreOfFabricCoordinator::NextPowerOf2Unsigned32_(std::max<uint32_t>(BIT_LENGTH_OF_A_PACKED_CELL, slot_count));
    //     cursor = DefaultFabricAlignment16Cell_(work_end);
    //     const size_t device_view_begin = cursor;
    //     const size_t device_view_end = device_view_begin + static_cast<size_t>(DeviceViewRecordCount_) * DEVICE_VIEW_WIDTH_OF_APC_FABRIC;

    //     cursor = DefaultFabricAlignment16Cell_(device_view_end);
    //     const size_t thread_table_begin = cursor;
    //     const size_t thread_table_end = thread_table_begin + static_cast<size_t>(ThreadTableCapacity_) *  THREAD_TABLE_RECORD_WIDTH;

    //     cursor = DefaultFabricAlignment16Cell_(thread_table_end);
    //     SegmentPoolBegin_ = DefaultFabricAlignment16Cell_(std::max(cursor, DEFAULT_FABRIC_CONTROLIO_LENGTH));
    //     SegmentPoolEnd_ = SegmentPoolBegin_ + static_cast<size_t>(slot_count) * slot_cell_count;

    //     if (SegmentPoolEnd_ >= PackedCell64_t::MODE_48_MAX_UNSIGNED_LIMIT)
    //     {
    //         ResetScalarsofTheFabric_();
    //         ReleseInitGuard();
    //         return false;
    //     }

    //     SlabCellCount_ = SegmentPoolEnd_;

    //     const packed64_t idle_free32 = PackedCell64_t::MakeModeledFabricValidPackedCell(PackedMode::MODEL32);

    //     for (size_t i = 0; i < SlabCellCount_; i++)
    //     {
    //         SlabBasePtr_[i].store(idle_free32, MoStoreSeq_);
    //     }

    //     WriteFabricMetaHeader_(table_directory_begin, table_directory_end);

    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::GLOBAL_AND_CONFIG, global_config_begin, global_config_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::RECORD_BOOK_OF_TABLE_SEGMENT_CLASSES, table_directory_begin, table_directory_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::APC_DESCRIPTOR, slot_directory_begin, slot_directory_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::BRANCH_HASH, branch_hash_begin, branch_hash_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::LOGICAL_HASH, logical_hash_begin, logical_hash_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::SHARED_HASH, shared_hash_begin, shared_hash_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::EDGE_TABLE, edge_of_fabric_table_begin, edge_of_fabric_table_end, APCDataStructure::BRANCH_VERSION);      
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::FREE_RETIRE_TABLE, free_retire_begin, free_retire_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::READY_QUEUE, ready_begin, ready_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::WORK_QUEUE, work_begin, work_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::DEVICE_VIEW_TABLE, device_view_begin, device_view_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::THREAD_TABLE, thread_table_begin, thread_table_end, APCDataStructure::BRANCH_VERSION);
    //     WriteARecordBookOfTSCEntry_(FabricTableSegmentClasses::SEGMENT_POOL, SegmentPoolBegin_, SegmentPoolEnd_, APCDataStructure::BRANCH_VERSION);

    //     //hash table init
    //     InitializeHashTable_(FabricTableSegmentClasses::BRANCH_HASH);
    //     InitializeHashTable_(FabricTableSegmentClasses::LOGICAL_HASH);
    //     InitializeHashTable_(FabricTableSegmentClasses::SHARED_HASH);

        

    //     //continue

    //     return true;
    // }


}