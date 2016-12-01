#include "stdafx.h"
#include "AbnormalsModRestoreObj.h"

AbnormalsModGroupRestoreObj::AbnormalsModGroupRestoreObj(AbnormalsMod *m, AbnormalsModLocalData *d, int aGroup)
{
	abnormalsMod = m;
	abnormalsModLocalData = d;
	d->held = TRUE;
	group = aGroup;
	faces = abnormalsModLocalData->GetGroups();
	hasOldData = true;
}

void AbnormalsModGroupRestoreObj::Restore(int isUndo) 
{
	if (hasOldData)// Apply the old data, and save the new data.
	{
		MaxSDK::Array<BitArray> newFaces = abnormalsModLocalData->GetGroups();		
		abnormalsModLocalData->ClearGroup(group);

		// Copy bitarrays over.
		for (int i = 0; i < faces.length(); i++)
			*abnormalsModLocalData->allGroups[i] = faces[i];

		abnormalsMod->LocalDataChanged();
		faces = newFaces;
		hasOldData = false;
	}
}

void AbnormalsModGroupRestoreObj::Redo()
{
	if (!hasOldData)// Apply the old data, and save the new data.
	{
		MaxSDK::Array<BitArray> oldFaces = abnormalsModLocalData->GetGroups();
		abnormalsModLocalData->ClearGroup(group);

		// Copy bitarrays over.
		for (int i = 0; i < faces.length(); i++)
			*abnormalsModLocalData->allGroups[i] = faces[i];

		abnormalsMod->LocalDataChanged();
		faces = oldFaces;
		hasOldData = true;
	}
}