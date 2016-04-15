CreateAIBehavior("HumanDefendArea", "HumanBaseBehavior",
{
	Constructor = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
		behavior:SelectWeapon(entity)
	end,

	AnalyzeSituation = function(behavior, entity)
		if (DefendAreaAssignment:IsInsideDefendArea(entity)) then
			if (AI.GetAttentionTargetAIType(entity.id) ~= AIOBJECT_NONE) then
				AI.SetRefPointPosition(entity.id, entity.AI.defendArea.position)
				entity:StartOrRestartPipe("HumanDefendAreaStandAndCheckForCloseCover")
			else
				entity:StartOrRestartPipe("Empty")
			end
		else
			AI.SetBehaviorVariable(entity.id, "InsideDefendArea", false)
		end
	end,

	OnEnemySeen = function(behavior, entity, distance)
		if (behavior.parent.OnEnemySeen) then
			behavior.parent:OnEnemySeen(entity, distance)
		end

		behavior:AnalyzeSituation(entity)
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		behavior:AnalyzeSituation(entity)
	end,

	Reset = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
	end,
})

CreateAIBehavior("HumanDefendAreaInCover", "HumanBaseBehavior",
{
	LeaveCoverOnStart = false,

	Constructor = function(behavior, entity)
		entity:SelectPipe(0, "HumanFireInCover")
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		entity:LeaveCover()
	end,

	Reset = function(behavior, entity)
		entity:LeaveCover()
	end,
})

CreateAIBehavior("HumanMoveToDefendArea", "HumanBaseBehavior",
{
	Constructor = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
		behavior:SelectWeapon(entity)
	end,

	Destructor = function(behavior, entity)
		entity:SafeKillTimer(entity.fallbackTimer)
	end,

	AnalyzeSituation = function(behavior, entity)
		if (DefendAreaAssignment:IsInsideDefendArea(entity)) then
			AI.SetBehaviorVariable(entity.id, "InsideDefendArea", true)
		else
			behavior:MoveToDefendArea(entity)
		end
	end,

	MoveToDefendArea = function(behavior, entity)
		AI.SetRefPointPosition(entity.id, entity.AI.defendArea.position)
		if (AI.GetAttentionTargetAIType(entity.id) ~= AIOBJECT_NONE) then
			entity:StartOrRestartPipe("HumanMoveToDefendAreaUsingCover")
		else
			entity:StartOrRestartPipe("HumanMoveToDefendArea")
		end
	end,

	OnNoPathFound = function(behavior, entity, sender, data)
		entity:OnError("Could not find a path to the defend area")
		entity:SelectPipe(0, "HumanStandAndShoot");
		entity.fallbackTimer = Script.SetTimer(random(2500, 5000), function() behavior:AnalyzeSituation(entity); end)
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		behavior:AnalyzeSituation(entity)
	end,

	Reset = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
	end,
})

CreateAIBehavior("HumanHoldPosition", "HumanBaseBehavior",
{
	Constructor = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
		behavior:SelectWeapon(entity)
	end,

	AnalyzeSituation = function(behavior, entity)
		if (HoldPositionAssignment:IsAtHoldPosition(entity)) then
			if(AI.GetAttentionTargetAIType(entity.id) ~= AIOBJECT_NONE and not entity.AI.holdPosition.skipCover) then
				AI.SetRefPointPosition(entity.id, entity.AI.holdPosition.position)
				entity:SelectPipe(0, "HumanHoldPositionStandAndCheckForCloseCover")
			else
				entity:SelectPipe(0, "Empty")
			end
		else
			AI.SetBehaviorVariable(entity.id, "AtHoldPosition", false)
		end
	end,

	OnEnemySeen = function(behavior, entity, distance)
		if (behavior.parent.OnEnemySeen) then
			behavior.parent:OnEnemySeen(entity, distance)
		end

		behavior:AnalyzeSituation(entity)
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		behavior:AnalyzeSituation(entity)
	end,

	Reset = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
	end,
})

CreateAIBehavior("HumanHoldPositionInCover", "HumanBaseBehavior",
{
	LeaveCoverOnStart = false,

	Constructor = function(behavior, entity)
		entity:SelectPipe(0, "HumanFireInCover")
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		entity:LeaveCover()
	end,

	Reset = function(behavior, entity)
		entity:LeaveCover()
	end,
})

CreateAIBehavior("HumanMoveToHoldPosition", "HumanBaseBehavior",
{
	Constructor = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
		behavior:SelectWeapon(entity)
	end,

	Destructor = function(behavior, entity)
		entity:SafeKillTimer(entity.fallbackTimer)
	end,

	AnalyzeSituation = function(behavior, entity)
		if (HoldPositionAssignment:IsAtHoldPosition(entity)) then
			AI.SetBehaviorVariable(entity.id, "AtHoldPosition", true)
		else
			behavior:MoveToHoldPosition(entity)
		end
	end,

	MoveToHoldPosition = function(behavior, entity)
		AI.SetRefPointPosition(entity.id, entity.AI.holdPosition.position)
		AI.SetRefPointDirection(entity.id, entity.AI.holdPosition.direction)
		if (AI.GetAttentionTargetAIType(entity.id) ~= AIOBJECT_NONE and entity.AI.holdPosition.skipCover) then
			entity:StartOrRestartPipe("HumanMoveToHoldPositionUsingCover")
		else
			entity:StartOrRestartPipe("HumanMoveToHoldPosition")
		end
	end,

	OnNoPathFound = function(behavior, entity, sender, data)
		entity:OnError("Could not find a path to the hold position")
		entity:SelectPipe(0, "HumanStandAndShoot");
		entity.fallbackTimer = Script.SetTimer(random(2500, 5000), function() behavior:AnalyzeSituation(entity); end)
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		behavior:AnalyzeSituation(entity)
	end,

	Reset = function(behavior, entity)
		behavior:AnalyzeSituation(entity)
	end,
})

CreateAIBehavior("HumanCombatMove", "HumanBaseBehavior",
{
	Constructor = function(behavior, entity)
		behavior:Move(entity)
		behavior:SelectWeapon(entity)
	end,

	Destructor = function(behavior, entity)
		SafeKillBehaviorTimer(entity, entity.checkCoverTimer)
		SafeKillBehaviorTimer(entity, entity.fallbackTimer)
	end,

	Move = function(behavior, entity)
		AI.SetRefPointPosition(entity.id, entity.AI.combatMove.position)
		if (AI.GetAttentionTargetEntity(entity.id)) then
			behavior:MoveWithCover(entity)
		else
			behavior:MoveDirectly(entity)
		end
	end,

	MoveDirectly = function(behavior, entity)
		entity:StartOrRestartPipe("HumanCombatMoveDirectly")
	end,

	MoveWithCover = function(behavior, entity)
		entity:StartOrRestartPipe("HumanCombatMoveWithCover")
	end,

	MoveDone = function(behavior, entity)
		if (DistanceSqVectors(entity:GetWorldPos(), entity.AI.combatMove.position) > 25) then
			behavior:Move(entity)
		else
			entity:ClearAssignment()
		end
	end,

	StartCoverCheckTimer = function(behavior, entity)
		SafeKillBehaviorTimer(entity, entity.checkCoverTimer)
		entity.checkCoverTimer = SetTimerForBehaviorFunction(2000, behavior.CheckCover, behavior, entity)
	end,
	
	StopCoverCheckTimer = function(behavior, entity)
		SafeKillBehaviorTimer(entity, entity.checkCoverTimer)
	end,

	CheckCover = function(behavior, entity)
		if (AI.GetTacticalPoints(entity.id, "CombatMoveSimple", g_Vectors.temp)) then
			behavior:MoveWithCover(entity)
		else
			entity.checkCoverTimer = SetTimerForBehaviorFunction(2000, behavior.CheckCover, behavior, entity)
		end
	end,

	OnEnemySeen = function(behavior, entity, distance)
		if (behavior.parent.OnEnemySeen) then behavior.parent:OnEnemySeen(entity, distance) end
		behavior:MoveWithCover(entity)
	end,

	OnNoPathFound = function(behavior, entity, sender, data)
		entity:OnError("Could not find a path to the hold position")
		entity:SelectPipe(0, "StandAndShoot");
		entity.fallbackTimer = SetTimerForBehaviorFunction(random(2500, 5000), behavior.Move, behavior, entity)
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		behavior:MoveWithCover(entity)
	end,

	Reset = function(behavior, entity)
		behavior:Move(entity)
	end,
})

CreateAIBehavior("HumanCombatMoveInCover", "HumanBaseBehavior",
{
	LeaveCoverOnStart = false,

	Constructor = function(behavior, entity)
		entity:SelectPipe(0, "HumanBriefShootFromCover")
	end,

	OnCoverCompromised = function (behavior, entity, sender, data)
		entity:LeaveCover()
	end,

	Reset = function(behavior, entity)
		entity:LeaveCover()
	end,
})