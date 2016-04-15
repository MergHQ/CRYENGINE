local Behavior = CreateAIBehavior("InVehicleGunner", "HumanGruntUseMountedWeapon",
{
	Constructor = function(self, entity)
		self.parent.Constructor(self, entity);
		
		entity.AI.theVehicle:UserEntered(entity);
	end,

	Destructor = function(self, entity)
		self:LeaveMountedWeapon(entity);
		self.parent.Destructor(self, entity);
	end,
	
 	OnQueryUseObject = function ( self, entity, sender, extraData )
 		-- ignore this signal, execute DEFAULT
 		AIBehavior.DEFAULT:OnQueryUseObject( entity, sender, extraData );
 	end,

 	---------------------------------------------
	START_VEHICLE = function(self,entity,sender)
		AI.LogEvent(entity:GetName().." starting vehicle "..entity.AI.theVehicle:GetName().." with goal type "..entity.AI.theVehicle.AI.goalType);
		local signal = entity.AI.theVehicle.AI.BehaviourSignals[entity.AI.theVehicle.AI.goalType];
		if(type(signal) =="string") then
			AI.Signal(SIGNALFILTER_SENDER,0,signal, entity.AI.theVehicle.id);
		else
			AI.Warning("Wrong signal type in START_VEHICLE - aborting starting vehicle");
		end
	end,
	
	--------------------------------------------
	SHARED_ENTER_ME_VEHICLE = function( self,entity, sender )
		-- in vehicle already - don't do anything
	end,

	--------------------------------------------
	SHARED_LEAVE_ME_VEHICLE = function( self,entity, sender )

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
	EXIT_VEHICLE_STAND = function(self,entity,sender)
	  -- prevent multiple EXIT_VEHICLE_STAND
		if (entity.AI.theVehicle == nil) then
			return;
		end
		-- I'm the gunner, before I check if there are enemies around
		local targetType = AI.GetTargetType(entity.id);
		local targetfound = false;
		if(targetType ==AITARGET_ENEMY or targetType ==AITARGET_SOUND or targetType ==AITARGET_MEMORY) then
			entity:SelectPipe(0,"vehicle_gunner_cover_fire");
		else
			local groupTarget = AI.GetGroupTarget(entity.id,true);
			if(groupTarget) then
				entity:SelectPipe(0,"vehicle_gunner_cover_fire");
				if(groupTarget.id) then 
					entity:InsertSubpipe(0,"acquire_target",groupTarget.id);
				else
					entity:InsertSubpipe(0,"acquire_target",groupTarget);
				end
			else				
				self:EXIT_VEHICLE_DONE(entity,sender);
			end
		end
	end,
	
	--------------------------------------------
	EXIT_VEHICLE_DONE = function(self,entity,sender)
		entity.AI.theVehicle:LeaveVehicle(entity.id);
		entity.AI.theVehicle = nil;
		AI.Signal(SIGNALFILTER_LEADER, 10,"ORD_DONE",entity.id);
		--entity:SelectPipe(0,"stand_only");
	end,

	--------------------------------------------
	ORDER_EXIT_VEHICLE = function(self,entity,sender)
		AI.LogEvent(entity:GetName().." EXITING VEHICLE");
		if(entity.AI.theVehicle) then
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
	------------------------------------------------------------------------
	ORDER_FORM =  function(self,entity,sender)
		self:ORDER_EXIT_VEHICLE(entity,sender);
	end,
	------------------------------------------------------------------------
	ORDER_FOLLOW =  function(self,entity,sender)
		self:ORDER_EXIT_VEHICLE(entity,sender);
	end,
	------------------------------------------------------------------------
	ORDER_FOLLOW_FIRE =  function(self,entity,sender)
		self:ORDER_EXIT_VEHICLE(entity,sender);
	end,
	------------------------------------------------------------------------
	ORDER_FOLLOW_HIDE =  function(self,entity,sender)
		self:ORDER_EXIT_VEHICLE(entity,sender);
	end,
	
	--------------------------------------------------
	-- ignore grenade when in the vehicle
	OnGrenadeDanger = function( self, entity, sender, data )
	end,
	
	------------------------------------------------------------------------
	LeaveMountedWeapon = function( self, entity )
		local vehicle = entity.AI.theVehicle;
		if (not entity:IsDead() and vehicle and not vehicle:IsDriver(entity.id)) then
			entity.AI.theVehicle:LeaveVehicle(entity.id);
			entity.AI.theVehicle = nil;
		end
	end,
})