#pragma once


class GeomDecalClassDesc : public ClassDesc2
{
public:
	GeomDecalClassDesc();
	static GeomDecalClassDesc* GetInstance();

	//--------------- ClassDesc Interface --------------
	virtual int IsPublic();
	virtual void* Create(BOOL loading = FALSE);
	virtual const TCHAR* ClassName();
	virtual SClass_ID SuperClassID();
	virtual Class_ID ClassID();
	virtual const TCHAR* Category();
	virtual const TCHAR* InternalName();
	virtual HINSTANCE HInstance();

};