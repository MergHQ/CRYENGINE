local Behavior = CreateAIBehavior("VehicleGoto", "VehicleAct",{
	Constructor = function(self , entity )
	end,

	OnActivate = function(self, entity )
		self.allowed = 1;
	end,	
	OnEndPathOffset = function(self, entity)
		AI.LogEvent(entity:GetName().." couldn't reach the goto destination");
	end,
	---------------------------------------------
	OnNoTarget = function(self, entity )
	end,

	---------------------------------------------
	OnEnemyMemory = function( self, entity )
	end,

	--------------------------------------------
	OnEnemySeen = function( self, entity, fDistance )
	end,

	--------------------------------------------------------------------------
	OnSomebodyDied = function( self, entity, sender )
	end,

	OnGroupMemberDied = function( self, entity, sender )
	end,

	OnGroupMemberDiedNearest = function( self, entity, sender, data )
		AI.Signal(SIGNALFILTER_GROUPONLY, 1, "OnGroupMemberDied",entity.id,data);
	end,

	---------------------------------------------
	OnPlayerMemory = function(self, entity )
	end,
	---------------------------------------------
	OnDeadFriendSeen = function(self,entity )
	end,

	---------------------------------------------
	OnDied = function( self,entity )
	end,
	---------------------------------------------

	---------------------------------------------------------------------------------------------------------------------------------------
	---------------------------------------------------------------------------------------------------------------------------------------
	--
	--	FlowGraph	actions 
	--
	---------------------------------------------------------------------------------------------------------------------------------------
})
