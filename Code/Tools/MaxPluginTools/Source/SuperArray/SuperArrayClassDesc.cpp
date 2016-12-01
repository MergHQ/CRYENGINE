#include "stdafx.h"
#include "SuperArrayClassDesc.h"
#include "SuperArray.h"


SuperArrayClassDesc::SuperArrayClassDesc()
{

}

SuperArrayClassDesc* SuperArrayClassDesc::GetInstance()
{
	// Singleton instance of class desc.
	static SuperArrayClassDesc desc;
	return &desc;
}

//--------------- ClassDesc Interface --------------
int SuperArrayClassDesc::IsPublic()
{
	return TRUE;
}

void* SuperArrayClassDesc::Create(BOOL loading /*= FALSE*/)
{
	return new SuperArray(loading);
}

const TCHAR* SuperArrayClassDesc::ClassName()
{
	return GetString(IDS_SUPERARRAY);
}

SClass_ID SuperArrayClassDesc::SuperClassID()
{
	return GEOMOBJECT_CLASS_ID;
}

Class_ID SuperArrayClassDesc::ClassID()
{
	return SUPER_ARRAY_CLASS_ID;
}

const TCHAR* SuperArrayClassDesc::Category()
{
	return _T(SUPER_ARRAY_CATEGORY);
}

const TCHAR* SuperArrayClassDesc::InternalName()
{
	return _T(SUPER_ARRAY_CLASS_NAME); // Important to not use IDS_SUPERARRAY so this remains constant.
}

HINSTANCE SuperArrayClassDesc::HInstance()
{
	return hInstance;
}