local Behavior = CreateAIBehavior("HeliGotoCrysis2", "HeliIdle",
{
	Constructor = function (self, entity)
		self:AnalyzeSituation(entity)
	end,

	Destructor = function(self, entity)
	end,
	
	AnalyzeSituation = function(self, entity, sender, data)

		local targetPos = g_Vectors.temp_v1

		AI.SetRefPointPosition(entity.id, entity.AI.GoToData.Position)
		entity:SelectPipe(0, "HeliRelocate", 0, 0, 1)
	end,
	
	GoodLocationReached = function(self, entity, sender, data)
		AI.SetBehaviorVariable(entity.id, "GoodLocation", true)
	end,
	
	AdvantagePointNotFound = function(self, entity, sender, data)
		Log("%s - AdvantagePointNotFound", EntityName(entity))
	end,

	OnDestinationReached = function(self, entity)
		--entity:SelectPipe(0, "HeliHovering")
		--self:AnalyzeSituation(entity)
	end,

	OnEnemyMemory = function(self, entity, distance)
		self:AnalyzeSituation(entity)
	end,

	OnExecuteGoTo = function(self, entity)
		entity:Log("GoTo Succeeded")
		self:AnalyzeSituation(entity)
	end,

	OnCancelGoTo = function(self, entity)
		entity:Log("GoTo cancelled")
		AI.SetBehaviorVariable(entity.id, "Goto", false)
	end,

	
	ACT_SHOOTAT = function( self, entity, sender, data )
		AI.CreateGoalPipe("fly_shoot");
		AI.PushGoal("fly_shoot", "fireWeapons", 0, AI_REG_NONE, data.fValue, data.fValue, true, 0)

		entity:InsertSubpipe(AIGOALPIPE_SAMEPRIORITY, "fly_shoot");
	end,


	
	OnGoToSucceeded = function(self, entity)
		entity:Log("GoTo Succeeded")
		BroadcastEvent(entity, "GoToSucceeded")
		AI.SetBehaviorVariable(entity.id, "Goto", false)
	end,

	OnPrepareGoTo = function(self, entity, sender, data)

		local targetPos = g_Vectors.temp_v1
		AI.SetRefPointPosition(entity.id, entity.AI.GoToData.Position)
	end,

})