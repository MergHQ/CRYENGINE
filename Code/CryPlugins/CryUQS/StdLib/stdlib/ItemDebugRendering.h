// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		//===================================================================================
		//
		// - functions that add visual representations of items to an IDebugRenderWorldPersistent
		// - they're registered as callbacks within an item factory and are used to add persistent debug geometry for all items of an ongoing query
		// - not all types of items may have a visual representation (e. g. an 'int' or 'float' doesn't have one, but a 'CAIActor' does)
		//
		//===================================================================================

		void EntityId_AddToDebugRenderWorld(const EntityIdWrapper& item, core::IDebugRenderWorldPersistent& debugRW);

	}
}
