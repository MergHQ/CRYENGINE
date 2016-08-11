#pragma once

class CGameEntityNodeFactory;

// Helper struct to allow for default values in native properties
struct SNativeEntityPropertyInfo
{
	IEntityPropertyHandler::SPropertyInfo info;
	
	const char *defaultValue;
};

#include "Entities/Helpers/NativeEntityPropertyHandling.h"

class CGameFactory
{
	template<class T>
	struct CObjectCreator : public IGameObjectExtensionCreatorBase
	{
		IGameObjectExtensionPtr Create()
		{
			return ComponentCreate_DeleteWithRelease<T>();
		}

		void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
		{
			T::GetGameObjectExtensionRMIData(ppRMI, nCount);
		}
	};

public:
	enum eGameObjectRegistrationFlags
	{
		eGORF_None								= 0x0,
		eGORF_HiddenInEditor			= 0x1,
		eGORF_NoEntityClass				= 0x2,
	};

	static void Init();
	static void RegisterFlowNodes();

	template<class T>
	static void RegisterGameObject(const char *name, uint32 flags = 0)
	{
		bool registerClass = ((flags & eGORF_NoEntityClass) == 0);
		IEntityClassRegistry::SEntityClassDesc clsDesc;
		clsDesc.sName = name;
	
		static CObjectCreator<T> _creator;

		gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension(name, &_creator, registerClass ? &clsDesc : nullptr);
		T::SetExtensionId(gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->GetID(name));

		if ((flags & eGORF_HiddenInEditor) != 0)
		{ 
			IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE); 
		}
	}

	template<class T>
	static void RegisterGameObjectExtension(const char *name)
	{
		RegisterGameObject<T>(name, eGORF_NoEntityClass);
	}

	template<class T>
	static void RegisterNativeEntity(const char *name, const char *path, const char *editorIcon = "", uint32 flags = 0, SNativeEntityPropertyInfo *pProperties = nullptr, uint32 numProperties = 0)
	{
		const bool registerClass = ((flags & eGORF_NoEntityClass) == 0);
		IEntityClassRegistry::SEntityClassDesc clsDesc;
		clsDesc.sName = name;

		clsDesc.editorClassInfo.sCategory = path;
		clsDesc.editorClassInfo.sIcon = editorIcon;

		if(registerClass && numProperties > 0)
		{
			clsDesc.pPropertyHandler = new CNativeEntityPropertyHandler(pProperties, numProperties, 0);
		}

		static CObjectCreator<T> _creator;

		gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension(name, &_creator, registerClass ? &clsDesc : nullptr);
		T::SetExtensionId(gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem()->GetID(name));

		if ((flags & eGORF_HiddenInEditor) != 0)
		{ 
			IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			pEntityClass->SetFlags(pEntityClass->GetFlags() | ECLF_INVISIBLE); 
		}
	}
	
	static CGameEntityNodeFactory &RegisterEntityFlowNode(const char *className);

private:
	static std::map<string, CGameEntityNodeFactory*> s_flowNodeFactories;
};

struct IEntityRegistrator
{
	IEntityRegistrator()
	{
		if(g_pFirst == nullptr)
		{
			g_pFirst = this;
			g_pLast = this;
		}
		else
		{
			g_pLast->m_pNext = this;
			g_pLast = g_pLast->m_pNext;
		}
	}

	virtual void Register() = 0;

public:
	IEntityRegistrator *m_pNext;

	static IEntityRegistrator *g_pFirst;
	static IEntityRegistrator *g_pLast;
};

////////////////////////////////////////
// Property registration helpers
////////////////////////////////////////

// Used to aid registration of property groups
// Example use case:
// {
//		ENTITY_PROPERTY_GROUP("MyGroup", groupIndexStart, groupIndexEnd, pProperties);

//		ENTITY_PROPERTY(string, properties, MyProperty, "DefaultValue", "Description", 0, 1);
// }
struct SEditorPropertyGroup
{
	SEditorPropertyGroup(const char *name, int indexBegin, int indexEnd, SNativeEntityPropertyInfo *pProperties)
	: m_name(name)
	, m_indexEnd(indexEnd)
	, m_pProperties(pProperties)
	{
		pProperties[indexBegin].info.name = name;
		pProperties[indexBegin].info.type = IEntityPropertyHandler::FolderBegin;
	}

	~SEditorPropertyGroup()
	{
		m_pProperties[m_indexEnd].info.name = m_name;
		m_pProperties[m_indexEnd].info.type = IEntityPropertyHandler::FolderEnd;
	}

protected:
	string m_name;

	int m_indexEnd;
	SNativeEntityPropertyInfo *m_pProperties;
};

#define ENTITY_PROPERTY_GROUP(name, begin, end, properties) SEditorPropertyGroup group = SEditorPropertyGroup(name, begin, end, properties);

// Entity registration helpers
template <typename T>
inline void RegisterEntityProperty(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	CRY_ASSERT_MESSAGE(false, "Entity property of invalid type was not registered");
}

template <>
inline void RegisterEntityProperty<Vec3>(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::Vector;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "";
	pProperties[index].info.flags = 0;
	pProperties[index].info.limits.min = min;
	pProperties[index].info.limits.max = max;
}

template <>
inline void RegisterEntityProperty<ColorF>(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::Vector;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "color";
	pProperties[index].info.flags = 0;
	pProperties[index].info.limits.min = min;
	pProperties[index].info.limits.max = max;
}

template <>
inline void RegisterEntityProperty<float>(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::Float;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "";
	pProperties[index].info.flags = 0;
	pProperties[index].info.limits.min = min;
	pProperties[index].info.limits.max = max;
}

template <>
inline void RegisterEntityProperty<int>(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::Int;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "";
	pProperties[index].info.flags = 0;
	pProperties[index].info.limits.min = min;
	pProperties[index].info.limits.max = max;
}

template <>
inline void RegisterEntityProperty<bool>(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::Bool;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "b";
	pProperties[index].info.flags = 0;
	pProperties[index].info.limits.min = 0;
	pProperties[index].info.limits.max = 1;
}

template <>
inline void RegisterEntityProperty<string>(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::String;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "";
	pProperties[index].info.flags = 0;
	pProperties[index].info.limits.min = min;
	pProperties[index].info.limits.max = max;
}

inline void RegisterEntityPropertyTexture(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::String;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "tex";
	pProperties[index].info.flags = 0;
}

inline void RegisterEntityPropertyFlare(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::String;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "flare_";
	pProperties[index].info.flags = 0;
}

inline void RegisterEntityPropertyObject(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::String;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "obj";
	pProperties[index].info.flags = 0;
}

inline void RegisterEntityPropertyMaterial(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::String;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "material";
	pProperties[index].info.flags = 0;
}

inline void RegisterEntityPropertyEnum(SNativeEntityPropertyInfo *pProperties, int index, const char *name, const char *defaultVal, const char *desc, float min, float max)
{
	pProperties[index].info.name = name;
	pProperties[index].defaultValue = defaultVal;
	pProperties[index].info.type = IEntityPropertyHandler::String;
	pProperties[index].info.description = desc;
	pProperties[index].info.editType = "";
	pProperties[index].info.flags = IEntityPropertyHandler::ePropertyFlag_UIEnum | IEntityPropertyHandler::ePropertyFlag_Unsorted;
	pProperties[index].info.limits.min = min;
	pProperties[index].info.limits.max = max;
}

#define ENTITY_PROPERTY(type, properties, name, defaultVal, desc, min, max) RegisterEntityProperty<type>(properties, eProperty_##name, #name, defaultVal, desc, min, max);