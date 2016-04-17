local Behavior = CreateAIBehavior("VehicleIdle", "VehicleAct",{
	Constructor = function(self , entity )
		self.parent:Constructor( entity );
	end,
	
	OnSomebodyDied = function( self, entity, sender )
	end,

	OnGroupMemberDied = function( self, entity, sender )
	end,

	OnGroupMemberDiedNearest = function( self, entity, sender, data )
		AI.Signal(SIGNALFILTER_GROUPONLY, 1, "OnGroupMemberDied",entity.id,data);
	end,

	---------------------------------------------
	OnPathFound = function( self, entity, sender )
		-- called when the AI has requested a path and it's been computed succesfully
	end,	
	--------------------------------------------------------------------------
	OnNoTarget = function( self, entity )
		-- called when the AI stops having an attention target
	end,
	---------------------------------------------
	OnSomethingSeen = function( self, entity )
		-- called when the enemy sees a foe which is not a living player
	end,
	---------------------------------------------
	OnEnemySeen = function( self, entity, fDistance )
		-- called when the AI sees a living enemy
	end,
	---------------------------------------------
	OnSeenByEnemy = function( self, entity, sender )
	end,
	---------------------------------------------
	OnCloseContact= function( self, entity )
		-- called when AI gets at close distance to an enemy
	end,
	---------------------------------------------
	OnEnemyMemory = function( self, entity )
		-- called when the AI can no longer see its enemy, but remembers where it saw it last
	end,
	---------------------------------------------
	OnInterestingSoundHeard = function( self, entity )
		-- called when the AI hears an interesting sound
	end,
	---------------------------------------------
	OnThreateningSoundHeard = function( self, entity )
		-- called when the AI hears a threatening sound
	end,
	
	--------------------------------------------------------------------------
	OnBulletRain = function ( self, entity, sender, data )	
		self:OnEnemyDamage( entity, sender, data );
	end,
	--------------------------------------------------------------------------
	OnSoreDamage = function ( self, entity, sender, data )
		self:OnEnemyDamage( entity, sender, data );
	end,
	---------------------------------------------
	OnEnemyDamage = function ( self, entity, sender, data )
	end,
	---------------------------------------------
	OnDamage = function ( self, entity, sender, data )
	end,
	---------------------------------------------
	OnObjectSeen = function( self, entity, fDistance, data )
		-- called when the AI sees an object registered for this kind of signal
		-- data.iValue = AI object type
		-- example
		-- if (data.iValue == 150) then -- grenade
		--	 ...
		if ( data.iValue == AIOBJECT_RPG) then
			entity:InsertSubpipe(0,"devalue_target");
		end

	end,
	--------------------------------------------
	OnEnemySeen = function( self, entity, fDistance )

	end,

	---------------------------------------------
	-- CUSTOM
	---------------------------------------------

	---------------------------------------------
	STOP_VEHICLE = function(self,entity,sender,data)
		if ( data ~= nil ) then
			entity:AIDriver(0);
		else
			entity:SignalCrew("EXIT_VEHICLE_STAND");			
			entity:AIDriver(0);
		end
	end,
})
