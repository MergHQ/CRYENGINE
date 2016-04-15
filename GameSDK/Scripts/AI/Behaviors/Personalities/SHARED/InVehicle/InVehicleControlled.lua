local Behavior = CreateAIBehavior("InVehicleControlled", "InVehicle",
{	Exclusive = 1,

	Constructor = function( self, entity, sender, data )
		if ( entity.actor:IsPlayer() ) then
			AI.LogEvent("ERROR : InVehicleControlledGunner is used for the player");
		else
			self:INVEHICLE_CONTROL_START( entity );
		end
	end,

	INVEHICLE_CONTROL_START = function( self, entity )
		AI.CreateGoalPipe("Invehicle_controltimer");
		AI.PushGoal("Invehicle_controltimer","timeout",1,random(1,2));
		AI.PushGoal("Invehicle_controltimer","signal",0,1,"INVEHICLE_REQUEST_START_FIRE",SIGNALFILTER_SENDER);
		AI.PushGoal("Invehicle_controltimer","timeout",1,random(6,12));
		AI.PushGoal("Invehicle_controltimer","signal",0,1,"INVEHICLE_REQUEST_STOP_FIRE",SIGNALFILTER_SENDER);
		entity:InsertSubpipe(0,"Invehicle_controltimer");
	end,

	INVEHICLE_REQUEST_START_FIRE = function( self, entity )
		local vehicleId = entity.actor:GetLinkedVehicleId();
		if ( vehicleId ) then
			AI.Signal(SIGNALFILTER_SENDER, 1, "INVEHICLE_REQUEST_START_FIRE", vehicleId);
		end
	end,
	
	INVEHICLE_REQUEST_STOP_FIRE = function( self, entity )

		local vehicleId = entity.actor:GetLinkedVehicleId();
		if ( vehicleId ) then
			AI.Signal(SIGNALFILTER_SENDER, 1, "INVEHICLE_REQUEST_STOP_FIRE", vehicleId);
		end

	end,
})