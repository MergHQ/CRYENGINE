%include "CryEngine.swig"

%import "CryCommon.i"

%import "Cry3DEngine.i"
%import "CryScriptSystem.i"
%import "CryPhysics.i"
%import "CryRender.i"
%import "CryAnimation.i"

%{
#include <CryAnimation/ICryAnimation.h>
#include <CryNetwork/INetwork.h>

#include <CryEntitySystem/IEntity.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryEntitySystem/IBreakableManager.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <Cry3DEngine/IStatObj.h>
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryExtension/CryCreateClassInstance.h>
#include <CryExtension/CryGUID.h>
#include <CryExtension/CryTypeID.h>
#include <CryExtension/ICryFactoryRegistry.h>
#include <CryExtension/ICryFactory.h>

#include <IGameObjectSystem.h>
#include <IGameObject.h>
%}

%ignore IEntitySystemEngineModule;

%import "../../../../CryEngine/CryCommon/CryNetwork/INetwork.h"

%csconstvalue("0xFFFFFFFF") eEA_All;
%typemap(csbase) EEntityAspects "uint"
%ignore GameWarning;

%ignore GetType;

%feature("director") ICryUnknown;

%include "../../../CryEngine/CryCommon/CryExtension/CryCreateClassInstance.h"
%include "../../../CryEngine/CryCommon/CryExtension/CryGUID.h"
%include "../../../CryEngine/CryCommon/CryExtension/CryTypeID.h"
%include "../../../CryEngine/CryCommon/CryExtension/ICryFactoryRegistry.h"

%feature("director") IEntityComponent;
%feature("director") IEntityAudioProxy;
%include "../../../../CryEngine/CryCommon/CryEntitySystem/IEntityComponent.h"

%feature("director") IGameObjectExtension;
%feature("director") IGameObjectExtensionCreatorBase;

%include <std_shared_ptr.i>
%include "../../../CryEngine/CryAction/IGameObject.h"

// bugging out on that - no idea why - *cry* . . . worked with 3.7
%feature("director") GameObjectExtensionCreatorHelper;
struct SRMIData
{
	SRMIData() :
		m_nMessages(0),
		m_extensionId(-1)
	{}
	size_t m_nMessages;
	SGameObjectExtensionRMI m_vMessages[64];
	IGameObjectSystem::ExtensionID m_extensionId;
};
class GameObjectExtensionCreatorHelper
{
public:
	virtual IGameObjectExtension* Instantiate(IEntity* pEntity) = 0;
};
class GameObjectExtensionCreatorBase : public IGameObjectExtensionCreatorBase
{
public:
	GameObjectExtensionCreatorBase(GameObjectExtensionCreatorHelper* helper) : m_pHelper(helper) {}

	IGameObjectExtension* Create(IEntity* pEntity)
	{
		return m_pHelper->Instantiate(pEntity);
	}

	void GetGameObjectExtensionRMIData(void ** ppRMI, size_t * nCount)
	{
		*ppRMI = m_RMIdata.m_vMessages;
		*nCount = m_RMIdata.m_nMessages;
	}
protected:
	SRMIData							m_RMIdata;
	GameObjectExtensionCreatorHelper*	m_pHelper;
};
%{
struct SRMIData
{
	SRMIData() :
		m_nMessages(0),
		m_extensionId(-1)
	{}
	size_t m_nMessages;
	SGameObjectExtensionRMI m_vMessages[64];
	IGameObjectSystem::ExtensionID m_extensionId;
};
class GameObjectExtensionCreatorHelper
{
public:
	virtual IGameObjectExtension* Instantiate(IEntity* pEntity) = 0;
};
class GameObjectExtensionCreatorBase : public IGameObjectExtensionCreatorBase
{
public:
	GameObjectExtensionCreatorBase(GameObjectExtensionCreatorHelper* helper) : m_pHelper(helper) {}

	IGameObjectExtension* Create(IEntity* pEntity)
	{
		return m_pHelper->Instantiate(pEntity);
	}

	void GetGameObjectExtensionRMIData(void ** ppRMI, size_t * nCount)
	{
		*ppRMI = m_RMIdata.m_vMessages;
		*nCount = m_RMIdata.m_nMessages;
	}
protected:
	SRMIData							m_RMIdata;
	GameObjectExtensionCreatorHelper*	m_pHelper;
};
%}

// ~TEMPORARY
%feature("director") IEntityScriptProxy;
//TODO: %include "../../../CryEngine/CryCommon/IEntityAttributesProxy.h"
%include "../../../../CryEngine/CryCommon/CryEntitySystem/IEntityClass.h"
%ignore EPartIds;
%include "../../../../CryEngine/CryCommon/CryEntitySystem/IEntity.h"
%extend SEntityEvent {
	EventPhysCollision* GetEventPhysCollision()
	{
		if($self->nParam[0])
			return (EventPhysCollision*)$self->nParam[0];
		return nullptr;
	}

	INT_PTR GetIntPtrParameter(uint8 index)
	{
		if(index >= 4)
			return 0;

		return $self->nParam[index];
	}

	float GetFloatParameter(uint8 index)
	{
		if(index >= 3)
			return 0;

		return $self->fParam[index];
	}
}
%extend IEntity {
	SEntitySlotInfo* GetSlotInfo(int slot)
	{
		SEntitySlotInfo* info = new SEntitySlotInfo();
		if(!$self->GetSlotInfo(slot, *info))
		{
			delete info;
			return nullptr;
		}
		return info;
	}
	IEntity* GetAttachedChild(int partID)
	{
		return $self->UnmapAttachedChild(partID);
	}
}
//(maybe) TODO: %include "../../../CryEngine/CryCommon/IEntityRenderState.h"
//(maybe) TODO: %include "../../../CryEngine/CryCommon/IEntityRenderState_info.h"
//TODO: %include "../../../CryEngine/CryCommon/IEntitySerialize.h"
%typemap(csbase) IEntitySystem::SinkEventSubscriptions "uint"
%ignore CreateEntitySystem;
%feature("director") IEntityEventListener;
%feature("director") IEntitySystemSink;
%feature("director") IAreaManagerEventListener;
%include "../../../../CryEngine/CryCommon/CryEntitySystem/IEntitySystem.h"
%include "../../../../CryEngine/CryCommon/CryEntitySystem/IBreakableManager.h"
%extend SEntitySpawnParams {
	bool HasEntityNode()
	{
		IXmlNode* ptr = $self->entityNode;
		return ptr != nullptr;
	}
}