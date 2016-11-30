#include "stdafx.h"
#include "AbnormalsModClassDesc.h"
#include "AbnormalsMod.h"
#include "AbnormalsModActions.h"

AbnormalsModClassDesc::AbnormalsModClassDesc()
{

}

AbnormalsModClassDesc* AbnormalsModClassDesc::GetInstance()
{
	// Singleton instance of class desc.
	static AbnormalsModClassDesc desc;
	return &desc;
}

//--------------- ClassDesc Interface --------------
int AbnormalsModClassDesc::IsPublic()
{
	return TRUE;
}

void* AbnormalsModClassDesc::Create(BOOL loading /*= FALSE*/)
{
	return new AbnormalsMod();
}

const TCHAR* AbnormalsModClassDesc::ClassName()
{
	return GetString(IDS_ABNORMALSMOD);
}

SClass_ID AbnormalsModClassDesc::SuperClassID()
{
	return OSM_CLASS_ID;
}

Class_ID AbnormalsModClassDesc::ClassID()
{
	return ABNORMALS_MOD_CLASS_ID;
}

const TCHAR* AbnormalsModClassDesc::Category()
{
	return _T(ABNORMALS_MOD_CATEGORY);
}

const TCHAR* AbnormalsModClassDesc::InternalName()
{
	return _T(ABNORMALS_MOD_CLASS_NAME);
}

HINSTANCE AbnormalsModClassDesc::HInstance()
{
	return hInstance;
}

int AbnormalsModClassDesc::NumActionTables()
{ 
	return 1; 
}

ActionTable* AbnormalsModClassDesc::GetActionTable(int i)
{
	return AbnormalsModActionTable::GetInstance();
}