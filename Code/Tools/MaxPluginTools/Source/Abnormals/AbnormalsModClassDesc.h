#pragma once


class AbnormalsModClassDesc : public ClassDesc2
{
public:
	AbnormalsModClassDesc();
	static AbnormalsModClassDesc* GetInstance();

	//--------------- ClassDesc Interface --------------
	virtual int IsPublic();
	virtual void* Create(BOOL loading = FALSE);
	virtual const TCHAR* ClassName();
	virtual SClass_ID SuperClassID();
	virtual Class_ID ClassID();
	virtual const TCHAR* Category();
	virtual const TCHAR* InternalName();
	virtual HINSTANCE HInstance();
	virtual int NumActionTables();
	virtual ActionTable* GetActionTable(int i);

};