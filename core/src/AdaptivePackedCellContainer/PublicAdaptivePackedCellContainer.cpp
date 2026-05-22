#include "NeuromorphicTimeSpace/APCSegmentsCausalCordinator.hpp"
#include "PackedCellContainerManager.hpp"
#include <iostream>

namespace PredictedAdaptedEncoding
{
    class PackedCellContainerManager;
    
    uint32_t AdaptivePackedCellContainer::GetBranchId() noexcept
    {

        return ReadMetaCellValue32(MetaIndexOfAPCNode::BRANCH_ID);

    }

    uint32_t AdaptivePackedCellContainer::GetLogicalId() noexcept
    {
        return ReadMetaCellValue32(MetaIndexOfAPCNode::LOGICAL_NODE_ID);
    }

    uint32_t AdaptivePackedCellContainer::GetSharedId() noexcept
    {
            return ReadMetaCellValue32(MetaIndexOfAPCNode::SHARED_ID);
    }

    size_t AdaptivePackedCellContainer::ReserveProducerSlots(size_t number_of_slots) noexcept
    {
        if (!IfAPCBranchValid() || number_of_slots == 0)
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }
        const size_t payload_capacity = PayloadCapacityFromHeader();
        if (payload_capacity == 0)
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }

        if (payload_capacity == 1)
        {
            return PayloadBegin();
        }
        
        number_of_slots = std::min<size_t>(number_of_slots, payload_capacity - 1);
        number_of_slots = std::max<size_t>(number_of_slots, 1u);

        constexpr uint32_t HARD_RESERVE_GUARD = 4096u;

        for (uint32_t attempt = 0; attempt < HARD_RESERVE_GUARD; ++attempt)
        {
            uint32_t current_producer_cursor = GetProducerCursorPlacement();
            if (current_producer_cursor == BRANCH_SENTINAL || current_producer_cursor < PayloadBegin() || current_producer_cursor >= GetTotalCapacityForThisAPC())
            {
                current_producer_cursor = PayloadBegin();
            }
            const size_t current_offset = static_cast<size_t>(current_producer_cursor - PayloadBegin()) % payload_capacity;
            size_t next_offset = (current_offset + number_of_slots) % payload_capacity;

            if (next_offset == current_offset)
            {
                next_offset = (current_offset + 1u) % payload_capacity;
            }
            


            const uint32_t desired_cursor = static_cast<uint32_t>(PayloadBegin() + next_offset);
            bool changed = false;
            ProducerORConsumerCursorSetAndGet_(
                static_cast<uint32_t>(desired_cursor),
                0,
                &changed,
                MetaIndexOfAPCNode::PRODUCER_CURSOR_PLACEMENT
            );
            if (changed)
            {
                return static_cast<size_t>(current_producer_cursor);
            }
            if (AdaptiveBackoffOfAPCPtr_)
            {
                AdaptiveBackoffOfAPCPtr_->AdaptiveBackOffPacked(
                    ReadFullMetaCell(MetaIndexOfAPCNode::PRODUCER_CURSOR_PLACEMENT)
                );
            }
            else
            {
                std::this_thread::yield();
            }
        }

        TotalCASFailForThisBranchIncreaseAndGet(1);
        return APCDataStructure::APC_SIZE_SENTINAL;
    }

    void AdaptivePackedCellContainer::SetManagerForGlobalAPC(PackedCellContainerManager* pointer_of_global_apc_manager) noexcept
    {
        APCManagerPtr_ = pointer_of_global_apc_manager;
        if (AdaptiveBackoffOfAPCPtr_ == nullptr && APCManagerPtr_ != nullptr)
        {
            AdaptiveBackoffOfAPCPtr_ = &APCManagerPtr_->GetManagersAdaptiveBackoff();
        }
    }


    
    void AdaptivePackedCellContainer::InitOwned(size_t container_capacity,
        ContainerConf container_cfg
    )
    {
        
        FreeAll();
        if (!IsCapacityOfAPCLegal(container_capacity))
        {
            throw std::invalid_argument("Capacity is unbounded and not acceptable must be > METACELL_COUNT(95) && <= APC_MAX_LENGTH_OR_COUNTER ");
        }
        
        BackingPtr = AllocateAlignedAtomicCells_(container_capacity);
        BranchCapacity_ = container_capacity;
        packed64_t idle_cell = PackedCell64_t::MakeInitialPacked(container_cfg.InitialMode);
        for (size_t i = 0; i < container_capacity; i++)
        {
            BackingPtr[i].store(idle_cell, MoStoreUnSeq_);
        }

        try
        {
            if (APCManagerPtr_)
            {
                APCManagerPtr_->StartAPCManager();
                AdaptiveBackoffOfAPCPtr_ = &APCManagerPtr_->GetManagersAdaptiveBackoff();
            }
            else
            {
                AdaptiveBackoffOfAPCPtr_ = nullptr;
            }
            OwnedMasterClockConfPtr_ = std::make_unique<MasterClockConf>(this, LocalTimer48_);
            if (AdaptiveBackoffOfAPCPtr_ )
            {
                AdaptiveBackoffOfAPCPtr_->AttachMasterClockToAadaptiveBackOff(OwnedMasterClockConfPtr_.get());
            }
        }
        catch(...)
        {
            AdaptiveBackoffOfAPCPtr_ = nullptr;
            OwnedMasterClockConfPtr_.reset();
            FreeAlignedAtomicCells_(BackingPtr, container_capacity);
            BackingPtr = nullptr;
            throw;
        }
        
        const uint32_t new_branch_id = GlobalBranchIdAlloc_.fetch_add(1, std::memory_order_acq_rel);
        const uint32_t logical_node_id = new_branch_id;
        const uint32_t shared_id = new_branch_id;
        container_cfg.NodeGroupSize = 1u;

        InitRootOrChildBranch(
            new_branch_id,
            logical_node_id,
            shared_id,
            container_capacity,
            container_cfg,
            true,
            UNSIGNED_ZERO,
            UNSIGNED_ZERO
        );
        InitZeroState_();
        if (container_cfg.RegionSize > 0)
        {
            InitRegionIdx(container_cfg.RegionSize);
        }
        if (APCManagerPtr_)
        {
            APCManagerPtr_->RegisterAPCFromManager_(this);
        }
        RefreshAPCMeta_();
    }

    void AdaptivePackedCellContainer::InitAPCAsNode(
        size_t capacity,
        const ContainerConf& container_configuration,
        uint32_t aux_param_u32
    )
    {
        InitOwned(capacity, container_configuration);
        InitNodeSemantics(aux_param_u32);
        SetGraphNodeFlag();
    }


    void AdaptivePackedCellContainer::InitRegionIdx(size_t region_size) noexcept
    {
        if (!IfAPCBranchValid() || region_size == 0)
        {
            return;
        }
        const uint32_t current_region_size = ReadMetaCellValue32(MetaIndexOfAPCNode::REGION_SIZE);
        if (!JustUpdateValueOfMeta32(
            MetaIndexOfAPCNode::REGION_SIZE,
            current_region_size,
            static_cast<uint32_t>(region_size)
        ))
        {
            return;
        }
        const uint32_t region_count = static_cast<uint32_t>((PayloadCapacityFromHeader() + region_size - 1u) / region_size);
        const uint32_t current_region_count = ReadMetaCellValue32(MetaIndexOfAPCNode::REGION_COUNT);
        if (!JustUpdateValueOfMeta32(
            MetaIndexOfAPCNode::REGION_COUNT,
            current_region_count,
            region_count
        ))
        {
            return;
        }
        RebuildRegionIndexFromPayload_();
        TurnOnASegmentFlag(ControlEnumOfAPCSegment::HAS_REGION_INDEX);
    }

    size_t AdaptivePackedCellContainer::NextProducerSequence() noexcept
    {
        if (!IfAPCBranchValid())
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }

        struct ProducerBlockCacheTLS
        {
            const AdaptivePackedCellContainer* OwnerOfNode = nullptr;
            uint32_t OwnerBranchId = 0;
            size_t BlockBase = 0;
            size_t BlockLeft = 0;
        };

        thread_local ProducerBlockCacheTLS cache{};
        const uint32_t branch_id = GetBranchId();

        if (cache.OwnerOfNode != this || cache.OwnerBranchId != branch_id)
        {
            cache.OwnerOfNode = this;
            cache.OwnerBranchId = branch_id;
            cache.BlockBase = 0;
            cache.BlockLeft = 0;
        }

        const size_t payload_capacity = PayloadCapacityFromHeader();
        if (payload_capacity == UNSIGNED_ZERO)
        {
            return APCDataStructure::APC_SIZE_SENTINAL;
        }
        

        const size_t current_block_size = static_cast<size_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::PRODUCER_BLOCK_SIZE));
        const size_t safe_block = payload_capacity > 1 ? std::min<size_t>(
            std::max<size_t>(current_block_size, 1u),
            payload_capacity - 1u
        ) : 1u;

        if (cache.BlockLeft == 0)
        {
            const size_t base = ReserveProducerSlots(safe_block);
            if (base == APCDataStructure::APC_SIZE_SENTINAL)
            {
                return APCDataStructure::APC_SIZE_SENTINAL;
            }
            cache.BlockBase = base;
            cache.BlockLeft = safe_block;
        }
        const size_t sequence = cache.BlockBase;
        ++cache.BlockBase;
        --cache.BlockLeft;
        return sequence;
    }



    void AdaptivePackedCellContainer::TryCreateBranchIfNeeded(APCPagedNodeSegmentClasses rel_mask_hint) noexcept
    {
        if (!IfAPCBranchValid() || !APCManagerPtr_)
        {
            return;
        }
        
        if (!HasThisControlEnumFlag(
            ControlEnumOfAPCSegment::ENABLE_BRANCHING
        ))
        {
            return;
        }

        if(!ShouldSplitNow())
        {
            return;
        }

        GrowSharedNodeByRegionKind(rel_mask_hint);
        
    }
    

    AdaptivePackedCellContainer* AdaptivePackedCellContainer::FindSharedRootOrThis() noexcept
    {
        if (!IfAPCBranchValid() || !APCManagerPtr_)
        {
            return this;
        }
        AdaptivePackedCellContainer* current_apc_ptr = this;
        while (current_apc_ptr)
        {
            if (!current_apc_ptr)
            {
                break;
            }
            const uint32_t previous_id = current_apc_ptr->ReadMetaCellValue32(MetaIndexOfAPCNode::SHARED_PREVIOUS_ID);
            if (previous_id == UNSIGNED_ZERO || previous_id == BRANCH_SENTINAL)
            {
                break;
            }
            AdaptivePackedCellContainer* previous_apc_ptr = APCManagerPtr_->GetAPCPtrFromBranchId(previous_id);
            if (!previous_apc_ptr || previous_apc_ptr == current_apc_ptr || !previous_apc_ptr->IfAPCBranchValid())
            {
                break;
            }
            current_apc_ptr = previous_apc_ptr;
        }
        return current_apc_ptr ? current_apc_ptr : this;
        
    }

    AdaptivePackedCellContainer* AdaptivePackedCellContainer::GetNextSharedSegment() noexcept
    {
        if (!IfAPCBranchValid() || !APCManagerPtr_)
        {
            return nullptr;
        }

        const uint32_t next_apc_id = ReadMetaCellValue32(MetaIndexOfAPCNode::SHARED_NEXT_ID);

        if (next_apc_id == UNSIGNED_ZERO || next_apc_id == BRANCH_SENTINAL)
        {
            return nullptr;
        }
        AdaptivePackedCellContainer* next_apc_ptr = APCManagerPtr_->GetAPCPtrFromBranchId(next_apc_id);
        if (!next_apc_ptr || next_apc_ptr == this)
        {
            return nullptr;
        }
        return next_apc_ptr;
    }

    bool AdaptivePackedCellContainer::IsAPCSharedChainEmpty() noexcept
    {
        if (!IfAPCBranchValid() || !APCManagerPtr_)
        {
            return true;
        }
        AdaptivePackedCellContainer* current_apc_ptr = FindSharedRootOrThis();
        while (current_apc_ptr)
        {
            uint16_t published_occupancy = UNSIGNED_ZERO;
            uint16_t claimed_occupancy = UNSIGNED_ZERO;
            uint16_t faulty_occupancy = UNSIGNED_ZERO;
            const bool ok = current_apc_ptr->GetPublishedClaimedFaultyFromCentral(published_occupancy, claimed_occupancy, faulty_occupancy);

            if (!ok || published_occupancy > UNSIGNED_ZERO || claimed_occupancy > UNSIGNED_ZERO || faulty_occupancy > UNSIGNED_ZERO)
            {
                return false;
            }

            if (
                current_apc_ptr->HasThisControlEnumFlag(ControlEnumOfAPCSegment::LAYOUT_MUTATION_INFLIGHT) ||
                current_apc_ptr->HasThisControlEnumFlag(ControlEnumOfAPCSegment::SPLIT_INFLIGHT)
            )
            {
                return false;
            }
            if (!current_apc_ptr->IfAPCBranchValid())
            {
                break;
            }
            
            uint32_t next_apc_id = current_apc_ptr->ReadMetaCellValue32(MetaIndexOfAPCNode::SHARED_NEXT_ID);
            if (next_apc_id == UNSIGNED_ZERO || next_apc_id == BRANCH_SENTINAL)
            {
                break;
            }
            AdaptivePackedCellContainer* next_apc_ptr = APCManagerPtr_->GetAPCPtrFromBranchId(next_apc_id);
            if (!next_apc_ptr || next_apc_ptr == current_apc_ptr)
            {
                break;
            }
            current_apc_ptr = next_apc_ptr;
        }
        return true;
    }

    bool AdaptivePackedCellContainer::TryPublishRegionalSharedGrowthOnce(APCPagedNodeSegmentClasses region_kind, packed64_t packed_cell, std::atomic<uint64_t>* growth_counter) noexcept
    {
        PublishResult local_result = PublishCellByRegionMAskTraverseStartsFromThisAPC(region_kind, packed_cell);
        if (local_result.ResultStatus == PublishStatus::OK)
        {
            return true;
        }

        auto full_layout = ReadAndGetFullRegionLayout_();
        const uint32_t grow_amount = SuggestedInternalAPCExpension_(full_layout ? &(*full_layout) : nullptr, 50);
        if (grow_amount > 0 && TryExtendASegmentInOwnAPC(
            region_kind,
            grow_amount,
            ContainerConf::APCSegmentExtendOrder::PRIORITY
        ))
        {
            local_result = PublishCellByRegionMAskTraverseStartsFromThisAPC(region_kind, packed_cell);
            if (local_result.ResultStatus == PublishStatus::OK)
            {
                return true;
            }
        }

        AdaptivePackedCellContainer* grown_apc = GrowSharedNodeByRegionKind(region_kind, true);
        if (grown_apc)
        {
            if (growth_counter)
            {
                growth_counter->fetch_add(1, std::memory_order_relaxed);
            }
            const PublishResult ok = grown_apc->PublishCellByRegionMAskTraverseStartsFromThisAPC(region_kind, packed_cell);
            if (ok.ResultStatus == PublishStatus::OK)
            {
                return true;
            }
        }
        return false;
    }


    std::optional<packed64_t> AdaptivePackedCellContainer::ConsumeCellByRegionMaskTraverseStartFromThisAPC(APCPagedNodeSegmentClasses region_kind, size_t& scan_cursor) noexcept
    {
        if (!IfAPCBranchValid())
        {
            return std::nullopt;
        }
        AdaptivePackedCellContainer* root_apc_ptr = FindSharedRootOrThis();
        if (!root_apc_ptr)
        {
            return std::nullopt;
        }
        AdaptivePackedCellContainer* current_apc_ptr = root_apc_ptr;
        bool first = true;
        const uint32_t group_size = std::max<uint32_t>(1u, ReadMetaCellValue32(MetaIndexOfAPCNode::NODE_GROUP_SIZE));
        uint32_t chain_guard = 0;
        const uint32_t max_chain_steps = group_size + 2u;

        while (current_apc_ptr && chain_guard++ < max_chain_steps)
        {
            size_t local_cursor = first ? scan_cursor : PayloadBegin();
            auto maybe_cell = current_apc_ptr->TryConsumeAndIdleFromRegionLocal_(region_kind, local_cursor);
            if (maybe_cell)
            {
                if (first)
                {
                    scan_cursor = local_cursor;
                }
                return *maybe_cell;
            }
            AdaptivePackedCellContainer* next_apc_ptr = current_apc_ptr->GetNextSharedSegment();
            if (!next_apc_ptr || next_apc_ptr == current_apc_ptr)
            {
                break;
            }
            current_apc_ptr = next_apc_ptr;
            first = false;
        }
        return std::nullopt;
    }

    PublishResult AdaptivePackedCellContainer::PublishCellByRegionMAskTraverseStartsFromThisAPC(
        APCPagedNodeSegmentClasses page_class, packed64_t cell_to_publish,
        PackedCellNodeAuthority authority,
        std::optional<uint16_t> max_tries
    ) noexcept
    {

        if (!IfAPCBranchValid())
        {
            return PublishResult{};
        }
        const uint16_t resolved_tries = max_tries.has_value() ? *max_tries : ComputeAdaptivemaxTreies_(cell_to_publish);

        const PublishResult local_result = TryPublishToRegionLocal_(cell_to_publish, page_class, authority, resolved_tries);
        if (local_result.ResultStatus == PublishStatus::OK)
        {
            return local_result;
        }

        AdaptivePackedCellContainer* curren_or_next_container_ptr = GetNextSharedSegment();
        const uint32_t group_size = std::max<uint32_t>(1u, ReadMetaCellValue32(MetaIndexOfAPCNode::NODE_GROUP_SIZE));
        uint32_t chain_guard = 0;
        const uint32_t max_chain_steps = group_size + 2u;

        while (curren_or_next_container_ptr && chain_guard++ < max_chain_steps)
        {
            //using resolved_tries here results deadlock 
            const PublishResult sibling_result_publish = curren_or_next_container_ptr->TryPublishToRegionLocal_(cell_to_publish, page_class, authority, resolved_tries);
            if (sibling_result_publish.ResultStatus == PublishStatus::OK)
            {
                return sibling_result_publish;
            }
            AdaptivePackedCellContainer* next_apc_ptr = curren_or_next_container_ptr->GetNextSharedSegment();
            if (!next_apc_ptr || next_apc_ptr == curren_or_next_container_ptr)
            {
                break;
            }
            curren_or_next_container_ptr = next_apc_ptr;
        }
        if (ShouldSplitNow())
        {
            AdaptivePackedCellContainer* grown_apc = GrowSharedNodeByRegionKind(page_class, true);
            if (grown_apc)
            {
                return grown_apc->TryPublishToRegionLocal_(cell_to_publish, page_class, authority, resolved_tries);
            }
        }
        return local_result;
    }

    AdaptivePackedCellContainer* AdaptivePackedCellContainer::GrowSharedNodeByRegionKind(APCPagedNodeSegmentClasses desired_region_kind, bool enable_recursive_branching) noexcept
    {
        if (!IfAPCBranchValid() || !APCManagerPtr_)
        {
            return nullptr;
        }

        if (!HasThisControlEnumFlag(ControlEnumOfAPCSegment::ENABLE_BRANCHING))
        {
            return nullptr;
        }

        if (!TryMarkSplitInFlight())
        {
            return nullptr;
        }

        auto ClearSplitFlag = [this]() noexcept
        {
            ClearOneControlEnumFlagOfAPC(
                ControlEnumOfAPCSegment::SPLIT_INFLIGHT
            );
        };

        AdaptivePackedCellContainer* root_apc_ptr = FindSharedRootOrThis();
        if (!root_apc_ptr)
        {
            ClearSplitFlag();
            return nullptr;
        }
        const uint32_t root_branch_id = root_apc_ptr->GetBranchId();
        const uint32_t root_logical_id = root_apc_ptr->GetLogicalId();
        const uint32_t shared_group_id = (root_apc_ptr->GetSharedId() == UNSIGNED_ZERO || root_apc_ptr->GetSharedId() == BRANCH_SENTINAL) ?
                                            root_branch_id : root_apc_ptr->GetSharedId();
        const uint32_t parents_depth = CurrentBranchDepthRead();
        const uint32_t child_depth = parents_depth + 1u;
        const uint32_t max_depth = MaxDepthRead();
        if (child_depth > max_depth)
        {
            ClearSplitFlag();
            return nullptr;
        }
        
        ContainerConf child_configuration{};
        child_configuration.InitialMode = static_cast<PackedMode>(ReadMetaCellValue32(MetaIndexOfAPCNode::DEFINED_MODE_OF_CURRENT_APC));
        child_configuration.ProducerBlockSize = static_cast<size_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::PRODUCER_BLOCK_SIZE));
        child_configuration.RegionSize = static_cast<size_t>(ReadMetaCellValue32(MetaIndexOfAPCNode::REGION_SIZE));
        child_configuration.RetireBatchThreshold = ReadMetaCellValue32(MetaIndexOfAPCNode::RETIRE_BRANCH_THRASHOLD);
        child_configuration.BackgroundEpochAdvanceMS = ReadMetaCellValue32(MetaIndexOfAPCNode::BACKGROUND_EPOCH_ADVANCE_MS);
        child_configuration.EnableBranching = enable_recursive_branching;
        child_configuration.BranchSplitThresholdPercentage = ReadMetaCellValue32(MetaIndexOfAPCNode::SPLIT_THRESHOLD_PERCENTAGE);
        child_configuration.BranchMaxDepth = ReadMetaCellValue32(MetaIndexOfAPCNode::MAX_DEPTH);
        child_configuration.BranchMinChildCapacity = SuggestedChildCapacity_();

        AdaptivePackedCellContainer* new_child_segment_ptr = nullptr;
        try
        {
            new_child_segment_ptr = new AdaptivePackedCellContainer();
            new_child_segment_ptr->SetManagerForGlobalAPC(APCManagerPtr_);
            new_child_segment_ptr->InitOwned(child_configuration.BranchMinChildCapacity, child_configuration);
        }
        catch(...)
        {
            if (new_child_segment_ptr)
            {
                new_child_segment_ptr->FreeAll();
                delete new_child_segment_ptr;
            }
            ClearSplitFlag();
            return nullptr;
        }


        
        const uint32_t new_child_branch_id = new_child_segment_ptr->GetBranchId();
        if (new_child_branch_id == UNSIGNED_ZERO || new_child_branch_id == BRANCH_SENTINAL || new_child_branch_id == root_branch_id)
        {
            new_child_segment_ptr->FreeAll();
            delete new_child_segment_ptr;
            ClearSplitFlag();
            return nullptr;
        }
        
        new_child_segment_ptr->InitRootOrChildBranch(
            new_child_branch_id,
            root_logical_id,
            shared_group_id,
            new_child_segment_ptr->GetTotalCapacityForThisAPC(),
            child_configuration,
            false,
            root_apc_ptr->ReadMetaCellValue32(MetaIndexOfAPCNode::NODE_AUX_PARAM_U32),
            child_depth,
            static_cast<uint8_t>(root_apc_ptr->ReadMetaCellValue32(MetaIndexOfAPCNode::BRANCH_PRIORITY))
        );

        new_child_segment_ptr->SetSegmentRegionKind(desired_region_kind);
        auto CopyBranchSagmentMeta = [&](MetaIndexOfAPCNode idx) noexcept
        {
            const uint32_t root_src = root_apc_ptr->ReadMetaCellValue32(idx);
            const uint32_t child_dest = new_child_segment_ptr->ReadMetaCellValue32(idx);
            new_child_segment_ptr->JustUpdateValueOfMeta32(idx, child_dest, root_src);
        };

        CopyBranchSagmentMeta(MetaIndexOfAPCNode::FEEDFORWARD_IN_TARGET_ID);
        CopyBranchSagmentMeta(MetaIndexOfAPCNode::FEEDFORWARD_OUT_TARGET_ID);
        CopyBranchSagmentMeta(MetaIndexOfAPCNode::FEEDBACKWARD_IN_TARGET_ID);
        CopyBranchSagmentMeta(MetaIndexOfAPCNode::FEEDBACKWARD_OUT_TARGET_ID);
        CopyBranchSagmentMeta(MetaIndexOfAPCNode::LATERAL_0_TARGET_ID);
        CopyBranchSagmentMeta(MetaIndexOfAPCNode::LATERAL_1_TARGET_ID);

        AdaptivePackedCellContainer* tail_apc_ptr = root_apc_ptr;

        uint32_t guard = 0;
        const uint32_t max_chain_steps =
            std::max<uint32_t>(
                1u,
                root_apc_ptr->ReadMetaCellValue32(MetaIndexOfAPCNode::NODE_GROUP_SIZE)
            ) + 2u;

        while (tail_apc_ptr && guard++ < max_chain_steps)
        {
            AdaptivePackedCellContainer* next = tail_apc_ptr->GetNextSharedSegment();

            if (!next || next == tail_apc_ptr)
            {
                break;
            }

            tail_apc_ptr = next;
        }

        if (!tail_apc_ptr)
        {
            ClearSplitFlag();
            return nullptr;
        }
        if (
            !tail_apc_ptr->TryBindShareNext(new_child_branch_id) || 
            !new_child_segment_ptr->TryBindSharedPrevious(tail_apc_ptr->GetBranchId())
        )
        {
            new_child_segment_ptr->FreeAll();
            delete new_child_segment_ptr;
            ClearSplitFlag();
            return nullptr;
        }
        tail_apc_ptr->TurnOnASegmentFlag(
            ControlEnumOfAPCSegment::HAS_SHARED_NEXT
        );
        new_child_segment_ptr->TurnOnASegmentFlag(
            ControlEnumOfAPCSegment::HAS_SHARED_PREVIOUS
        );
        if (!root_apc_ptr->RebuildSharedChainSegmentMetatdataFromRoot_())
        {
            new_child_segment_ptr->FreeAll();
            delete new_child_segment_ptr;
            ClearSplitFlag();
            return nullptr;
        }
        root_apc_ptr->RefreshAPCMeta_();
        new_child_segment_ptr->RefreshAPCMeta_();
        ClearSplitFlag();
        return new_child_segment_ptr;
    }

    bool AdaptivePackedCellContainer::RebuildSharedChainSegmentMetatdataFromRoot_() noexcept
    {
        AdaptivePackedCellContainer* root_apc_ptr = FindSharedRootOrThis();
        
        const uint32_t shared_group_id = (
            root_apc_ptr->GetSharedId() == UNSIGNED_ZERO || root_apc_ptr->GetSharedId() == BRANCH_SENTINAL
        ) ? root_apc_ptr->GetBranchId() : root_apc_ptr->GetSharedId();
        std::vector<AdaptivePackedCellContainer*> apc_chain;
        AdaptivePackedCellContainer* current_apc = root_apc_ptr;

        uint32_t guard = 0;
        constexpr uint32_t HEARD_CHAIN_GUARD = 4096;

        while (current_apc && guard++ < HEARD_CHAIN_GUARD)
        {
            apc_chain.push_back(current_apc);
            AdaptivePackedCellContainer* next = current_apc->GetNextSharedSegment();
            if (!next || next == current_apc)
            {
                break;
            }
            current_apc = next;
        }
        const uint32_t group_size = static_cast<uint32_t>(apc_chain.size());
        for (size_t i = 0; i < apc_chain.size(); i++)
        {
            AdaptivePackedCellContainer* current_chain_index_apc = apc_chain[i];
            const uint32_t expected_group_size = current_chain_index_apc->ReadMetaCellValue32(MetaIndexOfAPCNode::NODE_GROUP_SIZE);
            current_chain_index_apc->JustUpdateValueOfMeta32(MetaIndexOfAPCNode::NODE_GROUP_SIZE, expected_group_size, group_size);

            const uint32_t expected_shared_id = current_chain_index_apc->ReadMetaCellValue32(MetaIndexOfAPCNode::SHARED_ID);
            current_chain_index_apc->JustUpdateValueOfMeta32(MetaIndexOfAPCNode::SHARED_ID, expected_shared_id, shared_group_id);

            const uint32_t previous_id = (i == 0) ? BRANCH_SENTINAL : apc_chain[i - 1]->GetBranchId();
            const uint32_t next_id = (i + 1 < apc_chain.size()) ? apc_chain[i + 1]->GetBranchId() : BRANCH_SENTINAL;
            current_chain_index_apc->WriteExactMetaCellJustNewValue(
                MetaIndexOfAPCNode::SHARED_PREVIOUS_ID,
                previous_id
            );
            current_chain_index_apc->WriteExactMetaCellJustNewValue(
                MetaIndexOfAPCNode::SHARED_NEXT_ID,
                next_id
            );

            if (i == 0)
            {
                current_chain_index_apc->TurnOnASegmentFlag(ControlEnumOfAPCSegment::IS_SHARED_ROOT);
                current_chain_index_apc->ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::IS_SHARED_MAMBER);
            }
            else
            {
                current_chain_index_apc->TurnOnASegmentFlag(ControlEnumOfAPCSegment::IS_SHARED_MAMBER);
                current_chain_index_apc->ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::IS_SHARED_ROOT);
                
            }
            
            if (previous_id == BRANCH_SENTINAL)
            {
                current_chain_index_apc->ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::HAS_SHARED_PREVIOUS);
            }
            else
            {
                current_chain_index_apc->TurnOnASegmentFlag(ControlEnumOfAPCSegment::HAS_SHARED_PREVIOUS);
            }

            if (next_id == BRANCH_SENTINAL)
            {
                current_chain_index_apc->ClearOneControlEnumFlagOfAPC(ControlEnumOfAPCSegment::HAS_SHARED_NEXT);
            }
            else
            {
                current_chain_index_apc->TurnOnASegmentFlag(ControlEnumOfAPCSegment::HAS_SHARED_NEXT);
            }
        }
        return true;
    }

    void AdaptivePackedCellContainer::ClearAllManagerLinksAndFlags() noexcept
    {
        RegistryNextAPCPtr_.store(nullptr, MoStoreSeq_);
        WorkNextAPCPtr_.store(nullptr, MoStoreSeq_);
        CleanupNextAPCPtr_.store(nullptr, MoStoreSeq_);
        if (!BackingPtr || !OwnedMasterClockConfPtr_)
        {
            return;
        }
        
        const packed64_t idle = OwnedMasterClockConfPtr_->ComposeValue32WithCurrentThreadStamp16(UNSIGNED_ZERO);
        BackingPtr[static_cast<size_t>(MetaIndexOfAPCNode::MANAGER_CONTROL_FLAGS)].store(idle, MoStoreSeq_);
    }


    void AdaptivePackedCellContainer::FreeAll() noexcept
    {
        if (!BackingPtr && !OwnedMasterClockConfPtr_)
        {
            return;
        }
        if (APCManagerPtr_)
        {
            APCManagerPtr_->UnRegisterAPCFromManager_(this);
        }
        
        
        ClearAllManagerLinksAndFlags();

        AdaptiveBackoffOfAPCPtr_ = nullptr;
        OwnedMasterClockConfPtr_.reset();   
        // const uint32_t capacity = GetTotalCapacityForThisAPC();
        FreeAlignedAtomicCells_(BackingPtr, BranchCapacity_);
        BackingPtr = nullptr;
        BranchCapacity_ = 0;
        RegionRelArray_.reset();
        RegionEpochArray_.reset();
        RelBitmaps_.clear();
    }


    uint32_t AdaptivePackedCellContainer::CountExactLocalRegionalOccupancy(APCPagedNodeSegmentClasses desired_region_class) noexcept
    {
        if (!IfAPCBranchValid())
        {
            return UNSIGNED_ZERO;
        }

        auto maybe_desired_class_bounds = ReadLayoutBoundsAndVersion(desired_region_class);
        if (!maybe_desired_class_bounds || maybe_desired_class_bounds->IsEmpty())
        {
            return UNSIGNED_ZERO;
        }

        uint32_t count = 0;
        for (size_t i = maybe_desired_class_bounds->BeginIndex; i < maybe_desired_class_bounds->EndIndex; i++)
        {
            const packed64_t current_packed_cell = BackingPtr[i].load(MoLoad_);
            if (maybe_desired_class_bounds->CanCellBEConsumedForThisPhysicalRegion(current_packed_cell, i))
            {
                count++;
            }
        }
        return count;
    }

    uint32_t AdaptivePackedCellContainer::CountExactTotalChainOccupancy(APCPagedNodeSegmentClasses desired_region_class) noexcept
    {
        uint32_t total = 0;
        AdaptivePackedCellContainer* current_apc = FindSharedRootOrThis();
        while (current_apc)
        {
            total += current_apc->CountExactLocalRegionalOccupancy(desired_region_class);
            AdaptivePackedCellContainer* next_apc = current_apc->GetNextSharedSegment();
            if (!next_apc || next_apc == current_apc)
            {
                break;
            }
            current_apc = next_apc;
        }
        return total;
    }


    bool AdaptivePackedCellContainer::RebuildExectReadyMask() noexcept
    {
        if (!IfAPCBranchValid())
        {
            return false;
        }

        uint32_t mask = 0;
        for (uint8_t rel_class = 0; rel_class < APCAndPagedNodeHelpers::SIZE_OF_APCPagedNodeRelMaskClasses; rel_class++)
        {
            const auto current_region = static_cast<APCPagedNodeSegmentClasses>(rel_class);
            if (CountExactTotalChainOccupancy(current_region) > UNSIGNED_ZERO)
            {
                mask |= APCAndPagedNodeHelpers::MakeOneAPCNodeClassReadyBit(current_region);
            }
        }
        const uint32_t old_ready_mask = ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT);
        return JustUpdateValueOfMeta32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT, old_ready_mask, mask);
    }


}
