// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MovementPlan.h"
#include <CryAISystem/MovementUpdateContext.h>

namespace Movement
{

void Plan::InstallPlanMonitor(IPlanMonitor* pMonitorToInstall)
{
	stl::push_back_unique(m_monitors, pMonitorToInstall);
}

void Plan::UninstallPlanMonitor(IPlanMonitor* pMonitorToUninstall)
{
	stl::find_and_erase(m_monitors, pMonitorToUninstall);
}

bool Plan::CheckForNeedToReplan(const MovementUpdateContext& context) const
{
	if (!m_monitors.empty())
	{
		for (IPlanMonitor* pMonitor : m_monitors)
		{
			if (pMonitor->CheckForReplanning(context))
			{
				return true;
			}
		}
	}
	return false;
}

IPlan::Status Plan::Execute(const MovementUpdateContext& context)
{
	if (m_current == kNoBlockIndex)
		ChangeToIndex(0, context.actor);

	m_lastStatus = IPlan::Status::Running;

	while (true)
	{
		assert(m_current != kNoBlockIndex);
		assert(m_current < m_blocks.size());

		const Block::Status blockStatus = m_blocks[m_current]->Update(context);
		switch (blockStatus)
		{
		case Block::Finished:
		{
			if (m_current + 1 < m_blocks.size())
			{
				ChangeToIndex(m_current + 1, context.actor);
				continue;
			}			
			ChangeToIndex(kNoBlockIndex, context.actor);
			m_lastStatus = IPlan::Status::Finished;
			break;
		}
		case Block::CantBeFinished:
			m_lastStatus = IPlan::Status::CantBeFinished;
			break;
		case Block::Running:
			m_lastStatus = IPlan::Status::Running;
			break;
		default:
			assert(0);
			break;
		}
		break;
	}
	return m_lastStatus;
}

void Plan::ChangeToIndex(const uint newIndex, IMovementActor& actor)
{
	const uint oldIndex = m_current;

	if (oldIndex != kNoBlockIndex)
		m_blocks[oldIndex]->End(actor);

	if (newIndex != kNoBlockIndex)
		m_blocks[newIndex]->Begin(actor);

	m_current = newIndex;
}

void Plan::Clear(IMovementActor& actor)
{
	ChangeToIndex(kNoBlockIndex, actor);
	m_blocks.clear();
	m_requestId = MovementRequestID();
	m_lastStatus = IPlan::Status::None;
}

void Plan::CutOffAfterCurrentBlock()
{
	if (m_current < m_blocks.size())
		m_blocks.resize(m_current + 1);
}

bool Plan::InterruptibleNow() const
{
	if (m_current == kNoBlockIndex)
		return true;

	return m_blocks[m_current]->InterruptibleNow();
}

const Block* Plan::GetBlock(uint32 index) const
{
	return m_blocks[index].get();
}
}
