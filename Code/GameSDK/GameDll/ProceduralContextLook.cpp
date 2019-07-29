// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProceduralContextLook.h"

CRYREGISTER_CLASS( CProceduralContextLook );

//////////////////////////////////////////////////////////////////////////
CProceduralContextLook::CProceduralContextLook()
	: m_gameRequestsLooking( true )
	, m_procClipRequestsLooking( false )
	, m_gameLookTarget( 0, 0, 0 )
{
}

//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::Initialise( IEntity& entity, IActionController& actionController )
{
	IProceduralContext::Initialise( entity, actionController );

	InitialisePoseBlenderLook();
	InitialiseGameLookTarget();
}

//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::InitialisePoseBlenderLook()
{
	CRY_ASSERT( m_entity );

	const int slot = 0;
	ICharacterInstance* pCharacterInstance = m_entity->GetCharacter( slot );
	if ( pCharacterInstance == NULL )
	{
		return;
	}

	ISkeletonPose* pSkeletonPose = pCharacterInstance->GetISkeletonPose();
	if ( pSkeletonPose == NULL )
	{
		return;
	}

	if (IAnimationPoseBlenderDir* pPoseBlenderLook = GetPoseBlenderLook())
	{
		float polarCoordinatesSmoothTimeSeconds = 0.1f;
		float polarCoordinatesMaxYawDegreesPerSecond = 360.f;
		float polarCoordinatesMaxPitchDegreesPerSecond = 360.f;
		float fadeInSeconds = 0.25f;
		float fadeOutSeconds = 0.25f;
		float fadeOutMinDistance = 0.f;

		IScriptTable* pScriptTable = m_entity->GetScriptTable();
		if ( pScriptTable )
		{
			SmartScriptTable pProceduralContextLookTable;
			pScriptTable->GetValue( "ProceduralContextLook", pProceduralContextLookTable );
			if ( pProceduralContextLookTable )
			{
				pProceduralContextLookTable->GetValue( "polarCoordinatesSmoothTimeSeconds", polarCoordinatesSmoothTimeSeconds );
				pProceduralContextLookTable->GetValue( "polarCoordinatesMaxYawDegreesPerSecond", polarCoordinatesMaxYawDegreesPerSecond );
				pProceduralContextLookTable->GetValue( "polarCoordinatesMaxPitchDegreesPerSecond", polarCoordinatesMaxPitchDegreesPerSecond );
				pProceduralContextLookTable->GetValue( "fadeInSeconds", fadeInSeconds );
				pProceduralContextLookTable->GetValue( "fadeOutSeconds", fadeOutSeconds );
				pProceduralContextLookTable->GetValue( "fadeOutMinDistance", fadeOutMinDistance );
			}
		}

		pPoseBlenderLook->SetPolarCoordinatesSmoothTimeSeconds( polarCoordinatesSmoothTimeSeconds );
		pPoseBlenderLook->SetPolarCoordinatesMaxRadiansPerSecond( Vec2( DEG2RAD( polarCoordinatesMaxYawDegreesPerSecond ), DEG2RAD( polarCoordinatesMaxPitchDegreesPerSecond ) ) );
		pPoseBlenderLook->SetFadeInSpeed( fadeInSeconds );
		pPoseBlenderLook->SetFadeOutSpeed( fadeOutSeconds );
		pPoseBlenderLook->SetFadeOutMinDistance( fadeOutMinDistance );
		pPoseBlenderLook->SetState( false );
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::InitialiseGameLookTarget()
{
	CRY_ASSERT( m_entity );

	m_gameLookTarget = ( m_entity->GetForwardDir() * 10.0f ) + m_entity->GetWorldPos();
	if (IAnimationPoseBlenderDir* pPoseBlenderLook = GetPoseBlenderLook())
	{
		pPoseBlenderLook->SetTarget( m_gameLookTarget );
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::Update( float timePassedSeconds )
{
	if (IAnimationPoseBlenderDir* pPoseBlenderLook = GetPoseBlenderLook())
	{
		const bool canLook = (m_gameRequestsLooking && m_procClipRequestsLooking);
		pPoseBlenderLook->SetState(canLook);
		if (canLook)
		{
			pPoseBlenderLook->SetTarget(m_gameLookTarget);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::UpdateGameLookingRequest( const bool lookRequest )
{
	m_gameRequestsLooking = lookRequest;
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::UpdateProcClipLookingRequest( const bool lookRequest )
{
	m_procClipRequestsLooking = lookRequest;
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::UpdateGameLookTarget( const Vec3& lookTarget )
{
	m_gameLookTarget = lookTarget;
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::SetBlendInTime( const float blendInTime )
{

	if (IAnimationPoseBlenderDir* pPoseBlenderLook = GetPoseBlenderLook())
	{
		pPoseBlenderLook->SetFadeInSpeed(blendInTime);
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::SetBlendOutTime( const float blendOutTime )
{
	if (IAnimationPoseBlenderDir* pPoseBlenderLook = GetPoseBlenderLook())
	{
		pPoseBlenderLook->SetFadeOutSpeed(blendOutTime);
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextLook::SetFovRadians( const float fovRadians )
{
	if (IAnimationPoseBlenderDir* pPoseBlenderLook = GetPoseBlenderLook())
	{
		pPoseBlenderLook->SetFadeoutAngle(fovRadians);
	}
}

IAnimationPoseBlenderDir* CProceduralContextLook::GetPoseBlenderLook()
{
	if (m_entity)
	{
		if (ICharacterInstance* pCharInst = m_entity->GetCharacter(0))
		{
			if (ISkeletonPose* pSkelPose = pCharInst->GetISkeletonPose())
			{
				if (IAnimationPoseBlenderDir* pPoseBlenderAim = pSkelPose->GetIPoseBlenderLook())
				{
					return pPoseBlenderAim;
				}
			}
		}
	}
	return nullptr;
}