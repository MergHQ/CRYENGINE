#pragma once

#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntitySystem.h>

// IEntityPropertyHandler handler implementation that keeps track of all property values in all instances of an entity class
class CNativeEntityPropertyHandler
	: public IEntityPropertyHandler
	, public IEntitySystemSink
{
public:
	typedef std::unordered_map<int, string> TPropertyStorage;
	typedef std::unordered_map<IEntity *, TPropertyStorage> TEntityPropertyMap;

	CNativeEntityPropertyHandler(SNativeEntityPropertyInfo *pProperties, int numProperties, uint32 scriptFlags);
	virtual ~CNativeEntityPropertyHandler();

	// IEntityPropertyHandler
	virtual void GetMemoryUsage(ICrySizer *pSizer) const override { pSizer->Add(m_pProperties); }
	virtual void RefreshProperties() override {}
	virtual void LoadEntityXMLProperties(IEntity* entity, const XmlNodeRef& xml) override;
	virtual void LoadArchetypeXMLProperties(const char* archetypeName, const XmlNodeRef& xml) override {}
	virtual void InitArchetypeEntity(IEntity* entity, const char* archetypeName, const SEntitySpawnParams& spawnParams) override {}

	virtual int GetPropertyCount() const override { return m_numProperties; }

	virtual bool GetPropertyInfo(int index, SPropertyInfo &info) const override
	{
		if (index >= m_numProperties)
			return false;

		info = m_pProperties[index].info;
		return true;
	}

	virtual void SetProperty(IEntity* entity, int index, const char* value) override;

	virtual const char* GetProperty(IEntity* entity, int index) const override;

	virtual const char* GetDefaultProperty(int index) const override
	{
		if (index >= m_numProperties)
			return "";

		return m_pProperties[index].defaultValue;
	}

	virtual void PropertiesChanged(IEntity *entity) override;

	virtual uint32 GetScriptFlags() const override { return m_scriptFlags; }
	// -IEntityPropertyHandler

	// IEntitySystemSink
	virtual bool OnBeforeSpawn(SEntitySpawnParams &params) override { return false; }
	virtual void OnSpawn(IEntity *pEntity, SEntitySpawnParams &params) override {}
	virtual void OnReused(IEntity *pEntity, SEntitySpawnParams &params) override {}
	virtual void OnEvent(IEntity *pEntity, SEntityEvent &event) override {}
	virtual bool OnRemove(IEntity *pEntity) override;
	// ~IEntitySystemSink

	void LoadEntityXMLGroupProperties(TPropertyStorage &entityProperties, const XmlNodeRef &groupNode, bool bRootNode);

protected:
	SNativeEntityPropertyInfo *m_pProperties;
	int m_numProperties;

	int m_scriptFlags;

	TEntityPropertyMap m_entityPropertyMap;
};