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

	m_bNativeAssembly = true;
}

CAppDomain::CAppDomain(MonoDomain* pMonoDomain)
{
	m_pDomain = pMonoDomain;

#ifndef _RELEASE
	mono_debug_domain_create(m_pDomain);
#endif

	m_bNativeAssembly = false;
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

	// Serialize statics and objects in the assemblies
	SerializeDomainData();

	// Unload assemblies
	for (auto it = m_loadedLibraries.rbegin(); it != m_loadedLibraries.rend(); ++it)
	{
		it->pLibrary->Unload();
	}

	// Kill the domain
	Unload();

	// Now start recreating the domain
	char *name = new char[m_name.size() + 1];
	cry_strcpy(name, m_name.size() + 1, m_name.c_str());

	CreateDomain(name, bWasActive);
	delete[] name;

	// Reload assemblies
	for (auto it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		it->pLibrary->Reload();
	}

	DeserializeDomainData();

	return true;
}

void CAppDomain::CreateSerializationUtilities(bool bWriting)
{
	// Now serialize the statics contained inside this library
	auto* pNetCoreLibrary = static_cast<CRootMonoDomain*>(gEnv->pMonoRuntime->GetRootDomain())->GetNetCoreLibrary();

	auto pMemoryStreamClass = pNetCoreLibrary->GetTemporaryClass("System.IO", "MemoryStream");
	m_pMemoryStream = pMemoryStreamClass->CreateInstance();

	m_serializationTicks = 0;

	if (bWriting)
	{
		auto pCryObjectWriterClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetTemporaryClass("CryEngine.Serialization", "ObjectWriter");

		void* writerConstructorArgs[1];
		writerConstructorArgs[0] = m_pMemoryStream->GetHandle();

		m_pSerializer = pCryObjectWriterClass->CreateInstance(writerConstructorArgs, 1);
	}
	else
	{
		auto pCryObjectReaderClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetTemporaryClass("CryEngine.Serialization", "ObjectReader");

		auto* pDomain = static_cast<CMonoDomain*>(pCryObjectReaderClass->GetAssembly()->GetDomain());

		auto* pMonoManagedBuffer = (MonoObject*)mono_array_new((MonoDomain*)pDomain->GetHandle(), mono_get_byte_class(), m_serializedDataSize);
		auto pBufferClass = pNetCoreLibrary->GetClassFromMonoClass(mono_object_get_class(pMonoManagedBuffer));

		auto pManagedBuffer = std::make_shared<CMonoObject>(pMonoManagedBuffer, pBufferClass);
		char* arrayBuffer = pManagedBuffer->GetArrayAddress(sizeof(char), 0);

		memcpy(arrayBuffer, m_pSerializedData, m_serializedDataSize);

		void* memoryStreamCtorArgs[2];
		memoryStreamCtorArgs[0] = pMonoManagedBuffer;

		bool bWritable = false;
		MonoObject* pBoolObject = mono_value_box((MonoDomain*)pDomain->GetHandle(), mono_get_boolean_class(), &bWritable);
		memoryStreamCtorArgs[1] = pBoolObject;

		auto pMemoryStream = pMemoryStreamClass->CreateInstance(memoryStreamCtorArgs, 2);

		void* readerConstructorParams[1];
		readerConstructorParams[0] = pMemoryStream->GetHandle();

		m_pSerializer = pCryObjectReaderClass->CreateInstance(readerConstructorParams, 1);
	}
}

void CAppDomain::SerializeDomainData()
{
	// Create the shared serialization utilities that may be called by libraries and objects
	CreateSerializationUtilities(true);

	for (auto it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		it->pLibrary->Serialize();
	}

	// Grab the serialized buffer
	auto pManagedBuffer = m_pMemoryStream->InvokeMethod("GetBuffer");

	m_serializedDataSize = pManagedBuffer->GetArraySize();
	char* arrayBuffer = pManagedBuffer->GetArrayAddress(sizeof(char), 0);

	m_pSerializedData = new char[m_serializedDataSize + 1];
	memcpy(m_pSerializedData, arrayBuffer, m_serializedDataSize);
	m_pSerializedData[m_serializedDataSize] = '\0';

	CryLogAlways("Total serialization time: %f ms, %iMB used", (1000.f * gEnv->pTimer->TicksToSeconds(m_serializationTicks)), m_serializedDataSize / (1024 * 1024));
}

void CAppDomain::DeserializeDomainData()
{
	CreateSerializationUtilities(false);

	for (auto it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		it->pLibrary->Deserialize();
	}

	delete[] m_pSerializedData;

	CryLogAlways("Total deserialization time: %f ms", (1000.f * gEnv->pTimer->TicksToSeconds(m_serializationTicks)));
}

void CAppDomain::Serialize(MonoObject* pObject, bool bIsAssembly)
{
	auto startTime = CryGetTicks();

	void* writeParams[1];
	writeParams[0] = pObject;

	m_pSerializer->InvokeMethod(bIsAssembly ? "WriteStatics" : "Write", writeParams, 1);

	m_serializationTicks += CryGetTicks() - startTime;
}

std::shared_ptr<IMonoObject> CAppDomain::Deserialize(bool bIsAssembly)
{
	auto startTime = CryGetTicks();

	// Now serialize the statics contained inside this library
	auto pReadResult = m_pSerializer->InvokeMethod(bIsAssembly ? "ReadStatics" : "Read");

	m_serializationTicks += CryGetTicks() - startTime;

	return pReadResult;
}