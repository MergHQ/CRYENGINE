#pragma once
#include "object.h"
#include "SubSelectionPolyMod/PolyMeshSelData.h"

class AbnormalsModLocalData : public PolyMeshSelData
{
	friend class AbnormalsModGroupRestoreObj;
private:
	// Bitarrays for faces we're applying to each group.
	// Note: The reason these are being saved to individual bitarrays instead of a list of integers is because bitarrays are much easier to save and load from file.
	BitArray group1;
	BitArray group2;
	BitArray group3;
	BitArray group4;

	MaxSDK::Array<BitArray*> allGroups; // Array referencing the groups, so they can be accessed by index.

	// True if any groups have been changed by this modifier.
	bool groupsModified;

public:
	AbnormalsModLocalData(MNMesh* mesh);
	AbnormalsModLocalData();
	virtual LocalModData* Clone() override;
private:
	void Init();

public:
	bool GetGroupsModified() { return groupsModified; }

	void SetFacesToGroup(BitArray faces, int group);
	void ClearGroup(int group);
	BitArray GetGroupFaces(int group);
	MaxSDK::Array<BitArray> GetGroups();
};