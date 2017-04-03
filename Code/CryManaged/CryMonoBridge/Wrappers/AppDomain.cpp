#include "StdAfx.h"
#include "AppDomain.h"

#include "MonoRuntime.h"
#include "MonoLibrary.h"
#include "MonoClass.h"

#include "RootMonoDomain.h"

#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>

#include <CrySystem/IProjectManager.h>

CAppDomain::CAppDomain(char *name, bool bActivate)
	: m_name(name)
{
	CreateDomain(name, bActivate);

	m_bNativeDomain = true;
}

CAppDomain::CAppDomain(MonoDomain* pMonoDomain)
{
	m_pDomain = pMonoDomain;

#ifndef _RELEASE
	mono_debug_domain_create(m_pDomain);
#endif

	m_bNativeDomain = false;
}

void CAppDomain::CreateDomain(char *name, bool bActivate)
{
	m_pDomain = mono_domain_create_appdomain(name, nullptr);

	char baseDirectory[_MAX_PATH];
	CryGetExecutableFolder(CRY_ARRAY_COUNT(baseDirectory), baseDirectory);

	string sConfigFile;
	sConfigFile.Format("%smono\\etc\\mono\\4.5\\machine.config", baseDirectory);

	char szEngineBinaryDir[_MAX_PATH];
	CryGetExecutableFolder(CRY_ARRAY_COUNT(szEngineBinaryDir), szEngineBinaryDir);

	mono_domain_set_config(m_pDomain, szEngineBinaryDir, sConfigFile);

	if (bActivate)
	{
		Activate();
	}

#ifndef _RELEASE
	mono_debug_domain_create(m_pDomain);
#endif
}

bool CAppDomain::Reload()
{
	// Make note of whether we were active or not, so we can reapply state to the new domain
	bool bWasActive = IsActive();

	std::vector<char> serializedData;

	// Serialize statics and objects in the assemblies
	SerializeDomainData(serializedData);

	// Unload assemblies
	for (auto it = m_loadedLibraries.rbegin(); it != m_loadedLibraries.rend(); ++it)
	{
		it->get()->Unload();
	}

	// Kill the domain
	Unload();

	// Now start recreating the domain
	// Copy the name, Mono doesn't actually touch the char array in memory but let's be safe.
	string name = m_name;
	CreateDomain(const_cast<char*>(name.c_str()), bWasActive);

	// Reload assemblies
	for (const std::unique_ptr<CMonoLibrary>& pLibrary : m_loadedLibraries)
	{
		pLibrary->Reload();
	}

	DeserializeDomainData(serializedData);

	return true;
}

void CAppDomain::SerializeDomainData(std::vector<char>& serializedData)
{
	// Create the shared serialization utilities that may be called by libraries and objects
	// Now serialize the statics contained inside this library
	auto* pNetCoreLibrary = static_cast<CRootMonoDomain*>(GetMonoRuntime()->GetRootDomain())->GetNetCoreLibrary();

	auto pMemoryStreamClass = pNetCoreLibrary->GetTemporaryClass("System.IO", "MemoryStream");
	std::shared_ptr<CMonoObject> pMemoryStream = std::static_pointer_cast<CMonoObject>(pMemoryStreamClass->CreateInstance());

	m_serializationTicks = 0;

	auto pCryObjectWriterClass = GetMonoRuntime()->GetCryCoreLibrary()->GetTemporaryClass("CryEngine.Serialization", "ObjectWriter");

	void* writerConstructorArgs[1];
	writerConstructorArgs[0] = pMemoryStream->GetHandle();

	std::shared_ptr<CMonoObject> pSerializer = std::static_pointer_cast<CMonoObject>(pCryObjectWriterClass->CreateInstance(writerConstructorArgs, 1));

	for (const std::unique_ptr<CMonoLibrary>& pLibrary : m_loadedLibraries)
	{
		pLibrary->Serialize(pSerializer.get());
	}

	// Grab the serialized buffer
	auto pManagedBuffer = pMemoryStream->GetClass()->FindMethod("GetBuffer")->Invoke(pMemoryStream.get());

	serializedData.resize(pManagedBuffer->GetArraySize());
	char* arrayBuffer = pManagedBuffer->GetArrayAddress(sizeof(char), 0);

	memcpy(serializedData.data(), arrayBuffer, serializedData.size());

	CryLogAlways("Total serialization time: %f ms, %iMB used", (1000.f * gEnv->pTimer->TicksToSeconds(m_serializationTicks)), serializedData.size() / (1024 * 1024));
}

void CAppDomain::DeserializeDomainData(const std::vector<char>& serializedData)
{
	// Now serialize the statics contained inside this library
	auto* pNetCoreLibrary = static_cast<CRootMonoDomain*>(GetMonoRuntime()->GetRootDomain())->GetNetCoreLibrary();

	m_serializationTicks = 0;

	auto pCryObjectReaderClass = GetMonoRuntime()->GetCryCoreLibrary()->GetTemporaryClass("CryEngine.Serialization", "ObjectReader");

	auto* pDomain = static_cast<CMonoDomain*>(pCryObjectReaderClass->GetAssembly()->GetDomain());

	auto* pMonoManagedBuffer = (MonoObject*)mono_array_new((MonoDomain*)pDomain->GetHandle(), mono_get_byte_class(), serializedData.size());
	auto pBufferClass = pNetCoreLibrary->GetClassFromMonoClass(mono_object_get_class(pMonoManagedBuffer));

	auto pManagedBuffer = std::make_shared<CMonoObject>(pMonoManagedBuffer, pBufferClass);
	char* arrayBuffer = pManagedBuffer->GetArrayAddress(sizeof(char), 0);

	memcpy(arrayBuffer, serializedData.data(), serializedData.size());

	void* memoryStreamCtorArgs[2];
	memoryStreamCtorArgs[0] = pMonoManagedBuffer;

	bool bWritable = false;
	MonoObject* pBoolObject = mono_value_box((MonoDomain*)pDomain->GetHandle(), mono_get_boolean_class(), &bWritable);
	memoryStreamCtorArgs[1] = pBoolObject;

	auto pMemoryStreamClass = pNetCoreLibrary->GetTemporaryClass("System.IO", "MemoryStream");
	std::shared_ptr<CMonoObject> pMemoryStream = std::static_pointer_cast<CMonoObject>(pMemoryStreamClass->CreateInstance(memoryStreamCtorArgs, 2));

	void* readerConstructorParams[1];
	readerConstructorParams[0] = pMemoryStream->GetHandle();

	std::shared_ptr<CMonoObject> pSerializer = std::static_pointer_cast<CMonoObject>(pCryObjectReaderClass->CreateInstance(readerConstructorParams, 1));

	for (const std::unique_ptr<CMonoLibrary>& pLibrary : m_loadedLibraries)
	{
		pLibrary->Deserialize(pSerializer.get());
	}

	CryLogAlways("Total de-serialization time: %f ms", (1000.f * gEnv->pTimer->TicksToSeconds(m_serializationTicks)));
}

void CAppDomain::SerializeObject(CMonoObject* pSerializer, MonoObject* pObject, bool bIsAssembly)
{
	auto startTime = CryGetTicks();

	void* writeParams[1];
	writeParams[0] = pObject;

	pSerializer->GetClass()->FindMethod(bIsAssembly ? "WriteStatics" : "Write", 1)->Invoke(pSerializer, writeParams);

	m_serializationTicks += CryGetTicks() - startTime;
}

std::shared_ptr<IMonoObject> CAppDomain::DeserializeObject(CMonoObject* pSerializer, bool bIsAssembly)
{
	auto startTime = CryGetTicks();

	// Now serialize the statics contained inside this library
	auto pReadResult = pSerializer->GetClass()->FindMethod(bIsAssembly ? "ReadStatics" : "Read")->Invoke(pSerializer);

	m_serializationTicks += CryGetTicks() - startTime;

	return pReadResult;
}