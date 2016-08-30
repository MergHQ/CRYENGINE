// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once


class CGameEntityNodeFactory;


struct SEntityScriptProperties
{
	SEntityScriptProperties()
		: pEntityTable(nullptr)
		, pEditorTable(nullptr)
		, pPropertiesTable(nullptr)
		,	pInstancePropertiesTable(nullptr)
	{
	}

	IScriptTable* pEntityTable;
	IScriptTable* pEditorTable;	
	IScriptTable* pPropertiesTable;
	IScriptTable* pInstancePropertiesTable;
};

class CGameFactory
{
public:
	static void Init();
	static void RegisterEntityFlowNodes();

private:
	enum eGameObjectRegistrationFlags
	{
		eGORF_None								= 0x0,
		eGORF_HiddenInEditor			= 0x1,
		eGORF_NoEntityClass				= 0x2,
		eGORF_InstanceProperties	= 0x4,
	};

	template<class T>
	static void RegisterGameObject(const string& name, const string& script, uint32 flags = 0);
	template<class T>
	static void RegisterNoScriptGameObject(const string& name, const string& path, uint32 flags = 0);
	static void CreateScriptTables(SEntityScriptProperties& out, uint32 flags);
	
	static std::map<string, CGameEntityNodeFactory*> s_flowNodeFactories;
};
