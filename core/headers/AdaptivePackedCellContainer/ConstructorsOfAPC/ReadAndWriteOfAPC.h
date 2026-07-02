#pragma once
#include <functional>
#include "FabricToAPCLinker.hpp"

namespace PredictedAdaptedEncoding
{
    class ReadAndWriteOfAPC : public FabricToAPCLinker
    {

    private:
        /* data */
    public:

        bool ReadCompleateMetaHeaderDirectlyNonAtomic(HeaderOrchestrator::APCMetaBuffer& a_default_buffer) noexcept;

        bool ReadCompleatLayoutBuffer(
            LayoutBoundsOrchestrator::LayoutBufferOfAPC& a_layout_buffer,
            bool is_claimed_required = false
        ) noexcept;

        bool UpdateCompleateLayoutOfAPCFromBuffer(
            const LayoutBoundsOrchestrator::LayoutBufferOfAPC& a_valid_layout_buffer,
            bool caller_holds_the_flag = false
        ) noexcept;

        packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept;
    };
    
    
}