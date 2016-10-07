#pragma once

#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntitySystem.h>

// IEntityPropertyHandler handler implementation that keeps track of all property values for an entity class and its instances
class CEntityPropertyHandler
	: public IEntityPropertyHandler
	, public IEntitySystemSink
{
public:
	typedef std::unordered_map<int, string> TPropertyStorage;
	typedef std::unordered_map<IEntity *, TPropertyStorage> TEntityPropertyMap;

	CEntityPropertyHandler();
	virtual ~CEntityPropertyHandler();

	// IEntityPropertyHandler
	virtual void GetMemoryUsage(ICrySizer *pSizer) const override { pSizer->AddContainer(m_propertyDefinitions); }
	virtual void RefreshProperties() override {}
	virtual void LoadEntityXMLProperties(IEntity* entity, const XmlNodeRef& xml) override;
	virtual void LoadArchetypeXMLProperties(const char* archetypeName, const XmlNodeRef& xml) override {}
	virtual void InitArchetypeEntity(IEntity* entity, const char* archetypeName, const SEntitySpawnParams& spawnParams) override {}

	virtual int RegisterProperty(const SPropertyInfo& info) override;

	virtual int GetPropertyCount() const override { return m_propertyDefinitions.size(); }

	virtual bool GetPropertyInfo(int index, SPropertyInfo &info) const override
	{
		if (index >= (int)m_propertyDefinitions.size())
			return false;

		info = m_propertyDefinitions[index];
		return true;
	}

	virtual void SetProperty(IEntity* entity, int index, const char* value) override;

	virtual const char* GetProperty(IEntity* entity, int index) const override;

	virtual void PropertiesChanged(IEntity *entity) override;

	virtual uint32 GetScriptFlags() const override { return 0; }
	// -IEntityPropertyHandler

	// IEntitySystemSink
	virtual bool OnBeforeSpawn(SEntitySpawnParams &params) override { return false; }
	virtual void OnSpawn(IEntity *pEntity, SEntitySpawnParams &params) override {}
	virtual void OnReused(IEntity *pEntity, SEntitySpawnParams &params) override {}
	virtual void OnEvent(IEntity *pEntity, SEntityEvent &event) override {}
	virtual bool OnRemove(IEntity *pEntity) override;
	// ~IEntitySystemSink

protected:
	void LoadEntityXMLGroupProperties(TPropertyStorage &entityProperties, const XmlNodeRef &groupNode, bool bRootNode);

	void AddSinkListener();

protected:
	std::vector<SPropertyInfo> m_propertyDefinitions;

	TEntityPropertyMap m_entityPropertyMap;
};