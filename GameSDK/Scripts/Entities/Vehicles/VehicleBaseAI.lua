--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2004.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Common code for AI functionality of all of the vehicle implementations
--	all the AI related stuff should be moved here
--  
--------------------------------------------------------------------------
--  History:
--  - 08/06/2005   15:36 : Created by Kirill Bulatsev
--
--------------------------------------------------------------------------

--Script.ReloadScript("Scripts/Utils/Math.lua");



VehicleBaseAI =
{

	--AI related	
	PropertiesInstance = 
	{
		groupid = 173,
		aibehavior_behaviour = "TankIdle",
		bCircularPath = 0,
		PathName = "",
		FormationType = "",
		bAutoDisable = 1,
	},
	--AI related	properties
	Properties = 
	{	
		soclasses_SmartObjectClass = "",
		bAutoGenAIHidePts = 0,

		esFaction = "",
		bFactionHostility = 1,

		aicharacter_character = "Tank",
		leaderName = "",
		followDistance = 5.0,
		attackrange = 100,
		commrange = 100,
		accuracy = 1,
		bCanUsePrimaryWeapon = 1,
		bCanUseSecondaryWeapon = 0,

		Perception =
		{
			--fov/angle related
			FOVPrimary = -1,			-- normal fov
			FOVSecondary = -1,		-- periferial vision fov
			--ranges			
			sightrange = 100,
			sightrangeVehicle = -1,	-- how far do i see vehicles
			--how heights of the target affects visibility
			stanceScale = 1,
		},
		
		AIPressureAOE =
		{
			bHostileEnable = 0,
			hostileRadius = 0.0,
			hostilePressureInc = 0.0,
			
			bFriendlyEnable = 0,
			friendlyRadius = 0.0,
			friendlyPressureDec = 0.0,
		},
	},

	AI = {
	},
	-- the AI movement ability of the vehicle.
	AIMovementAbility =
	{
		walkSpeed = 5.0,
		runSpeed = 8.0,
		sprintSpeed = 10.0,
		maneuverSpeed = 4.0,
		b3DMove = 0,
		minTurnRadius = 1,	-- meters
		maxTurnRadius = 30,	-- meters
		maxAccel = 1000000.0,
		maxDecel = 1000000.0,
		usePathfinder = 1,	-- for prototyping
		pathType = AIPATH_DEFAULT,	-- pathfinding properties of this agent
		passRadius = 3.0,		-- the radius used in path finder. 		
		pathLookAhead = 5,
		pathRadius = 2,
		maneuverTrh = 2.0,	--cosine of forward^desired angle, after which to start maneuvering
		velDecay	= 30,			-- how fast velocity decays with cos(fwdDir^pathDir) 
	},
	
	-- information about fire type that are used by AI
	AIFireProperties = {
	},
	AISoundRadius = 120,
	hidesUser=1,	

	-- now fast I forget the target (S-O-M speed)
	forgetTimeTarget = 16.0,
	forgetTimeSeek = 20.0,
	forgetTimeMemory = 20.0,
}


--------------------------------------------------------------------------
function VehicleBaseAI:AIDriver( enable )
	
	if (not self.State)	then
		return
	end
	
	if (not AI) then return end

	if( enable == 0 ) then
	AI.LogEvent(" >>>> VehicleBaseAI:AIDriver disabling "..self:GetName());	
		self:ChangeFaction();
		self:TriggerEvent(AIEVENT_DRIVER_OUT);
		self.State.aiDriver = nil;
		self:EnableMountedWeapons(true);
	else
		-- no triggering on if dead
		if (self.health and self.health <= 0) then
			return
		end;
	AI.LogEvent(" >>>> VehicleBaseAI:AIDriver enabling "..self:GetName());			
		self:TriggerEvent(AIEVENT_DRIVER_IN);
		if(self.Behavior and self.Behavior.Constructor) then 
			self.Behavior:Constructor(self);
		end
		self:EnableMountedWeapons(false);
		
		return 1
	end
end


--------------------------------------------------------------------------
function VehicleBaseAI:InitAI()
	if (not AI) then return end

	self.AI = {
	}
	--create AI object of the vehicle
	CryAction.RegisterWithAI(self.id, self.AIType, self.Properties, self.PropertiesInstance, self.AIMovementAbility);

	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_TARGET,self.forgetTimeTarget);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_SEEK,self.forgetTimeSeek);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_MEMORY,self.forgetTimeMemory);

	if(self.AICombatClass) then
		AI.ChangeParameter(self.id,AIPARAM_COMBATCLASS,self.AICombatClass);
	end

	local Perception = self.Properties.Perception;
	if (Perception.config and Perception.config ~= "") then
		local TargetTracks = Perception.TargetTracks;
		AI.RegisterTargetTrack(self.id, Perception.config, TargetTracks.targetLimit, TargetTracks.classThreat);
	end


	-- Mark AI hideable flag.
	if (self.Properties.bAutoGenAIHidePts and self.Properties.bAutoGenAIHidePts == 1) then
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
	else
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
	end
end


--------------------------------------------------------------------------
function VehicleBaseAI:ResetAI()

	if (not AI) then return end

	self.AI = {
	}
	self:AIDriver( 0 );

	AI.ResetParameters(self.id, false, self.Properties, self.PropertiesInstance, nil);
	if(self.AICombatClass) then
		AI.ChangeParameter(self.id,AIPARAM_COMBATCLASS,self.AICombatClass);
	end
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_TARGET,self.forgetTimeTarget);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_SEEK,self.forgetTimeSeek);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_MEMORY,self.forgetTimeMemory);

	-- Mark AI hideable flag.
	if (self.Properties.bAutoGenAIHidePts and self.Properties.bAutoGenAIHidePts == 1) then
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
	else
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
	end

	local Perception = self.Properties.Perception;
	if (Perception.config and Perception.config ~= "") then
		local TargetTracks = Perception.TargetTracks;
		AI.RegisterTargetTrack(self.id, Perception.config, TargetTracks.targetLimit, TargetTracks.classThreat);
	end

end

----------------------------------------------------------------------------------------------------
function VehicleBaseAI:OnLoadAI(saved)

	self.AI = {};
	if(saved.AI) then 
		self.AI = saved.AI;
	end
end

----------------------------------------------------------------------------------------------------
function VehicleBaseAI:OnSaveAI(save)

	if(self.AI) then 
		save.AI = self.AI;
	end
end


--------------------------------------------------------------------------
function VehicleBaseAI:DestroyAI()
end

--------------------------------------------------------------------------
function VehicleBaseAI:SignalCrew(signalText,data)
	if (not AI) then return end
	if (self.Seats) then
		for i,seat in pairs(self.Seats) do
			if (seat.passengerId) then
				AI.Signal(SIGNALFILTER_SENDER,0,signalText,seat.passengerId,data);
			end
		end
	end	
end


----------------------------------------------------------------------------------
function CreateVehicleAI(child)

	-- [michaelr]: merge of AI properties moved here during xmlisation
	if (child.AIProperties) then
	  mergef(child, child.AIProperties, 1);	
	end
	child.AIProperties = nil;
	mergef(child, VehicleBaseAI, 1);
	
	end

--------------------------------------------------------------------------
function VehicleBaseAI:UserEntered(user)

	if not AI then return end

	if(self:IsDriver(user.id)) then
		self:ChangeFaction(user, 1);
	end
	
	self:PrepareSeatMountedWeapon(user);

end


----------------------------------------------------------------------------------------------------------------------------------------------------
--
--			FlowGraph action stuff
--
----------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------
function VehicleBaseAI:Act_Unload( params )
	if not AI then return end
	AI.Signal(0, 1, "STOP_VEHICLE", self.id);
end

----------------------------------------------------------------------------------------------------------------------------
function VehicleBaseAI:OnVehicleDestroyedAI()
	if not AI then return end
	self:DisableAI();
	
	-- notify spawner - so it counts down and updates
	if(self.AI.spawnerListenerId) then
		local spawnerEnt = System.GetEntity(self.AI.spawnerListenerId);
		if(spawnerEnt) then
			spawnerEnt:UnitDown();
		end
	end

	self:TriggerEvent(AIEVENT_AGENTDIED);
	
	if (self.AI.MaybeNextLevel) then
		--Log(" - VehicleDamages:OnVehicleDestroyed() will call MaybeNextLevel");
		self:MaybeNextLevel();
	end
end

--------------------------------------------------------------------------
function VehicleBaseAI:OnVehicleImmobilized()
end

--------------------------------------------------------------------------
function VehicleBaseAI:DisableAI()

	-- make every AI get out of the vehicle
	self:SignalCrew("SHARED_LEAVE_ME_VEHICLE");
	
	-- disable AI object of the vehicle
	if (self.AIDriver) then
	  self:AIDriver( 0 );
	end

end

--------------------------------------------------------------------------
function VehicleBaseAI:GetSeatWithWeapon(weapon)
	for i,seat in pairs(self.Seats) do
		--if (seat.Weapons) then
			local wc = seat.seat:GetWeaponCount()
			for j=1, wc do			
				if (seat.seat:GetWeaponId(j) == weapon.id) then 
					return i;
				end
			end
			
		--end
	end
	return nil;
end

--------------------------------------------------------------------------
function VehicleBaseAI:ChangeFaction(driver, isEntered)
	if not AI then return end

	-- driver out - reset species
	if(isEntered == 0 or isEntered == nil) then
		AI.ChangeParameter(self.id, AIPARAM_FACTION, "");	
		self.AI.hostileSet=nil;
		return
	end
	
	local faction = AI.GetFactionOf(driver.id)

	-- driver enters enemy vehicle -- enemies don't recognize the vehicle as hostile
	if (self.Properties.bHidesPlayer==1 and isEntered==1 and driver.ai==nil and faction ~= self.defaultFaction) then
		return 
	end
	
	-- vehicle does not hide driver OR
	-- driver exposed himself by shooting/damaging enemy
	AI.ChangeParameter(self.id, AIPARAM_FACTION, faction);
	self.AI.hostileSet=1;
end


--------------------------------------------------------------------------
function VehicleBaseAI:SpawnCopyAndLoad()

		local params = {
			class = self.class;
			position = self:GetPos(),
			orientation = self:GetDirectionVector(1),
			scale = self:GetScale(),
			properties = self.Properties,
			propertiesInstance = self.PropertiesInstance,
		}
		local rndOffset = 1;
		params.position.x = params.position.x + random(0,rndOffset*2)-rndOffset;
		params.position.y = params.position.y + random(0,rndOffset*2)-rndOffset;
		params.name = self:GetName()
		local ent = System.SpawnEntity(params)
		if ent then
			self.spawnedEntity = ent.id
			if not ent.Events then ent.Events = {} end
			local evts = ent.Events
			for name, data in pairs(self.FlowEvents.Outputs) do
				if not evts[name] then evts[name] = {} end
				table.insert(evts[name], {self.id, name})
			end
			self:LoadLinked( ent );
		end
end


--------------------------------------------------------------------------
function VehicleBaseAI:LoadLinked(newVehicle)
	if not AI then return end

	local i=0;
	local link=self:GetLink(i);
	while (link) do
--AI.LogEvent("AISpawner >>> hiding linked "..i);	
		if (link and link.Event_SpawnKeep) then
			-- spawn the guy
			link:Event_SpawnKeep();	
			local newEntity = System.GetEntity(link.spawnedEntity);
			
			if(link.PropertiesInstance.bAutoDisable ~= 1) then
				AI.AutoDisable( newEntity.id, 0 );
			end
			newEntity:SetName(newEntity:GetName().."_vspawned");
			
			-- make the guy enter the vehicle
			if(newEntity) then
				g_SignalData.fValue = i+1					--seatId, indexing starts from 1
				g_SignalData.id = newVehicle.id 	--vehicleId
				g_SignalData.iValue2 = 1 					--fast flag
				AI.Signal(SIGNALFILTER_SENDER,0,"ACT_ENTERVEHICLE",newEntity.id,g_SignalData);	
			end	
		end
		i=i+1;
		link=self:GetLink(i);
	end
end

--------------------------------------------------------------------------
function VehicleBaseAI:AutoDisablePassangers( status )
	if not AI then return end

	for i,seat in pairs(self.Seats) do	  
		local passengerId = seat.GetPassengerId();	  
		if (passengerId) then		  
			AI.AutoDisable( passengerId, status );
		end
	end
end

--------------------------------------------------------------------------
function VehicleBaseAI:OnPassengerDead( passenger )

	if ( self.class == "Asian_helicopter" ) then

		for i,seat in pairs(self.Seats) do
			if( seat.passengerId ) then
				local member = System.GetEntity( seat.passengerId );
				if( member ~= nil ) then
				  if (seat.isDriver) then
						if ( passenger.id == member.id ) then
							Script.SetTimerForFunction( 4000 , "VehicleBaseAI.PassengerDeadExplode", self );
						end
					end
				end
			end
		end

	end

end

--------------------------------------------------------------------------
function VehicleBaseAI:PassengerDeadExplode()
	g_gameRules:CreateExplosion(self.id,self.id,1000,self:GetPos(),nil,10);
end

--------------------------------------------------------------------------
function VehicleBaseAI:IsEntityOnVehicle(entityId)
	if(entityId) then		
		local numSeats = count( self.Seats );
		for i=1,numSeats do
			local seat = self:GetSeatByIndex( i );
			if ( seat ) then
				local PassengerId = seat:GetPassengerId();
				if ( PassengerId == entityId) then
					do return true end
				end
			end
		end
	end
end


--------------------------------------------------------------------------
function VehicleBaseAI:IsAllowedToUsePrimaryWeapon()
	return self.Properties.bCanUsePrimaryWeapon;
end


--------------------------------------------------------------------------
function VehicleBaseAI:IsAllowedToUseSecondaryWeapon()
	return self.Properties.bCanUseSecondaryWeapon;
end

