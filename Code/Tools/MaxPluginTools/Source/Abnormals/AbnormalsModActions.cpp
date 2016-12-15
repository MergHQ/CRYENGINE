#include "stdafx.h"
#include "AbnormalsModActions.h"
#include "AbnormalsMod.h"

#include <WinUser.h>

static ActionDescription abnormalsActions[] =
{
	// Id, description, short name, category
	ID_ABNORMALS_ASSIGN_GROUP1,IDS_ACTION_GROUP1,IDS_ACTION_GROUP1,IDS_ABNORMALSMOD,
	ID_ABNORMALS_ASSIGN_GROUP2,IDS_ACTION_GROUP2,IDS_ACTION_GROUP2,IDS_ABNORMALSMOD,
	ID_ABNORMALS_ASSIGN_GROUP3,IDS_ACTION_GROUP3,IDS_ACTION_GROUP3,IDS_ABNORMALSMOD,
	ID_ABNORMALS_ASSIGN_GROUP4,IDS_ACTION_GROUP4,IDS_ACTION_GROUP4,IDS_ABNORMALSMOD,
};

AbnormalsModActionTable* AbnormalsModActionTable::instance = NULL;

AbnormalsModActionTable* AbnormalsModActionTable::GetInstance()
{
	if (instance == NULL)
	{
		TSTR name = GetString(IDS_ABNORMALSMOD);
		HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL_ABNORMALS));
		int numOps = NUM_ELEMENTS(abnormalsActions);
		instance = new AbnormalsModActionTable(ABNORMALS_MOD_ACTION_TABLE_ID, ABNORMALS_MOD_CONTEXT_ID, name, hAccel, numOps, abnormalsActions, hInstance);
		GetCOREInterface()->GetActionManager()->RegisterActionContext(ABNORMALS_MOD_CONTEXT_ID, name.data());
	}
	return instance;
}

BOOL AbnormalModActionCallback::ExecuteAction(int id)
{
	if (abnormalsMod->GetCurrentSubSelectionType() != SubSelectionPolyMod::SEL_FACE && abnormalsMod->GetCurrentSubSelectionType() != SubSelectionPolyMod::SEL_ELEMENT)
		return FALSE; // Don't do any of these actions while not in face or element selection

	switch (id)
	{
	case ID_ABNORMALS_ASSIGN_GROUP1:
		abnormalsMod->AssignGroupToSelectedFaces(0, true);
		return TRUE;
	case ID_ABNORMALS_ASSIGN_GROUP2:
		abnormalsMod->AssignGroupToSelectedFaces(1, true);
		return TRUE;
	case ID_ABNORMALS_ASSIGN_GROUP3:
		abnormalsMod->AssignGroupToSelectedFaces(2, true);
		return TRUE;
	case ID_ABNORMALS_ASSIGN_GROUP4:
		abnormalsMod->AssignGroupToSelectedFaces(3, true);
		return TRUE;
	default:
		return FALSE;
	}
}