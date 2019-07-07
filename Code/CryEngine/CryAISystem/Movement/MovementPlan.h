// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef MovementPlan_h
	#define MovementPlan_h

struct IMovementActor;
struct MovementUpdateContext;

	#include <CryAISystem/MovementBlock.h>
	#include <CryAISystem/MovementRequestID.h>
	#include <CryAISystem/IMovementPlan.h>

namespace Movement
{
// This is a plan that will satisfy a movement request such as
// "reach this destination" or "stop immediately".
//
// It's comprised of one or more movement blocks that each take care
// of a certain part of plan. They are executed in order.
//
// As an example a plan to satisfy the request "move to this cover" could
// be comprised of the following movement blocks:
// [FollowPath] -> [UseSmartObject] -> [FollowPath] -> [InstallInCover]
class Plan final : public IPlan
{
public:
	Plan()
		: m_current(kNoBlockIndex)
		, m_lastStatus(Status::Running)
	{
	}

	// IPlan
	virtual void             InstallPlanMonitor(IPlanMonitor* pMonitorToInstall) override;
	virtual void             UninstallPlanMonitor(IPlanMonitor* pMonitorToUninstall) override;
	virtual void             AddBlock(const std::shared_ptr<Block>& block) override { m_blocks.push_back(block); }
	virtual bool             CheckForNeedToReplan(const MovementUpdateContext& context) const override;
	virtual Status           Execute(const MovementUpdateContext& context) override;
	virtual bool             HasBlocks() const override { return !m_blocks.empty(); }
	virtual void             Clear(IMovementActor& actor) override;
	virtual void             CutOffAfterCurrentBlock() override;
	virtual bool             InterruptibleNow() const override;
	virtual uint32           GetCurrentBlockIndex() const override { return m_current; }
	virtual uint32           GetBlockCount() const override { return m_blocks.size(); }
	virtual const Block*     GetBlock(uint32 index) const override;
	virtual const MovementRequestID& GetRequestId() const override { return m_requestId; }
	virtual void             SetRequestId(const MovementRequestID& requestId) override { m_requestId = requestId; }
	virtual Status           GetLastStatus() const override { return m_lastStatus; }
	// ~IPlan

	template<typename BlockType>
	void AddBlock()
	{
		m_blocks.push_back(std::shared_ptr<Block>(new BlockType()));
	}

private:
	void                     ChangeToIndex(const uint newIndex, IMovementActor& actor);

private:
	typedef std::vector<IPlanMonitor*> PlanMonitors;
	typedef std::vector<BlockPtr> Blocks;

	PlanMonitors      m_monitors;
	Blocks            m_blocks;
	uint32            m_current;
	MovementRequestID m_requestId;
	Status            m_lastStatus;
};

}

#endif // MovementPlan_h
