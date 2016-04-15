local Behavior = CreateAIBehavior("InVehicleControlledGunner", "InVehicleGunner",
{
	Alertness = 2,
	Exclusive = 1,

	Constructor = function( self, entity, sender, data )
		if ( data==nil ) then
			entity:SelectPipe(0,"vehicle_gunner_shoot");
			return;
		end

		if ( entity.actor:IsPlayer() ) then
			AI.LogEvent("ERROR : InVehicleControlledGunner is used for the player");
		else
			entity.AI.vVel = {};
			CopyVector( entity.AI.vVel, entity:GetDirectionVector(1) );
			entity.AI.bGunnerActive = true;
			entity.AI.GunnerAimCount = 0;
			entity.AI.ShootingLimit = data.fValue;
			AI.ChangeParameter( entity.id, AIPARAM_COMBATCLASS,AICombatClasses.VehicleGunner );	
			if ( data~=nil and data.fValue~=nil ) then	
				AI.ChangeParameter( entity.id, AIPARAM_ATTACKRANGE  ,data.fValue * 2.0 );
				AI.ChangeParameter( entity.id, AIPARAM_SIGHTRANGE	  ,data.fValue );
				AI.ChangeParameter( entity.id, AIPARAM_FOVPRIMARY   ,-1 );
				AI.ChangeParameter( entity.id, AIPARAM_FOVSECONDARY	,-1 );
			end

			AI.ChangeParameter(entity.id,AIPARAM_AIM_TURNSPEED,0);
			AI.ChangeParameter(entity.id,AIPARAM_FIRE_TURNSPEED,0);
			AI.ChangeParameter( entity.id, AIPARAM_ACCURACY, 1.0 );
			self:INVEHICLEGUNNER_SHOOT_NEXT( entity );
		end
	end,
	
	Destructor = function( self, entity )	

		-- to make him default

		AI.ChangeParameter( entity.id, AIPARAM_COMBATCLASS,		AICombatClasses.Infantry );		
		AI.ChangeParameter( entity.id, AIPARAM_ACCURACY,			entity.Properties.accuracy );

		AI.ChangeParameter( entity.id, AIPARAM_ATTACKRANGE,		entity.Properties.attackrange );
		AI.ChangeParameter( entity.id, AIPARAM_SIGHTRANGE	,		entity.Properties.Perception.sightrange );

		AI.ChangeParameter( entity.id, AIPARAM_FOVPRIMARY,		entity.Properties.Perception.FOVPrimary);
		AI.ChangeParameter( entity.id, AIPARAM_FOVSECONDARY,	entity.Properties.Perception.FOVSecondary);

 		AI.ChangeParameter( entity.id, AIPARAM_COMBATCLASS, AICombatClasses.Infantry);		

		AI.ChangeParameter(entity.id, AIPARAM_AIM_TURNSPEED, entity.AI.oldAimTurnSpeed);
		AI.ChangeParameter(entity.id, AIPARAM_FIRE_TURNSPEED, entity.AI.oldFireTurnSpeed);

		entity:SelectPipe(0,"do_nothing");
		entity:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE,"clear_all");

	end,
 
 	---------------------------------------------
	OnEnemySeen = function( self, entity, fDistance )
	
		-- called when the enemy sees a living player
		
		if ( entity.actor ) then

			local vehicleId = entity.actor:GetLinkedVehicleId();
			if ( vehicleId ) then

				AI.Signal(SIGNALFILTER_SENDER, 1, "INVEHICLEGUNNER_FOUND_TARGET", vehicleId );

			end

		end
		
	end,

	--------------------------------------------
	-- for the 2nd gunner of the vehicle 
	-- the second gunner is requested to shoot by the vehicle AI

	INVEHICLEGUNNER_REQUEST_SHOOT = function(self,entity,sender,data)
	
		if (entity.AI.bGunnerActive==false) then
			if (sender.id==entity.id) then
				entity.AI.bGunnerActive = true;
				entity.AI.GunnerAimCount = 0;
				entity:SelectPipe(0,"do_nothing");
				AI.CreateGoalPipe("invehiclegunner_request_shoot");
				AI.PushGoal("invehiclegunner_request_shoot","devalue",1,0,1);
				AI.PushGoal("invehiclegunner_request_shoot","signal",0,1,"INVEHICLEGUNNER_SHOOT_NEXT",SIGNALFILTER_SENDER);
				entity:SelectPipe(0,"invehiclegunner_request_shoot");
			end
		end
		
	end,
	
	INVEHICLEGUNNER_SHOOT_NEXT = function(self,entity)

		if ( entity.actor ) then
			local vehicleId = entity.actor:GetLinkedVehicleId();
			if ( vehicleId ) then
				local	vehicleEntity = System.GetEntity( vehicleId );
				if( vehicleEntity ) then
					if ( AI.GetTypeOf( vehicleId ) == AIOBJECT_VEHICLE ) then
						if ( AI.GetSubTypeOf( vehicleId ) == AIOBJECT_HELICOPTER or AI.GetSubTypeOf( vehicleId ) == AIOBJECT_BOAT ) then
							entity.AI.bGunnerActive = false;
							self:INVEHICLEGUNNER_REQUEST_SHOOT2(entity,sender,data);
							return;
						end
					end
				end
			end
		end
	
		local targetEntity = AI.GetAttentionTargetEntity( entity.id );
		if ( targetEntity and AI.Hostile( entity.id, targetEntity.id ) ) then

			AI.CreateGoalPipe("invehiclegunner_shoot_next");
				AI.PushGoal("invehiclegunner_shoot_next","locate",0,"atttarget");
				AI.PushGoal("invehiclegunner_shoot_next","acqtarget",0,"");
				AI.PushGoal("invehiclegunner_shoot_next","firecmd",0,FIREMODE_BURST_DRAWFIRE);
				AI.PushGoal("invehiclegunner_shoot_next","timeout",1,4.0,7.0);
				AI.PushGoal("invehiclegunner_shoot_next","firecmd",0,FIREMODE_OFF);
				AI.PushGoal("invehiclegunner_shoot_next","timeout",1,0.2,0.5);
				AI.PushGoal("invehiclegunner_shoot_next","signal",0,1,"INVEHICLEGUNNER_SHOOT_END",SIGNALFILTER_SENDER);
			entity:SelectPipe(0,"invehiclegunner_shoot_next");

			return;
		else
			entity.AI.bGunnerActive = false;
			entity:SelectPipe(0,"do_nothing");
		end
	
	end,

	INVEHICLEGUNNER_SHOOT_END = function(self,entity)

		local maxCount = 3;
		AI.ChangeParameter( entity.id, AIPARAM_STRAFINGPITCH, 30.0 );

		entity.AI.GunnerAimCount = entity.AI.GunnerAimCount + 1;
		if ( entity.AI.GunnerAimCount < 3 ) then
			AI.CreateGoalPipe("invehiclegunner_shoot_end");
			--AI.PushGoal("invehiclegunner_shoot_end","devalue",0,1);
			--AI.PushGoal("invehiclegunner_shoot_end","timeout",1,0.1);
			AI.PushGoal("invehiclegunner_shoot_end","signal",0,1,"INVEHICLEGUNNER_SHOOT_NEXT",SIGNALFILTER_SENDER);
			entity:SelectPipe(0,"invehiclegunner_shoot_end");
		else
			entity.AI.bGunnerActive = false;
			entity:SelectPipe(0,"do_nothing");
		end

	end,

	--------------------------------------------------------------------------
	INVEHICLEGUNNER_CHECK_VEHICLETARGET = function(self,entity)

		local target = AI.GetAttentionTargetEntity( entity.id );
		if ( target and AI.Hostile( entity.id, target.id ) ) then

			local subType = AI.GetSubTypeOf( target.id );
			if ( subType == AIOBJECT_HELICOPTER ) then
				return	true;
			end
			if ( subType == AIOBJECT_CAR ) then
			  if ( target.AIMovementAbility.pathType == AIPATH_TANK ) then
			  	return true;
			  end
			end

			if ( target.actor ) then
				local vehicleId = target.actor:GetLinkedVehicleId();
				if ( vehicleId ) then
					local	vehicle = System.GetEntity( vehicleId );
					if( vehicle ) then
						local subType = AI.GetSubTypeOf( vehicleId );
						if ( subType == AIOBJECT_HELICOPTER ) then
							return	true;
						end
						if ( subType == AIOBJECT_CAR ) then
						  if ( vehicle.AIMovementAbility.pathType == AIPATH_TANK ) then
						  	return true;
						  end
						end
					end
				end
			end

		end

		return false;

	end,

	----------------------------------------------------------------------------------------
	-- for the heli/vtol special
	
	INVEHICLEGUNNER_REQUEST_SHOOT2 = function(self,entity,sender,data)

		-- MG gun for the heli

		if (entity.AI.bGunnerActive==false) then

			entity:GetVelocity( entity.AI.vVel );
			entity.AI.GunnerAimCount = 0;
			entity.AI.bGunnerActive = true;

			AI.ChangeParameter( entity.id, AIPARAM_STRAFINGPITCH, 40.0 );

			AI.CreateGoalPipe("heliMG_startfire");
			AI.PushGoal("heliMG_startfire","firecmd",0,FIREMODE_FORCED);

			AI.CreateGoalPipe("heliMG_stopfire");
			AI.PushGoal("heliMG_stopfire","firecmd",0,FIREMODE_OFF);

			AI.CreateGoalPipe("invehiclegunner_shoot2_start");
			AI.PushGoal("invehiclegunner_shoot2_start","timeout",1,0.2);
			AI.PushGoal("invehiclegunner_shoot2_start","signal",0,1,"INVEHICLEGUNNER_SHOOT2_START",SIGNALFILTER_SENDER);
			AI.PushGoal("invehiclegunner_shoot2_start","branch",0,-2,BRANCH_ALWAYS);
			entity:SelectPipe(0,"invehiclegunner_shoot2_start");

		end
		
	end,

	INVEHICLEGUNNER_SHOOT2_START = function(self,entity,sender,data)

		local maxTime = 25;
		local maxTime2 = 26;

		local vehicleId = entity.actor:GetLinkedVehicleId();
		if ( vehicleId ) then
			if ( AI.GetSubTypeOf( vehicleId ) == AIOBJECT_BOAT ) then
				maxTime  = 5;
				maxTime2 = 5+random(1,4);
			end
		end

		local targetEntity = AI.GetAttentionTargetEntity( entity.id );
		if ( targetEntity and AI.Hostile( entity.id, targetEntity.id ) ) then

			local stopshoot = false;
			if ( entity.actor ) then
				local vehicleId = entity.actor:GetLinkedVehicleId();
				if ( vehicleId ) then
					local myVehicle = System.GetEntity( vehicleId );
					if ( myVehicle ) then
						local vVec = {};
						CopyVector( vVec, myVehicle:GetDirectionVector(1) );
						vVec.z = 0;
						NormalizeVector( vVec );
						local vVec2 = {};
						SubVectors( vVec2, targetEntity:GetPos(), entity:GetPos() );
						vVec2.z = 0;
						NormalizeVector( vVec2 );
						local dot = dotproduct3d( vVec, vVec2 );
						if ( dot < math.cos( 150.0 * 3.1416 * 2.0 / 360.0 ) ) then
							--stopshoot = true;
						end
					end
				end
			end

			if ( stopshoot == true ) then 
				if ( entity.AI.GunnerAimCount < maxTime ) then
					if ( entity.AI.GunnerAimCount > 0 ) then
						entity.AI.GunnerAimCount =maxTime;
						return;
					end
				end
			end

			if ( entity.AI.GunnerAimCount > 0 ) then
				if ( AI.GetTypeOf( targetEntity.id ) == AIOBJECT_VEHICLE and AI.GetSubTypeOf( targetEntity.id ) == AIOBJECT_CAR ) then
					local	vehicle = System.GetEntity( targetEntity.id );
					if ( vehicle.AIMovementAbility.pathType ~= AIPATH_TANK ) then
						entity:InsertSubpipe(0,"heliMG_stopfire");
						entity.AI.GunnerAimCount = 0;
						return;
					end
				end
			end

			if ( entity.AI.GunnerAimCount == 0 ) then
				if ( stopshoot == true ) then
					return;
				end
				if ( entity.actor ) then
					local vehicleId = entity.actor:GetLinkedVehicleId();
					if ( vehicleId ) then
						if ( AI.GetSubTypeOf( vehicleId ) == AIOBJECT_BOAT ) then
							local targetType = AI.GetTargetType( entity.id );
							if ( targetType ~= AITARGET_MEMORY and targetType ~= AITARGET_SOUND ) then
								if ( entity.AI.ShootingLimit ) then
									if ( DistanceVectors( entity:GetPos(), targetEntity:GetPos() ) < entity.AI.ShootingLimit ) then
										entity:InsertSubpipe(0,"heliMG_startfire");
									end
								else
									entity:InsertSubpipe(0,"heliMG_startfire");
								end
								--System.Log("Boat attack ");
							else
								--System.Log("Boat memory ");
							end
						else
							if ( entity.AI.ShootingLimit ) then
								local targetType = AI.GetTargetType( entity.id );
								if ( targetType ~= AITARGET_SOUND ) then
									if ( entity.AI.ShootingLimit ) then
										if ( DistanceVectors( entity:GetPos(), targetEntity:GetPos() ) < entity.AI.ShootingLimit ) then
											entity:InsertSubpipe(0,"heliMG_startfire");
										end
									else
											entity:InsertSubpipe(0,"heliMG_startfire");
									end
								end
							end
						end
					end
				end
			elseif ( entity.AI.GunnerAimCount == maxTime ) then
				entity:InsertSubpipe(0,"heliMG_stopfire");
			elseif ( entity.AI.GunnerAimCount > maxTime2 ) then
				entity.AI.GunnerAimCount = 0;
				return;
			end

			entity.AI.GunnerAimCount = entity.AI.GunnerAimCount + 1;
			return;

		else
			if ( entity.AI.GunnerAimCount > 0 ) then
				entity.AI.GunnerAimCount =0;
				entity:InsertSubpipe(0,"heliMG_stopfire");
			end
		end

	end,

	INVEHICLEGUNNER_REQUEST_STOP_SHOOT = function( self, entity )
		entity:SelectPipe(0,"do_nothing");
		entity:InsertSubpipe(0,"heliMG_stopfire");
		entity.AI.bGunnerActive=false;
	end,
})
