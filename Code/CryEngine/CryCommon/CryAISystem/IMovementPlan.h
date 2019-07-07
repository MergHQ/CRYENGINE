// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/MovementUpdateContext.h>
#include <CryAISystem/MovementBlock.h>
#include <CryAISystem/IMovementActor.h>


namespace Movement
{

	/// Plan monitors can be installed in an IPlan to keep track of the "health status" of specific movement blocks.
	/// The underlying planner will periodically consult these monitors and re-plan if they say so.
	struct IPlanMonitor
	{
		virtual bool        CheckForReplanning(const MovementUpdateContext& context) = 0;

	protected:
		~IPlanMonitor() {} // not intended to get deleted via base-class pointers
	};

	struct IPlan
	{
		enum class Status
		{
			None,
			Running,
			Finished,
			CantBeFinished,

			// keep this the last entry
			Count
		};

		static const uint32              kNoBlockIndex = ~0u;

		virtual                          ~IPlan() {}
		virtual void                     InstallPlanMonitor(IPlanMonitor* pMonitorToInstall) = 0;
		virtual void                     UninstallPlanMonitor(IPlanMonitor* pMonitorToUninstall) = 0;
		virtual void                     AddBlock(const std::shared_ptr<Block>& block) = 0;
		virtual bool                     CheckForNeedToReplan(const MovementUpdateContext& context) const = 0;
		virtual Status                   Execute(const MovementUpdateContext& context) = 0;
		virtual bool                     HasBlocks() const = 0;
		virtual void                     Clear(IMovementActor& actor) = 0;
		virtual void                     CutOffAfterCurrentBlock() = 0;
		virtual bool                     InterruptibleNow() const = 0;
		virtual uint32                   GetCurrentBlockIndex() const = 0;
		virtual uint32                   GetBlockCount() const = 0;
		virtual const Block*             GetBlock(uint32 index) const = 0;
		virtual const MovementRequestID& GetRequestId() const = 0;
		virtual void                     SetRequestId(const MovementRequestID& requestId) = 0;
		virtual Status                   GetLastStatus() const = 0;
	};

}
