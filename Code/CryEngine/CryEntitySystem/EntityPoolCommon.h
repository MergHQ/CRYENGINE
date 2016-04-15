// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: Common definitions for Entity Pool

   -------------------------------------------------------------------------
   History:
   - 12:04:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __ENTITYPOOLCOMMON_H__
#define __ENTITYPOOLCOMMON_H__

// Include debugging info about pools in build.
#ifndef _RELEASE
	#define ENTITY_POOL_DEBUGGING
#endif //_RELEASE

// Perform signature tests on Entity classes included in Pool definitions.
// This is a saftey check to make sure Pool definitions are properly defined.
#if defined(ENTITY_POOL_DEBUGGING) && CRY_PLATFORM_WINDOWS
	#define ENTITY_POOL_SIGNATURE_TESTING
#endif //_RELEASE

typedef uint32 TEntityPoolId;
static const TEntityPoolId INVALID_ENTITY_POOL = ~0;

typedef uint32 TEntityPoolDefinitionId;
static const TEntityPoolDefinitionId INVALID_ENTITY_POOL_DEFINITION = ~0;

class CEntityPool;
class CEntityPoolDefinition;
class CEntityPoolManager;

#endif //__ENTITYPOOLCOMMON_H__
