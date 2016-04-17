local Behavior = CreateAIBehavior("InVehicle", "Dumb",
{
	Constructor = function(self, entity)
		entity.AI.theVehicle:UserEntered(entity);
	end,
	
	Destructor = function( self, entity )	
		entity:SelectPipe(0,"do_nothing");
		entity:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE,"clear_all");
	end,
	
 	OnQueryUseObject = function ( self, entity, sender, extraData )
 		-- ignore this signal, execute DEFAULT
 		AIBehavior.DEFAULT:OnQueryUseObject( entity, sender, extraData );
 	end,

 	---------------------------------------------
	START_VEHICLE = function(self,entity,sender)
		AI.LogEvent(entity:GetName().." starting vehicle "..entity.AI.theVehicle:GetName().." with goal type "..(entity.AI.theVehicle.AI.goalType or "nil"));
		local signal = entity.AI.theVehicle.AI.BehaviourSignals[entity.AI.theVehicle.AI.goalType];
		if(type(signal) =="string") then
			AI.LogEvent(">>>> signal "..signal);
			AI.Signal(SIGNALFILTER_SENDER,0,signal, entity.AI.theVehicle.id);
		else
			AI.Warning("Wrong signal type in START_VEHICLE - aborting starting vehicle");
		end
		entity:SelectPipe(0,"do_nothing");
	end,
	
	--------------------------------------------------
	SHARED_ENTER_ME_VEHICLE = function( self,entity, sender )
		-- in vehicle already - don't do anything
	end,

	--------------------------------------------------
	SHARED_LEAVE_ME_VEHICLE = function( self, entity, sender )

		if( entity.ai == nil ) then return end

		if (entity.AI.theVehicle == nil) then
			return;
		end

		local vUp = { x=0.0, y=0.0, z=1.0 };	
		if ( entity.AI.theVehicle:GetDirectionVector(2) ) then
			if ( dotproduct3d( entity.AI.theVehicle:GetDirectionVector(2), vUp ) < math.cos( 60.0*3.1416 / 180.0 ) ) then
				return;
			end
		end
			
		entity.AI.theVehicle:LeaveVehicle(entity.id);
		entity.AI.theVehicle = nil;

		-- this handler is called when needs a emergency exit.
		-- should not request any goal pipe and entity.AI.theVehicle:AIDriver(0);
		-- 20/16/2006 Tetsuji

	end,

	--------------------------------------------
	exited_vehicle = function( self,entity, sender )
		AI.Signal(SIGNALFILTER_LEADER,10,"ORD_DONE",entity.id);
	end,

	--------------------------------------------
	exited_vehicle_investigate = function( self,entity, sender )

	end,

	--------------------------------------------
	do_exit_vehicle = function( self,entity, sender )

--		entity:TriggerEvent(AIEVENT_ENABLE);
--		entity:SelectPipe(0,"reevaluate");
AI.LogEvent( "puppet -------------------------------- exited_vehicle " );
--		entity:SelectPipe(0,"standingthere");		
		
		AI.SetIgnorant(entity.id,0);
		entity:SelectPipe(0,"b_user_getout", entity:GetName().."_land");
		
--		Previous();
	end,

	--------------------------------------------
	-- no need to run away from cars
	OnVehicleDanger = function(self,entity,sender)
	end,

	--------------------------------------------
	EXIT_VEHICLE_STAND_PRE = function(self,entity,sender,data)

		AI.CreateGoalPipe("exitvehiclestandpre");
		AI.PushGoal("exitvehiclestandpre","timeout",1,data.fValue+0.1);
		AI.PushGoal("exitvehiclestandpre","signal",1,1,"EXIT_VEHICLE_STAND",SIGNALFILTER_SENDER);
		entity:SelectPipe(0,"exitvehiclestandpre");

	end,

	--------------------------------------------
	EXIT_VEHICLE_STAND = function(self,entity,sender)
	  -- prevent multiple EXIT_VEHICLE_STAND
		if (entity.AI.theVehicle == nil) then
			return;
		end
		if(entity.AI.theVehicle:IsDriver(entity.id)) then
			-- disable vehicle's AI
			entity.AI.theVehicle:AIDriver(0);
		end
		entity.AI.theVehicle:LeaveVehicle(entity.id);
		entity.AI.theVehicle = nil;
		AI.Signal(SIGNALFILTER_LEADER, 10,"ORD_DONE",entity.id);
		--entity:SelectPipe(0,"stand_only");
	end,
	
	--------------------------------------------
	ORDER_EXIT_VEHICLE = function(self,entity,sender)
		--AI.LogEvent(entity:GetName().." EXITING VEHICLE");
		if (entity.AI.theVehicle) then
			entity.AI.theVehicle:LeaveVehicle(entity.id);
			entity.AI.theVehicle = nil;
		end
		AI.Signal(SIGNALFILTER_LEADER, 10,"ORD_DONE",entity.id);
		--entity:SelectPipe(0,"stand_only");
	end,
	
	------------------------------------------------------------------------
	ORDER_ENTER_VEHICLE = function(self,entity,sender)
		AI.Signal(SIGNALFILTER_LEADER,10,"ORD_DONE",entity.id);
	end,
	
 	---------------------------------------------
	-- ignore this orders when in vehicle
	ORDER_FOLLOW = function(self,entity,sender)
	end,
	ORDER_HIDE = function(self,entity,sender)
	end,
	ORDER_FIRE = function(self,entity,sender)
	end,
	ACT_FOLLOW = function(self,entity,sender)
	end,
	DO_SOMETHING_IDLE = function( self,entity , sender)
	end,
	
	--------------------------------------------------
	-- ignore grenade when in the vehicle
	OnGrenadeDanger = function( self, entity, sender, data )
	end,
	
	--------------------------------------------------
	-- for the driver
	-- the driver is requested to contorl vehicle
	INVEHICLE_REQUEST_CONTROL = function(self,entity,sender,data)
		AI.Signal(SIGNALFILTER_SENDER,1,"controll_vehicle",entity.id,data);
	end,

	--------------------------------------------------
	-- instant changing seat
	INVEHICLE_CHANGESEAT_TODRIVER = function(self,entity,sender,data)
		entity.AI.bChangeSeat = true;
	end,

	--------------------------------------------------
	ACT_DUMMY = function(self,entity,sender,data)
	
		if ( entity.AI.bChangeSeat ~=nil and entity.AI.bChangeSeat == true ) then

		else
			AIBehavior.DEFAULT:ACT_DUMMY( entity, sender, data );
		end

		entity.AI.bChangeSeat = false;

	end,
})