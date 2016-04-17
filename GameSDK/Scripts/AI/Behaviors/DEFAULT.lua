-- Default behavior - implements all the system callbacks and does something
-- this is so that any enemy has a behavior to fallback to
local Behavior = CreateAIBehavior("DEFAULT", {
	-- this signal is sent when a smart object should be used
	---------------------------------------------
	OnUseSmartObject = function ( self, entity, sender, extraData )
		-- by default we just execute the requested action
		-- note: extraData.iValue is Action id and sender is the Object which should be used
		AI.ExecuteAction( extraData.ObjectName, entity.id, sender.id, extraData.iValue );
	end,
	
	-- send this signal to abort all actions
	---------------------------------------------
	ABORT_ALL_ACTIONS = function( self, entity )--, sender, data )
		AI.AbortAction( entity.id );
	end,
	
	-- this signal should be sent only by smart objects
	---------------------------------------------
	OnReinforcementRequested = function ( self, entity, sender, extraData )
		local pos = {};
		AI.GetBeaconPosition( extraData.id, pos );
		AI.SetBeaconPosition( entity.id, pos );
		AI.Signal( SIGNALFILTER_SENDER, 1, "GO_TO_SEEK", entity.id, sender.id );
	end,
	
	---------------------------------------------
	OnQueryUseObject = function ( self, entity, sender, extraData )
	end,

	---------------------------------------------
	OnLeaderDied = function ( self, entity, sender)
		entity.AI.InSquad = 0;
	end,
	
	---------------------------------------------
	SHARED_USE_THIS_MOUNTED_WEAPON = function( self, entity )
		local weapon = entity.AI.current_mounted_weapon;
		if(entity:GetDistance(weapon.id)<3) then 
			--AI.LogEvent(entity:GetName().." Uses mounted weapon |>>><| "..entity.AI.current_mounted_weapon:GetName());
			entity.actor:HolsterItem( true );
			local mwitem = entity.AI.current_mounted_weapon.item;
			if(mwitem and not mwitem:IsUsed()) then 
				mwitem:StartUse( entity.id );		
			end
			weapon.listPotentialUsers = nil;
		else
			-- something went wrong with reaching weapon
			AI.Signal(SIGNALFILTER_SENDER,1,"TOO_FAR_FROM_WEAPON",entity.id);
			entity:DrawWeaponNow();
		end
		entity.AI.approachingMountedWeapon = false;
		local targettype = AI.GetTargetType(entity.id);
		if(targettype==AITARGET_ENEMY) then 
			entity:SelectPipe(0,"fire_mounted_weapon");
		else
			entity:SelectPipe(0,"mounted_weapon_look_around");
		end
--		elseif(targettype~=AITARGET_NONE and targettype~=AITARGET_FRIENDLY) then 
--			entity:SelectPipe(0,"near_mounted_weapon_blind_fire");
--		end
	end,
	
	---------------------------------------------
	LOOK_AT_MOUNTED_WEAPON_DIR = function(self,entity,sender)
			local pos = g_Vectors.temp;
			-- workaround to make the guy not snap the MG orientation
			local weapon = entity.AI.current_mounted_weapon;
			if ( weapon == nil ) then
				AI.LogEvent("WARNING: weapon is nil in LOOK_AT_MOUNTED_WEAPON_DIR for "..entity:GetName());
			else
				FastSumVectors(pos,weapon:GetPos(),weapon.item:GetMountedDir());
				FastSumVectors(pos,pos,weapon.item:GetMountedDir());
				AI.SetRefPointPosition(entity.id,pos);
				AI.SetRefPointDirection(entity.id,weapon.item:GetMountedDir());
				local targetType = AI.GetTargetType(entity.id);
	--			if(targetType==AITARGET_NONE or targetType==AITARGET_FRIENDLY) then 
	--				entity:SelectPipe(0,"mounted_weapon_look_around");
	--			else
	--				entity:SelectPipe(0,"do_nothing");
	--			end
	--	   	entity:InsertSubpipe(0, "look_at_refpoint_if_no_target");
				AI.Signal(SIGNALFILTER_SENDER, 1, "SHARED_USE_THIS_MOUNTED_WEAPON", entity.id);
			end
	end,
	
	---------------------------------------------
	SET_MOUNTED_WEAPON_PERCEPTION = function(self,entity,sender)
			local perceptionTable = entity.Properties.Perception;
			local newSightRange = perceptionTable.sightrange * 2;

			AI.ChangeParameter( entity.id, AIPARAM_ACCURACY, 1 );

			-- make bigger sight-range/attack-range; mounted weapon has to shoot a lot
 			AI.ChangeParameter(entity.id,AIPARAM_SIGHTRANGE, newSightRange);	-- add 100% to sight range
--			AI.ChangeParameter(entity.id,AIPARAM_ATTACKRANGE,entity.Properties.attackrange*2);	-- 
			AI.ChangeParameter(entity.id,AIPARAM_ATTACKRANGE, newSightRange);	
			AI.ChangeParameter(entity.id,AIPARAM_FOVSECONDARY, perceptionTable.FOVPrimary);
	end,
	
	MaxDistanceToMountedWeapon = 7,
	
	---------------------------------------------
	MOUNTED_WEAPON_USABLE = function(self,entity,sender,data) 
		-- sent by smart object rule
		local weapon = System.GetEntity(data.id);
		-- if use RPG - can not use MG
		local curWeapon = entity.inventory:GetCurrentItem();
		if(curWeapon and curWeapon.class=="LAW") then
			weapon = nil;
		end	
		
		-- if MG SO class assigned not to weapon but something else (designers mistake) - this will happen
		if(weapon and weapon.item==nil) then
			AI.LogEvent("trying to use "..weapon:GetName().." as weapon. Please check SO class");
			weapon = nil;
		end

		if(weapon and Game.IsMountedWeaponUsableWithTarget(entity.id,weapon.id,MaxDistanceToMountedWeapon,entity.AI.SkipTargetCheck)) then 
			weapon.reserved = entity;
			entity.AI.current_mounted_weapon = weapon;
			local parent = weapon:GetParent();
			if(parent and parent.vehicle) then 
				-- the weapon is mounted on a vehicle
				g_SignalData.fValue = mySeat;
				g_SignalData.id = parent.id;
				g_SignalData.iValue2 = 0; -- no "fast entering"
				g_SignalData.iValue = -148; -- this is just a random number used as goal pipe id
				AI.Signal(SIGNALFILTER_SENDER, 1, "ACT_ENTERVEHICLE", entity.id, g_SignalData);
			else
				AI.Signal(SIGNALFILTER_SENDER, 0, "USE_MOUNTED_WEAPON", entity.id);
			end
			AI.ModifySmartObjectStates(entity.id,"Busy");				
		else
			if(weapon) then 
				AI.ModifySmartObjectStates(weapon.id,"Idle,-Busy");				
			end
			AI.ModifySmartObjectStates(entity.id,"-Busy");			
		end
	end,

	---------------------------------------------
	ORDER_HIDE = function( self, entity, sender, data )
		AI.SetRefPointPosition(entity.id, data.point);
	end,

	---------------------------------------------
	ORDER_SEARCH = function( self, entity, sender, data )
		AI.SetRefPointPosition(entity.id, data.point);
	end,

	---------------------------------------------
	OnFriendInWay = function ( self, entity, sender)
--		local rnd=random(1,4);
--		entity:InsertSubpipe(0,"friend_circle");
	end,

	---------------------------------------------
	OnReceivingDamage = function ( self, entity, sender)
		-- called when the enemy is damaged
		-- called as default handling by some behaviors
	end,

	---------------------------------------------
	MAKE_ME_IGNORANT = function ( self, entity, sender)
		AI.SetIgnorant(entity.id,1);
	end,
	
	---------------------------------------------
	MAKE_ME_UNIGNORANT = function ( self, entity, sender)
		AI.SetIgnorant(entity.id,0);
	end,

	---------------------------------------------
	OnRestoreVehicleDanger = function(self, entity)
		entity.AI.avoidingVehicleTime = nil;
	end,
	
	---------------------------------------------
	RETURN_TO_FIRST = function( self, entity, sender )
	end,

	-- Everyone has to be able to warn anyone around him that he died
	--------------------------------------------------
	OnDeath = function ( self, entity, sender)
	end,

	-- do melee when really close
	--------------------------------------------------
	OnCloseContact = function ( self, entity, sender,data)
	end,

	--------------------------------------------------
	OnOutOfAmmo = function (self,entity, sender)
	-- player would not have Reload implemented
	if(entity.Reload == nil)then
	do return end
	end
		entity:Reload();
	end,

	--------------------------------------------------	
	WPN_SHOOT= function(self, entity, sender)
	end,

	--------------------------------------------------
	THROW_GRENADE_DONE= function(self, entity, sender)
	end,

	--------------------------------------------------
	SMART_THROW_GRENADE = function( self, entity, sender )
		if (AI_Utils:CanThrowGrenade(entity) == 1) then
			AI.PlayCommunication(entity.id, "comm_throwing_grenade", "ImmeadiateThreat", 1.5);		
			entity:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE,"throw_grenade_execute");
		end
	end,
		
	---------------------------------------------		
	OnDamage = function(self,entity,sender,data)
	end,

	---------------------------------------------
	exited_vehicle = function( self,entity, sender )
--		AI.Signal(SIGNALID_READIBILITY, 2, "AI_AGGRESSIVE",entity.id);	
--		entity.EventToCall = "OnSpawn";	
		AI.Signal(0,1,"OnSpawn",entity.id);
	end,

	---------------------------------------------
	JOIN_TEAM = function ( self, entity, sender)
		AI.LogEvent(entity:GetName().." JOINING TEAM");
		entity.AI.InSquad = 1;
	end,

	---------------------------------------------
	BREAK_TEAM = function ( self, entity, sender)
		entity.AI.InSquad = 0;
	end,

	--------------------------------------------------------------
	FOLLOW_LEADER = function(self,entity,sender,data)
		entity.AI.InSquad= 1;
	--	g_SignalData.ObjectName = "line_follow2";--formation to be used if not in follow mode
		AI.Signal(SIGNALFILTER_LEADER,10,"OnJoinTeam",entity.id);--,g_SignalData);
	end,
	
	---------------------------------------------
	HOLSTERITEM_TRUE = function( self, entity, sender )
		entity.actor:HolsterItem( true );
	end,
	
	---------------------------------------------
	HOLSTERITEM_FALSE = function( self, entity, sender )
		entity:DrawWeaponNow();
	end,
	
	---------------------------------------------
	ORDER_TIMEOUT = function( self, entity, sender,data )
		if(data.fValue and data.fValue>0) then 
			g_StringTemp1 = "order_timeout"..math.floor(data.fValue*10)/10;
			AI.CreateGoalPipe(g_StringTemp1);
			AI.PushGoal(g_StringTemp1, "timeout",1,data.fValue);
			AI.PushGoal(g_StringTemp1, "signal", 0, 10, "ORD_DONE", SIGNALFILTER_LEADER);
			entity:InsertSubpipe(0,g_StringTemp1);
		else
			entity:InsertSubpipe(0, "order_timeout");
		end
	end,
	
	---------------------------------------------
	ORDER_ACQUIRE_TARGET = function(self , entity, sender, data)
		if(data.id ~= NULL_ENTITY) then
			entity:InsertSubpipe(0,"acquire_target",data.id);
		else
			entity:InsertSubpipe(0,"acquire_target",data.ObjectName);
		end
		AI.Signal(SIGNALFILTER_LEADER,10,"ORD_DONE",entity.id);
	end,

	---------------------------------------------
	ORDER_COVER_SEARCH = function(self,entity,sender)
		-- ignore this order by default
		AI.Signal(SIGNALFILTER_LEADER,10,"ORD_DONE",entity.id);
	end,	
	
	---------------------------------------------
	CORNER = function( self, entity, sender )
		AI.SmartObjectEvent( "CORNER", entity.id, sender.id );
	end,	

	---------------------------------------------
	OnUpdateItems = function( self, entity, sender )
		-- check does he have RPG and update his combat class
		if ( entity.inventory:GetItemByClass("LAW") ) then
			AI.ChangeParameter( entity.id, AIPARAM_COMBATCLASS, AICombatClasses.InfantryRPG );
			AI.LogEvent( entity:GetName().." changes his combat class to InfantryRPG on signal OnUpdateItems" );
		end
	end,	

	---------------------------------------------------------------------------------------------------------------------------------------
	--
	--	FlowGraph	actions 
	--
	---------------------------------------------------------------------------------------------------------------------------------------

	---------------------------------------------
	ACT_DUMMY = function( self, entity, sender, data )
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
	end,
	
	---------------------------------------------
	ACT_EXECUTE = function( self, entity, sender, data )
	--	entity:InsertSubpipe( 0, "action_execute", nil, data.iValue );
		if(data) then
			AI.ExecuteAction( data.ObjectName, entity.id, sender.id, data.fValue, data.iValue );
		else
			AI.ExecuteAction( data.ObjectName, entity.id, entity.id, sender.fValue, sender.iValue );
		end	
		--AI.ChangeParameter(entity.id,AIPARAM_AWARENESS_PLAYER,0);
	end,
	
	---------------------------------------------
	ACT_FOLLOWPATH = function( self, entity, sender, data )
		local pathfind = data.point.x;
		local reverse = data.point.y;
		local startNearest = data.point.z;
		local loops = data.point2.x;
		local speed = data.fValue;

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
    AI.PushGoal(g_StringTemp1, "followpath", 1, pathfind, reverse, startNearest, loops, speed);
		AI.PushGoal(g_StringTemp1, "signal", 1, 1, "END_ACT_FOLLOWPATH",0);
    
    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, g_StringTemp1, nil, data.iValue );
	end,

	---------------------------------------------
	ACT_GOTO = function( self, entity, sender, data )
		if ( data and data.point ) then
			if (entity.class == "HumanInfected") then
				AI.GoTo(entity.id, data.point);
			else
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
				AI.PushGoal(g_StringTemp1, "signal", 1, 1, "END_ACT_GOTO", 0);

				entity.gotoSubpipeID = data.iValue; -- Store the goal pipe ID here so we can cancel it correctly later /Jonas
				entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, g_StringTemp1, nil, data.iValue );
			end
		end
	end,
	
	---------------------------------------------
	CANCEL_CURRENT = function( self, entity )
		if (entity.gotoSubpipeID) then
			entity:CancelSubpipe(entity.gotoSubpipeID);
		end
	end,

	---------------------------------------------
	ACT_LOOKATPOINT = function( self, entity, sender, data )
		if ( data and data.point and (data.point.x~=0 or data.point.y~=0 or data.point.z~=0) ) then
			AI.SetRefPointPosition( entity.id, data.point );
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_lookatpoint", nil, data.iValue );
		else
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_resetlookat", nil, data.iValue );
		end
	end,

	-- make the guy shoot at the provided point	for fValue seconds
	---------------------------------------------
	ACT_SHOOTAT = function( self, entity, sender, data )

		-- Select correct weapon
		local bUseSecondary = data.iValue2 ~= 0;
		if (bUseSecondary and entity.SelectSecondaryWeapon) then
			AI.SetUseSecondaryVehicleWeapon(entity.id, true);
			entity:SelectSecondaryWeapon();
		elseif (not bUseSecondary and entity.SelectPrimaryWeapon) then
			AI.SetUseSecondaryVehicleWeapon(entity.id, false);
			entity:SelectPrimaryWeapon();
		end

		-- use dynamically created goal pipe to set shooting time
		AI.CreateGoalPipe("action_shoot_at");
		AI.PushGoal("action_shoot_at", "locate", 0, "refpoint");
		AI.PushGoal("action_shoot_at", "+firecmd",0,FIREMODE_FORCED,AILASTOPRES_USE);
		AI.PushGoal("action_shoot_at", "+timeout",1,data.fValue);
		AI.PushGoal("action_shoot_at", "script", 0, "AI.SetUseSecondaryVehicleWeapon(entity.id, false); return true;");
		AI.PushGoal("action_shoot_at", "firecmd",0,0);
		
		AI.SetRefPointPosition( entity.id, data.point );
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_shoot_at", nil, data.iValue );

		-- draw weapon
		-- vehicles have no holster
		if (entity.DrawWeaponNow) then
			entity:DrawWeaponNow();
		end

	end,

	-- make the guy aim at the provided point	for fValue seconds
	---------------------------------------------
	ACT_AIMAT = function( self, entity, sender, data )

		-- Select correct weapon
		local bUseSecondary = data.iValue2 ~= 0;
		if (bUseSecondary and entity.SelectSecondaryWeapon) then
			AI.SetUseSecondaryVehicleWeapon(entity.id, true);
			entity:SelectSecondaryWeapon();
		elseif (not bUseSecondary and entity.SelectPrimaryWeapon) then
			AI.SetUseSecondaryVehicleWeapon(entity.id, false);
			entity:SelectPrimaryWeapon();
		end

		-- use dynamically created goal pipe to set shooting time
		AI.CreateGoalPipe("action_aim_at");
		AI.PushGoal("action_aim_at", "locate", 0, "refpoint");
		AI.PushGoal("action_aim_at", "+firecmd",0,FIREMODE_AIM,AILASTOPRES_USE);
		AI.PushGoal("action_aim_at", "+timeout",1,data.fValue);
		AI.PushGoal("action_aim_at", "script", 0, "AI.SetUseSecondaryVehicleWeapon(entity.id, false); return true;");
		AI.PushGoal("action_aim_at", "firecmd",0,0);
		
		AI.SetRefPointPosition( entity.id, data.point );
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_aim_at", nil, data.iValue );

		-- draw weapon
		-- vehicles have no holster
		if(entity.DrawWeaponNow) then
			entity:DrawWeaponNow();	
		end	

	end,

	---------------------------------------------
	ACT_ANIM = function( self, entity, sender, data )
		AI.CreateGoalPipe( "act_animation" );
		AI.PushGoal( "act_animation", "timeout", 1, 0.1 );
		AI.PushGoal( "act_animation", "branch", 1, -1, BRANCH_ALWAYS );
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "act_animation", nil, data.iValue );
		
	end,
	
	---------------------------------------------
	ACT_DIALOG = function( self, entity, sender, data )
		AI.CreateGoalPipe( "act_animation" );
		AI.PushGoal( "act_animation", "timeout", 1, 0.1 );
		AI.PushGoal( "act_animation", "branch", 1, -1, BRANCH_ALWAYS );
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "act_animation", nil, data.iValue );
	end,	
	
	---------------------------------------------
	ACT_FOLLOW = function( self, entity,sender,data )
		if ( data == nil ) then
			-- this should never happen - lets warn
			AI.Warning("ACT_FOLLOW "..entity:GetName()..": nil data!");
	    -- insert and cancel the goal pipe to notify the node
	    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, sender.iValue );
	    entity:CancelSubpipe( sender.iValue );
	    return;
		end
		self:ACT_JOINFORMATION(entity,sender,data);
	end,
	
	---------------------------------------------
	END_ACT_FORM = function(self,entity,sender)
	end,

	---------------------------------------------
	ACT_JOINFORMATION = function( self, entity, sender, data )
	
		if ( data == nil ) then
			-- this should never happen - lets warn
			AI.Warning("ACT_JOINFORMATION "..entity:GetName()..": nil data!");
	    -- insert and cancel the goal pipe to notify the node
	    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, sender.iValue );
	    entity:CancelSubpipe( sender.iValue );
	    return;
		end

		if ( sender==nil) then
			-- insert and cancel the goal pipe to notify the node
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
			return;
		elseif(sender==entity) then 
			if ( entity.AI.followGoalPipeId and entity.AI.followGoalPipeId ~=0 ) then
				entity:CancelSubpipe( entity.AI.followGoalPipeId );
			end
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity.AI.followGoalPipeId = data.iValue;
			return;
		end

		entity.AI.followGoalPipeId = data.iValue;
		
		local stance = AI.GetStance(sender.id);
		
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "stay_in_formation_moving", nil, data.iValue );

		if(stance==BODYPOS_CROUCH or  stance==BODYPOS_PRONE) then
			AI.SetStance(entity.id,stance);
		end

--		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, pipeName, nil, data.iValue );
		if(stance==BODYPOS_RELAX) then
			AI.SetStance(entity.id,stance);
		else -- join the formation in combat stance if it's to be crouch or prone after
			AI.SetStance(entity.id,BODYPOS_STAND);
		end
	end,
	
	ACT_JOIN_FORMATION = function( self, entity, sender, data )
	
		if ( data == nil ) then
			-- this should never happen - lets warn
			AI.Warning("ACT_JOINFORMATION "..entity:GetName()..": nil data!");
	    -- insert and cancel the goal pipe to notify the node
	    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, sender.iValue );
	    entity:CancelSubpipe( sender.iValue );
	    return;
		end

		if ( sender==nil) then
			-- insert and cancel the goal pipe to notify the node
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
			return;
		elseif(sender==entity) then 
			if ( entity.AI.followGoalPipeId and entity.AI.followGoalPipeId ~=0 ) then
				entity:CancelSubpipe( entity.AI.followGoalPipeId );
			end
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity.AI.followGoalPipeId = data.iValue;
			return;
		end

		entity.AI.followGoalPipeId = data.iValue;
		
		local stance = AI.GetStance(sender.id);
		
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "MoveWhileInFormation", nil, data.iValue );

		if(stance==BODYPOS_CROUCH or  stance==BODYPOS_PRONE) then
			AI.SetStance(entity.id,stance);
		end

--		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, pipeName, nil, data.iValue );
		if(stance==BODYPOS_RELAX) then
			AI.SetStance(entity.id,stance);
		else -- join the formation in combat stance if it's to be crouch or prone after
			AI.SetStance(entity.id,BODYPOS_STAND);
		end
	end,

	---------------------------------------------
	ACT_GRAB_OBJECT = function( self, entity, sender, data )
	
		if ( data == nil ) then
			-- this should never happen - lets warn
			AI.Warning("ACT_GRAB_OBJECT "..entity:GetName()..": nil data!");
	    -- insert and cancel the goal pipe to notify the node
	    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, sender.iValue );
	    entity:CancelSubpipe( sender.iValue );
	    return;
		end
	
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			AI.Signal(SIGNALFILTER_SENDER,0,"GO_TO_GRABBED",sender.id);
			if ( entity:GrabObject( sender ) ~= 1 ) then
				entity:CancelSubpipe( data.iValue );
			end;
	end,
	
	---------------------------------------------
	ACT_DROP_OBJECT = function( self, entity, sender, data )
		if ( entity.grabParams and entity.grabParams.entityId ) then
			local grab = System.GetEntity( entity.grabParams.entityId );
			AI.Signal(SIGNALFILTER_SENDER,0,"OnDropped",entity.grabParams.entityId);
 			entity:DropObject( true, data.point, 0 );
 			
-- 			if ( grab ) then
--				--System.Log( "ACT_DROP_OBJECT received!!! Impulse is x:"..data.point.x.." y:"..data.point.y.." z:"..data.point.z );
-- 				grab:AddImpulse( -1, nil, data.point, LengthVector( data.point ), 1 );
-- 			end
 		end
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
		--AI.Signal( SIGNALFILTER_SENDER, 10, "ACTION_DONE", entity.id );
	end,
	
	---------------------------------------------
	ACT_WEAPONDRAW = function( self, entity, sender, data )
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_weapondraw", nil, data.iValue );
	end,
	
	---------------------------------------------
	ACT_WEAPONHOLSTER = function( self, entity, sender, data )
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_weaponholster", nil, data.iValue );
	end,

	---------------------------------------------
	ACT_USEOBJECT = function( self, entity, sender, data )
	
		if ( data == nil ) then
			-- this should never happen - lets warn
			AI.Warning("ACT_USEOBJECT "..entity:GetName()..": nil data!");
	    -- insert and cancel the goal pipe to notify the node
	    entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, sender.iValue );
	    entity:CancelSubpipe( sender.iValue );
	    return;
		end
	
		if ( sender ) then
			sender:OnUsed( entity, 2 );
			AI.SmartObjectEvent( "OnUsed", sender.id, entity.id );
		end

		if (not sender.OnAIUsed) then
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
		else
			sender:OnAIUsed(entity, data.iValue);			
		end
	end,

	---------------------------------------------
	ACT_WEAPONSELECT = function( self, entity, sender, data )
		ItemSystem.SetActorItemByName( entity.id, data.ObjectName,false );
		entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
	end,
	
	---------------------------------------------
	ACT_ALERTED = function( self, entity, sender, data )
		entity:MakeAlerted();
	end,
	
	---------------------------------------------
	SETUP_ENTERING = function( self, entity )
		-- signal sent from action_enter goal pipe
		if ( entity.AI.theVehicle ) then
			if ( entity.AI.theVehicle:EnterVehicle( entity.id, entity.AI.mySeat, true ) ~= true ) then
				entity:CancelSubpipe();
			end
		else
			entity:CancelSubpipe();
		end
	end,

	---------------------------------------------
	SETUP_ENTERING_FAST = function( self, entity )
		-- signal sent from action_enter goal pipe
		if ( entity.AI.theVehicle ) then
			if ( entity.AI.theVehicle:EnterVehicle( entity.id, entity.AI.mySeat, false ) ~= true ) then
				entity:CancelSubpipe();
			end
		else
			entity:CancelSubpipe();
		end
	end,

	---------------------------------------------
	ACT_ENTERVEHICLE = function( self, entity, sender, data )
		if ( entity.AI.theVehicle) then
			-- fail if already inside a vehicle
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
			return;
		end
		
		-- get the vehicle
		local vehicle = System.GetEntity( data.id );
	 	if ( vehicle == nil) then
	 		-- no vehicle found 
			AI.LogEvent( entity:GetName().." couldn't find a vehicle to enter" );
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
	 		return;
	 	end
	 	
	 	if (not vehicle.AIDriver) then
	 		-- vehicle not suitable for AI 
			AI.LogEvent( vehicle:GetName().." has no AI and can't be entered by "..entity:GetName() );
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
	 		return;
	 	end


		local numSeats = count( vehicle.Seats );
		--local numMembers = AI.GetGroupCount( entity.id, GROUP_ENABLED, AIOBJECT_ACTOR );

		--local seatIndex = data.fValue;
		if ( data.fValue<1 or data.fValue>numSeats ) then
			entity.AI.mySeat = vehicle:RequestClosestSeat( entity.id );
		else
			entity.AI.mySeat = data.fValue;
		end
		
		if ( entity.AI.mySeat==nil ) then
			AI.LogEvent(entity:GetName().." aborting enter vehicle "..vehicle:GetName());
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
			return
		end
		
		entity.AI.theVehicle = vehicle;
		
		vehicle:ReserveSeat( entity.id, entity.AI.mySeat );
		if ( vehicle:IsDriver(entity.id) ) then
			-- I'm the driver
--			entity.AI.theVehicle.AI.driver = entity;
--			entity.AI.theVehicle.AI.countVehicleCrew = 0;
			
			--if ( numSeats<numMembers ) then
			--	entity.AI.theVehicle.AI.vehicleCrewNumber = numSeats;
			--else
			--	entity.AI.theVehicle.AI.vehicleCrewNumber = numMembers;
			--end			
		end

		--	After getting out of the vehicle the agent will be in the alert stance, and since
		--	the animation graph will not be updated until the agent is unlinked with the vehicle,
		--	the stance needs to be set here, before the link is done, for a correct blend between 
		--	exit and alerted animations /Mario
		AI.SetStance(entity.id, STANCE_ALERTED)

		-- check is fast entering needed
		if ( data.iValue2 == 1 ) then
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_enter_fast", nil, data.iValue );
		else
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_enter", nil, data.iValue );
		end

		--entity.AI.theVehicle.AI.goalType	= data.iValue;
--		AI.LogEvent(entity:GetName().." is going to enter vehicle "..entity.AI.theVehicle:GetName().." with goal type = "..(entity.AI.theVehicle.AI.goalType or "nil"));
		AI.Signal( SIGNALFILTER_SENDER, 0, "ENTERING_VEHICLE", entity.id );
	end,	

	---------------------------------------------
	ACT_EXITVEHICLE = function( self, entity, sender, data )
		if ( entity.AI.theVehicle == nil ) then
			-- fail if not inside a vehicle
			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_dummy", nil, data.iValue );
			entity:CancelSubpipe( data.iValue );
			return;
		end

		if ( entity.AI.theVehicle:IsDriver(entity.id) ) then
			-- I'm the driver
--			entity.AI.theVehicle.AI.driver = entity;
--			entity.AI.theVehicle.AI.countVehicleCrew = 0;
			
			--if ( numSeats<numMembers ) then
			--	entity.AI.theVehicle.AI.vehicleCrewNumber = numSeats;
			--else
			--	entity.AI.theVehicle.AI.vehicleCrewNumber = numMembers;
			--end			
		end
		
		entity.AI.theVehicle:LeaveVehicle( entity.id );
		entity.AI.theVehicle = nil;

		entity:InsertSubpipe( AIGOALPIPE_HIGHPRIORITY, "action_exit", nil, data.iValue );
	end,

	---------------------------------------------
	ACT_ANIMEX = function( self, entity, sender, data )
		if ( data ) then
			-- use dynamically created goal pipe to set parameters
			AI.CreateGoalPipe( "action_animEx" );
			AI.PushGoal( "action_animEx", "locate", 0, "refpoint" );
--			AI.PushGoal( "action_animEx", "+animtarget", 0, data.iValue2, data.ObjectName, data.point.x, data.fValue, data.point2.x );
			AI.PushGoal( "action_animEx", "+animtarget", 0, data.iValue2, data.ObjectName, data.point.x, data.fValue, 0);
			AI.PushGoal( "action_animEx", "+locate", 0, "animtarget" );
			AI.PushGoal( "action_animEx", "+approach", 1, 0.0, AILASTOPRES_USE );
			AI.PushGoal( "action_animEx", "+branch", 0, "FAILED", IF_LASTOP_FAILED );
			AI.PushGoal( "action_animEx", "branch", 0, "END", BRANCH_ALWAYS );
			AI.PushLabel( "action_animEx", "FAILED" );
			AI.PushGoal( "action_animEx", "script", 1,"System.Log(\"ACT_ANIMEX: Failed on the approach for entity " .. entity:GetName() .. "\");entity:CancelSubpipe(" .. data.iValue .. ");");
			AI.PushLabel( "action_animEx", "END" );

			entity:InsertSubpipe( AIGOALPIPE_SAMEPRIORITY, "action_animEx", nil, data.iValue );
		end
	end,

	---------------------------------------------
	SPOTTED_BY_SHARK = function(self,entity,sender,data)
		-- sent by SO rule, data.id = shark entity id
		local shark = System.GetEntity(data.id);
		if(shark) then 
			shark:Chase(entity);
		end
	end,

	---------------------------------------------
	DO_NOTHING = function(self,entity,sender)
			entity:SelectPipe(0,"standingthere");
			entity:SelectPipe(0,"do_nothing");			
	end,

	---------------------------------------------
	TARGETLASER_ON = function (self, entity, sender)
		AI.EnableWeaponAccessory(entity.id, AIWEPA_LASER, true);
	end,	
	
	---------------------------------------------
	TARGETLASER_OFF = function (self, entity, sender)
		AI.EnableWeaponAccessory(entity.id, AIWEPA_LASER, false);
	end,	

	---------------------------------------------
	SYS_FLASHLIGHT_ON = function (self, entity, sender)
		AI.EnableWeaponAccessory(entity.id, AIWEPA_COMBAT_LIGHT, true);
	end,	
	
	---------------------------------------------
	SYS_FLASHLIGHT_OFF = function (self, entity, sender)
		AI.EnableWeaponAccessory(entity.id, AIWEPA_COMBAT_LIGHT, false);
	end,	

	---------------------------------------------
	PATROL_FLASHLIGHT_ON = function (self, entity, sender)
		AI.EnableWeaponAccessory(entity.id, AIWEPA_PATROL_LIGHT, true);
	end,	
	
	---------------------------------------------
	PATROL_FLASHLIGHT_OFF = function (self, entity, sender)
		AI.EnableWeaponAccessory(entity.id, AIWEPA_PATROL_LIGHT, false);
	end,	

	---------------------------------------------
	NEW_SPAWN = function(self,entity,sender)
	
			if(entity.AI.reinfPoint) then
				entity:SelectPipe(0,"goto_point",entity.AI.reinfPoint);
			else
				entity:SelectPipe(0,"goto_point",g_SignalData.ObjectName);
			end
	end,

	---------------------------------------------
	OnBeginStumbling = function(self, entity, sender, data, direction, offset)
	
		local blackboard = AI.GetBehaviorBlackBoard(entity.id);
		if (blackboard) then
			blackboard.stumble_lastRefPoint = AI.GetRefPointPosition(entity.id);
			blackboard.stumbleGoalPipeId = data.iValue;
		end
		
		-- Project point ahead based on forward direction
		local vRecoverPos = {x=0,y=0,z=0};
		FastScaleVector(vRecoverPos, direction, offset);
		FastSumVectors(vRecoverPos, vRecoverPos, entity:GetPos());
		
		-- Goal pipe is used by hit reaction system - forces AI to move in their forward direction for a brief period
		-- Note: Should be created in Code by System?
		AI.CreateGoalPipe("STUMBLE_RECOVER");
		AI.PushGoal("STUMBLE_RECOVER","strafe",0,99,99,99);
		AI.PushGoal("STUMBLE_RECOVER","locate",0,"refpoint");
		AI.PushGoal("STUMBLE_RECOVER","backoff",1,offset,0,AILASTOPRES_USE + AI_MOVE_FORWARD);
		AI.PushGoal("STUMBLE_RECOVER","signal",0,1,"OnFinishedStumbling",SIGNALFILTER_SENDER);
		
		entity:InsertSubpipe(AIGOALPIPE_HIGHPRIORITY + AIGOALPIPE_KEEP_ON_TOP, "STUMBLE_RECOVER", nil, data.iValue);
		AI.SetRefPointPosition(entity.id, vRecoverPos);
	
	end,
	
	---------------------------------------------
	OnBeginStumblingForward = function(self, entity, sender, data)
	
		local direction = entity:GetDirectionVector(1);
		self:OnBeginStumbling(entity, sender, data, direction, 6.0);
	
	end,
	
	---------------------------------------------
	OnBeginStumblingBack = function(self, entity, sender, data)
	
		local direction = {x=0,y=0,z=0};
		RotateVectorAroundR(direction, entity:GetDirectionVector(1), g_Vectors.v001, g_Pi);
		self:OnBeginStumbling(entity, sender, data, direction, 6.0);
	
	end,
	
	---------------------------------------------
	OnBeginStumblingLeft = function(self, entity, sender, data)
	
		local direction = {x=0,y=0,z=0};
		RotateVectorAroundR(direction, entity:GetDirectionVector(0), g_Vectors.v001, g_Pi);
		self:OnBeginStumbling(entity, sender, data, direction, 4.0);
	
	end,
	
	---------------------------------------------
	OnBeginStumblingRight = function(self, entity, sender, data)
	
		local direction = entity:GetDirectionVector(0);
		self:OnBeginStumbling(entity, sender, data, direction, 4.0);
	
	end,
	
	---------------------------------------------
	OnFinishedStumbling = function(self, entity, sender, data)
	
		-- Recover last ref point before this happened
		local blackboard = AI.GetBehaviorBlackBoard(entity.id);
		if (blackboard) then
			
			if (blackboard.stumble_lastRefPoint) then
				AI.SetRefPointPosition(entity.id, blackboard.stumble_lastRefPoint);
			end
			
			if (blackboard.stumbleGoalPipeId) then
				entity:CancelSubpipe(blackboard.stumbleGoalPipeId);
			end
			
			blackboard.stumble_lastRefPoint = nil;
			blackboard.stumbleGoalPipeId = nil;
		end
	
	end,
	
	---------------------------------------------
	OnHitDeathReactionInterrupted = function(self, entity, sender, data)
	
		self:OnFinishedStumbling(entity, sender, data);
	
	end,
})
