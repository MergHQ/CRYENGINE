#include "stdafx.h"
#include "GeomDecalClassDesc.h"
#include "GeomDecal.h"

GeomDecalClassDesc::GeomDecalClassDesc()
{

}

GeomDecalClassDesc* GeomDecalClassDesc::GetInstance()
{
	// Singleton instance of class desc.
	static GeomDecalClassDesc desc;
	return &desc;
}

//--------------- ClassDesc Interface --------------
int GeomDecalClassDesc::IsPublic()
{
	return TRUE;
}

void* GeomDecalClassDesc::Create(BOOL loading /*= FALSE*/)
{
	return new GeomDecal(loading);
}

const TCHAR* GeomDecalClassDesc::ClassName()
{
	return GetString(IDS_GEOM_DECAL);
}

SClass_ID GeomDecalClassDesc::SuperClassID()
{
	return GEOMOBJECT_CLASS_ID;
}

Class_ID GeomDecalClassDesc::ClassID()
{
	return GEOM_DECAL_CLASS_ID;
}

const TCHAR* GeomDecalClassDesc::Category()
{
	return _T(GEOM_DECAL_CATEGORY);
}

const TCHAR* GeomDecalClassDesc::InternalName()
{
	return _T(GEOM_DECAL_CLASS_NAME); // Important to not use IDS_GEOM_DECAL so this remains constant.
}

HINSTANCE GeomDecalClassDesc::HInstance()
{
	return hInstance;
}