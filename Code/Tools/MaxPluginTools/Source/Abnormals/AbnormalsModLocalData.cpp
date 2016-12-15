#include "stdafx.h"
#include "AbnormalsModLocalData.h"

AbnormalsModLocalData::AbnormalsModLocalData(MNMesh* mesh) : PolyMeshSelData(mesh)
{
	Init();
}

AbnormalsModLocalData::AbnormalsModLocalData() : PolyMeshSelData()
{
	Init();
}

LocalModData* AbnormalsModLocalData::Clone()
{
	AbnormalsModLocalData *d = new AbnormalsModLocalData(mesh);
	// Clone PolyMeshSelData first
	d->vertSel = vertSel;
	d->faceSel = faceSel;
	d->edgeSel = edgeSel;
	d->vselSet = vselSet;
	d->eselSet = eselSet;
	d->fselSet = fselSet;
	// Then clone AbnormalsModLocalData
	d->group1 = group1;
	d->group2 = group2;
	d->group3 = group3;
	d->group4 = group4;
	d->groupsModified = groupsModified;
	return d;
}

void AbnormalsModLocalData::Init()
{
	groupsModified = false;
	allGroups.removeAll(); // Clear the all groups array, just incase.
	allGroups.append(&group1);
	allGroups.append(&group2);
	allGroups.append(&group3);
	allGroups.append(&group4);
}

void AbnormalsModLocalData::SetFacesToGroup(BitArray faces, int group)
{
	// Make sure groups are all at least the size of the faces array
	for (int g = 0; g < allGroups.length(); g++)
	{
		if (allGroups[g]->GetSize() < faces.GetSize())
		{
			allGroups[g]->SetSize(faces.GetSize(), true);
		}
	}

	// Set the faces specified to true in the specified group, and false in all other groups
	for (int g = 0; g < allGroups.length(); g++)
	{
		for (int f = 0; f < faces.GetSize(); f++)
		{
			if (faces[f])
			{
				if (g == group)
				{
					groupsModified = true;
					allGroups[g]->Set(f, true);
				}
				else
					allGroups[g]->Set(f, false);
			}
		}
	}
}

void AbnormalsModLocalData::ClearGroup(int group)
{
	allGroups[group]->ClearAll();
}

BitArray AbnormalsModLocalData::GetGroupFaces(int group)
{
	if (group < 0 || group >= allGroups.length())
		return *allGroups[allGroups.length() - 1]; // If specified group is out of range, return last element (lowest priority).
	else
		return *allGroups[group];
}

MaxSDK::Array<BitArray> AbnormalsModLocalData::GetGroups()
{
	MaxSDK::Array<BitArray> retArray;
	for (int i = 0; i < allGroups.length(); i++)
		retArray.append(*allGroups[i]);
	return retArray;
}