// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <LevelEditor/Tools/ObjectMode.h>
#include <CryAISystem/MovementRequestID.h>

class CAIMoveSimulation : public CObjectMode::ISubTool
{
public:
	virtual ~CAIMoveSimulation();

	bool UpdateAIMoveSimulation(CViewport* pView, const CPoint& point);
	bool HandleMouseEvent(CViewport* view, EMouseEvent event, CPoint& point, int flags) override;
private:
	void CancelMove();

	bool GetAIMoveSimulationDestination(CViewport* pView, const CPoint& point, Vec3& outGotoPoint) const;
	MovementRequestID SendAIMoveSimulation(IEntity* pEntity, const Vec3& vGotoPoint);

	struct SMovingAI
	{
		CryGUID           m_id;
		MovementRequestID m_movementRequestID;
	};
	std::vector<SMovingAI> m_movingAIs;
};