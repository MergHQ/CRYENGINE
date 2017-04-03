#pragma once

#include "MonoDomain.h"
#include <CryMono/IMonoObject.h>

// Wrapped manager of a mono app domain
class CAppDomain final : public CMonoDomain
{
public:
	CAppDomain(char *name, bool bActivate = false);
	CAppDomain(MonoDomain* pMonoDomain);
	virtual ~CAppDomain() {}

	// CMonoDomain
	virtual bool IsRoot() override { return false; }

	virtual bool Reload() override;
	// ~CMonoDomain

	void SerializeObject(CMonoObject* pSerializer, MonoObject* pObject, bool bIsAssembly);
	std::shared_ptr<IMonoObject> DeserializeObject(CMonoObject* pSerializer, bool bIsAssembly);

protected:
	void CreateDomain(char *name, bool bActivate);

	void SerializeDomainData(std::vector<char>& bufferOut);
	void DeserializeDomainData(const std::vector<char>& serializedData);

protected:
	string m_name;

	uint64 m_serializationTicks;
};