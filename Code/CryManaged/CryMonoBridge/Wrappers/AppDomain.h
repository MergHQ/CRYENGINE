#pragma once

#include "MonoDomain.h"
#include <CryMono/IMonoObject.h>

// Wrapped manager of a mono app domain
class CAppDomain : public CMonoDomain
{
public:
	CAppDomain(char *name, bool bActivate = false);
	CAppDomain(MonoDomain* pMonoDomain);
	virtual ~CAppDomain() {}

	// CMonoDomain
	virtual void Release() override { delete this; }

	virtual bool IsRoot() override { return false; }

	virtual bool Reload() override;
	// ~CMonoDomain

	void Serialize(MonoObject* pObject, bool bIsAssembly);
	std::shared_ptr<IMonoObject> Deserialize(bool bIsAssembly);

protected:
	void CreateDomain(char *name, bool bActivate);

	void CreateSerializationUtilities(bool bWriting);

	void SerializeDomainData();
	void DeserializeDomainData();

protected:
	string m_name;

	std::shared_ptr<IMonoObject> m_pMemoryStream;
	std::shared_ptr<IMonoObject> m_pSerializer;

	char* m_pSerializedData;
	size_t m_serializedDataSize;

	uint64 m_serializationTicks;
};