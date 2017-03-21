// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

struct IMonoDomain;
struct IMonoAssembly;
struct IMonoNativeToManagedInterface;

namespace BehaviorTree { class Node; }

struct IMonoListener
{
	virtual ~IMonoListener() {}

	//! Forwarded from CrySystem
	//! \param flags One or more flags from ESystemUpdateFlags structure.
	//! \param nPauseMode 0=normal(no pause), 1=menu/pause, 2=cutscene.
	virtual void OnUpdate(int updateFlags, int nPauseMode) = 0;
};

struct IManagedNodeCreator
{
	IManagedNodeCreator() {}

	virtual BehaviorTree::Node* Create() = 0;
};

struct IMonoEngineModule : public Cry::IDefaultModule
{
	CRYINTERFACE_DECLARE(IMonoEngineModule, 0xAE47C9890FFA4876, 0xB0B5FBB833C2B4EF);
};

struct IMonoRuntime
{
	virtual ~IMonoRuntime() {}

	virtual bool                        Initialize() = 0;
	virtual std::shared_ptr<ICryPlugin> LoadBinary(const char* szBinaryPath) = 0;

	virtual void                        Update(int updateFlags = 0, int nPauseMode = 0) = 0;

	virtual void                        RegisterListener(IMonoListener* pListener) = 0;
	virtual void                        UnregisterListener(IMonoListener* pListener) = 0;
	
	virtual IMonoDomain*                GetRootDomain() = 0;
	virtual IMonoDomain*                GetActiveDomain() = 0;
	virtual IMonoDomain*                CreateDomain(char* name, bool bActivate = false) = 0;
	
	virtual IMonoAssembly*              GetCryCommonLibrary() const = 0;
	virtual IMonoAssembly*              GetCryCoreLibrary() const = 0;

	virtual void						RegisterNativeToManagedInterface(IMonoNativeToManagedInterface& interface) = 0;

	virtual void                        RegisterManagedActor(const char* className) = 0;

	virtual void                        RegisterManagedNodeCreator(const char* szClassName, IManagedNodeCreator* pCreator) = 0;
};