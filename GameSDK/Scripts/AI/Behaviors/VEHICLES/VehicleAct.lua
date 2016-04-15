local Behavior = CreateAIBehavior("VehicleAct",
{
	Constructor = function(self , entity )
	end,
	
	---------------------------------------------
	ACT_GOTO = function( self,entity,sender,data )
		System.Log(">>>> ACT_MOVE "..entity:GetName());	
	--	entity:Act_Move( data );

		if ( entity.AIMovementAbility and entity.AIMovementAbility.pathType == AIPATH_TANK ) then
			AI.SetAdjustPath(entity.id,1);
		end
	
		-- fail if there's no ai driver inside
		if (entity.State.aiDriver ~= 1) then
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
			
			-- disable vehicle's AI once again since it was enabled by the FG node to process this signal only
			entity:AIDriver(0);
			return;
		end

		-- AIBehavior.DEFAULT:ACT_GOTO( entity,sender,data );
		if ( data and data.point ) then
			AI.SetRefPointPosition( entity.id, data.point );

			-- use dynamically created goal pipe to set approach distance
			g_StringTemp1 = "action_goto"..data.fValue;
			AI.CreateGoalPipe(g_StringTemp1);
			AI.PushGoal(g_StringTemp1, "locate", 0, "refpoint");
			AI.PushGoal(g_StringTemp1, "+stick", 1, data.point2.x, AILASTOPRES_USE, 1, data.fValue);	-- noncontinuous stick
			AI.PushGoal(g_StringTemp1, "+branch", 0, "NO_PATH", IF_LASTOP_FAILED );
			AI.PushGoal(g_StringTemp1, "branch", 0, "END", BRANCH_ALWAYS );
			AI.PushLabel(g_StringTemp1, "NO_PATH" );
			AI.PushGoal(g_StringTemp1, "signal", 1, 1, "CANCEL_CURRENT", 0);
			AI.PushLabel(g_StringTemp1, "END" );
			AI.PushGoal(g_StringTemp1, "signal", 0, 1, "VEHICLE_GOTO_DONE",SIGNALFILTER_SENDER);		
			

			entity.gotoSubpipeID = data.iValue; -- Store the goal pipe ID here so we can cancel it correctly later /Jonas			
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, g_StringTemp1, nil, data.iValue );
		end
		
	end,

	---------------------------------------------
	ACT_FOLLOWPATH = function( self, entity, sender, data )

		local pathfind = data.point.x;
		local reverse = data.point.y;
		local startNearest = data.point.z;
		local loops = data.point2.x;
		local speed = data.fValue;
		local controlSpeed = true;
		if(data.iValue2 == 1) then
			controlSpeed = false;
		end		
			--System.Log("Vehicle loops: " .. loops);
		AI.SetPathAttributeToFollow( entity.id, true );

		g_StringTemp1 = "follow_path";
		if(pathfind > 0) then
			g_StringTemp1 = g_StringTemp1.."_pathfind";
		end
		if(reverse > 0) then
			g_StringTemp1 = g_StringTemp1.."_reverse";
		end
		if(startNearest > 0) then
			g_StringTemp1 = g_StringTemp1.."_nearest";
		end
		
	  AI.CreateGoalPipe(g_StringTemp1);
    AI.PushGoal(g_StringTemp1, "followpath", 1, pathfind, reverse, startNearest, loops, 0.1, false, controlSpeed, speed);
		AI.PushGoal(g_StringTemp1, "signal", 1, 1, "END_ACT_FOLLOWPATH",0);
    
    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, g_StringTemp1, nil, data.iValue );
	end,

	---------------------------------------------
	ACT_VEHICLESTICKPATH = function( self, entity, sender, data )
		
		AI.SetVehicleStickTarget(entity.id, data.id);

		local canReverse = (data.point.z > 0);
		local finishInRange = (data.iValue2 == 0);

		AI.CreateGoalPipe("vehicle_stickpath");
		AI.PushGoal("vehicle_stickpath", "stickpath", 1, finishInRange, data.point.y, data.point.x, canReverse);

		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "vehicle_stickpath", nil, data.iValue );

	end,

	---------------------------------------------
	UnloadSeat = function( self,entity,seatId )
		local seat = entity:GetSeatByIndex( seatId );
		if ( seat ) then
			local PassengerId = seat:GetPassengerId();
			
			if ( PassengerId ) then
				local Passenger = System.GetEntity(PassengerId);
				if (Passenger and (not Passenger.actor:IsPlayer())) then
					entity.AI.unloadCount = entity.AI.unloadCount + 1;
					entity:LeaveVehicle( PassengerId );
					local Passenger = System.GetEntity( PassengerId );
					Passenger.AI.theVehicle = nil;
					--Passenger:SelectPipe( 0, "stand_only" );
					return true;
				end
			end
		end
		return false;
	end,
	
	
	---------------------------------------------
	DO_UNLOAD = function( self,entity,sender )
		entity.AI.unloadCount = entity.AI.unloadCount - 1;
		if (entity.AI.unloadCount == 0) then
			AI.Signal(SIGNALFILTER_SENDER, 9, "UNLOAD_DONE", entity.id); -- 9 is to skip normal processing of signal, only notify the waitsignal goal op.
			if (entity.State.aiDriver == nil) then
				--self:AIDriver(0);
			end;
		end
	end,
	
	---------------------------------------------
	ACT_UNLOAD = function( self,entity,sender,data )
		local numSeats = count( entity.Seats );
		entity.AI.unloadCount = 1; -- counter increased by one to wait for DO_UNLOAD signal to be processed
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "check_driver" );
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_unload", nil, data.iValue );
		if ( data.fValue == 0 ) then
			-- exit all
			while ( numSeats > 0 ) do
				self:UnloadSeat( entity, numSeats );
				numSeats = numSeats - 1;
			end
		elseif ( data.fValue == -1 ) then
			-- exit all except the driver
			while ( numSeats > 1 ) do
				self:UnloadSeat( entity, numSeats );
				numSeats = numSeats - 1;
			end
		elseif ( data.fValue == -2 ) then
			-- exit all except the gunner
			while ( numSeats > 2 ) do
				self:UnloadSeat( entity, numSeats );
				numSeats = numSeats - 1;
			end
			self:UnloadSeat( entity, 1 );
		else
			-- exit one specified passenger
			if ( not self:UnloadSeat(entity, data.fValue) ) then
				entity:CancelSubpipe( data.iValue );
				return;
			end
		end
	end,

	MAKE_ME_IGNORANT = function ( self, entity, sender)
		entity.AI.vehicleIgnorantIssued = true;
		AI.SetIgnorant(entity.id,1);
	end,
	
	MAKE_ME_UNIGNORANT = function ( self, entity, sender)
		entity.AI.vehicleIgnorantIssued = false;
		AI.SetIgnorant(entity.id,0);
	end,

	ACT_SHOOTAT_HELI  = function( self, entity, sender, data )

		local vIpos = {};
		local vIdir = { x=0.0, y=0.0, z=-1.0 };
		FastScaleVector( vIpos, entity:GetDirectionVector(1), 2.5 );
		FastSumVectors( vIpos, vIpos, entity:GetCenterOfMassPos() );
		entity:AddImpulse( -1, vIpos, vIdir, entity:GetMass()* 10.0 );	
	
	end,
	
	-- make the guy shoot at the provided point	for fValue seconds
	ACT_SHOOTAT = function( self, entity, sender, data )

		if ( AI.GetTypeOf( entity.id ) == AIOBJECT_VEHICLE and AI.GetSubTypeOf( entity.id ) == AIOBJECT_HELICOPTER ) then
		
			AI.CreateGoalPipe("action_shoot_at");
			AI.PushGoal("action_shoot_at", "locate",0,"refpoint");
			AI.PushGoal("action_shoot_at", "lookat",0,0,0,true,1);
			AI.PushGoal("action_shoot_at", "+timeout",1,3.0);
			AI.PushGoal("action_shoot_at", "signal",0,1,"ACT_SHOOTAT_HELI",SIGNALFILTER_SENDER);
			AI.PushGoal("action_shoot_at", "locate",0,"refpoint");
			AI.PushGoal("action_shoot_at", "+firecmd",0,FIREMODE_CONTINUOUS,AILASTOPRES_USE);
			AI.PushGoal("action_shoot_at", "+timeout",1,3.0);
			AI.PushGoal("action_shoot_at", "firecmd",0,0);
			AI.SetRefPointPosition( entity.id, data.point );
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_shoot_at", nil, data.iValue );
		else
			-- for tank/AAA/warrior/
			AI.CreateGoalPipe("action_shoot_at");
			AI.PushGoal("action_shoot_at", "locate",0,"refpoint");
			AI.PushGoal("action_shoot_at", "+firecmd",0,FIREMODE_CONTINUOUS,AILASTOPRES_USE);
			AI.PushGoal("action_shoot_at", "+timeout",1,data.fValue);
			AI.PushGoal("action_shoot_at", "firecmd",0,0);
			AI.SetRefPointPosition( entity.id, data.point );
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_shoot_at", nil, data.iValue );

		end

	end,
})
