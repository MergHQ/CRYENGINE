// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

   a simple bridge between game tokens and DRS data.
   it will send an DRS signal every time a (bool) gametoken changes its value

   /************************************************************************/

#pragma once

#include <CryGame/IGameTokens.h>

namespace Cry
{
namespace DRS
{
struct IResponseActor;
} // namespace DRS
} // namespace Cry

class CGameTokenSignalCreator:public IGameTokenEventListener
{
public:
	CGameTokenSignalCreator(Cry::DRS::IResponseActor* pSenderForTokenSignals);
	~CGameTokenSignalCreator();

	//////////////////////////////////////////////////////////
	// IGameTokenEventListener implementation
	virtual void OnGameTokenEvent(EGameTokenEvent event, IGameToken* pGameToken);
	virtual void GetMemoryUsage(class ICrySizer* pSizer) const;
	//////////////////////////////////////////////////////////

private:
	Cry::DRS::IResponseActor* m_pSenderForTokenSignals;
};
