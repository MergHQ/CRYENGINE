Script.ReloadScript("Scripts/Entities/AI/Shared/BasicAITable.lua");
Script.ReloadScript("Scripts/Entities/AI/Shared/BasicAIEvent.lua");
Script.ReloadScript("Scripts/Entities/AI/AITerritory.lua");
Script.ReloadScript("Scripts/AI/Anchor.lua");


BasicAI = {
	ai = 1,

	IsAIControlled = function()
		return true
	end,

	primaryWeapon = "FY71",
	secondaryWeapon = "SOCOM",
	auxiliarWeapon = "",

	Server = {},
	Client = {},

	Editor = {
		Icon = "User.bmp",
		IconOnTop = 1,
	},
}


function BasicAI:OnPropertyChange()
	--do not rephysicalize at each property change

	local forceResetAI = System.IsEditor();
	self:RegisterAI(forceResetAI);
	self:OnReset();

end


----------------------------------------------------------------------------------------------------
function BasicAI:OnLoadAI(saved)

	self.AI = {};
	if(saved.AI) then
		self.AI = saved.AI;
	end

	if(saved.Events) then
		self.Events = {};
		local evts = self.Events;
		for name, data in pairs(saved.Events) do
			local eventTargets = saved.Events[name];
			if not evts[name] then evts[name] = {} end
			for i, target in pairs(eventTargets) do
				local TargetId = target[1];
				local TargetEvent = target[2];
				table.insert(evts[name], {TargetId, TargetEvent})
			end
		end
	else
		self.Events = nil;
	end

	if(saved.spawnedEntity) then
		self.spawnedEntity = saved.spawnedEntity;
	else
		self.spawnedEntity = nil;
	end

	if(saved.lastSpawnedEntity) then
		self.lastSpawnedEntity = saved.lastSpawnedEntity;
	else
		self.lastSpawnedEntity = nil;
	end

	if(saved.InitialPosition) then
		self.InitialPosition = saved.InitialPosition;
	else
		self.InitialPosition = nil;
	end
end


----------------------------------------------------------------------------------------------------
function BasicAI:OnSaveAI(save)
	if(self.AI) then
		save.AI = self.AI;
	end

	if(self.Events) then
		save.Events = {};
		local evtsSaved = save.Events
		for name, data in pairs(self.Events) do
			if not evtsSaved[name] then evtsSaved[name] = {} end
			for i, target in pairs(data) do
				local TargetId = target[1];
				local TargetEvent = target[2];
				table.insert(evtsSaved[name], {TargetId, TargetEvent})
			end
		end
	end

	if(self.spawnedEntity) then
		save.spawnedEntity = self.spawnedEntity;
	end
	if(self.lastSpawnedEntity) then
		save.lastSpawnedEntity = self.lastSpawnedEntity;
	end
	if(self.InitialPosition) then
	  save.InitialPosition = self.InitialPosition;
	end
end


-----------------------------------------------------------------------------------------------------
function BasicAI:RegisterAI(bForce)

	-- (KEVIN) Don't re-register if already has an AI and not forced
	-- This is so an entity container reused doesn't create a new AI object
	if (not bForce or bForce == false) then
		if (CryAction.HasAI(self.id)) then
			return;
		end
	end

	if (self ~= g_localActor) then
		if ( self.AIType == nil ) then
			CryAction.RegisterWithAI(self.id, AIOBJECT_ACTOR, self.Properties, self.PropertiesInstance, self.AIMovementAbility,self.melee);
		else
			CryAction.RegisterWithAI(self.id, self.AIType, self.Properties, self.PropertiesInstance, self.AIMovementAbility,self.melee);
		end
		AI.ChangeParameter(self.id,AIPARAM_COMBATCLASS,AICombatClasses.Infantry);
		AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_TARGET,self.forgetTimeTarget);
		AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_SEEK,self.forgetTimeSeek);
		AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_MEMORY,self.forgetTimeMemory);

		self._enabled=true;

		-- If the entity is hidden during
		if (self:IsHidden()) then
			AI.LogEvent(self:GetName()..": The entity is hidden during init -> disable AI.");
			self:TriggerEvent(AIEVENT_DISABLE);
			self._enabled=false;
		end
	end
end

function BasicAI:ResetAIParameters(bFromInit, bIsReload)
	--Log("%s:ResetAIParameters(%s, %s)", self:GetName(), tostring(bFromInit), tostring(bIsReload))
	if ((not bFromInit) or bIsReload) then
		AI.ResetParameters(self.id, bIsReload, self.Properties, self.PropertiesInstance, self.AIMovementAbility, self.melee)
		self._enabled = true

		if (self:IsHidden()) then
			AI.LogEvent(self:GetName()..": The entity is hidden during init -> disable AI.")
			self:TriggerEvent(AIEVENT_DISABLE)
			self._enabled = false
		end
	end

	AI.ChangeParameter(self.id,AIPARAM_COMBATCLASS, AICombatClasses.Infantry);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_TARGET, self.forgetTimeTarget);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_SEEK, self.forgetTimeSeek);
	AI.ChangeParameter(self.id,AIPARAM_FORGETTIME_MEMORY, self.forgetTimeMemory);
end

-----------------------------------------------------------------------------------------------------
function BasicAI:OnReset(bFromInit, bIsReload)
	-- In some cases we want to perform a clean-up on the entity
	-- (stopping effects, kill timers, etc.). This function will
	-- allow us to do this, without running the risk of loosing
	-- table self.AI and such).
	if (self.OnPreReset ~= nil) then
		self:OnPreReset(bFromInit, bIsReload)
	end

	if (self.ResetOnUsed) then
		self:ResetOnUsed();
	end

	self.ignorant = nil;
	self.isFallen = nil;

	local Properties = self.Properties;
	local PropertiesInstance = self.PropertiesInstance;

	if AI then
		-- Reset all properties to editor set values.
		self:ResetAIParameters(bFromInit, bIsReload);

		-- free mounted weapon
		if (self.AI.current_mounted_weapon) then
			self.AI.current_mounted_weapon.reserved = nil;
			self.AI.current_mounted_weapon.listPotentialUsers = nil;
			self.AI.current_mounted_weapon = nil;
		end

		if (PropertiesInstance and PropertiesInstance.bAlarmed ~= 0) then
			AI.SetAlarmed(self.id);
		end
	end

	if (self.AI.bIsLeader) then
		AI.SetLeader(self.id);
		AI.Signal(SIGNALFILTER_SENDER, 0, "REQUEST_JOIN_TEAM",self.id);
	end

	self:NetPresent(1);

	self.UpdateTime = 0.05;
	self:SetScriptUpdateRate(self.UpdateTime);

	self.useAction = AIUSEOP_NONE;
	self.groupid = self.PropertiesInstance.groupid;

	-- now the same for special fire animations

		BasicActor.Reset(self, bFromInit, bIsReload);

	self:AssignPrimaryWeapon();

	-- Register with target tracks
	local Perception = self.Properties.Perception;
	if (Perception.config and Perception.config ~= "") then
		local TargetTracks = Perception.TargetTracks;
		AI.RegisterTargetTrack(self.id, Perception.config, TargetTracks.targetLimit, TargetTracks.classThreat);
	end
	
	if (self.OnResetCustom) then
		self:OnResetCustom(bFromInit, bIsReload)
	end
	
	if (self.OnEnabled) then
		self:OnEnabled()
	end

	self.AI.theVehicle = nil;
	self.AI.NextWeaponAccessory = nil;
	self.AI.WeaponAccessoryMountType = nil;
	self.AI.MountingAccessory = nil;

	self:CheckWeaponAttachments();
	if AI then
		AI.EnableWeaponAccessory(self.id, AIWEPA_LASER, true);
	end

	self:SetColliderMode(Properties.eiColliderMode);

	-- To support spawn at the initial (rather than current) position
	self.InitialPosition = self:GetPos();

	self.AI.invulnerable = self.Properties.bInvulnerable ~= 0;

	self:CheckShaderParamCallbacks();

	if (self.aiSequenceUser) then
		AI.SetBehaviorVariable(self.id, "ExecuteSequence", false)
		AI.SetBehaviorVariable(self.id, "ExecuteInterruptibleSequence", false)
	end
end


---------------------------------------------------------------------
function BasicAI:OnModularBehaviorTreeInitialized()
	-- Warning! The AIActor is still being constructed/serialized
	-- when this function is called. Be careful with what you call.
	-- The modular behavior tree and its variables have been setup
	-- at this point so you can safely set/get variables.

	if (self.OnResetSavedAssignment ~= nil) then
		self:OnResetSavedAssignment()
	end
end


---------------------------------------------------------------------
function BasicAI:OnSpawn(bIsReload)
	-- System.Log("BasicAI.Server:OnSpawn()"..self:GetName());

	-- Register with AI
	self:RegisterAI(not bIsReload);
end


---------------------------------------------------------------------
function BasicAI:OnBeingReused()
	self:Event_Disabled(self);
end


---------------------------------------------------------------------
function BasicAI:GetReturnToPoolWeight(isUrgent)

	-- Don't consider me if I'm not dead unless its urgent
	local isDead = self:IsDead();
	if (not isDead and not isUrgent) then
		return 0;
	end

	local weight = 0.0;

	-- Add large bonus if dead
	if (isDead) then
		weight = weight + 1000.0;
	end

	-- Add distance from player
	if (g_localActor) then
		local distance = self:GetDistance(g_localActor.id);
		weight = weight + distance;
	end

	return weight;

end

---------------------------------------------------------------------
function BasicAI:OnGetPoolSignature(signature)

	if (self.AIType) then
		signature.AIType = self.AIType;
	end

end



--------------------------------------------------------------------------------------------------------
function BasicAI.Server:OnInit(bIsReload)
	--Log("%s.Server:OnInit(%s)", self:GetName(), tostring(bIsReload));

	if (not self.AI) then
		self.AI = {};
	end

	self:OnReset(true, bIsReload);

	-- Go back to wave if being reloaded
	if (bIsReload and self.SetupTerritoryAndWave) then
		self:SetupTerritoryAndWave();
	end

	-- Output via FG node that I'm enabled if I was reloaded and not in a wave
	if (bIsReload and not self.AI.Wave) then
		self:Event_Enabled(self);
	end
end


function BasicAI.Server:OnInitClient( channelId )
	if (self._enabled) then
		self.onClient:ClAIEnable(channelId);
	else
		self.onClient:ClAIDisable(channelId);
	end
end


function BasicAI.Client:ClAIEnable()
	if (not CryAction.IsServer()) then
		self:Hide(0)
		self:Event_Enabled(self);
	end
end


function BasicAI.Client:ClAIDisable()
	if (not CryAction.IsServer()) then
		self:Hide(1)
		self:TriggerEvent(AIEVENT_DISABLE);
	end
end



--------------------------------------------------------------------------------------------------------
function BasicAI.Client:OnShutDown()
	BasicActor.ShutDown(self);
end


--------------------------------------------------------------------------------------------------------
function BasicAI:MakeAlerted( noDrawWeapon )

	if(noDrawWeapon~=nil) then return end

	self:DrawWeaponNow( );

end

--------------------------------------------------------------------------------------------------------
function BasicAI:MakeIdle( holsterWeapon )
	-- Make this guy idle
	AI.ChangeParameter(self.id,AIPARAM_SIGHTRANGE,self.Properties.Perception.sightrange);
	AI.ChangeParameter(self.id,AIPARAM_FOVPRIMARY,self.Properties.Perception.FOVPrimary);

	--self:SelectPipe(0,"stand_only");
	--self:InsertSubpipe(0,"setup_idle");
	--self:InsertSubpipe(0,"clear_all"); -- to allow receive again onplayerseen

	self:SelectPipe(0,"do_nothing");

	if (holsterWeapon) then
		self.actor:HolsterItem(true);
	end
end

--------------------------------------------------------------------------------------------------------
function BasicAI:InitAIRelaxed(forceWeaponHolster)

	self:MakeIdle(forceWeaponHolster);
	AI.SetStance(self.id, STANCE_RELAXED);
end

--------------------------------------------------------------------------------------------------------
function BasicAI:AssignPrimaryWeapon()
  -- this is the new way of equiping actors
	local equipmentPack = self.Properties.equip_EquipmentPack;
	if (equipmentPack and equipmentPack ~= "") then
		self.primaryWeapon = ItemSystem.GetPackPrimaryItem(equipmentPack) or "";

    -- get secondary weapon
    if (ItemSystem.GetPackNumItems(equipmentPack)>1) then
	    self.secondaryWeapon = ItemSystem.GetPackItemByIndex(equipmentPack, 1) or "";
			-- make sure any kind of grenades are not considered as secondary weapon
	    if( self.secondaryWeapon == "AIFlashbangs" or
			    self.secondaryWeapon == "AISmokeGrenades" or
	  		  self.secondaryWeapon == "AIGrenades" or
	  		  self.secondaryWeapon == "AIEMPGrenades" ) then
		    		self.secondaryWeapon = "";
    end
	    --Log("%s has secondary weapon %s", self:GetName(), self.secondaryWeapon);
    end

    -- get third/auxiliar weapon
    if (ItemSystem.GetPackNumItems(equipmentPack)>2) then
	    self.auxiliarWeapon = ItemSystem.GetPackItemByIndex(equipmentPack, 2) or "";
	    --Log("%s has secondary weapon %s", self:GetName(), self.secondaryWeapon);
    end
	end

	--Log("%s:AssignPrimaryWeapon(): %s", self:GetName(), tostring(self.primaryWeapon));
end

--------------------------------------------------------------------------------------------------------
function BasicAI:DrawWeaponNow( skipCheck )
	if ( skipCheck~=1 and self.inventory:GetCurrentItem() ) then
		-- there is something in his hands - don't change weapon, just make sure it's out
		return
	end

	local weapon = self.inventory:GetCurrentItem();
	-- make sure we select primary weapon
	if (weapon==nil or weapon.class~=self.primaryWeapon) then
		self.actor:SelectItemByName(self.primaryWeapon);
	end

	-- lets set burst fire mode - only Kuang has it currently
	weapon = self.inventory:GetCurrentItem();
	if(weapon~=nil and weapon.weapon~=nil and weapon.class==self.primaryWeapon) then
		weapon.weapon:SetCurrentFireMode("burst");
	end

--	self:UseLAM("FlashLight",true);
--	self:UseLAM("Laser",true);
end

--
--------------------------------------------------------------------------------------------------------
-- check selected weapon
-- returns nil if no weapon selected, 0 if primary weapon selected, 1 if secondary
--
function BasicAI:CheckCurWeapon( checkDistance )

	if(checkDistance~=nil) then
		local targetDist = AI.GetAttentionTargetDistance(self.id);
		if(targetDist and targetDist>10.5) then return nil end
	end

	local currentWeapon = self.inventory:GetCurrentItem();
	if(currentWeapon==nil) then return nil end
	if(currentWeapon.class==self.primaryWeapon) then return 0 end
	if(currentWeapon.class==self.secondaryWeapon) then return 1 end

end

--------------------------------------------------------------------------------------------------------
function BasicAI:HasSecondaryWeapon()
	local secondaryWeaponId=self.inventory:GetItemByClass(self.secondaryWeapon);
	-- see if secondary weapon is awailable
	if(secondaryWeaponId==nil) then		return nil	end
	do return 1 end
end

--------------------------------------------------------------------------------------------------------
function BasicAI:SelectSecondaryWeapon()

	if(self:IsOnVehicle()) then
		return nil;
	end

	local secondaryWeaponId=self.inventory:GetItemByClass(self.secondaryWeapon);
	-- see if secondary weapon is awailable
	if(secondaryWeaponId==nil) then		return nil	end
	-- see if it is already selected
	local currentWeapon = self.inventory:GetCurrentItem();
	--AI.LogComment(entity:GetName().." ScoutIdle:Constructor weapon = "..weapon.class);
	if(currentWeapon~=nil and currentWeapon.class==self.secondaryWeapon) then return nil end

	self.actor:SelectItemByName( self.secondaryWeapon );

	do return 1 end
end

--------------------------------------------------------------------------------------------------------
function BasicAI:SelectAuxiliarWeapon()

	if(self:IsOnVehicle()) then
		return nil;
	end

	local auxiliarWeaponId=self.inventory:GetItemByClass(self.auxiliarWeapon);
	-- see if auxiliar weapon is awailable
	if(auxiliarWeaponId==nil) then		return nil	end
	-- see if it is already selected
	local currentWeapon = self.inventory:GetCurrentItem();
	--AI.LogComment(entity:GetName().." ScoutIdle:Constructor weapon = "..weapon.class);
	if(currentWeapon~=nil and currentWeapon.class==self.auxiliarWeapon) then return 1 end

	self.actor:SelectItemByName( self.auxiliarWeapon );

	do return 1 end
end

--------------------------------------------------------------------------------------------------------
function BasicAI:SelectPrimaryWeapon(forceFastSelect)
	--Log("%s: SelectPrimaryWeapon() %s", self:GetName(), self.primaryWeapon)
	if (not self:IsOnVehicle()) then
		self.actor:SelectItemByName(self.primaryWeapon, forceFastSelect)
	end
end

--------------------------------------------------------------------------------------------------------
function BasicAI:Reload()
	local weapon = self.inventory:GetCurrentItem();
	if(weapon~=nil and weapon.weapon~=nil) then
		weapon.weapon:Reload();
	else
		AI.LogEvent(">>>>"..self:GetName().." FAILED TO RELOAD WEAPON!");
	end
end

-----------------------------------------------------------------------------------
function BasicAI:DropItem()
	local item = self.inventory:GetCurrentItem();
	if (item) then
		item:Drop();
	end
end

-----------------------------------------------------------------------------------
function BasicAI:IsOnVehicle()
if (self.vehicleId) then
		return true;
	end

	return false;
end

-----------------------------------------------------------------------------------
function BasicAI:AnimationEvent(event,value)
	--Log("BasicAI:AnimationEvent "..event.." "..value);
	if ( event == "useObject" ) then
		local navObject = AI.GetLastUsedSmartObject( self.id );
		if ( navObject and navObject.OnUsed ) then
			navObject:OnUsed( self, 2 );
			AI.SmartObjectEvent( "OnUsed", navObject.id, self.id );
		end
	elseif ( event == "dropItem" ) then
		self:DropItem();
	elseif ( event == "kickObject" ) then
		local navObject = AI.GetLastUsedSmartObject( self.id );
		if ( navObject ) then
			if ( navObject.BreachDoor ) then
				navObject:BreachDoor();
			else
				navObject:AddImpulse( -1, nil, self:GetDirectionVector(1), self:GetMass(), 1 );
			end
		end
--	elseif ( event == "ThrowGrenade" ) then
	elseif ( BasicActor.AnimationEvent ) then
		BasicActor.AnimationEvent(self,event,value);
	end
end

--------------------------------------------------------------------------------
function BasicAI:CreateFormation(otherLeader, bPersistent )
	local target;
	if(g_StringTemp1) then
		target = System.GetEntityByName(g_StringTemp1);
	end
	g_SignalData.point = g_SignalData_point;
	if(target~=nil) then
		CopyVector(g_SignalData.point, target:GetWorldPos());
		g_SignalData.point.z = self:GetWorldPos().z;
	else
		CopyVector(g_SignalData.point, g_Vectors.v000);
	end
	if(otherLeader and not otherLeader:IsDead()) then
		g_SignalData.id = otherLeader.id;
	else
		g_SignalData.id = self.id;
	end
	if(bPersistent) then
		g_SignalData.fValue = 1;
	else
		g_SignalData.fValue = 0;
	end
	g_SignalData.iValue = AI.GetGroupOf(self.id);
	self.AI.Follow = true;
	AI.Signal(SIGNALFILTER_LEADER,0,"ORD_FOLLOW",self.id,g_SignalData);
 	g_StringTemp1 = ""; -- safer for further calls, since it's an optional parameter
end


--------------------------------------------------------------------------------
function BasicAI:JoinFormation(groupid)
	g_SignalData.iValue = groupid;
	self.AI.Follow = true;
	AI.Signal(SIGNALFILTER_LEADER,0,"ORD_FOLLOW",self.id,g_SignalData);
end


function BasicAI.OnDeath( entity )
	AI.SetSmartObjectState( entity.id, "Dead" );

	-- notify spawner - so it counts down and updates
	if(entity.AI.spawnerListenerId) then
		local spawnerEnt = System.GetEntity(entity.AI.spawnerListenerId);
		if(spawnerEnt) then
			spawnerEnt:UnitDown();
		end
	end

 	if(entity.AI.theVehicle and entity.AI.theVehicle:IsDriver(entity.id)) then
 			-- disable vehicle's AI
 		if (entity.AI.theVehicle.AIDriver) then
 		  entity.AI.theVehicle:AIDriver(0);
 		end
 		entity.AI.theVehicle=nil;
 	end

	GameAI.UnregisterWithAllModules(entity.id);
	AI.UnregisterTargetTrack(entity.id);

	if(entity.Event_Dead) then
		entity:Event_Dead(entity);
	end

	-- free mounted weapon
	if (entity.AI.current_mounted_weapon) then
		if (entity.AI.current_mounted_weapon.item:GetOwnerId() == entity.id) then
			entity.AI.current_mounted_weapon.item:Use( entity.id );--Stop using
			entity.AI.current_mounted_weapon.reserved = nil;
			AI.ModifySmartObjectStates(entity.AI.current_mounted_weapon.id,"Idle,-Busy");
		end
		entity.AI.current_mounted_weapon.listPotentialUsers = nil;
		entity.AI.current_mounted_weapon = nil;
		AI.ModifySmartObjectStates(entity.id,"-Busy");
	end
	-- check ammo count modifier
	if(entity.AI.AmmoCountModifier and entity.AI.AmmoCountModifier>0) then
		entity:ModifyAmmo();
	end
end

----------------------------------------------------------------------------------
function BasicAI:ModifyAmmo(multiplier)

	local item = self.inventory:GetCurrentItem();
	if(item) then
		local currWeapon = item.weapon;
		if(currWeapon ) then
			if(multiplier) then
				local ammoCount = currWeapon:GetClipSize();
				currWeapon:SetAmmoCount(nil, ammoCount*multiplier)
			elseif(self.AI.AmmoCountModifier) then
				if( self.AI.AmmoCountModifier==0) then
					self.AI.AmmoCountModifier=1;
				end
				local ammoCount = currWeapon:GetAmmoCount();
				currWeapon:SetAmmoCount(nil, ammoCount/self.AI.AmmoCountModifier);
			end
			self.AI.AmmoCountModifier = multiplier;
		end
	end
end


----------------------------------------------------------------------------------

function BasicAI:Expose()
	Net.Expose{
		Class = self,
		ClientMethods = {
			ClAIEnable={ RELIABLE_ORDERED, PRE_ATTACH },
			ClAIDisable={ RELIABLE_ORDERED, PRE_ATTACH },
		},
		ServerMethods = {
		},
		ServerProperties = {
		}
	};
end


----------------------------------------------------------------------------------
function BasicAI:CheckWeaponAttachments()
	self:CheckSingleWeaponAttachment("SCARAB", "LaserSight", true)
	self:CheckSingleWeaponAttachment("CellSCARAB", "LaserSight", true)
	self:CheckSingleWeaponAttachment("SCAR", "LaserSight", true)
	self:CheckSingleWeaponAttachment("CellSCAR", "LaserSight", true)
	self:CheckSingleWeaponAttachment("Mk60", "LaserSight", true)
	self:CheckSingleWeaponAttachment("Feline", "LaserSight", true)
	self:CheckSingleWeaponAttachment("CellFeline", "LaserSight", true)

	self:CheckSingleWeaponAttachment("AY69", "PistolLaserSight", true)
	self:CheckSingleWeaponAttachment("Nova", "PistolLaserSight", true)
	self:CheckSingleWeaponAttachment("Hammer", "PistolLaserSight", true)
	self:CheckSingleWeaponAttachment("CellHammer", "PistolLaserSight", true)

	self:CheckSingleWeaponAttachment("SCAR", "Reflex", true)
	self:CheckSingleWeaponAttachment("CellSCAR", "Reflex", true)
	self:CheckSingleWeaponAttachment("SCARAB", "Reflex", true)
	self:CheckSingleWeaponAttachment("CellSCARAB", "Reflex", true)
	self:CheckSingleWeaponAttachment("Marshall", "Reflex", true)
	self:CheckSingleWeaponAttachment("Feline", "Reflex", true)
	self:CheckSingleWeaponAttachment("CellFeline", "Reflex", true)
	self:CheckSingleWeaponAttachment("DSG1", "Reflex", true)
	self:CheckSingleWeaponAttachment("Grendel", "Reflex", true)
	self:CheckSingleWeaponAttachment("Gauss", "Reflex", true)
	self:CheckSingleWeaponAttachment("CellGauss", "Reflex", true)
	self:CheckSingleWeaponAttachment("AY69", "Reflex", true)
	self:CheckSingleWeaponAttachment("K-Volt", "Reflex", true)
	self:CheckSingleWeaponAttachment("CellK-Volt", "Reflex", true)

	self:CheckSingleWeaponAttachment("Majestic", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("SCAR", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("CellSCAR", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("DSG1", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("Gauss", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("CellGauss", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("Mk60", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("Grendel", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("K-Volt", "AssaultScope", true)
	self:CheckSingleWeaponAttachment("CellK-Volt", "AssaultScope", true)

	self:CheckSingleWeaponAttachment("DSG1", "SniperScope", true)
	self:CheckSingleWeaponAttachment("Gauss", "SniperScope", true)
	self:CheckSingleWeaponAttachment("CellGauss", "SniperScope", true)

	self:CheckSingleWeaponAttachment("Nova", "SilencerPistol", true)
	self:CheckSingleWeaponAttachment("Hammer", "SilencerPistol", true)
	self:CheckSingleWeaponAttachment("CellHammer", "SilencerPistol", true)

	self:CheckSingleWeaponAttachment("Marshall", "Silencer", true)
	self:CheckSingleWeaponAttachment("Feline", "Silencer", true)
	self:CheckSingleWeaponAttachment("CellFeline", "Silencer", true)
	self:CheckSingleWeaponAttachment("SCARAB", "Silencer", true)
	self:CheckSingleWeaponAttachment("CellSCARAB", "Silencer", true)
	self:CheckSingleWeaponAttachment("DSG1", "Silencer", true)

	self:CheckSingleWeaponAttachment("SCAR", "GrenadeLauncher", true)
	self:CheckSingleWeaponAttachment("CellSCAR", "GrenadeLauncher", true)
	self:CheckSingleWeaponAttachment("Grendel", "GrenadeLauncher", true)

	self:CheckSingleWeaponAttachment("SCARAB", "LightShotgun", true)
	self:CheckSingleWeaponAttachment("CellSCARAB", "LightShotgun", true)
	self:CheckSingleWeaponAttachment("Grendel", "LightShotgun", true)

	self:CheckSingleWeaponAttachment("AY69", "ExtendedClip", true)
	self:CheckSingleWeaponAttachment("Jackal", "ExtendedClip", true)
	self:CheckSingleWeaponAttachment("Feline", "ExtendedClip", true)
	self:CheckSingleWeaponAttachment("CellFeline", "ExtendedClip", true)
	self:CheckSingleWeaponAttachment("SCARAB", "ExtendedClip", true)
	self:CheckSingleWeaponAttachment("CellSCARAB", "ExtendedClip", true)
	self:CheckSingleWeaponAttachment("Mk60", "ExtendedClip", true)

	self:CheckSingleWeaponAttachment("SCAR", "GaussAttachment", true)
	self:CheckSingleWeaponAttachment("CellSCAR", "GaussAttachment", true)
end

----------------------------------------------------------------------------------
function BasicAI:CheckSingleWeaponAttachment(weaponClass,attachmentClass,attach)
		local itemId = self.inventory:GetItemByClass(weaponClass);
		if (itemId) then
  			local item = System.GetEntity(itemId);
  			local att = self.inventory:HasAccessory(attachmentClass);
  			if(item and att) then
  				local currWeapon = item.weapon;
  				local attached = item.item:HasAccessory(attachmentClass);
  				if(currWeapon and currWeapon:SupportsAccessory(attachmentClass) and not attached) then
  					currWeapon:SwitchAccessory(attachmentClass);
  				end
  			end
		end
end

----------------------------------------------------------------------------------
function BasicAI.Client:OnPreparedFromPool()
	if (self.AI.Wave) then
	  wave = GetAIWaveFromName( self.AI.Wave );
	  if (wave) then
	  	wave:OnEntityPreparedFromPool( self );
	  end
	end
end

function BasicAI:IsInvulnerable()
	return self.AI.invulnerable
end

----------------------------------------------------------------------------------
function CreateAI(child)

	local newt={}
	mergef(newt,child,1);
	mergef(newt,BasicAI,1);
	mergef(newt,BasicAIEvent,1);
	mergef(newt,BasicAITable,1);

	MakeSpawnable(newt)

	return newt;
end

----------------------------------------------------------------------------------
