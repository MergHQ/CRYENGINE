local Behavior = CreateAIBehavior("PlayerIdle",{
---------------------------------------------
	Constructor = function (self, entity)
		-- called when the behaviour is selected
		-- the extra data is from the signal that caused the behavior transition
		
		AI.ChangeParameter(entity.id, AIPARAM_COMBATCLASS, AICombatClasses.Player);
	end,
	---------------------------------------------
	Destructor = function (self, entity,data)
		-- called when the behaviour is de-selected
		-- the extra data is from the signal that is causing the behavior transition
	end,

	---------------------------------------------
	ACT_ENTERVEHICLE = function( self, entity, sender, data )
		local vehicle = entity.AI.theVehicle;
		if ( entity.AI.theVehicle ) then
			-- fail if already inside a vehicle
			--Log( "Player is already inside a vehicle" );
			return;
		end

		-- get the vehicle
		entity.AI.theVehicle = System.GetEntity( data.id );
		local vehicle = entity.AI.theVehicle;
	 	if ( vehicle == nil ) then
	 		-- no vehicle found
	 		return;
	 	end

		local numSeats = count( vehicle.Seats );

		if ( data.fValue<1 or data.fValue>numSeats ) then
			entity.AI.mySeat = vehicle:RequestClosestSeat( entity.id );
		else
			entity.AI.mySeat = data.fValue;
		end
		
		if ( entity.AI.mySeat==nil ) then
			Log( "Can't find the seat" );
			return;
		end
		
		vehicle:ReserveSeat( entity.id, entity.AI.mySeat );
		
		-- always do fast entering on the player
		vehicle:EnterVehicle( entity.id, entity.AI.mySeat, false );
		if(vehicle:IsDriver(entity.id)) then 
			AI.Signal(SIGNALFILTER_LEADER,1,"OnDriverEntered",entity.id);
		end
	end,	
	---------------------------------------------
	ACT_ANIM = function( self, entity, sender )
	end,
	
	
	-----------------------------------------------------------------
	
})