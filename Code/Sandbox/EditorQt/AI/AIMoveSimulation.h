// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/MovementRequestID.h>

class CAIMoveSimulation
{
public:
	CAIMoveSimulation();
	virtual ~CAIMoveSimulation();

	bool UpdateAIMoveSimulation(CViewport* pView, const CPoint& point);

private:
	void              CancelMove();

	bool              GetAIMoveSimulationDestination(CViewport* pView, const CPoint& point, Vec3& outGotoPoint) const;
	MovementRequestID SendAIMoveSimulation(IEntity* pEntity, const Vec3& vGotoPoint);

	struct SMovingAI
	{
		CryGUID           m_id;
		MovementRequestID m_movementRequestID;
	};
	std::vector<SMovingAI> m_movingAIs;
};
