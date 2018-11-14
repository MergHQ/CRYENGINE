// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "Common/IEnvironmentConnection.h"
#include "Common/IEvent.h"
#include "Common/IFile.h"
#include "Common/IListener.h"
#include "Common/IObject.h"
#include "Common/IParameterConnection.h"
#include "Common/ISettingConnection.h"
#include "Common/IStandaloneFileConnection.h"
#include "Common/ISwitchStateConnection.h"
#include "Common/ITriggerConnection.h"
#include "Common/ITriggerInfo.h"
#include "Common/FileInfo.h"

namespace CryAudio
{
namespace Impl
{
namespace Null
{
struct SEvent final : IEvent
{
	virtual ERequestStatus Stop() override { return ERequestStatus::Success; }
};

struct SListener final : IListener
{
	virtual void                   Update(float const deltaTime) override                            {}
	virtual void                   SetName(char const* const szName) override                        {}
	virtual void                   SetTransformation(CTransformation const& transformation) override {}
	virtual CTransformation const& GetTransformation() const override                                { return CTransformation::GetEmptyObject(); }
};

struct STrigger final : ITriggerConnection
{
	virtual ERequestStatus Load() const override                             { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                           { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override { return ERequestStatus::Success; }
};

struct SObject final : IObject
{
	virtual void                   Update(float const deltaTime) override                                                                        {}
	virtual void                   SetTransformation(CTransformation const& transformation) override                                             {}
	virtual CTransformation const& GetTransformation() const override                                                                            { return CTransformation::GetEmptyObject(); }
	virtual void                   SetEnvironment(IEnvironmentConnection const* const pIEnvironment, float const amount) override                {}
	virtual void                   SetParameter(IParameterConnection const* const pIParameterConnection, float const value) override             {}
	virtual void                   SetSwitchState(ISwitchStateConnection const* const pISwitchStateConnection) override                          {}
	virtual void                   SetOcclusion(float const occlusion) override                                                                  {}
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override                                                 {}
	virtual ERequestStatus         ExecuteTrigger(ITriggerConnection const* const pITriggerConnection, IEvent* const pIEvent) override           { return ERequestStatus::Success; }
	virtual void                   StopAllTriggers() override                                                                                    {}
	virtual ERequestStatus         PlayFile(IStandaloneFileConnection* const pIStandaloneFileConnection) override                                { return ERequestStatus::Success; }
	virtual ERequestStatus         StopFile(IStandaloneFileConnection* const pIStandaloneFileConnection) override                                { return ERequestStatus::Success; }
	virtual ERequestStatus         SetName(char const* const szName) override                                                                    { return ERequestStatus::Success; }
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override                              {}
	virtual void                   DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
};

///////////////////////////////////////////////////////////////////////////
void CImpl::Update()
{
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeRelease()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Release()
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLibraryData(XmlNodeRef const pNode, bool const isLevelSpecific)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeLibraryDataChanged()
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnLoseFocus()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnGetFocus()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::MuteAll()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::UnmuteAll()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::PauseAll()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ResumeAll()
{
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalParameter(IParameterConnection const* const pIParameterConnection, float const value)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalSwitchState(ISwitchStateConnection const* const pISwitchStateConnection)
{
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	pFileInfo->memoryBlockAlignment = 0;
	pFileInfo->size = 0;
	pFileInfo->bLocalized = false;
	pFileInfo->pFileData = nullptr;
	pFileInfo->pImplData = nullptr;
	pFileInfo->szFileName = nullptr;
	return ERequestStatus::Failure; // This is the correct behavior: the NULL implementation does not recognize any file nodes.
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructFile(IFile* const pIFile)
{
}

//////////////////////////////////////////////////////////////////////////
char const* const CImpl::GetFileLocation(SFileInfo* const pFileInfo)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetInfo(SImplInfo& implInfo) const
{
	implInfo.name = "null-impl";
	implInfo.folderName = "";
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	return static_cast<IObject*>(new SObject());
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	return static_cast<IObject*>(new SObject());
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	delete pIObject;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructListener(IListener* const pIListener)
{
	delete pIListener;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	return static_cast<IListener*>(new SListener());
}

///////////////////////////////////////////////////////////////////////////
IEvent* CImpl::ConstructEvent(CEvent& event)
{
	return static_cast<IEvent*>(new SEvent());
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEvent(IEvent const* const pIEvent)
{
	delete pIEvent;
}

//////////////////////////////////////////////////////////////////////////
IStandaloneFileConnection* CImpl::ConstructStandaloneFileConnection(CStandaloneFile& standaloneFile, char const* const szFile, bool const bLocalized, ITriggerConnection const* pITriggerConnection /*= nullptr*/)
{
	return new IStandaloneFileConnection();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructStandaloneFileConnection(IStandaloneFileConnection const* const pIStandaloneFileConnection)
{
	delete pIStandaloneFileConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadConnected(DeviceId const deviceUniqueID)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadDisconnected(DeviceId const deviceUniqueID)
{
}

///////////////////////////////////////////////////////////////////////////
ITriggerConnection const* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection const* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection const* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection const* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection const* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection const* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSettingConnection(ISettingConnection const* const pISettingConnection)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnRefresh()
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLanguage(char const* const szLanguage)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetFileData(char const* const szName, SFileData& fileData) const
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float& posY, bool const showDetailedInfo)
{
}
} // namespace Null
} // namespace Impl
} // namespace CryAudio
