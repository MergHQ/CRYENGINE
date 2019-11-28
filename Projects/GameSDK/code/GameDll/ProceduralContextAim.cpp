// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProceduralContextAim.h"


CRYREGISTER_CLASS( CProceduralContextAim );


//////////////////////////////////////////////////////////////////////////
CProceduralContextAim::CProceduralContextAim()
: m_gameRequestsAiming( true )
, m_procClipRequestsAiming( false )
, m_gameAimTarget( 0, 0, 0 )
, m_defaultPolarCoordinatesMaxSmoothRateRadiansPerSecond( DEG2RAD( 360 ), DEG2RAD( 360 ) )
, m_defaultPolarCoordinatesSmoothTimeSeconds( 0.1f )
{
}

//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::Initialise( IEntity& entity, IActionController& actionController )
{
	IProceduralContext::Initialise( entity, actionController );

	InitialisePoseBlenderAim();
	InitialiseGameAimTarget();
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::InitialisePoseBlenderAim()
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

	if (IAnimationPoseBlenderDir* pPoseBlenderAim = GetPoseBlenderAim())
	{
		m_defaultPolarCoordinatesSmoothTimeSeconds = 0.1f;
		float polarCoordinatesMaxYawDegreesPerSecond = 360.f;
		float polarCoordinatesMaxPitchDegreesPerSecond = 360.f;
		float fadeInSeconds = 0.25f;
		float fadeOutSeconds = 0.25f;
		float fadeOutMinDistance = 0.f;

		IScriptTable* pScriptTable = m_entity->GetScriptTable();
		if ( pScriptTable )
		{
			SmartScriptTable pProceduralContextAimTable;
			pScriptTable->GetValue( "ProceduralContextAim", pProceduralContextAimTable );
			if ( pProceduralContextAimTable )
			{
				pProceduralContextAimTable->GetValue( "polarCoordinatesSmoothTimeSeconds", m_defaultPolarCoordinatesSmoothTimeSeconds );
				pProceduralContextAimTable->GetValue( "polarCoordinatesMaxYawDegreesPerSecond", polarCoordinatesMaxYawDegreesPerSecond );
				pProceduralContextAimTable->GetValue( "polarCoordinatesMaxPitchDegreesPerSecond", polarCoordinatesMaxPitchDegreesPerSecond );
				pProceduralContextAimTable->GetValue( "fadeInSeconds", fadeInSeconds );
				pProceduralContextAimTable->GetValue( "fadeOutSeconds", fadeOutSeconds );
				pProceduralContextAimTable->GetValue( "fadeOutMinDistance", fadeOutMinDistance );
			}
		}

		m_defaultPolarCoordinatesMaxSmoothRateRadiansPerSecond = Vec2( DEG2RAD( polarCoordinatesMaxYawDegreesPerSecond ), DEG2RAD( polarCoordinatesMaxPitchDegreesPerSecond ) );

		pPoseBlenderAim->SetPolarCoordinatesSmoothTimeSeconds( m_defaultPolarCoordinatesSmoothTimeSeconds );
		pPoseBlenderAim->SetPolarCoordinatesMaxRadiansPerSecond( m_defaultPolarCoordinatesMaxSmoothRateRadiansPerSecond );
		pPoseBlenderAim->SetFadeInSpeed( fadeInSeconds );
		pPoseBlenderAim->SetFadeOutSpeed( fadeOutSeconds );
		pPoseBlenderAim->SetFadeOutMinDistance( fadeOutMinDistance );
		pPoseBlenderAim->SetState( false );
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::InitialiseGameAimTarget()
{
	CRY_ASSERT( m_entity );
	m_gameAimTarget = (m_entity->GetForwardDir() * 10.0f) + m_entity->GetWorldPos();

	if (IAnimationPoseBlenderDir* pPoseBlenderAim = GetPoseBlenderAim())
	{
		pPoseBlenderAim->SetTarget( m_gameAimTarget );
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::Update( float timePassedSeconds )
{
	IAnimationPoseBlenderDir* pPoseBlenderAim = GetPoseBlenderAim();
	if (pPoseBlenderAim == NULL )
	{
		return;
	}

	const bool canAim = ( m_gameRequestsAiming && m_procClipRequestsAiming );
	pPoseBlenderAim->SetState( canAim );

	UpdatePolarCoordinatesSmoothingParameters();

	if ( canAim )
	{
		pPoseBlenderAim->SetTarget( m_gameAimTarget );
	}
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::UpdatePolarCoordinatesSmoothingParameters()
{
	IAnimationPoseBlenderDir* pPoseBlenderAim = GetPoseBlenderAim();
	CRY_ASSERT(pPoseBlenderAim);

	Vec2 polarCoordinatesMaxSmoothRateRadiansPerSecond = m_defaultPolarCoordinatesMaxSmoothRateRadiansPerSecond;
	float polarCoordinatesSmoothTimeSeconds = m_defaultPolarCoordinatesSmoothTimeSeconds;

	const size_t requestCount = m_polarCoordinatesSmoothingParametersRequestList.GetCount();
	if ( 0 < requestCount )
	{
		const SPolarCoordinatesSmoothingParametersRequest& request = m_polarCoordinatesSmoothingParametersRequestList.GetRequest( requestCount - 1 );

		polarCoordinatesMaxSmoothRateRadiansPerSecond = request.maxSmoothRateRadiansPerSecond;
		polarCoordinatesSmoothTimeSeconds = request.smoothTimeSeconds;
	}

	pPoseBlenderAim->SetPolarCoordinatesMaxRadiansPerSecond( polarCoordinatesMaxSmoothRateRadiansPerSecond );
	pPoseBlenderAim->SetPolarCoordinatesSmoothTimeSeconds( polarCoordinatesSmoothTimeSeconds );
}


IAnimationPoseBlenderDir* CProceduralContextAim::GetPoseBlenderAim()
{
	if (m_entity)
	{
		if (ICharacterInstance* pCharInst = m_entity->GetCharacter(0))
		{
			if (ISkeletonPose* pSkelPose = pCharInst->GetISkeletonPose())
			{
				if (IAnimationPoseBlenderDir* pPoseBlenderAim = pSkelPose->GetIPoseBlenderAim())
				{
					return pPoseBlenderAim;
				}
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::UpdateGameAimingRequest( const bool aimRequest )
{
	m_gameRequestsAiming = aimRequest;
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::UpdateProcClipAimingRequest( const bool aimRequest )
{
	m_procClipRequestsAiming = aimRequest;
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::UpdateGameAimTarget( const Vec3& aimTarget )
{
	m_gameAimTarget = aimTarget;
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::SetBlendInTime( const float blendInTime )
{
	IAnimationPoseBlenderDir* pPoseBlenderAim = GetPoseBlenderAim();
	if (pPoseBlenderAim == NULL )
	{
		return;
	}

	pPoseBlenderAim->SetFadeInSpeed( blendInTime );
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::SetBlendOutTime( const float blendOutTime )
{
	IAnimationPoseBlenderDir* pPoseBlenderAim = GetPoseBlenderAim();
	if (pPoseBlenderAim == NULL)
	{
		return;
	}

	pPoseBlenderAim->SetFadeOutSpeed( blendOutTime );
}


//////////////////////////////////////////////////////////////////////////
uint32 CProceduralContextAim::RequestPolarCoordinatesSmoothingParameters( const Vec2& maxSmoothRateRadiansPerSecond, const float smoothTimeSeconds )
{
	SPolarCoordinatesSmoothingParametersRequest request;
	request.maxSmoothRateRadiansPerSecond = maxSmoothRateRadiansPerSecond;
	request.smoothTimeSeconds = smoothTimeSeconds;

	return m_polarCoordinatesSmoothingParametersRequestList.AddRequest( request );
}


//////////////////////////////////////////////////////////////////////////
void CProceduralContextAim::CancelPolarCoordinatesSmoothingParameters( const uint32 requestId )
{
	m_polarCoordinatesSmoothingParametersRequestList.RemoveRequest( requestId );
}