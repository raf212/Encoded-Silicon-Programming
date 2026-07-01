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
            HeaderOrchestrator::LayoutBufferOfAPC& a_layout_buffer,
            bool is_claimed_required = false
        ) noexcept;

        bool WriteCompleateLayoutToAPCFromBuffer() noexcept;

        packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept;
    };
    
    
}