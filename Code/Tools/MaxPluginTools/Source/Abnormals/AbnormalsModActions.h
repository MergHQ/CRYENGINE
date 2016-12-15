#pragma once
#include "actiontable.h"

#define ABNORMALS_MOD_ACTION_TABLE_ID ActionTableId(0x62bd70bb)
#define ABNORMALS_MOD_CONTEXT_ID ActionContextId(0x1aded67)

class AbnormalsMod;

class AbnormalsModActionTable : public ActionTable
{
	static AbnormalsModActionTable* instance; // Memory owned and deleted by the action system.
public:
	AbnormalsModActionTable(ActionTableId id, ActionContextId contextId, TSTR& name, HACCEL hDefaults, int numIds, ActionDescription* pOps, HINSTANCE hInst)
		: ActionTable(id, contextId, name, hDefaults, numIds, pOps, hInst) {}
	~AbnormalsModActionTable() {};

	static AbnormalsModActionTable* GetInstance();
	BOOL IsChecked(int cmdId) override { return false; }	
};

class AbnormalModActionCallback : public ActionCallback
{
	AbnormalsMod *abnormalsMod;
public:
	AbnormalModActionCallback(AbnormalsMod *m) : abnormalsMod(m) { }
	virtual BOOL ExecuteAction(int id) override;
};