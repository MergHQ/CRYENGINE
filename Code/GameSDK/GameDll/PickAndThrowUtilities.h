// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
-------------------------------------------------------------------------

Utility classes for pick&throw mechanic

Created by Benito G.R., code refactored from PickAndThrow.cpp

*************************************************************************/

#pragma once

#ifndef _PICKANDTHROW_UTILITIES_H_
#define _PICKANDTHROW_UTILITIES_H_

#include <Cry3DEngine/IStatObj.h>
#include <CryEntitySystem/IEntityBasicTypes.h>
#include <CryPhysics/IntersectionTestQueue.h>

struct IActor;

namespace PickAndThrow
{
	class CObstructionCheck
	{

	public:
		CObstructionCheck();
		~CObstructionCheck();

		void DoCheck( IActor* pOwnerActor, EntityId objectId);
		void IntersectionTestComplete(const QueuedIntersectionID& intersectionID, const IntersectionTestResult& result);

		void Reset();

		ILINE bool IsObstructed() const { return m_obstructed; }

	private:

		QueuedIntersectionID	m_queuedPrimitiveId;
		bool					m_obstructed;
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	IStatObj::SSubObject* FindHelperObject( const char* pHelperName, const EntityId objectId, const int slot );
	IStatObj::SSubObject* FindHelperObject_Basic( const char* pHelperName, const EntityId objectId, const int slot );
	IStatObj::SSubObject* FindHelperObject_Extended( const char* pHelperName, const EntityId objectId, const int slot );
	IStatObj::SSubObject* FindHelperObject_RecursivePart( IStatObj* pObj, const char* pHelperName );
	int FindActiveSlot( const EntityId objectId );

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	bool TargetEntityWithinFrontalCone(const Vec3& attackerLocation,const Vec3& victimLocation,const Vec3& attackerFacingdir, const float targetConeRads, float& theta);
	bool AllowedToTargetPlayer(const EntityId attackerId, const EntityId victimEntityId);

	ILINE float SelectAnimDurationOverride(const float cVarOverride, const float XMLOverride)
	{
		// This is the equivalent of the following more verbose version...
		/*

		float desiredDuration     =  cVarOverride;
		
		// If CVAR override not set.. we fall back to XML
		if(cVarOverride < 0.0f)
		{
			desiredDuration = XMLOverride;
		}

		return desiredDuration;

		*/

		return static_cast<float>(__fsel(cVarOverride, cVarOverride, XMLOverride));
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
}

#endif //_PICKANDTHROW_UTILITIES_H_
