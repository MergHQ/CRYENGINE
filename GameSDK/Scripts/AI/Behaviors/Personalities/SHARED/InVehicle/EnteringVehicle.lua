local Behavior = CreateAIBehavior("EnteringVehicle", "Dumb",{
	Alertness = -1,
	Exclusive = 1,

	-- SYSTEM EVENTS			-----
	Constructor = function(self, entity)
		-- assuming that the refpoint is near the seat's place
	--	entity:SelectPipe(0,"approach_refpoint");
	--	entity:InsertSubpipe(0,"do_it_running");
	--	entity:InsertSubpipe(0,"do_it_standing");
	--	entity:InsertSubpipe(0,"ignore_all");
	--	entity:InsertSubpipe(0,"clear_all");
	--	entity.AI.theVehicle:EnterVehicle(entity.id,entity.AI.mySeat);

		-- TEMPORARY - TO DO: remove following line
		--AI.Signal(SIGNALFILTER_SENDER,0,"REFPOINT_REACHED",entity.id);
	end,
	
	---------------------------------------------
	OnVehicleLeaving = function ( self, entity, sender )
		-- sent by the CLeader, driver is driving the vehicle away
		-- cancel the enter action
		if(entity.AI.theVehicle) then 
			entity.AI.theVehicle:LeaveVehicle(entity.id, true);
--			entity:SelectPipe(0,"do_nothing");
		end
	end,
	
	---------------------------------------------
	OnQueryUseObject = function ( self, entity, sender, extraData )
		-- ignore this signal, execute DEFAULT
		AIBehavior.DEFAULT:OnQueryUseObject( entity, sender, extraData );
	end,
	---------------------------------------------
	OnSelected = function( self, entity )	
	end,
	---------------------------------------------
	OnSpawn = function( self, entity )

	end,
	---------------------------------------------
	OnActivate = function( self, entity )
		-- called when enemy receives an activate event (from a trigger, for example)
	end,
	---------------------------------------------
	OnNoTarget = function( self, entity )
	
		-- called when the enemy stops having an attention target
	end,
	---------------------------------------------
	OnEnemySeen = function( self, entity, fDistance )
	
	end,
	---------------------------------------------
	OnSomethingSeen = function( self, entity, fDistance )
	end,
	---------------------------------------------
	OnThreateningSeen = function( self, entity )
	end,
	---------------------------------------------
	OnSeenByEnemy = function( self, entity )
	
	end,
	
	---------------------------------------------
	OnEnemyDamage = function( self, entity )
		
	end,
	---------------------------------------------
	OnFriendlyDamage = function( self, entity )
		
	end,
	---------------------------------------------
	OnDamage = function( self, entity )
		
	end,
	---------------------------------------------
	OnNearMiss = function( self, entity )
		
	end,
	
	---------------------------------------------
	OnEnemyMemory = function( self, entity )
		-- called when the enemy can no longer see its foe, but remembers where it saw it last
	end,
	---------------------------------------------
	OnInterestingSoundHeard = function( self, entity )
		-- called when the enemy hears an interesting sound
	end,
	---------------------------------------------
	OnThreateningSoundHeard = function( self, entity )
		-- called when the enemy hears a scary sound
	end,
	---------------------------------------------
	OnReload = function( self, entity )
		-- called when the enemy goes into automatic reload after its clip is empty
	end,
	---------------------------------------------
	OnGroupMemberDied = function( self, entity )
		-- called when a member of the group dies
	end,
	OnGroupMemberDiedNearest = function ( self, entity, sender)
	end,
	---------------------------------------------
	OnNoHidingPlace = function( self, entity, sender )
		-- called when no hiding place can be found with the specified parameters
	end,	
	--------------------------------------------------
	OnNoFormationPoint = function ( self, entity, sender)
		-- called when the enemy found no formation point
	end,
	---------------------------------------------
	OnReceivingDamage = function ( self, entity, sender)
		-- called when the enemy is damaged
	end,
	---------------------------------------------
	OnCoverRequested = function ( self, entity, sender)
		-- called when the enemy is damaged
	end,
	--------------------------------------------------
	OnBulletRain = function ( self, entity, sender)
		-- called when the enemy detects bullet trails around him
	end,
	--------------------------------------------------
	OnDeath = function( self,entity )
		AI:Signal(SIGNALFILTER_GROUPONLY, 1, "OnGroupMemberDied",entity.id);	
		-- if driver killed - make all the passengers unignorant, get out/stop entering
		
		if(entity.AI.theVehicle )	then
			local tbl = VC.FindUserTable( entity.AI.theVehicle, entity );
			if( tbl and tbl.type == PVS_DRIVER ) then
				AI:Signal(SIGNALFILTER_GROUPONLY, -1, "MAKE_ME_UNIGNORANT",entity.id);		
			end
		end	
	end,
	
	---------------------------------------------
	---------------------------------------------	--------------------------------------------------
	-- CUSTOM
	--------------------------------------------
	exited_vehicle = function( self,entity, sender )
		AI.Signal(SIGNALFILTER_LEADER,10,"ORD_DONE",entity.id);
	end,

	exited_vehicle_investigate = function( self,entity, sender )
		--AI.Signal(SIGNALFILTER_LEADER,10,"OnUnitResumed",entity.id);

	end,

	--------------------------------------------
	do_exit_vehicle = function( self,entity, sender )
		--AI.Signal(SIGNALFILTER_LEADER,10,"OnUnitResumed",entity.id);

	end,


	--------------------------------------------------
	ENTER_VEHICLE = function( self,entity, sender )
	
	-- going in vehicle already - don't do anything

	end,
	--------------------------------------------------
	
	ENTER_VEHICLE_DRIVER = function( self,entity, sender )
	-- in vehicle already - don't do anything
	end,
	--------------------------------------------------
	
	ENTER_VEHICLE_NODRIVER = function( self,entity, sender )
	-- in vehicle already - don't do anything
	end,
	--------------------------------------------
	OnVehicleSuggestion = function(self, entity) --Luciano
		-- I'm already entering a vehicle, I won't change it at any condition!
	end,
	
	--------------------------------------------
	desable_me = function( self,entity, sender )
		entity:TriggerEvent(AIEVENT_DISABLE);
	end,

	--------------------------------------------
	-- no need to run away from cars? maybe hide
	OnVehicleDanger = function(self,entity,sender)
	end,
	
	--------------------------------------------------
	HEADS_UP_GUYS = function( self,entity, sender )
	-- no matter what, I'll keep my head down to my vehicle seat...
	end,


	REFPOINT_REACHED = function(self,entity,sender)
--		entity.AI.theVehicle:EnterVehicle(entity.id,entity.AI.mySeat,true);
		entity:SelectPipe(0,"do_nothing");
	end,

 	---------------------------------------------

	ORDER_FOLLOW = function(self,entity,sender)
	end,

 	---------------------------------------------	


	ORDER_EXIT_VEHICLE = function(self,entity,sender)
		--AI.LogEvent(entity:GetName().." EXITING VEHICLE");
		entity.AI.theVehicle:LeaveVehicle(entity.id,true);
		entity.AI.theVehicle = nil;
		AI.Signal(SIGNALFILTER_LEADER, 10,"ORD_DONE",entity.id);
		--entity:SelectPipe(0,"stand_only");
	end,

	---------------------------------------------
	CANCEL_CURRENT = function(self,entity,sender)
		entity:CancelSubpipe();
		entity.AI.theVehicle.vehicle:ExitVehicle(entity.id);
		entity.AI.theVehicle = nil;
		AI.Signal(SIGNALFILTER_SENDER,1,"do_exit_vehicle",entity.id);
	end,
})
