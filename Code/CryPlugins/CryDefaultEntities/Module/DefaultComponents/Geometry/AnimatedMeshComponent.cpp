// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AnimatedMeshComponent.h"

#include <Cry3DEngine/IRenderNode.h>
#include <CrySerialization/Decorators/Resources.h>
#include <CryType/Type.h>
#include <CrySerialization/IArchive.h>
#include <CryCore/Assert/CryAssert.h>

namespace Cry
{
namespace DefaultComponents
{
void CAnimatedMeshComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAnimatedMeshComponent::PlayAnimation, "{DCEBBA52-CECE-4823-AC35-1A590A944F99}"_cry_guid, "PlayAnimation");
		pFunction->SetDescription("Plays a low-level animation file on the animated mesh");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::None);
		pFunction->BindInput(1, 'name', "Animation File");
		pFunction->BindInput(2, 'loop', "Looped");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAnimatedMeshComponent::PlayAnimationByName, "{37B044D0-A419-47A9-B8AE-5BF03FC99C1A}"_cry_guid, "PlayAnimationByName");
		pFunction->SetDescription("Plays a low-level animation on the animated mesh by using the raw animation name.");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::None);
		pFunction->BindInput(1, 'name', "Raw Animation Name");
		pFunction->BindInput(2, 'loop', "Looped");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAnimatedMeshComponent::SetPlaybackSpeed, "{DDF693D5-100C-43C2-8351-46FD6D382425}"_cry_guid, "SetPlaybackSpeed");
		pFunction->SetDescription("Sets the speed at which animations are played");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'sped', "Speed");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAnimatedMeshComponent::SetPlaybackWeight, "{79D18B21-F255-43C5-98F8-42E131FB2915}"_cry_guid, "SetPlaybackWeight");
		pFunction->SetDescription("Sets the weight at which animations are played");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'weig', "Weight");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAnimatedMeshComponent::SetLayer, "{3CF9CE51-5520-4D6D-8077-8112DA4A48B6}"_cry_guid, "SetLayer");
		pFunction->SetDescription("Sets the layer at which animations are played");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'layr', "Layer");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAnimatedMeshComponent::SetMeshType, "{2C8885E0-79F4-42A3-B67A-2F450DDBDFD0}"_cry_guid, "SetType");
		pFunction->BindInput(1, 'type', "Type");
		pFunction->SetDescription("Changes the type of the object");
		pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member });
		componentScope.Register(pFunction);
	}
}

void CAnimatedMeshComponent::Initialize()
{
	LoadFromDisk();
	ResetObject();
}

void CAnimatedMeshComponent::LoadFromDisk()
{
	if (m_filePath.value.size() > 0)
	{
		m_pCachedCharacter = gEnv->pCharacterManager->CreateInstance(m_filePath.value);
	}
	else
	{
		m_pCachedCharacter = nullptr;
	}

	m_defaultAnimation.FillAnimationList(m_pCachedCharacter ? m_pCachedCharacter->GetIAnimationSet() : nullptr);
}

void CAnimatedMeshComponent::ResetObject()
{
	m_defaultAnimation.isUsingResourcePicker = !m_bIsUsingRawAnimationName;

	if (m_pCachedCharacter == nullptr)
	{
		FreeEntitySlot();
		return;
	}

	m_pEntity->SetCharacter(m_pCachedCharacter, GetOrMakeEntitySlotId(), false);

	if (!m_materialPath.value.empty())
	{
		if (IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_materialPath.value, false))
		{
			m_pEntity->SetSlotMaterial(GetEntitySlotId(), pMaterial);
		}
	}

	m_animationParams.m_fPlaybackSpeed = m_defaultAnimationSpeed;

	if (m_bIsUsingRawAnimationName)
	{
		if (!m_defaultAnimation.animationString.empty())
		{
			PlayAnimationByName(m_defaultAnimation.animationString.c_str(), m_bLoopDefaultAnimation);
		}
	}
	else
	{
		if (!m_defaultAnimation.animationString.empty())
		{
			PlayAnimation(m_defaultAnimation.animationString.c_str(), m_bLoopDefaultAnimation);
		}
	}
}

int CAnimatedMeshComponent::FindAnimId(const char* szFilepath) 
{
	int32 crc32 = CCrc32::ComputeLowercase(szFilepath);
	CRY_ASSERT(m_pCachedCharacter);
	IAnimationSet* pAnimSet = m_pCachedCharacter->GetIAnimationSet();
	if (CRY_VERIFY(pAnimSet))
	{
		for (int i = 0; i < pAnimSet->GetAnimationCount(); ++i)
		{
			if (crc32 == pAnimSet->GetFilePathCRCByAnimID(i))
			{
				return i;
			}
		}
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "Animation file '%s' not found in Animation List of character '%s'!", szFilepath, m_pCachedCharacter->GetFilePath());
	}
	return -1;
}

void CAnimatedMeshComponent::ProcessEvent(const SEntityEvent& event)
{
	if (event.event == EEntityEvent::EditorPropertyChanged)
	{
		m_pEntity->UpdateComponentEventMask(this);
		
		LoadFromDisk();
		ResetObject();

		// Update Editor UI to show the default object material
		if (m_materialPath.value.empty() && m_pCachedCharacter != nullptr)
		{
			if (IMaterial* pMaterial = m_pCachedCharacter->GetMaterial())
			{
				m_materialPath = pMaterial->GetName();
			}
		}
	}

	CBaseMeshComponent::ProcessEvent(event);
}

void CAnimatedMeshComponent::SetCharacterFile(const char* szPath)
{
	m_filePath = szPath;
}

void CAnimatedMeshComponent::SetIsUsingRawAnimationName(bool bIsUsingRawAnimationName)
{
	m_bIsUsingRawAnimationName = bIsUsingRawAnimationName;
	m_defaultAnimation.isUsingResourcePicker = !m_bIsUsingRawAnimationName;
}

void CAnimatedMeshComponent::SetDefaultAnimationName(const char* szPath)
{
	m_defaultAnimation.animationString = szPath;
}

bool CAnimatedMeshComponent::SetMaterial(int slotId, const char* szMaterial)
{
	if (slotId == GetEntitySlotId())
	{
		if (IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(szMaterial, false))
		{
			m_materialPath = szMaterial;
			m_pEntity->SetSlotMaterial(GetEntitySlotId(), pMaterial);
		}
		else if (szMaterial[0] == '\0')
		{
			m_materialPath.value.clear();
			m_pEntity->SetSlotMaterial(GetEntitySlotId(), nullptr);
		}

		return true;
	}

	return false;
}

}
}
