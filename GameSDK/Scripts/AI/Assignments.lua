if (assignmentsHaveBeenCreated == nil) then
	assignmentsHaveBeenCreated = true

	AssignmentFunctionality = {
		Assignments = {},

		OnResetSavedAssignment = function( entity )
			-- Warning! The AIActor is still being constructed/serialized
			-- when this function is called. Be careful with what you call.
			-- The modular behavior tree and its variables have been setup
			-- at this point so you can safely set/get variables.

			if ( entity.AI.currentAssignment ~= nil and entity.AI.currentAssignment ~= Assignment_NoAssignment) then
				entity:SetAssignment( entity.AI.currentAssignment, entity.AI.currentAssignmentData )
			end
		end,

		SetAssignment = function(entity, assignment, data)
			if (assignment == Assignment_NoAssignment) then
				entity:ClearAssignment()
				return
			end

			local resetAssignment = (entity.AI.currentAssignment and entity.AI.currentAssignment == assignment)
			if ( resetAssignment ) then
				entity:CallBehaviorFunction('Reset')
				AI.Signal(SIGNALFILTER_SENDER, 1, "OnResetAssignment", entity.id)
			end

			entity:ClearAssignment()

			local assignmentTable = entity.Assignments[assignment]
			if (assignmentTable == nil) then
				entity:OnError("Assignment not available for this entity : "..assignment)
				return
			end

			if (assignmentTable:Start(entity, data)) then
				entity.AI.currentAssignment = assignment
				entity.AI.currentAssignmentData = data
			else
				entity:OnError("Assignment failed to start")
			end
		end,

		ClearAssignment = function(entity)
			if (entity.AI.currentAssignment ~= nil) then
				local currentAssignmentTable = entity.Assignments[entity.AI.currentAssignment]
				currentAssignmentTable:Stop(entity)
				entity.AI.currentAssignment = nil
				entity.AI.currentAssignmentData = nil
			end
		end,
	}

	function InjectAssignmentFunctionality(entity)
		mergef(entity, AssignmentFunctionality, 1)
	end

	DefendAreaAssignment = {
		Start = function(assignment, entity, data)
			if (IsNullVector(data.position)) then
				entity:OnError("DefendArea : position not valid")
				return false
			end

			entity.AI.defendArea = {
				position = {x=0, y=0, z=0},
				radiusSq = 225,
			}
			CopyVector(entity.AI.defendArea.position, data.position)
			-- if (data.radius > 0) then
				-- entity.AI.defendArea.radiusSq = data.radius * data.radius
			-- end

			AI.SetBehaviorVariable(entity.id, "DefendArea", true)
			AI.SetBehaviorVariable(entity.id, "InsideDefendArea", assignment:IsInsideDefendArea(entity))
			return true
		end,

		Stop = function(assignment, entity)
			AI.RequestToStopMovement(entity.id)
			AI.SetBehaviorVariable(entity.id, "DefendArea", false)
		end,

		IsInsideDefendArea = function(assignment, entity)
			return DistanceSqVectors(entity:GetWorldPos(), entity.AI.defendArea.position) < entity.AI.defendArea.radiusSq
		end,
	}

	function AddDefendAreaAssignment(entity)
		entity.Assignments[Assignment_DefendArea] = DefendAreaAssignment
	end

	HoldPositionAssignment = {
		Start = function(assignment, entity, data)
			if (IsNullVector(data.position)) then
				entity:OnError("HoldPosition : position not valid")
				return false
			end

			entity.AI.holdPosition = {
				position = {x=0, y=0, z=0},
				direction = {x=0, y=0, z=0},
				radiusSq = 9,
				useCover = data.useCover,
			}
			CopyVector(entity.AI.holdPosition.position, data.position)
			CopyVector(entity.AI.holdPosition.direction, data.direction)
			if (data.radius > 0) then
				entity.AI.holdPosition.radiusSq = data.radius * data.radius
			end

			AI.SetBehaviorVariable(entity.id, "HoldPosition", true)
			AI.SetBehaviorVariable(entity.id, "AtHoldPosition", assignment:IsAtHoldPosition(entity))
			return true
		end,

		Stop = function(assignment, entity)
			AI.RequestToStopMovement(entity.id)
			AI.SetBehaviorVariable(entity.id, "HoldPosition", false)
		end,

		IsAtHoldPosition = function(assignment, entity)
			return DistanceSqVectors(entity:GetWorldPos(), entity.AI.holdPosition.position) < entity.AI.holdPosition.radiusSq
		end,
	}

	function AddHoldPositionAssignment(entity)
		entity.Assignments[Assignment_HoldPosition] = HoldPositionAssignment
	end

	CombatMoveAssignment = {
		Start = function(assignment, entity, data)
			if (IsNullVector(data.position)) then
				entity:OnError("CombatMove : position not valid")
				return false
			end

			entity.AI.combatMove = {
				position = {x=0, y=0, z=0},
				useCover = data.useCover,
			}
			CopyVector(entity.AI.combatMove.position, data.position)

			AI.SetBehaviorVariable(entity.id, "CombatMove", true)
			return true
		end,

		Stop = function(assignment, entity)
			AI.RequestToStopMovement(entity.id)
			AI.SetBehaviorVariable(entity.id, "CombatMove", false)
		end,
	}

	function AddCombatMoveAssignment(entity)
		entity.Assignments[Assignment_CombatMove] = CombatMoveAssignment
	end

	ScanSpotAssignment = {
		Start = function(assignment, entity, data)
			if (data.scanSpotEntityId == 0) then
				entity:Error("ScanSpotAssignment : ScanSpot entityId not set")
				return false
			end

			entity.AI.scanSpotAssignment = {}
			entity.AI.scanSpotAssignment.scanSpotEntityId = data.scanSpotEntityId

			AI.SetBehaviorVariable(entity.id, "ScanSpotAssignment", true)
			return true
		end,

		Stop = function(assignment, entity)
			AI.RequestToStopMovement(entity.id)
			entity.AI.scanSpotAssignment = nil
			AI.SetBehaviorVariable(entity.id, "ScanSpotAssignment", false)
		end,
	}

	function AddScanSpotAssignment(entity)
		entity.Assignments[Assignment_ScanSpot] = ScanSpotAssignment
	end

	ScorchSpotAssignment = {
		Start = function(assignment, entity, data)
			local scorchSpotEntityID = data.scorchSpotEntityID
			if (scorchSpotEntityID == 0) then
				entity:OnError("ScorchSpotAssignment : Entity ID invalid!")
				return false
			end

			entity.AI.scorchSpotAssignment = {
				scorchSpotEntityID = scorchSpotEntityID,
				aimOffset = data.aimOffset,
				firingPattern = data.firingPattern
			}

			AI.SetBehaviorVariable(entity.id, "ScorchSpotAssignment", true)
			return true
		end,

		Stop = function(assignment, entity)
			AI.SetBehaviorVariable(entity.id, "ScorchSpotAssignment", false)
		end,
	}

	function AddScorchSpotAssignment(entity)
		entity.Assignments[Assignment_ScorchSpot] = ScorchSpotAssignment
	end

	PsychoCombatAllowedAssignment = {
		Start = function(assignment, entity, data)

			AI.SetBehaviorVariable(entity.id, "PsychoCombatAllowed", true)
			return true
		end,

		Stop = function(assignment, entity)
			AI.SetBehaviorVariable(entity.id, "PsychoCombatAllowed", false)
		end,
	}

	function AddPsychoCombatAllowedAssignment(entity)
		entity.Assignments[Assignment_PsychoCombatAllowed] = PsychoCombatAllowedAssignment
	end

	StalkerAggressiveAssignment = {
		Start = function(assignment, entity, data)

			AI.SetBehaviorVariable(entity.id, "StalkerAggressive", true)
			return true
		end,

		Stop = function(assignment, entity)
			AI.SetBehaviorVariable(entity.id, "StalkerAggressive", false)
		end,
	}

	function AddStalkerAggressiveAssignment(entity)
		entity.Assignments[Assignment_StalkerAggressive] = StalkerAggressiveAssignment
	end

end