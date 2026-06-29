#include "AdaptivePackedCellContainer/AdaptivePackedCellContainer.hpp"
#include "NeuromorphicTimeSpace/VagueTemoraryPremativeFabric.hpp"

namespace PredictedAdaptedEncoding
{

    bool ReadAndWriteOfAPC::ReadCompleateMetaHeaderDirectlyNonAtomic_(HeaderOrchestrator::APCMetaBuffer& a_default_buffer) noexcept
    {
        APCSegmentPoolRange range_of_this_apc{};
        if (!IsThisAPCValidRange_(APCDataStructure::METACELL_COUNT, &range_of_this_apc))
        {
            return false;
        }

        if (!CopyFromAPCToANArrayBuffer(UNSIGNED_ZERO, APCDataStructure::METACELL_COUNT, a_default_buffer))
        {
            return false;
        }
        
        return true;
    }


}