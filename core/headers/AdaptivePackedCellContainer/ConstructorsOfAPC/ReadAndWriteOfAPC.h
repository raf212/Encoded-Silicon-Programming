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

        bool ReadCompleateMetaHeaderDirectlyNonAtomic_(HeaderOrchestrator::APCMetaBuffer& a_default_buffer) noexcept;

        bool ReadCompleatLayoutBuffer_(
            HeaderOrchestrator::LayoutBufferOfAPC& a_layout_buffer,
            bool is_claimed_required = false
        ) noexcept;

        packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept;
    };
    
    
}