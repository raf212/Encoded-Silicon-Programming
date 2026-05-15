// APC-native policy:
// dead -> can stay logically removed from registry, physical free happens by owner
// reclaim requested -> only clear reclaim bit after a grace-period boundary

IDEA:
Peimarily there will be 2->16x3=48->MODE48 model one for occupancy onr for region bounds with version 
//BEFORE fixing initilization version number to > 0 && version < UINT16_MAX-1 the APC will have deadlock


///
change note
1. DoesPublishedCellContributeToRegionOccupancy -> IsThisCellAppropriateAndGenericToConsume
2. IsPublishedDataCellForRegion -> IsCellAppropriatelyPagedAndPublishedAsGeneric
3. InspectPackedCell -> GetAuthoritiveViewsForACell