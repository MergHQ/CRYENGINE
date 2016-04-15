local Behavior = CreateAIBehavior("HeliIdle",
{
	Constructor = function (self, entity)
		self:AnalyzeSituation(entity)
	end,

	Destructor = function(self, entity)
	end,
	
	AnalyzeSituation = function(self, entity, sender, data)
		entity:SelectPipe(0, "HeliIdle")
	end,

	OnEnemySeen = function(self, entity, distance)
--		Log("%s - OnEnemySeen Heli", EntityName(sender))
	end,
	
	GoodLocationReached = function(self, entity, sender, data)
		AI.SetBehaviorVariable(entity.id, "GoodLocation", true)
	end,
	
	AdvantagePointNotFound = function(self, entity, sender, data)
--		Log("%s - AdvantagePointNotFound", EntityName(entity))
	end,

	OnCancelGoTo = function(self, entity)
--		entity:Log("GoTo cancelled")
		AI.SetBehaviorVariable(entity.id, "Goto", false)
	end,

	


	ACT_SHOOTAT = function( self, entity, sender, data )

		local target = System.GetEntity(data.id)
		if (target) then
			local data1 = { {name = "concurrent", value="HeliShootTest", id=data.iValue}, { name = "target", value=data.id,}, {name="useSecondary",  value=data.iValue2,}, { name="fireDuration", value=data.fValue,}, };
			entity:PassParamsToPipe(data1);
		else
			AI.SetRefPointPosition(entity.id, data.point)
			local data1 = { {name = "concurrent", value="HeliShootTest", id=data.iValue}, {name="useSecondary", value=data.iValue2,}, { name="fireDuration", value=data.fValue,}, };
			entity:PassParamsToPipe(data1);
		end

	end,

	OnShootAtDone = function( self, entity, sender, data )

		entity:PassParamsToPipe(data1);
	end,

	ACT_AIMAT = function( self, entity, sender, data )

		local target = System.GetEntity(data.id)
		if (target) then
			local data1 = {{name = "concurrent", value="HeliShootTest", id=data.iValue}, { name = "target", value=data.id,}, {name="onlyLookAt", value=true,}, {name="useSecondary", value=data.iValue2,}, { name="fireDuration", value=data.fValue,}, };
			entity:PassParamsToPipe(data1);
		else
			AI.SetRefPointPosition(entity.id, data.point);
			local data1 = {{name = "concurrent", value="HeliShootTest", id=data.iValue}, {name="onlyLookAt", value=true,}, {name="useSecondary", value=data.iValue2,}, { name="fireDuration", value=data.fValue,}, };
			entity:PassParamsToPipe(data1);
		end

	end,

	---------------------------------------------
	ACT_CHASETARGET = function( self, entity, sender, data )
		
		AI.SetVehicleStickTarget(entity.id, data.id);

		local tbl = AI.__chaseTargetTmp
		if (not tbl) then
			tbl = {
				{}, 
				{}, 
				{}, 
			}
			AI.__chaseTargetTmp = tbl;
		end
		
		tbl[1].name = "desiredSpeed"
		tbl[1].value = data.fValue
		
		tbl[2].name = "distanceMin"
		tbl[2].value = data.point.x

		tbl[3].name = "distanceMax"
		tbl[3].value = data.point.y

		entity:SelectPipe(0, "HeliChaseTarget", 0, 0, 1, tbl)
	end,

	---------------------------------------------
	ACT_VEHICLESTICKPATH = function( self, entity, sender, data )
		
		AI.SetVehicleStickTarget(entity.id, data.id);

		local canReverse = (data.point.z > 0);
		local finishInRange = (data.iValue2 == 0);

		--AI.CreateGoalPipe("vehicle_stickpath");
		--AI.PushGoal("vehicle_stickpath", "stickpath", 1, finishInRange, data.point.y, data.point.x, canReverse);

		--entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "vehicle_stickpath", nil, data.iValue );


		local data1 = {{ name = "desiredSpeed", value=data.fValue,}, {name="continuous", value=data.iValue2,}, { name="targetEntity", value=data.id,}, {name="minDistance", value=data.point.x,}, {name="maxDistance", value=data.point.y,}, }
		entity:SelectPipe(0, "HeliFollowPath", 0, 0, 1, data1)

	end,

	OnFollowPathDone = function(self, entity)
		BroadcastEvent(entity, "GoToSucceeded")
	end,

})
