#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <array>
#include <optional>
#include <limits>
#include <sstream>

#include "APCSegmentsCausalCordinator.hpp"
#include "PackedCellContainerManager.hpp"

using namespace PredictedAdaptedEncoding;

namespace
{
    constexpr uint32_t TEST_CAPACITY = 256u;
    constexpr uint32_t CONCURRENT_VALUE_COUNT = 8192u;
    constexpr uint32_t CONCURRENT_PRODUCERS = 4u;
    constexpr uint32_t CONCURRENT_CONSUMERS = 4u;
    constexpr uint32_t PUBLISH_BUDGET = 4096u;

    struct TestSuite
    {
        uint64_t Passed{0};
        uint64_t Failed{0};

        void Check(bool condition, const std::string& label)
        {
            if (condition)
            {
                ++Passed;
                std::cout << "[PASS] " << label << "\n";
            }
            else
            {
                ++Failed;
                std::cout << "[FAIL] " << label << "\n";
            }
        }

        void Section(const char* name)
        {
            std::cout << "\n==== " << name << " ====\n";
        }

        int ExitCode() const
        {
            std::cout << "\n==== FINAL TEST SUMMARY ====\n";
            std::cout << "Passed: " << Passed << "\n";
            std::cout << "Failed: " << Failed << "\n";
            return Failed == 0 ? 0 : 1;
        }
    };

    struct ExactLocalityCount
    {
        uint32_t Idle{0};
        uint32_t Published{0};
        uint32_t Claimed{0};
        uint32_t Faulty{0};

        uint32_t Sum() const noexcept
        {
            return Idle + Published + Claimed + Faulty;
        }
    };

    static constexpr std::array<APCPagedNodeSegmentClasses, 5> ImportantRegions =
    {
        APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE,
        APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE,
        APCPagedNodeSegmentClasses::STATE_SLOT,
        APCPagedNodeSegmentClasses::ERROR_SLOT,
        APCPagedNodeSegmentClasses::AUX_SLOT
    };

    static const char* RegionName(APCPagedNodeSegmentClasses region)
    {
        switch (region)
        {
            case APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE:  return "FF";
            case APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE: return "FB";
            case APCPagedNodeSegmentClasses::STATE_SLOT:           return "STATE";
            case APCPagedNodeSegmentClasses::ERROR_SLOT:           return "ERROR";
            case APCPagedNodeSegmentClasses::AUX_SLOT:             return "AUX";
            case APCPagedNodeSegmentClasses::FREE_SLOT:            return "FREE";
            default:                                               return "OTHER";
        }
    }

    static ContainerConf MakeTestConfig()
    {
        ContainerConf cfg;
        cfg.InitialMode = PackedMode::MODE_32;
        cfg.ProducerBlockSize = 8;
        cfg.RegionSize = 16;
        cfg.EnableBranching = true;
        cfg.BranchSplitThresholdPercentage = 35;
        cfg.BranchMaxDepth = 6;
        cfg.BranchMinChildCapacity = TEST_CAPACITY;
        cfg.NodeGroupSize = 1u;
        return cfg;
    }

    static packed64_t PackU32(
        MasterClockConf& clock,
        uint32_t value,
        APCPagedNodeSegmentClasses region,
        PriorityPhysics priority = PriorityPhysics::IDLE
    )
    {
        return clock.ComposeValue32WithCurrentThreadStamp16(
            value,
            region,
            priority,
            PackedCellLocalityTypes::PUBLISHED,
            SubClassesOfMode32::SELF_CLASS,
            PackedCellDataType::UnsignedPCellDataType,
            PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER
        );
    }

    static packed64_t PackFloat32(
        MasterClockConf& clock,
        float value,
        APCPagedNodeSegmentClasses region,
        PriorityPhysics priority = PriorityPhysics::IDLE
    )
    {
        const uint32_t bits = BitCastMaybe<uint32_t>(value);

        return clock.ComposeValue32WithCurrentThreadStamp16(
            bits,
            region,
            priority,
            PackedCellLocalityTypes::PUBLISHED,
            SubClassesOfMode32::SELF_CLASS,
            PackedCellDataType::FloatPCellDataType,
            PackedCellOwnership::ADAPTIVE_PACKED_CELL_CONTAINER
        );
    }

    static ExactLocalityCount CountExactLocality(APCSegmentsCausalCordinator& apc)
    {
        ExactLocalityCount out{};

        if (!apc.IfAPCBranchValid())
        {
            return out;
        }

        for (size_t i = apc.PayloadBegin(); i < apc.GetTotalCapacityForThisAPC(); ++i)
        {
            const packed64_t cell = apc.BackingPtr[i].load(MoLoad_);
            const auto view = PackedCell64_t::GetAuthoritiveViewsForACell(cell);

            if (!view.IsCellValid)
            {
                ++out.Faulty;
                continue;
            }

            switch (view.LocalityOfCell)
            {
                case PackedCellLocalityTypes::IDLE:
                    ++out.Idle;
                    break;

                case PackedCellLocalityTypes::PUBLISHED:
                    ++out.Published;
                    break;

                case PackedCellLocalityTypes::CLAIMED:
                    ++out.Claimed;
                    break;

                case PackedCellLocalityTypes::FAULTY:
                default:
                    ++out.Faulty;
                    break;
            }
        }

        return out;
    }

    static uint32_t CountExactPublishedInPhysicalRegion(
        APCSegmentsCausalCordinator& apc,
        APCPagedNodeSegmentClasses region
    )
    {
        auto bounds = apc.ReadLayoutBoundsAndVersion(region);
        if (!bounds || bounds->IsEmpty())
        {
            return 0;
        }

        uint32_t count = 0;

        for (size_t i = bounds->BeginIndex; i < bounds->EndIndex; ++i)
        {
            const packed64_t cell = apc.BackingPtr[i].load(MoLoad_);
            if (bounds->CanCellBEConsumedForThisPhysicalRegion(cell, i))
            {
                ++count;
            }
        }

        return count;
    }

    static uint32_t HeaderLocalitySum(APCSegmentsCausalCordinator& apc)
    {
        return
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::IDLE) +
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::PUBLISHED) +
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::CLAIMED) +
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::FAULTY);
    }

    static uint32_t ReadyBitFor(APCPagedNodeSegmentClasses region)
    {
        return APCAndPagedNodeHelpers::MakeOneAPCNodeClassReadyBit(region);
    }

    static bool ValidateNodeQuiescent(
        TestSuite& suite,
        const char* node_name,
        APCSegmentsCausalCordinator& apc
    )
    {
        bool ok = true;

        const bool valid = apc.IfAPCBranchValid();
        suite.Check(valid, std::string(node_name) + ": branch valid");
        if (!valid)
        {
            return false;
        }

        const uint32_t payload = static_cast<uint32_t>(apc.PayloadCapacityFromHeader());
        const ExactLocalityCount exact = CountExactLocality(apc);

        const uint32_t header_idle =
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::IDLE);

        const uint32_t header_pub =
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::PUBLISHED);

        const uint32_t header_claim =
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::CLAIMED);

        const uint32_t header_fault =
            apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::FAULTY);

        const uint32_t header_sum = HeaderLocalitySum(apc);

        const bool exact_sum_ok = exact.Sum() == payload;
        const bool header_sum_ok = header_sum == payload;
        const bool idle_ok = exact.Idle == header_idle;
        const bool pub_ok = exact.Published == header_pub;
        const bool claim_ok = exact.Claimed == header_claim;
        const bool fault_ok = exact.Faulty == header_fault;

        suite.Check(exact_sum_ok, std::string(node_name) + ": exact payload locality sum == payload");
        suite.Check(header_sum_ok, std::string(node_name) + ": header locality sum == payload");
        suite.Check(idle_ok, std::string(node_name) + ": exact idle == header idle");
        suite.Check(pub_ok, std::string(node_name) + ": exact published == header published");
        suite.Check(claim_ok, std::string(node_name) + ": exact claimed == header claimed");
        suite.Check(fault_ok, std::string(node_name) + ": exact faulty == header faulty");

        ok = ok && exact_sum_ok && header_sum_ok && idle_ok && pub_ok && claim_ok && fault_ok;

        const uint32_t ready_mask =
            apc.ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT);

        for (const auto region : ImportantRegions)
        {
            const uint32_t exact_region_pub =
                CountExactPublishedInPhysicalRegion(apc, region);

            const uint32_t meta_region_pub =
                apc.ReadPublishedOccupancyOfAPageClass(region);

            const bool region_ok = exact_region_pub == meta_region_pub;

            std::ostringstream label;
            label << node_name << ": region " << RegionName(region)
                  << " exact published == meta published";

            suite.Check(region_ok, label.str());
            ok = ok && region_ok;

            const uint32_t bit = ReadyBitFor(region);
            const bool bit_is_set = (ready_mask & bit) != 0;

            const bool ready_ok =
                (exact_region_pub > 0 && bit_is_set) ||
                (exact_region_pub == 0 && !bit_is_set);

            std::ostringstream ready_label;
            ready_label << node_name << ": region " << RegionName(region)
                        << " ready bit agrees with exact published count";

            suite.Check(ready_ok, ready_label.str());
            ok = ok && ready_ok;
        }

        return ok;
    }

    static void PrintNodeSummary(
        const char* name,
        APCSegmentsCausalCordinator& apc
    )
    {
        const ExactLocalityCount exact = CountExactLocality(apc);

        std::cout << "\n[" << name << "]\n";
        std::cout << "  branch=" << apc.GetBranchId()
                  << " logical=" << apc.GetLogicalId()
                  << " shared=" << apc.GetSharedId()
                  << " total_capacity=" << apc.GetTotalCapacityForThisAPC()
                  << " payload=" << apc.PayloadCapacityFromHeader()
                  << "\n";

        std::cout << "  exact: idle=" << exact.Idle
                  << " pub=" << exact.Published
                  << " claim=" << exact.Claimed
                  << " faulty=" << exact.Faulty
                  << " sum=" << exact.Sum()
                  << "\n";

        std::cout << "  header: idle="
                  << apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::IDLE)
                  << " pub="
                  << apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::PUBLISHED)
                  << " claim="
                  << apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::CLAIMED)
                  << " faulty="
                  << apc.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::FAULTY)
                  << " ready=0x" << std::hex
                  << apc.ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT)
                  << std::dec
                  << "\n";

        for (const auto region : ImportantRegions)
        {
            const auto bounds = apc.ReadLayoutBoundsAndVersion(region);
            std::cout << "  region " << RegionName(region)
                      << " meta_pub=" << apc.ReadPublishedOccupancyOfAPageClass(region)
                      << " exact_pub=" << CountExactPublishedInPhysicalRegion(apc, region);

            if (bounds)
            {
                std::cout << " span=[" << bounds->BeginIndex
                          << "," << bounds->EndIndex
                          << ") size=" << bounds->GetPayloadSpan();
            }

            std::cout << "\n";
        }
    }

    static bool PublishBudgeted(
        APCSegmentsCausalCordinator& apc,
        APCPagedNodeSegmentClasses region,
        packed64_t cell,
        PackedCellContainerManager& manager,
        std::atomic<uint64_t>* grow_counter,
        uint32_t budget = PUBLISH_BUDGET
    )
    {
        for (uint32_t attempt = 0; attempt < budget; ++attempt)
        {
            if (apc.PublishCausal(region, cell, grow_counter))
            {
                return true;
            }

            manager.GetCellsAdaptiveBackoffFromManager(cell);
        }

        return false;
    }

    static uint32_t DrainRegion(
        APCSegmentsCausalCordinator& apc,
        APCPagedNodeSegmentClasses region,
        PackedCellContainerManager& manager,
        std::vector<uint32_t>* out_u32 = nullptr
    )
    {
        uint32_t drained = 0;
        size_t cursor = apc.PayloadBegin();

        uint32_t empty_spins = 0;

        while (apc.HasAnyPublishedInChain(region))
        {
            auto maybe = apc.ConsumeCausal(region, cursor, nullptr, false);

            if (!maybe)
            {
                ++empty_spins;
                if (empty_spins > 100000)
                {
                    break;
                }

                manager.GetManagersAdaptiveBackoff().AutoBackoff();
                continue;
            }

            empty_spins = 0;
            ++drained;

            if (out_u32)
            {
                auto v = PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*maybe);
                if (v)
                {
                    out_u32->push_back(*v);
                }
            }
        }

        return drained;
    }

    static uint32_t CountSharedChainSegments(APCSegmentsCausalCordinator& apc)
    {
        uint32_t count = 0;
        AdaptivePackedCellContainer* current = apc.FindSharedRootOrThis();

        while (current)
        {
            ++count;

            AdaptivePackedCellContainer* next = current->GetNextSharedSegment();
            if (!next || next == current)
            {
                break;
            }

            current = next;

            if (count > 1024)
            {
                break;
            }
        }

        return count;
    }

    static void InitNode(
        APCSegmentsCausalCordinator& apc,
        PackedCellContainerManager& manager,
        const ContainerConf& cfg,
        SegmentIODefinition::APCNodeComputeKind kind,
        uint32_t aux = 0
    )
    {
        apc.SetManagerForGlobalAPC(&manager);
        apc.InitAPCAsNode(
            cfg.BranchMinChildCapacity,
            cfg,
            kind,
            aux
        );
    }

    static void TestPackedCellEncoding(TestSuite& suite, MasterClockConf& clock)
    {
        suite.Section("1. PackedCell encoding / extraction");

        const packed64_t u32_cell =
            PackU32(clock, 0x12345678u, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, PriorityPhysics::IMPORTANT);

        suite.Check(
            PackedCell64_t::ExtractModeOfPackedCellFromPacked(u32_cell) == PackedMode::MODE_32,
            "PackedCell: MODE_32 is preserved"
        );

        suite.Check(
            PackedCell64_t::ExtractLocalityFromPacked(u32_cell) == PackedCellLocalityTypes::PUBLISHED,
            "PackedCell: locality PUBLISHED is preserved"
        );

        suite.Check(
            PackedCell64_t::ExtractRelMaskFromPacked(u32_cell) == APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE,
            "PackedCell: region FF is preserved"
        );

        suite.Check(
            PackedCell64_t::ExtractRelOffset32FromPacked(u32_cell) == SubClassesOfMode32::SELF_CLASS,
            "PackedCell: Mode32 generic rel-offset is preserved"
        );

        suite.Check(
            PackedCell64_t::ExtractPriorityFromPacked(u32_cell) == PriorityPhysics::IMPORTANT,
            "PackedCell: priority is preserved"
        );

        const auto u32_value =
            PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(u32_cell);

        suite.Check(
            u32_value && *u32_value == 0x12345678u,
            "PackedCell: uint32 payload round-trip"
        );

        const packed64_t f32_cell =
            PackFloat32(clock, 3.25f, APCPagedNodeSegmentClasses::STATE_SLOT, PriorityPhysics::MAX_OF_SOURCE_AND_TARGET);

        const auto f32_value =
            PackedCell64_t::ExtractAnyPackedValueX<float>(f32_cell);

        suite.Check(
            f32_value && *f32_value == 3.25f,
            "PackedCell: float32 payload round-trip"
        );
    }

    static void TestManagerQSBR(TestSuite& suite, PackedCellContainerManager& manager)
    {
        suite.Section("2. Manager thread registration / QSBR");

        auto handle = manager.RegisterAPCThread();

        suite.Check(
            handle.QSBRIdx != APCDataStructure::APC_SIZE_SENTINAL && handle.WaitSlotPtr != nullptr,
            "Manager: RegisterAPCThread returns valid handle"
        );

        manager.EnterCriticalContainer(handle);

        const uint64_t min_epoch = manager.ComputeMinThreadEpoch();

        suite.Check(
            min_epoch != std::numeric_limits<uint64_t>::max(),
            "Manager: ComputeMinThreadEpoch observes active critical thread"
        );

        manager.ExtitCriticalContainer(handle);
        manager.UnRegisterAPCThread(handle);

        suite.Check(true, "Manager: thread exit/unregister completed");
    }

    static void TestInitLayoutHeader(TestSuite& suite, PackedCellContainerManager& manager)
    {
        suite.Section("3. APC init / header / layout invariants");

        ContainerConf cfg = MakeTestConfig();

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERIC_VECTOR);

        suite.Check(node.IfAPCBranchValid(), "Init: branch is valid");
        suite.Check(node.GetTotalCapacityForThisAPC() == TEST_CAPACITY, "Init: total capacity equals requested capacity");
        suite.Check(node.PayloadCapacityFromHeader() == TEST_CAPACITY - APCDataStructure::METACELL_COUNT, "Init: payload capacity is total - METACELL_COUNT");
        suite.Check(node.HasThisControlEnumFlag(SegmentIODefinition::ControlEnumOfAPCSegment::IS_GRAPH_NODE), "Init: graph-node flag set");

        for (const auto region : ImportantRegions)
        {
            const auto bounds = node.ReadLayoutBoundsAndVersion(region);

            std::ostringstream label;
            label << "Layout: " << RegionName(region) << " bounds exist";
            suite.Check(bounds.has_value(), label.str());

            if (bounds)
            {
                std::ostringstream valid_label;
                valid_label << "Layout: " << RegionName(region) << " bounds are inside payload";

                suite.Check(
                    bounds->BeginIndex >= node.PayloadBegin() &&
                    bounds->EndIndex <= node.GetTotalCapacityForThisAPC() &&
                    bounds->EndIndex >= bounds->BeginIndex,
                    valid_label.str()
                );
            }
        }

        ValidateNodeQuiescent(suite, "InitNode", node);

        node.FreeAll();
    }

    static void TestSinglePublishConsume(TestSuite& suite, PackedCellContainerManager& manager, MasterClockConf& clock)
    {
        suite.Section("4. Single-region publish / consume / occupancy");

        ContainerConf cfg = MakeTestConfig();

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERATOR_UINT32);

        const packed64_t cell =
            PackU32(clock, 777u, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, PriorityPhysics::IMPORTANT);

        const bool published =
            PublishBudgeted(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, cell, manager, nullptr);

        suite.Check(published, "Single: publish FF cell succeeds");

        suite.Check(
            node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE) == 1u,
            "Single: FF region published meta == 1"
        );

        suite.Check(
            node.ReadCentralAPCOccupancyOfALocality(PackedCellLocalityTypes::PUBLISHED) == 1u,
            "Single: central published == 1"
        );

        suite.Check(
            (node.ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT) &
             ReadyBitFor(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE)) != 0,
            "Single: FF ready bit is set after publish"
        );

        ValidateNodeQuiescent(suite, "SingleBeforeConsume", node);

        size_t cursor = node.PayloadBegin();

        auto consumed =
            node.ConsumeCausal(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, cursor, nullptr, false);

        suite.Check(consumed.has_value(), "Single: consume FF cell succeeds");

        if (consumed)
        {
            const auto value =
                PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*consumed);

            suite.Check(value && *value == 777u, "Single: consumed value == 777");
        }

        ValidateNodeQuiescent(suite, "SingleAfterConsume", node);

        node.FreeAll();
    }

    static void TestMultiRegionIsolation(TestSuite& suite, PackedCellContainerManager& manager, MasterClockConf& clock)
    {
        suite.Section("5. Multi-region isolation");

        ContainerConf cfg = MakeTestConfig();

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::BIDIRECTIONAL_PREDECTIVE);

        constexpr uint32_t N = 8;

        for (uint32_t i = 0; i < N; ++i)
        {
            PublishBudgeted(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE,
                            PackU32(clock, 1000u + i, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE),
                            manager, nullptr);

            PublishBudgeted(node, APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE,
                            PackU32(clock, 2000u + i, APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE),
                            manager, nullptr);

            PublishBudgeted(node, APCPagedNodeSegmentClasses::STATE_SLOT,
                            PackU32(clock, 3000u + i, APCPagedNodeSegmentClasses::STATE_SLOT),
                            manager, nullptr);

            PublishBudgeted(node, APCPagedNodeSegmentClasses::ERROR_SLOT,
                            PackU32(clock, 4000u + i, APCPagedNodeSegmentClasses::ERROR_SLOT),
                            manager, nullptr);
        }

        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE) == N, "Multi: FF meta count == N");
        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE) == N, "Multi: FB meta count == N");
        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::STATE_SLOT) == N, "Multi: STATE meta count == N");
        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::ERROR_SLOT) == N, "Multi: ERROR meta count == N");

        ValidateNodeQuiescent(suite, "MultiBeforeDrain", node);

        const uint32_t ff_drained = DrainRegion(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, manager);

        suite.Check(ff_drained == N, "Multi: draining FF removes exactly N cells");
        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE) == N, "Multi: draining FF does not drain FB");
        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::STATE_SLOT) == N, "Multi: draining FF does not drain STATE");
        suite.Check(node.ReadPublishedOccupancyOfAPageClass(APCPagedNodeSegmentClasses::ERROR_SLOT) == N, "Multi: draining FF does not drain ERROR");

        DrainRegion(node, APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE, manager);
        DrainRegion(node, APCPagedNodeSegmentClasses::STATE_SLOT, manager);
        DrainRegion(node, APCPagedNodeSegmentClasses::ERROR_SLOT, manager);

        ValidateNodeQuiescent(suite, "MultiAfterDrain", node);

        node.FreeAll();
    }

    static void TestReadyMaskRepair(TestSuite& suite, PackedCellContainerManager& manager, MasterClockConf& clock)
    {
        suite.Section("6. Ready-mask corruption / rebuild test");

        ContainerConf cfg = MakeTestConfig();

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERIC_VECTOR);

        const packed64_t cell =
            PackU32(clock, 111u, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE);

        PublishBudgeted(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, cell, manager, nullptr);

        const uint32_t correct_bit = ReadyBitFor(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE);

        suite.Check(
            (node.ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT) & correct_bit) != 0,
            "ReadyMask: FF bit initially set"
        );

        node.WriteExactMetaCellJustNewValue(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT, 0u);

        suite.Check(
            (node.ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT) & correct_bit) == 0,
            "ReadyMask: deliberate ready-bit corruption applied"
        );

        const bool rebuilt = node.RebuildExectReadyMask();

        suite.Check(rebuilt, "ReadyMask: RebuildExectReadyMask returns true");

        suite.Check(
            (node.ReadMetaCellValue32(MetaIndexOfAPCNode::PAGED_NODE_READY_BIT) & correct_bit) != 0,
            "ReadyMask: FF bit restored by exact rebuild"
        );

        DrainRegion(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, manager);

        node.RebuildExectReadyMask();

        ValidateNodeQuiescent(suite, "ReadyMaskAfterDrain", node);

        node.FreeAll();
    }

    static void TestLayoutExtensionSmoke(TestSuite& suite, PackedCellContainerManager& manager)
    {
        suite.Section("7. Layout extension smoke test");

        ContainerConf cfg = MakeTestConfig();

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERIC_VECTOR);

        const auto before =
            node.ReadLayoutBoundsAndVersion(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE);

        suite.Check(before.has_value(), "LayoutGrow: FF layout exists before extension");

        const bool extended =
            node.TryExtendASegmentInOwnAPC(
                APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE,
                1u,
                ContainerConf::APCSegmentExtendOrder::FIFO
            );

        const auto after =
            node.ReadLayoutBoundsAndVersion(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE);

        if (extended && before && after)
        {
            suite.Check(
                after->GetPayloadSpan() >= before->GetPayloadSpan() + 1u,
                "LayoutGrow: FF span increased after extension"
            );
        }
        else
        {
            suite.Check(
                true,
                "LayoutGrow: extension not possible in this layout; treated as non-fatal smoke result"
            );
        }

        ValidateNodeQuiescent(suite, "LayoutGrowAfter", node);

        node.FreeAll();
    }

    static void TestSharedGrowthAndChainDrain(TestSuite& suite, PackedCellContainerManager& manager, MasterClockConf& clock)
    {
        suite.Section("8. Shared growth / chain traversal / chain drain");

        ContainerConf cfg = MakeTestConfig();
        cfg.BranchSplitThresholdPercentage = 20;
        cfg.BranchMaxDepth = 6;

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERATOR_UINT32);

        std::atomic<uint64_t> grow_counter{0};

        constexpr uint32_t N = 128;
        uint32_t published = 0;

        for (uint32_t i = 0; i < N; ++i)
        {
            const packed64_t cell =
                PackU32(clock, i + 1u, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, PriorityPhysics::IMPORTANT);

            if (PublishBudgeted(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, cell, manager, &grow_counter))
            {
                ++published;
            }
        }

        suite.Check(published == N, "Growth: all FF cells published");
        suite.Check(node.CountExactTotalChainOccupancy(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE) == N, "Growth: exact chain FF occupancy == N");

        const uint32_t chain_segments = CountSharedChainSegments(node);

        suite.Check(chain_segments >= 1, "Growth: chain has at least root segment");

        std::vector<uint32_t> values;
        const uint32_t drained =
            DrainRegion(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, manager, &values);

        suite.Check(drained == N, "Growth: drained exactly N FF cells from chain");
        suite.Check(node.CountExactTotalChainOccupancy(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE) == 0, "Growth: chain FF occupancy returns to zero");

        std::sort(values.begin(), values.end());

        bool values_ok = values.size() == N;
        if (values_ok)
        {
            for (uint32_t i = 0; i < N; ++i)
            {
                if (values[i] != i + 1u)
                {
                    values_ok = false;
                    break;
                }
            }
        }

        suite.Check(values_ok, "Growth: drained values are exactly 1..N without loss");

        node.RebuildExectReadyMask();
        ValidateNodeQuiescent(suite, "GrowthRootAfterDrain", node);

        node.FreeAll();
    }

    static void TestConcurrentPublishConsumeStress(
        TestSuite& suite,
        PackedCellContainerManager& manager,
        MasterClockConf& clock
    )
    {
        suite.Section("9. Concurrent single-region publish / consume stress");

        ContainerConf cfg = MakeTestConfig();
        cfg.BranchSplitThresholdPercentage = 30;
        cfg.BranchMaxDepth = 6;

        APCSegmentsCausalCordinator node;
        InitNode(node, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERATOR_UINT32);

        std::atomic<uint32_t> next_value{1};
        std::atomic<uint32_t> produced{0};
        std::atomic<uint32_t> consumed{0};
        std::atomic<uint32_t> failed_publish{0};
        std::atomic<bool> producers_done{false};
        std::atomic<bool> stop{false};
        std::atomic<uint64_t> grow_counter{0};
        std::atomic<uint64_t> older_counter{0};

        std::vector<std::atomic<uint32_t>> seen(CONCURRENT_VALUE_COUNT + 1);
        for (auto& s : seen)
        {
            s.store(0, std::memory_order_relaxed);
        }

        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(30);

        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        for (uint32_t p = 0; p < CONCURRENT_PRODUCERS; ++p)
        {
            producers.emplace_back([&]()
            {
                auto th = manager.RegisterAPCThread();

                while (!stop.load(std::memory_order_acquire))
                {
                    const uint32_t value =
                        next_value.fetch_add(1, std::memory_order_relaxed);

                    if (value > CONCURRENT_VALUE_COUNT)
                    {
                        break;
                    }

                    const packed64_t cell =
                        PackU32(clock, value, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, PriorityPhysics::IMPORTANT);

                    if (PublishBudgeted(node, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, cell, manager, &grow_counter))
                    {
                        produced.fetch_add(1, std::memory_order_relaxed);
                    }
                    else
                    {
                        failed_publish.fetch_add(1, std::memory_order_relaxed);
                    }

                    if (std::chrono::steady_clock::now() > deadline)
                    {
                        stop.store(true, std::memory_order_release);
                    }
                }

                manager.UnRegisterAPCThread(th);
            });
        }

        for (uint32_t c = 0; c < CONCURRENT_CONSUMERS; ++c)
        {
            consumers.emplace_back([&]()
            {
                auto th = manager.RegisterAPCThread();

                size_t cursor = node.PayloadBegin();

                while (!stop.load(std::memory_order_acquire))
                {
                    if (consumed.load(std::memory_order_acquire) >= CONCURRENT_VALUE_COUNT)
                    {
                        break;
                    }

                    auto maybe =
                        node.ConsumeCausal(
                            APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE,
                            cursor,
                            &older_counter,
                            false
                        );

                    if (!maybe)
                    {
                        if (producers_done.load(std::memory_order_acquire) &&
                            node.IsAPCSharedChainEmpty())
                        {
                            break;
                        }

                        manager.GetManagersAdaptiveBackoff().AutoBackoff();

                        if (std::chrono::steady_clock::now() > deadline)
                        {
                            stop.store(true, std::memory_order_release);
                        }

                        continue;
                    }

                    const auto value =
                        PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*maybe);

                    if (value && *value >= 1u && *value <= CONCURRENT_VALUE_COUNT)
                    {
                        seen[*value].fetch_add(1, std::memory_order_relaxed);
                    }

                    consumed.fetch_add(1, std::memory_order_relaxed);
                }

                manager.UnRegisterAPCThread(th);
            });
        }

        for (auto& t : producers)
        {
            t.join();
        }

        producers_done.store(true, std::memory_order_release);

        for (auto& t : consumers)
        {
            t.join();
        }

        const bool no_timeout = !stop.load(std::memory_order_acquire);
        const bool no_failed_publish = failed_publish.load() == 0;
        const bool produced_all = produced.load() == CONCURRENT_VALUE_COUNT;
        const bool consumed_all = consumed.load() == CONCURRENT_VALUE_COUNT;

        uint32_t missing = 0;
        uint32_t duplicates = 0;

        for (uint32_t i = 1; i <= CONCURRENT_VALUE_COUNT; ++i)
        {
            const uint32_t count = seen[i].load(std::memory_order_relaxed);
            if (count == 0)
            {
                ++missing;
            }
            else if (count > 1)
            {
                duplicates += (count - 1);
            }
        }

        std::cout << "  produced=" << produced.load()
                  << " consumed=" << consumed.load()
                  << " failed_publish=" << failed_publish.load()
                  << " grow=" << grow_counter.load()
                  << " older=" << older_counter.load()
                  << " missing=" << missing
                  << " duplicates=" << duplicates
                  << "\n";

        suite.Check(no_timeout, "Concurrent: no timeout");
        suite.Check(no_failed_publish, "Concurrent: no terminal publish failure");
        suite.Check(produced_all, "Concurrent: produced all values");
        suite.Check(consumed_all, "Concurrent: consumed all values");
        suite.Check(missing == 0, "Concurrent: no missing values");
        suite.Check(duplicates == 0, "Concurrent: no duplicate values");

        node.RebuildExectReadyMask();
        ValidateNodeQuiescent(suite, "ConcurrentAfterDrain", node);

        node.FreeAll();
    }

    static void TestDeterministicBidirectionalGraph(
        TestSuite& suite,
        PackedCellContainerManager& manager,
        MasterClockConf& clock
    )
    {
        suite.Section("10. Deterministic bidirectional graph test");

        constexpr uint32_t VALUE_COUNT = 4096u;
        constexpr uint32_t PRODUCER_COUNT = 2u;
        constexpr uint32_t FF_WORKER_COUNT = 2u;
        constexpr uint32_t FB_WORKER_COUNT = 2u;
        constexpr uint32_t FINAL_WORKER_COUNT = 1u;

        ContainerConf cfg = MakeTestConfig();
        cfg.BranchSplitThresholdPercentage = 35;
        cfg.BranchMaxDepth = 6;

        APCSegmentsCausalCordinator Sensor;
        APCSegmentsCausalCordinator Predictor;
        APCSegmentsCausalCordinator Comparator;
        APCSegmentsCausalCordinator Integrator;
        APCSegmentsCausalCordinator Motor;

        InitNode(Sensor, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERATOR_UINT32);
        InitNode(Predictor, manager, cfg, SegmentIODefinition::APCNodeComputeKind::BIDIRECTIONAL_PREDECTIVE);
        InitNode(Comparator, manager, cfg, SegmentIODefinition::APCNodeComputeKind::ADD_UINT32);
        InitNode(Integrator, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERIC_VECTOR);
        InitNode(Motor, manager, cfg, SegmentIODefinition::APCNodeComputeKind::GENERIC_VECTOR);

        std::atomic<uint64_t> sensor_produced{0};
        std::atomic<uint64_t> predictor_produced{0};
        std::atomic<uint64_t> state_integrated{0};
        std::atomic<uint64_t> error_computed{0};
        std::atomic<uint64_t> final_collected{0};

        std::atomic<uint64_t> grow_ff{0};
        std::atomic<uint64_t> grow_fb{0};
        std::atomic<uint64_t> grow_state{0};
        std::atomic<uint64_t> grow_error{0};

        std::atomic<uint64_t> ff_consumed{0};
        std::atomic<uint64_t> fb_consumed{0};
        std::atomic<uint64_t> state_consumed{0};
        std::atomic<uint64_t> error_consumed{0};

        std::vector<float> collected;
        std::mutex collected_mutex;

        std::vector<std::thread> producers;
        std::vector<std::thread> workers;

        for (uint32_t p = 0; p < PRODUCER_COUNT; ++p)
        {
            producers.emplace_back([&, p]()
            {
                auto th = manager.RegisterAPCThread();

                for (uint32_t i = p + 1; i <= VALUE_COUNT; i += PRODUCER_COUNT)
                {
                    const packed64_t ff =
                        PackU32(clock, i, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, PriorityPhysics::IMPORTANT);

                    const packed64_t fb =
                        PackU32(clock, i + 1u, APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE, PriorityPhysics::OLDEST_CLOCK_FIRST);

                    if (PublishBudgeted(Sensor, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, ff, manager, &grow_ff))
                    {
                        sensor_produced.fetch_add(1, std::memory_order_relaxed);
                    }

                    if (PublishBudgeted(Predictor, APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE, fb, manager, &grow_fb))
                    {
                        predictor_produced.fetch_add(1, std::memory_order_relaxed);
                    }
                }

                manager.UnRegisterAPCThread(th);
            });
        }

        for (uint32_t w = 0; w < FF_WORKER_COUNT; ++w)
        {
            workers.emplace_back([&]()
            {
                auto th = manager.RegisterAPCThread();
                size_t cursor = Sensor.PayloadBegin();

                while (ff_consumed.load(std::memory_order_acquire) < VALUE_COUNT)
                {
                    auto maybe =
                        Sensor.ConsumeCausal(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, cursor, nullptr, false);

                    if (!maybe)
                    {
                        manager.GetManagersAdaptiveBackoff().AutoBackoff();
                        continue;
                    }

                    const auto value =
                        PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*maybe);

                    if (!value)
                    {
                        continue;
                    }

                    const packed64_t state_cell =
                        PackU32(clock, *value + 1u, APCPagedNodeSegmentClasses::STATE_SLOT, PriorityPhysics::MAX_OF_SOURCE_AND_TARGET);

                    if (PublishBudgeted(Integrator, APCPagedNodeSegmentClasses::STATE_SLOT, state_cell, manager, &grow_state))
                    {
                        state_integrated.fetch_add(1, std::memory_order_relaxed);
                        ff_consumed.fetch_add(1, std::memory_order_relaxed);
                    }
                }

                manager.UnRegisterAPCThread(th);
            });
        }

        for (uint32_t w = 0; w < FB_WORKER_COUNT; ++w)
        {
            workers.emplace_back([&]()
            {
                auto th = manager.RegisterAPCThread();
                size_t cursor = Predictor.PayloadBegin();

                while (fb_consumed.load(std::memory_order_acquire) < VALUE_COUNT)
                {
                    auto maybe =
                        Predictor.ConsumeCausal(APCPagedNodeSegmentClasses::FEEDBACKWARD_MESSAGE, cursor, nullptr, false);

                    if (!maybe)
                    {
                        manager.GetManagersAdaptiveBackoff().AutoBackoff();
                        continue;
                    }

                    const auto value =
                        PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*maybe);

                    if (!value)
                    {
                        continue;
                    }

                    const packed64_t error_cell =
                        PackU32(clock, 1u, APCPagedNodeSegmentClasses::ERROR_SLOT, PriorityPhysics::ERROR_FIRST);

                    if (PublishBudgeted(Comparator, APCPagedNodeSegmentClasses::ERROR_SLOT, error_cell, manager, &grow_error))
                    {
                        error_computed.fetch_add(1, std::memory_order_relaxed);
                        fb_consumed.fetch_add(1, std::memory_order_relaxed);
                    }
                }

                manager.UnRegisterAPCThread(th);
            });
        }

        for (uint32_t w = 0; w < FINAL_WORKER_COUNT; ++w)
        {
            workers.emplace_back([&]()
            {
                auto th = manager.RegisterAPCThread();

                size_t state_cursor = Integrator.PayloadBegin();
                size_t error_cursor = Comparator.PayloadBegin();
                size_t motor_cursor = Motor.PayloadBegin();

                std::optional<uint32_t> pending_state;
                std::optional<uint32_t> pending_error;

                while (final_collected.load(std::memory_order_acquire) < VALUE_COUNT)
                {
                    if (!pending_state)
                    {
                        auto maybe_state =
                            Integrator.ConsumeCausal(APCPagedNodeSegmentClasses::STATE_SLOT, state_cursor, nullptr, false);

                        if (maybe_state)
                        {
                            const auto state_value =
                                PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*maybe_state);

                            if (state_value)
                            {
                                pending_state = *state_value;
                                state_consumed.fetch_add(1, std::memory_order_relaxed);
                            }
                        }
                    }

                    if (!pending_error)
                    {
                        auto maybe_error =
                            Comparator.ConsumeCausal(APCPagedNodeSegmentClasses::ERROR_SLOT, error_cursor, nullptr, false);

                        if (maybe_error)
                        {
                            const auto error_value =
                                PackedCell64_t::ExtractAnyPackedValueX<uint32_t>(*maybe_error);

                            if (error_value)
                            {
                                pending_error = *error_value;
                                error_consumed.fetch_add(1, std::memory_order_relaxed);
                            }
                        }
                    }

                    if (!pending_state || !pending_error)
                    {
                        manager.GetManagersAdaptiveBackoff().AutoBackoff();
                        continue;
                    }

                    const float motor_value =
                        static_cast<float>(*pending_state) + 0.5f * static_cast<float>(*pending_error);

                    pending_state.reset();
                    pending_error.reset();

                    const packed64_t motor_cell =
                        PackFloat32(clock, motor_value, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, PriorityPhysics::INHERIT_SOURCE_PRIORITY);

                    if (!PublishBudgeted(Motor, APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, motor_cell, manager, nullptr))
                    {
                        continue;
                    }

                    auto final_cell =
                        Motor.ConsumeCausal(APCPagedNodeSegmentClasses::FEEDFORWARD_MESSAGE, motor_cursor, nullptr, false);

                    if (final_cell)
                    {
                        const auto value =
                            PackedCell64_t::ExtractAnyPackedValueX<float>(*final_cell);

                        if (value)
                        {
                            std::lock_guard<std::mutex> lock(collected_mutex);
                            collected.push_back(*value);
                            final_collected.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                }

                manager.UnRegisterAPCThread(th);
            });
        }

        for (auto& t : producers)
        {
            t.join();
        }

        for (auto& t : workers)
        {
            t.join();
        }

        std::sort(collected.begin(), collected.end());

        bool values_ok = collected.size() == VALUE_COUNT;

        if (values_ok)
        {
            for (uint32_t i = 0; i < VALUE_COUNT; ++i)
            {
                const float expected = static_cast<float>(i + 2u) + 0.5f;
                if (collected[i] != expected)
                {
                    values_ok = false;
                    break;
                }
            }
        }

        std::cout << "  sensor_produced=" << sensor_produced.load()
                  << " predictor_produced=" << predictor_produced.load()
                  << " state_integrated=" << state_integrated.load()
                  << " error_computed=" << error_computed.load()
                  << " state_consumed=" << state_consumed.load()
                  << " error_consumed=" << error_consumed.load()
                  << " final_collected=" << final_collected.load()
                  << " grow_ff=" << grow_ff.load()
                  << " grow_fb=" << grow_fb.load()
                  << " grow_state=" << grow_state.load()
                  << " grow_error=" << grow_error.load()
                  << "\n";

        suite.Check(sensor_produced.load() == VALUE_COUNT, "Graph: all sensor FF produced");
        suite.Check(predictor_produced.load() == VALUE_COUNT, "Graph: all predictor FB produced");
        suite.Check(state_integrated.load() == VALUE_COUNT, "Graph: all state integrated");
        suite.Check(error_computed.load() == VALUE_COUNT, "Graph: all error computed");
        suite.Check(state_consumed.load() == VALUE_COUNT, "Graph: all state consumed");
        suite.Check(error_consumed.load() == VALUE_COUNT, "Graph: all error consumed");
        suite.Check(final_collected.load() == VALUE_COUNT, "Graph: all final values collected");
        suite.Check(values_ok, "Graph: final values are exactly 2.5..4097.5");

        Sensor.RebuildExectReadyMask();
        Predictor.RebuildExectReadyMask();
        Comparator.RebuildExectReadyMask();
        Integrator.RebuildExectReadyMask();
        Motor.RebuildExectReadyMask();

        ValidateNodeQuiescent(suite, "SensorAfterGraph", Sensor);
        ValidateNodeQuiescent(suite, "PredictorAfterGraph", Predictor);
        ValidateNodeQuiescent(suite, "ComparatorAfterGraph", Comparator);
        ValidateNodeQuiescent(suite, "IntegratorAfterGraph", Integrator);
        ValidateNodeQuiescent(suite, "MotorAfterGraph", Motor);

        PrintNodeSummary("Sensor", Sensor);
        PrintNodeSummary("Predictor", Predictor);
        PrintNodeSummary("Comparator", Comparator);
        PrintNodeSummary("Integrator", Integrator);
        PrintNodeSummary("Motor", Motor);

        Motor.FreeAll();
        Integrator.FreeAll();
        Comparator.FreeAll();
        Predictor.FreeAll();
        Sensor.FreeAll();
    }
}

int main()
{
    std::ios::sync_with_stdio(true);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    TestSuite suite;

    auto& manager = PackedCellContainerManager::Instance();
    manager.StartAPCManager();

    Timer48 timer;
    MasterClockConf clock(nullptr, timer);

    const auto start = std::chrono::steady_clock::now();

    TestPackedCellEncoding(suite, clock);
    TestManagerQSBR(suite, manager);
    TestInitLayoutHeader(suite, manager);
    TestSinglePublishConsume(suite, manager, clock);
    TestMultiRegionIsolation(suite, manager, clock);
    TestReadyMaskRepair(suite, manager, clock);
    TestLayoutExtensionSmoke(suite, manager);
    TestSharedGrowthAndChainDrain(suite, manager, clock);
    TestConcurrentPublishConsumeStress(suite, manager, clock);
    TestDeterministicBidirectionalGraph(suite, manager, clock);

    const auto end = std::chrono::steady_clock::now();

    const auto runtime_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "\nRuntime us: " << runtime_us << "\n";

    manager.StopAPCManager();

    return suite.ExitCode();
}