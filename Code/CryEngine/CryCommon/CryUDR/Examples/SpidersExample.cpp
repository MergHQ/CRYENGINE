// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <CryEntitySystem/IEntity.h>
#include <CryUDR/InterfaceIncludes.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

void SpidersDebug() // <----- pretend that this gets called on every frame
{
	Cry::UDR::CScope_FixedString scope("AI");
	{
		Cry::UDR::CScope_FixedString scope("Spiders");
		{
			Cry::UDR::CScope_FixedString scope(m_pEntity->GetName());
			scope->AddSphere(m_pEntity->GetPos(), 0.1f, Col_Blue);
			scope->AddLine(m_pEntity->GetPos(), m_pEntity->GetPos() + m_pEntity->GetUpDir(), Col_Blue);
		}
	}
}

//  Let's say that we have 3 spider entities running around in the game with unique names like "spider-1", "spider-2", "spider-3".
//  Then, according to the code above, a whole tree will get built, and more and more render-primitives will get added to the individual scopes over time:
//
//
//  "AI"
//   |
//   +-- "Spiders"
//        |
//        +-- "spider-1"
//        |     * sphere
//        |     * line
//        |     * sphere
//        |     * line
//        |     * ...
//        |
//        |
//        +-- "spider-2"
//        |     * sphere
//        |     * line
//        |     * sphere
//        |     * line
//        |     * ...
//        |
//        |
//        +-- "spider-3"
//        |     * sphere
//        |     * line
//        |     * sphere
//        |     * line
//        |     * ...
//