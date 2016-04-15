Script.ReloadScript( "SCRIPTS/Entities/actor/BasicActor.lua");

PlayerHeavy =
{

	AnimationGraph = "player.xml",
	UpperBodyGraph = "",

	type = "Player",

	Properties =
	{
		-- AI-related properties
		soclasses_SmartObjectClass = "Player",
		groupid = 0,
		esFaction = "Players",
		commrange = 40; -- Luciano - added to use SIGNALFILTER_GROUPONLY
		-- AI-related properties over

		aicharacter_character = "Player",

		Damage =
		{
			bLogDamages = 0,
			health = 90, -- See  Player:SetIsMultiplayer()  for MP value
			--playerMult = 1.0,
			--AIMult = 1.0,
			FallSleepTime = 6.0,
		},

		CharacterSounds =
		{
			footstepEffect = "footstep_player", -- Footstep mfx library to use
			remoteFootstepEffect = "footstep", -- Footstep mfx library to use for remote players
			bFootstepGearEffect = 0, -- This plays a sound from materialfx
			footstepIndGearAudioSignal_Walk = "Player_Footstep_Gear_Walk", -- This directly plays the specified audiosignal on every footstep
			footstepIndGearAudioSignal_Run = "Player_Footstep_Gear_Run", -- This directly plays the specified audiosignal on every footstep
			foleyEffect = "foley_player", -- Foley signal effect name
		},

		Perception =
		{
			--ranges
			sightrange = 50,
		},

		fileModel = "Objects/Characters/Human/sdk_player/sdk_player.cdf",
		shadowFileModel = "Objects/Characters/Human/sdk_player/sdk_player.cdf",
		clientFileModel = "Objects/Characters/Human/sdk_player/sdk_player.cdf",

		fileHitDeathReactionsParamsDataFile = "Libs/HitDeathReactionsData/HitDeathReactions_PlayerSP.xml",
	},

	PropertiesInstance = 
	{
		aibehavior_behaviour = "PlayerIdle",
	},

	gameParams =
	{
		inventory =
		{
			items =
			{
				{ name = "Zeus", equip = true },
			},
			ammo =
			{
				{ name = "lightbullet", amount = 28},
			},
		},

		Damage =
		{
			health = 90,
		},


		stance =
		{
			{
				stanceId = STANCE_STAND,
				maxSpeed = 4.0,
				heightCollider = 1.7,
				heightPivot = 0.0,
				size = {x=0.9,y=0.9,z=0.1},
				modelOffset = {x=0,y=0.0,z=0},
				viewOffset = {x=0,y=-2.00,z=3.2},
				weaponOffset = {x=0.2,y=0.0,z=1.55},
				name = "stand",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_ALERTED,
				normalSpeed = 1.0,
				maxSpeed = 50.0,
				heightCollider = 1.8,
				heightPivot = 0.0,
				size = {x=0.9,y=0.9,z=0.1},
				modelOffset = {x=0,y=0.0,z=0},
				viewOffset = {x=0,y=-2.00,z=3.0},
				weaponOffset = {x=0.2,y=0.0,z=1.55},
				name = "alerted",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_STEALTH,
				normalSpeed = 0.6,
				maxSpeed = 50.0,
				heightCollider = 1.0,
				heightPivot = 0.0,
				size = {x=0.4,y=0.4,z=0.1},
				modelOffset = {x=0.0,y=-0.0,z=0},
				viewOffset = {x=0,y=-2.0,z=3.35},
				weaponOffset = {x=0.2,y=0.0,z=1.1},
				name = "stealth",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_CROUCH,
				normalSpeed = 0.5,
				maxSpeed = 1.5,
				heightCollider = 0.8,
				heightPivot = 0.0,
				size = {x=0.4,y=0.4,z=0.1},
				modelOffset = {x=0.0,y=0.0,z=0},
				viewOffset = {x=0,y=-2.0,z=3.1},
				weaponOffset = {x=0.2,y=0.0,z=0.85},

				name = "crouch",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_LOW_COVER,
				normalSpeed = 0.5,
				maxSpeed = 50.0,
				heightCollider = 0.8,
				heightPivot = 0.0,
				size = {x=0.4,y=0.4,z=0.1},
				modelOffset = {x=0.0,y=0.0,z=0},
				viewOffset = {x=0,y=0.0,z=0.9},
				weaponOffset = {x=0.2,y=0.0,z=0.85},

				name = "coverLow",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_HIGH_COVER,
				normalSpeed = 1.0,
				maxSpeed = 50.0,
				heightCollider = 1.2,
				heightPivot = 0.0,
				size = {x=0.4,y=0.4,z=0.2},
				modelOffset = {x=0,y=-0.0,z=0},
				viewOffset = {x=0,y=0.10,z=1.7},
				weaponOffset = {x=0.2,y=0.0,z=1.1},

				name = "coverHigh",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_SWIM,
				normalSpeed = 1.0, -- this is not even used?
				maxSpeed = 50.0, -- this is ignored, overridden by pl_swim* cvars.
				heightCollider = 0.9,
				heightPivot = 0.5,
				size = {x=0.4,y=0.4,z=0.1},
				modelOffset = {x=0,y=0,z=0.0},
				viewOffset = {x=0,y=0.1,z=0.5},
				weaponOffset = {x=0.2,y=0.0,z=0.3},
				name = "swim",
				useCapsule = 1,
			},
			--AI states
			{
				stanceId = STANCE_RELAXED,
				normalSpeed = 1.0,
				maxSpeed = 50.0,
				heightCollider = 1.8,
				heightPivot = 0.0,
				size = {x=0.9,y=0.9,z=0.1},
				modelOffset = {x=0,y=0.0,z=0},
				viewOffset = {x=0,y=0.10,z=2.0},
				weaponOffset = {x=0.2,y=0.0,z=1.55},
				name = "relaxed",
				useCapsule = 1,
			},
		},

		autoAimTargetParams =
		{
			primaryTargetBone = BONE_SPINE,
			physicsTargetBone = BONE_SPINE,
			secondaryTargetBone = BONE_HEAD,
			fallbackOffset = 1.2,
			innerRadius = 1.0,
			outerRadius = 3.5,
			snapRadius = 1.0,
			snapRadiusTagged = 1.0,
		},

--[[	boneIDs =
		{
			BONE_SPINE = "Spine01",
			BONE_SPINE2 = "Spine02",
			BONE_SPINE3 = "Spine03",
			BONE_HEAD = "Head",
			BONE_WEAPON = "weapon_bone",
			BONE_FOOT_R = "R Foot",
			BONE_FOOT_L = "L Foot",
		},
	]]
		boneIDs =
		{
			BONE_BIP01 = "Bip01",
			BONE_SPINE = "Bip01 Spine1",
			BONE_SPINE2 = "Bip01 Spine2",
			BONE_SPINE3 = "Bip01 Spine3",
			BONE_HEAD = "Bip01 Head",
			BONE_EYE_R = "eye_right_bone",
			BONE_EYE_L = "eye_left_bone",
			BONE_WEAPON = "weapon_bone",
			BONE_WEAPON2 = "Lweapon_bone",
			BONE_FOOT_R = "Bip01 R Foot",
			BONE_FOOT_L = "Bip01 L Foot",
			BONE_ARM_R = "Bip01 R Forearm",
			BONE_ARM_L = "Bip01 L Forearm",
			BONE_CALF_R = "Bip01 R Calf",
			BONE_CALF_L = "Bip01 L Calf",
			BONE_CAMERA = "Bip01 Camera",
			BONE_HUD = "Bip01 HUD",
		},

		characterDBAs =
		{
			"HumanPlayer",
		},

		lookFOV = 90,
		aimFOV = 180,

		proceduralLeaningFactor = 0.0, -- disable procedural leaning as we have turn assets now
	},

	SoundSignals =
	{
		FeedbackHit_Armor = -1,
		FeedbackHit_Flesh = -1,
	},

	Server = {},
	Client = {},
	squadFollowMode = 0,

	squadTarget = {x=0,y=0,z=0},
	SignalData = {},

	AI = {},
	OnUseEntityId = NULL_ENTITY,
	OnUseSlot = 0,
	useHoldFiredAlready = false,

	-- Hack: Needed since use and pickup actions for keyboard will make self.use called twice which causes an exchange weapon error
	usePressFiredForPickup = true,
	usePressFiredForUse = true,
}

-- Function called from C++ to set up multiplayer parameters
function PlayerHeavy:SetIsMultiplayer()

end

function PlayerHeavy:Expose()
	Net.Expose{
		Class = self,
		ClientMethods = {
			Revive = { RELIABLE_ORDERED, POST_ATTACH },
			MoveTo = { RELIABLE_ORDERED, POST_ATTACH, VEC3 },
			AlignTo = { RELIABLE_ORDERED, POST_ATTACH, VEC3 },
			ClearInventory = { RELIABLE_ORDERED, POST_ATTACH },
		},
		ServerMethods = {
			--UseEntity = { RELIABLE_ORDERED, POST_ATTACH, ENTITYID, INT16, BOOL},
		},
		ServerProperties = {
		}
	};
end

function PlayerHeavy.Server:OnInit(bIsReload)
	--self.actor:SetPhysicalizationProfile("alive");

	if AI then
		--AI related: create ai representation for the player
		CryAction.RegisterWithAI(self.id, AIOBJECT_PLAYER, self.Properties,self.PropertiesInstance);

		--AI related: player is leader of squad-mates always
		--AI.SetLeader(self.id);
	end

	self:OnInit(bIsReload);
end

function PlayerHeavy:PhysicalizeActor()
	--BasicActor.PhysicalizeActor(self);
end


function PlayerHeavy:SetModel(model, arms, frozen, fp3p)
	if (model) then
		if (fp3p) then
			self.Properties.clientFileModel = fp3p;
		end
		self.Properties.fileModel = model;
	end
end


function PlayerHeavy.Server:OnInitClient( channelId )
end


function PlayerHeavy.Server:OnPostInitClient( channelId )
	--for i,v in ipairs(self.inventory) do
		--self.onClient:PickUpItem(channelId, v, false);
	--end

	--if (self.inventory:GetCurrentItemId()) then
		--self.onClient:SetCurrentItem(channelId, self.inventory:GetCurrentItemId());
	--end
end


function PlayerHeavy.Client:Revive()
	self.actor:Revive();
end


function PlayerHeavy.Client:MoveTo(pos)
	self:SetWorldPos(pos);
end


function PlayerHeavy.Client:AlignTo(ang)
	self.actor:SetAngles(ang);
end

function PlayerHeavy.Client:ClearInventory()
	self.inventory:Clear();
end

function PlayerHeavy.Client:OnSetPlayerId()
--	HUD:Spawn(self);
end


function PlayerHeavy.Client:OnInit(bIsReload)
	self:OnInit(bIsReload);
end

function PlayerHeavy:OnInit(bIsReload)

--	AI.RegisterWithAI(self.id, AIOBJECT_PLAYER, self.Properties);
	self:SetAIName(self:GetName());
	----------------------------------------

--	self:InitSounds();

	self:OnReset(true, bIsReload);
	--self:SetTimer(0,1);
end


function PlayerHeavy:OnReset(bFromInit, bIsReload)

	self.SoundSignals.FeedbackHit_Armor = GameAudio.GetSignal("PlayerFeedback_HitArmor" );
	self.SoundSignals.FeedbackHit_Flesh = GameAudio.GetSignal("PlayerFeedback_HitFlesh" );

	g_aimode = nil;

	BasicActor.Reset(self, bFromInit, bIsReload);

	self:SetTimer(0,500);

	mergef(PlayerHeavy.SignalData,g_SignalData,1);

	self.squadFollowMode = 0;


	self.Properties.esFaction = "Players";
	-- Reset all properties to editor set values.
	if AI then AI.ResetParameters(self.id, bIsReload, self.Properties, self.PropertiesInstance, nil,self.melee) end;

	self.lastOverloadTime = nil;

end

function PlayerHeavy:SetOnUseData(entityId, slot)
	self.OnUseEntityId = entityId
	self.OnUseSlot = slot
end

function PlayerHeavy:ItemPickUpMechanic(action, activation)
	local isOnlyPickupAction = (action == "itemPickup");
	local isOnlyUseAction = (action == "use");
	local isUseAction = (isOnlyUseAction or isOnlyPickupAction);

	if (action == "heavyweaponremove" and activation == "press" and self.actor:IsCurrentItemHeavy()) then
		self:UseEntity( self.OnUseEntityId, self.OnUseSlot, true);
		self.useHoldFiredAlready = true;
	elseif (isUseAction and activation == "press") then
		-- HACK
		-- This will happen twice on keyboard since Use and itemPickup use same input
		-- To avoid refactoring code right now, only allow self.UseEntity to be called once per input frame
		-- But make sure this functions properly even if use and itemPickup inputs are changed
		local fireUseEntity = false;

		if (isOnlyPickupAction) then
			if (self.usePressFiredForUse) then -- 2nd call this frame, don't call
				self.usePressFiredForUse = false; -- Reset
				self.usePressFiredForPickup = false; -- Reset
			else
				fireUseEntity = true;
				self.usePressFiredForPickup = true;
			end
		elseif (isOnlyUseAction) then
			if (self.usePressFiredForPickup) then -- 2nd call this frame, don't call
				self.usePressFiredForPickup = false; -- Reset
				self.usePressFiredForUse = false; -- Reset
			else
				fireUseEntity = true;
				self.usePressFiredForUse = true;
			end
		end

		if (fireUseEntity) then
			Log("[tlh] @ Player:OnAction: action: "..action.." press path");
			self:UseEntity( self.OnUseEntityId, self.OnUseSlot, true);
		end
	elseif (isUseAction and activation == "hold" and self.useHoldFiredAlready == false) then
		if(self.OnUseEntityId ~= NULL_ENTITY) then
			self.useHoldFiredAlready = true;
			self:UseEntity( self.OnUseEntityId, self.OnUseSlot, true);
		end;
	elseif (isUseAction and activation == "release") then
		self.useHoldFiredAlready = false;
	end
end

function PlayerHeavy:OnActionUse(press)
	self:UseEntity( self.OnUseEntityId, self.OnUseSlot, press);
end

function PlayerHeavy:OnUpdateView(frameTime)
--	HUD:UpdateHUD(self, frameTime, true);--not self:IsHidden());
end

function PlayerHeavy:GrabObject(object, query)
	BasicActor.GrabObject(self, object, query);
end

function PlayerHeavy.Client:OnTimer(timerId,mSec)
	if(timerId==SWITCH_WEAPON_TIMER) then
		if AI then AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT,1,"CheckNextWeaponAccessory",self.id) end;

		-- set player combat class depending on weapon
		local item = self.inventory:GetCurrentItem();
		if(item and item.class=="LAW") then
			if AI then AI.ChangeParameter( self.id, AIPARAM_COMBATCLASS, AICombatClasses.PlayerRPG ) end
		else
			if AI then AI.ChangeParameter( self.id, AIPARAM_COMBATCLASS, AICombatClasses.Player ) end
		end
	else
		BasicActor.Client.OnTimer(self,timerId,mSec);
	end
end

function PlayerHeavy.Client:OnHit(hit, remote)
	BasicActor.Client.OnHit(self,hit,remote);
end

function PlayerHeavy:UseEntity(entityId, slot, press)
	assert(entityId)
	assert(slot)

	if ((self.actor:GetHealth() <= 0) or (self.actor:GetSpectatorMode()~=0)) then
		return;
	end

	local usedEntity = false;
	local entity = System.GetEntity(entityId)
	if entity then

		local onUsed = entity.OnUsed;
		local onUsedRelease = entity.OnUsedRelease;

		if (not onUsed) then
			local state = entity:GetState();
			if (state ~= "" and entity[state]) then
				onUsed = entity[state].OnUsed;
			end
		end

		if (not onUsedRelease) then
			local state = entity:GetState();
			if (state ~= "" and entity[state]) then
				onUsedRelease = entity[state].OnUsedRelease;
			end
		end

		--special case for grabbing
		if (self.grabParams.entityId) then
			if (press) then
				self.grabParams.dropTime = _time;
				return;
			elseif (self.grabParams.dropTime) then
				--drop it
				press = true;
			else
				return;
			end
		end

		if (onUsed and press) then
			usedEntity = onUsed(entity,self,slot);
			if AI then AI.SmartObjectEvent("OnUsed",entity.id,self.id) end;
		end

		if(onUsedRelease and not press) then
			onUsedRelease(entity,self,slot);
			if AI then AI.SmartObjectEvent("OnUsedRelease",entity.id,self.id) end;
		end
	end

	return usedEntity;
end

function PlayerHeavy.Client:OnShutDown()
	BasicActor.ShutDown(self);

end


function PlayerHeavy:OnEvent( EventId, Params )
end


function PlayerHeavy:OnSave(save)
	BasicActor.OnSave(self, save);
end


function PlayerHeavy:OnLoad(saved)
	BasicActor.OnLoad(self, saved);
end

function PlayerHeavy:OnLoadAI(saved)
	self.AI = {};
	if(saved.AI) then
		self.AI = saved.AI;
	end
end

function PlayerHeavy:OnSaveAI(save)
	if(self.AI) then
		save.AI = self.AI;
	end
end

--FIXME
function PlayerHeavy:CanPickItem(item)
	return self:CanChangeItem();
end

function PlayerHeavy:CanChangeItem()
	--if weapon is holstered, its not possible to switch weapons either
	if (self.holsteredItemId) then
		return false;
	end

	return true;
end

function PlayerHeavy:DropItem()
	local item;

	item = self.inventory:GetCurrentItem();
	if (item) then
		item:Drop();
	end
end

function PlayerHeavy:SetFollowMode( )
	AIBehaviour.PlayerIdle:Follow(self);
end


function PlayerHeavy:Goto()

end

function PlayerHeavy:OnEndCommandSound(timerid)
	-- note: self is not the player but the entity which the signal is sent
	if AI then AI.Signal(SIGNALFILTER_SENDER,1,"ON_COMMAND_TOLD",self.id,PlayerHeavy.SignalData) end
end

function PlayerHeavy:OnEndCommandSoundGroup(timerid)
	if AI then AI.Signal(SIGNALFILTER_GROUPONLY,1,"ON_COMMAND_TOLD",self.id,PlayerHeavy.SignalData) end
end

----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
-- FUNCTIONS TO BE REMOVED AFTER CXP GO HERE :)

function PlayerHeavy:IsSquadAlive()
	if not AI then return false end
	local count = AI.GetGroupCount(self.id);
	for i=1,count do
		local mate = AI.GetGroupMember(self.id,i);
		if(mate and mate ~=self and not mate:IsDead()) then
			return true;
		end
	end
	return false
end

function PlayerHeavy:DoFeedbackHit2DSounds(hit)
	GameAudio.JustPlaySignal(self.SoundSignals.FeedbackHit_Flesh);
end

function PlayerHeavy:IsUsable(user)
	return 1;
end

function PlayerHeavy:ShouldIgnoreHit(hit)

	if (hit.type ~= "collision") then
		return false;
	end

	if (hit.shooter and hit.shooter.IsOnVehicle and hit.shooter:IsOnVehicle()) then
		return false;
	end

	if (hit.shooter and hit.shooter.Properties and hit.shooter.Properties.bDamagesPlayerOnCollisionSP == 1) then
 		return false;
	end

	return true;
end


CreateActor(PlayerHeavy);
PlayerHeavy:Expose();
