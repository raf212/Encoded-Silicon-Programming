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

        bool ReadCompleateMetaHeaderDirectlyNonAtomic_(APCDataStructure::APCMetaBuffer& a_default_buffer) noexcept;

        packed64_t ReadFullMetaCell(MetaIndexOfAPCNode idx) noexcept
        {
            if (ValidMetaIdx(idx))
            {
                return BackingPtr[static_cast<size_t>(idx)].load(MoLoad_);
            }
            return PACKED_CELL_SENTENAL;
        }

    };
    
    
}