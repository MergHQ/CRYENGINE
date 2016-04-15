----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2004.
----------------------------------------------------------------------------------------------------
--  Description: Common functions for actors (players, AIs..)
----------------------------------------------------------------------------------------------------
--  History:
--  20:10:2004 : Created by Filippo De Luca
----------------------------------------------------------------------------------------------------

--various constants
BLEED_TIMER = 1;
PAIN_TIMER = 2;
SWITCH_WEAPON_TIMER = 3;
BLOOD_POOL_TIMER = 4;
HEAVY_LAND_EXPLOSION = 5;


----------------------------------------------------------------------------------------------------
--BasicActor
BasicActor =
{
	AnimationGraph = "humanfullbody.xml",
	UpperBodyGraph = "humanupperbody.xml",

	Properties =
	{
		fileHitDeathReactionsParamsDataFile = "Libs/HitDeathReactionsData/HitDeathReactions_Default.xml",
		bEnableHitReaction = 1,

		soclasses_SmartObjectClass = "Actor",
		physicMassMult = 1.0,

		Damage =
		{
			bLogDamages = 0,
			health = 100,
		},

		Rendering =
		{
			bWrinkleMap = 0,
		},

		CharacterSounds =
		{
			footstepEffect = "footstep",			-- Footstep mfx library to use
			remoteFootstepEffect = "footstep",		-- Footstep mfx library to use for remote players
			bFootstepGearEffect = 1,				-- This plays a sound from materialfx
			footstepIndGearAudioSignal_Walk = "",	-- This directly plays the specified audiosignal on every footstep
			footstepIndGearAudioSignal_Run = "",	-- This directly plays the specified audiosignal on every footstep
			foleyEffect = "",						-- Foley signal effect name
		},

		equip_EquipmentPack="",
	},

	tempSetStats =
	{
	},

	Server = {},
	Client = {},

	grabParams =
	{
		collisionFlags = 0, 			-- Geom_colltype_player,
		holdPos = {x=0.0,y=0.4,z=1.25},	-- Position where grab is holded
		grabDelay = 0,					-- If IK is used, its the time delay where the limb tries to reach the object.
		followSpeed = 5.5,

		limbs =
		{
			"rightArm",
			"leftArm",
		},

		useIKRotation = 0,
	},

	IKLimbs =
	{
		{0,"rightArm","Bip01 R UpperArm","Bip01 R Forearm","Bip01 R Hand", IKLIMB_RIGHTHAND},
		{0,"leftArm","Bip01 L UpperArm","Bip01 L Forearm","Bip01 L Hand", IKLIMB_LEFTHAND},

		{5,"rightArmShadow","Bip01 R UpperArm","Bip01 R Forearm","Bip01 R Hand", IKLIMB_RIGHTHAND},
		{5,"leftArmShadow","Bip01 L UpperArm","Bip01 L Forearm","Bip01 L Hand", IKLIMB_LEFTHAND},
	},

	bloodFlowEffectWater = "",
	bloodFlowEffect = "",
	bloodSplatWall =
	{
		"materials/decals/blood/blood_splat1",
		"materials/decals/blood/blood_splat2",
		"materials/decals/blood/blood_splat3",
		"materials/decals/blood/blood_splat4",
		"materials/decals/blood/blood_splat5",
	},

	--[[ Uncomment to get wall alpha-animated blood decals when the actor is killed
	-- (instead of the non-animated bloodSplatWall decals)
	deathBloodSplatWall={
		"materials/decals/blood/human/blood_splatx1",
		"materials/decals/blood/human/blood_splatx2",
		"materials/decals/blood/human/blood_splatx3",
	},
	--]]

	-- Each element of this table below is a subtable in which the first element is the decal material.
	-- It can have a property named bAnimated that if true indicates the decal is alpha-animated.
	bloodSplatGround =
	{
		{"materials/decals/blood/blood_splatx1", bAnimated = true},
		{"materials/decals/blood/blood_splatx2", bAnimated = true},
		{"materials/decals/blood/blood_splatx3", bAnimated = true},
	},

	bloodSplatGroundDir = {x=0, y=0, z=-1},

	actorStats =
	{
	},

	particleEffects =
	{
		waterJumpSplash = "collisions.small.water",
	},
}


BasicActorParams =
{
	physicsParams =
	{
		flags = 0,
		mass = 110,
		mass_Player = 120,
		mass_Grunt = 180,

		stiffness_scale = 73,
		fallNPlayStiffness_scale = 100,

		Living =
		{
			gravity = 9.81,
			mass = 110,
			mass_Player = 120,
			mass_Grunt = 180,
			air_resistance = 0.2, -- Used in zeroG
			k_air_control = 0.9,
			inertia = 10.0, -- The more, the faster the speed change: 1 is very slow, 10 is very fast
			inertiaAccel = 11.0, -- Same as inertia, but used when the player accel
			max_vel_ground = 16,
			min_slide_angle = 45.0,
			max_climb_angle = 50.0,
			min_fall_angle = 50.0,
			timeImpulseRecover = 1.0,
			colliderMat = "mat_player_collider",
		},
	},

	gameParams =
	{
		autoAimTargetParams =
		{
			primaryTargetBone = BONE_SPINE,
			physicsTargetBone = BONE_SPINE,
			secondaryTargetBone = BONE_HEAD,
			fallbackOffset = 1.2,
			innerRadius = 0.4,
			outerRadius = 0.5,
			snapRadius = 2.0,
			snapRadiusTagged = 4.0,
		},
		sprintMultiplier = 1.5,		-- Speed is multiplied by this ammount if sprint key is pressed
		crouchMultiplier = 1.0,		-- Speed is multiplied by this ammount if crouching
		strafeMultiplier = 1.0,		-- Speed is multiplied by this ammount when strafing
		backwardMultiplier = 0.7,	-- Speed is multiplied by this ammount when going backward
		jumpHeight = 1.5, 			-- In meters
		meeleHitRagdollImpulseScale = 1.0,	-- Scales melee impulse force (when being hit)
		leanShift = 0.35,			-- How much the view shift on the side when leaning
		leanAngle = 15,				-- How much the view rotate when leaning
		lookFOV = 80,				-- Total FOV for looking (degrees)
		lookInVehicleFOV = 80,		-- Total FOV for looking in vehicles (degrees)
		aimFOV = 70,				-- Total FOV for aiming (degrees)
	},
}

function CreateDefaultStances(child)

	if (child) then
		if (not child.gameParams) then
			child.gameParams = {};
		end

		if (not child.gameParams.stance) then
			child.gameParams.stance =
			{
				{
					stanceId = STANCE_STAND,
					normalSpeed = 1.0,
					maxSpeed = 50.0,
					heightCollider = 1.2,
					heightPivot = 0.0,
					size = {x=0.4,y=0.4,z=0.2},
					modelOffset = {x=0,y=-0.0,z=0},
					viewOffset = {x=0,y=0.10,z=1.625},
					weaponOffset = {x=0.2,y=0.0,z=1.35},
					leanLeftViewOffset = {x=-0.5,y=0.10,z=1.525},
					leanRightViewOffset = {x=0.5,y=0.10,z=1.525},
					leanLeftWeaponOffset = {x=-0.45,y=0.0,z=1.30},
					leanRightWeaponOffset = {x=0.65,y=0.0,z=1.30},
					name = "stand",
					useCapsule = 1,
				},
				{
					stanceId = STANCE_STEALTH,
					normalSpeed = 0.6,
					maxSpeed = 3.0,
					heightCollider = 1.0,
					heightPivot = 0.0,
					size = {x=0.4,y=0.4,z=0.1},
					modelOffset = {x=0.0,y=-0.0,z=0},
					viewOffset = {x=0,y=0.3,z=1.35},
					weaponOffset = {x=0.2,y=0.0,z=1.1},
					name = "stealth",
					useCapsule = 1,
				},
				{
					stanceId = STANCE_CROUCH,
					normalSpeed = 0.5,
					maxSpeed = 3.0,
					heightCollider = 0.8,
					heightPivot = 0.0,
					size = {x=0.4,y=0.4,z=0.1},
					modelOffset = {x=0.0,y=0.0,z=0},
					viewOffset = {x=0,y=0.0,z=1.1},
					weaponOffset = {x=0.2,y=0.0,z=0.85},
					leanLeftViewOffset = {x=-0.55,y=0.0,z=0.95},
					leanRightViewOffset = {x=0.55,y=0.0,z=0.95},
					leanLeftWeaponOffset = {x=-0.5,y=0.0,z=0.65},
					leanRightWeaponOffset = {x=0.5,y=0.0,z=0.65},
					name = "crouch",
					useCapsule = 1,
				},
				{
					stanceId = STANCE_SWIM,
					normalSpeed = 1.0, -- this is not even used?
					maxSpeed = 2.5, -- this is ignored, overridden by pl_swim* cvars.
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
					heightCollider = 1.2,
					heightPivot = 0.0,
					size = {x=0.4,y=0.4,z=0.2},
					modelOffset = {x=0,y=0,z=0},
					viewOffset = {x=0,y=0.10,z=1.625},
					weaponOffset = {x=0.2,y=0.0,z=1.3},
					name = "relaxed",
					useCapsule = 1,
				},
			};
		end
	end
end

function CreateActor(child)
	mergef(child,BasicActorParams,1);
	mergef(child,BasicActor,1);

	CreateDefaultStances(child);
	SetupCollisionFiltering(child);
end

function BasicActor:CacheResources()
	-- Particles
	self:PreLoadParticleEffect( self.particleEffects.waterJumpSplash );
	self:PreLoadParticleEffect( self.bloodFlowEffectWater );
	self:PreLoadParticleEffect( self.bloodFlowEffect );

	-- Blood splats materials
	local bloodSplatWallCount = table.getn(self.bloodSplatWall)
	for i = 1, bloodSplatWallCount do
		Game.CacheResource("BasicActor.lua", self.bloodSplatWall[i], eGameCacheResourceType_Material, 0);
	end

	local bloodSplatGroundCount = table.getn(self.bloodSplatGround)
	for i = 1, bloodSplatGroundCount do
		Game.CacheResource("BasicActor.lua", self.bloodSplatGround[i][1], eGameCacheResourceType_Material, 0);
	end
end

function BasicActor:ResetCommon(bFromInit, bIsReload)

	-- Set health
	local health = self.Properties.Damage.health;

	if (self.actor) then
		if (self:IsDead() or System.IsEditor()) then
			-- Revive if dead
			self.actor:Revive();
		end
		self.actor:SetHealth(health);
		self.actor:SetMaxHealth(health);
		self.lastHealth = self.actor:GetHealth();
		self.actor:RegisterInAutoAimManager();
	end

	-- Hit reaction
	if (self.Properties.bEnableHitReaction and self.Properties.bEnableHitReaction == 0) then
		self.actor:DisableHitReaction();
	else
		self.actor:EnableHitReaction();
	end

	-- Sounds
	self:StopSounds();

	-- Drop grab, if present
	self:DropObject();

	self:RemoveDecals();

	self.lastDeathImpulse = 0;

	self:KillTimer(PAIN_TIMER);
	self:KillTimer(BLOOD_POOL_TIMER);
	self.painSoundTriggered = nil;

	if (self.lastSpawnPoint) then
		self.lastSpawnPoint = 0;
	end
	self.AI = {};
end

--------------------------------------------------------------------------
function BasicActor:Reset(bFromInit, bIsReload)

	BasicActor.ResetCommon(self, bFromInit, bIsReload);

	-- Misc resetting
	if (self.actor) then
		self.actor:SetMovementTarget(g_Vectors.v000,g_Vectors.v000,g_Vectors.v000,1);
	end
	self.lastVehicleId = nil;
	self.AI.theVehicle = nil;

	self:ResetBleeding();

	if (bFromInit and CryAction.IsServer()) then
		if (g_gameRules and g_gameRules.EquipActor) then
			g_gameRules:EquipActor(self);
		end
	end

end

function BasicActor:ResetLoad()
end

function BasicActor:InitIKLimbs()
	if (self.IKLimbs) then
		for i,limb in pairs(self.IKLimbs) do
			self.actor:CreateIKLimb(limb[1],limb[2],limb[3],limb[4],limb[5],limb[6] or 0);
		end
	end
end

function BasicActor:ShutDown()
	self:ResetAttachment(0, "weapon");
	self:ResetAttachment(0, "right_item_attachment");
	self:ResetAttachment(0, "left_item_attachment");
	self:ResetAttachment(0, "laser_attachment");
	self.actor:ShutDown();
end

function BasicActor:GetBloodFlowPosition()
	return self:GetBonePos(self:GetBloodFlowBone(), g_Vectors.temp_v1);
end

function BasicActor:GetBloodFlowBone()
	return "Bip01 Spine2";
end

function BasicActor:WallBloodSplat(hit, killingHit)
	-- ToDo:
	-- * Move to c++ (optionally)
	-- * Expose magic numbers here somewhere (distances, lifetime, max/min sizes, etc.)
	-- * Use deferred raycasts
	if ((killingHit and self.deathBloodSplatWall) or self.bloodSplatWall) then
		local dist = 2.5;
		local dir = vecScale(hit.dir, dist);
		local hits = Physics.RayWorldIntersection(hit.pos,dir,1,ent_all,hit.targetId,nil,g_HitTable);

		local splat = g_HitTable[1];
		if ((hits > 0) and splat and ((splat.dist or 0) > 0.5)) then -- Why are we removing hits closer than 0.5 meter?
			local bloodSplatWallMats = self.bloodSplatWall;
			local growTimeAlpha = nil;
			local angle = math.random() * 360;

			-- Death blood splat walls are only allowed in vertical surfaces, so normal angle with the horizontal is not allowed to be greater than 20º
			if (killingHit and self.deathBloodSplatWall and (splat.normal.z < 0.342)) then
				bloodSplatWallMats = self.deathBloodSplatWall;
				growTimeAlpha = 4.0;
				angle = nil;
			end

			if (bloodSplatWallMats) then
				local n = table.getn(bloodSplatWallMats);
				local i = math.random(1,n);
				local s = 0.25+(splat.dist/dist)*0.35; -- Scale from 0.32 (since splat.dist is always greater than 0.5) to 0.6 depending on distance from hit point
				Particle.CreateMatDecal(splat.pos, splat.normal, s, 300, bloodSplatWallMats[i], angle, hit.dir, ((not splat.renderNode) and (splat.entity and splat.entity.id)) or nil, splat.renderNode, nil, growTimeAlpha);
			end
		end
	end
end

function BasicActor:DoBloodPool()

	-- If we are still moving, let's not do a bloodpool, yet
	self:GetVelocity(g_Vectors.temp_v1);

	if (LengthSqVector(g_Vectors.temp_v1) > 0.2) then
		self:SetTimer(BLOOD_POOL_TIMER,1000);
		return;
	end
	self:KillTimer(BLOOD_POOL_TIMER);

	local dist = 2.5;
	local pos = self:GetBloodFlowPosition();

	if(pos == nil) then
		Log("Blood pool failed to be created, invalid spawn position "..self:GetName());
		return;
	end

	if (self.bloodSplatGround) then
		pos.z = pos.z + 1;
		local dir = vecScale(self.bloodSplatGroundDir, dist);
		local hits = Physics.RayWorldIntersection(pos,dir,1,ent_terrain+ent_static, nil, nil, g_HitTable);

		local splat = g_HitTable[1];
		if (hits > 0 and splat) then
			local n = table.getn(self.bloodSplatGround);
			local i = math.random(1,n);
			local s = 0.6;
			local decalInfo = self.bloodSplatGround[i];
			local decalMat = decalInfo[1];
			local growTimeAlpha = (decalInfo.bAnimated and 6.0) or nil;
			local growScale = (not decalInfo.bAnimated and 6.0) or nil;
			Particle.CreateMatDecal(splat.pos, splat.normal, s, 300, decalMat, math.random()*360, vecNormalize(dir), splat.entity and splat.entity.id, splat.renderNode, growScale, growTimeAlpha, true);
		end
	end
end

function BasicActor:StartBleeding()
	self:DestroyAttachment(0, "wound");
	self:CreateBoneAttachment(0, self:GetBloodFlowBone(), "wound", false);

	local effect=self.bloodFlowEffect;
	local pos = self:GetBloodFlowPosition();

	if (not pos) then
		return;
	end

	local level,normal,flow=CryAction.GetWaterInfo(pos);
	if (level and level>=pos.z) then
		effect=self.bloodFlowEffectWater;
	end

	self.bleeding = true;
	self:SetAttachmentEffect(0, "wound", effect, g_Vectors.v000, g_Vectors.v010, 1, 0);
end

function BasicActor:StopBleeding()
	if (self.bleeding == true) then
		self:DestroyAttachment(0, "wound");
	end
	self.bleeding=false;
end

function BasicActor:IsBleeding()
	return self.bleeding;
end

function BasicActor:ResetBleeding()
	self.bleeding = false;
	self:StopBleeding();
	self:KillTimer(BLEED_TIMER);
end

function BasicActor:HealthChanged()

	local health = self.actor:GetHealth();
	local damage = self.lastHealth - health;

	if (self.Properties.Damage.bLogDamages ~= 0) then
		Log(self:GetName().." health:"..health.." (last damage:"..damage..")");
	end

	self.lastHealth = health;
end


function BasicActor.Server:OnDeadHit(hit)
	--Log("BasicActor.Server:OnDeadHit()");

	local frameID = System.GetFrameID();
	if ((frameID - self.lastDeathImpulse) > 10) then
		local dir = g_Vectors.temp_v2;
		CopyVector(dir, hit.dir);
		dir.z = dir.z + 1;

		local angAxis = g_Vectors.temp_v3;
		angAxis.x = math.random() * 2 - 1;
		angAxis.y = math.random() * 2 - 1;
		angAxis.z = math.random() * 2 - 1;

		local imp1 = math.random() * 20 + 10;
		local imp2 = math.random() * 20 + 10;
		--System.Log("DeadHit linear: "..string.format(" %.03f,%.03f,%.03f",dir.x,dir.y,dir.z).." -> "..tostring(imp1));
		--System.Log("DeadHit angular: "..string.format(" %.03f,%.03f,%.03f",angAxis.x,angAxis.y,angAxis.z).." -> "..tostring(imp2));

		if(hit.type ~= "silentMelee" and (not g_gameRules:IsStealthHealthHit(hit.type))) then
			self:AddImpulse( hit.partId, hit.pos, dir, imp1, 1, angAxis, imp2, 8);
			self.lastDeathImpulse = frameID;
		end
	end
end

function BasicActor:HasSilencedWeapon(weaponId)
	-- Check if the weapon is silenced
	local item = System.GetEntity(weaponId)
	if(item) then
		if(item.weapon and (item.weapon:GetAccessory("Silencer") or item.weapon:GetAccessory("SilencerPistol")))  then
			return true;
		end
	end
	return false
end

function BasicActor.Server:OnHit(hit)
	if (self.actor:GetSpectatorMode()~=0) then
		return false;
	end

	local isMultiplayer = System.IsMultiplayer();

	if(not isMultiplayer) then
		-- ShouldIgnoreHit can be defined on the entity to handle special cases where the hits should be ignored.
		if(self.ShouldIgnoreHit and self:ShouldIgnoreHit(hit)) then
			return false
		end

		--if (shooter and (self == g_localActor)) then
		--	if (AI.GetReactionOf(self.id, shooter.id) ~= Friendly) then
		--		g_SignalData.id = shooterId;
		--		g_SignalData.fValue = 0;
		--		g_SignalData.iValue = LAS_DEFAULT;
		--		shooter:GetWorldPos(g_SignalData.point);
		--
		--		if AI then
		--			AI.Signal(SIGNALFILTER_SUPERGROUP, 1, "OnPlayerHit", self.id, g_SignalData);
		--		end
		--	end
		--end

		-- Check if the weapon is silenced
		local isSilenced = self:HasSilencedWeapon(hit.weaponId);

		-- Request pain sounds here, assume communication manager will handle throttling them.
		if (hit.damage > 5) then
			if (self.actor:GetHealth() > 0) then
				self:DoPainSounds(false, hit.type, isSilenced);
			end
		end
	end

	local isPlayer = self.actor:IsPlayer();

	if (isPlayer and g_gameRules and ((not (hit.shooterId == nil) and hit.damage>=1) or (hit.damage > 3.0) or (isMultiplayer and (hit.shooterId ~= nil)))) then
		g_gameRules.game:SendDamageIndicator(hit.targetId, hit.shooterId or NULL_ENTITY, hit.weaponId or NULL_ENTITY, hit.dir, hit.damage, hit.projectileClassId or -1, hit.typeId);
	end

	-- Log("BasicActor.Server:OnHit (%s)", self:GetName());

	if (self:IsOnVehicle() and hit.type~="heal") then
		local vehicle = System.GetEntity(self.actor:GetLinkedVehicleId());
		if (vehicle and vehicle.vehicle) then
			local newDamage = vehicle.vehicle:ProcessPassengerDamage(self.id, self.actor:GetHealth(), hit.damage, hit.typeId, hit.explosion or false);
			if (newDamage <= 0.0) then
				return false;
			end
		end
	end

	-- Log("OnHit >>>>>>>>> "..self:GetName().."   damage: "..hit.damage);

	local died = g_gameRules:ProcessActorDamage(hit);

	-- Need to return a value, or the hit wont cause any reaction. Sometimes, hits with damage=0 need to cause reactions (throwing an AI into another AI, for example)
	if(hit.damage==0) then
		return died;
	end

	if (hit.type=="heal") then return end

	-- Some AI related
	if (not isPlayer) then
		local theShooter=hit.shooter;
		-- upade hide-in-vehicle
		if (theShooter and theShooter.IsOnVehicle) then
			local shootersVehicleId = theShooter:IsOnVehicle();
			if (shootersVehicleId) then
				local shootersVehicle = System.GetEntity(shootersVehicleId);
				if(shootersVehicle and shootersVehicle.ChangeFaction and (shootersVehicle.AI==nil or shootersVehicle.AI.hostileSet~=1)) then
					shootersVehicle:ChangeFaction(theShooter, 2);
				end
			end
		end

		if (hit.type and hit.type ~= "collision" and hit.type ~= "fall" and hit.type ~= "event") then
			if (theShooter) then
				CopyVector(g_SignalData.point, theShooter:GetWorldPos());
				g_SignalData.id = hit.shooterId;
			else
				g_SignalData.id = NULL_ENTITY;
				CopyVector(g_SignalData.point, g_Vectors.v000);
			end

			g_SignalData.fValue = hit.damage;
			if AI then
				if (theShooter and AI.Hostile(self.id,hit.shooterId)) then
					AI.Signal(SIGNALFILTER_SENDER,0,"OnEnemyDamage",self.id,g_SignalData);
					AI.UpTargetPriority(self.id, hit.shooterId, 0.2); -- Make the target more important
					-- Check for greeting player in case of "nice shot"
					if(died and theShooter == g_localActor) then
						local ratio = self.lastHealth / self.Properties.Damage.health;
						if( ratio> 0.9 and hit.material_type and hit.material_type=="head" and hit.type and hit.type=="bullet") then
							AI.Signal(SIGNALFILTER_GROUPONLY,0,"OnPlayerNiceShot",g_localActor.id);
						end
					end
--			elseif(hit.shooter and hit.shooter==g_localActor and self.Properties.species==hit.shooter.Properties.species) then
--				AI.Signal(SIGNALFILTER_SENDER,0,"OnFriendlyDamageByPlayer",self.id,g_SignalData);
				elseif (theShooter ~= nil and theShooter~=self) then
					if(hit.weapon and hit.weapon.vehicle) then
						AI.Signal(SIGNALFILTER_SENDER,0,"OnDamage",self.id,g_SignalData);
					else
						AI.Signal(SIGNALFILTER_SENDER,0,"OnFriendlyDamage",self.id,g_SignalData);
					end
				else
					AI.Signal(SIGNALFILTER_SENDER,0,"OnDamage",self.id,g_SignalData);
				end
			end
		end

		if (self.RushTactic) then
			self:RushTactic(5);
		end

	-- elseif (not died) then
	-- Log("Player Damage: damage="..hit.damage.." t=".._time);
		-- design request : decouple energy from health
		--self.actor:CreateCodeEvent({event = "addSuitEnergy",amount=-hit.damage*0.2});
	end

	-- AI.DebugReportHitDamage(self.id, hit.shooter.id, hit.damage, hit.material_type);

	-- This is for debugging AI accurracy
	-- self.actor:CreateCodeEvent({event = "aiHitDebug",shooterId=hit.shooterId});

	self:HealthChanged();
	return died;
end

function BasicActor:KnockedOutByDoor(hit,mass,vel)
	if (self == g_localActor or (AI.GetReactionOf(self.id, g_localActorId) == Friendly) or self:IsDead()) then return end;
	local force=clamp((mass * vel) * 0.02, 0, 100);

	if(force>3)then
		-- REINSTANTIATE!!!
		-- self:PlaySoundEvent("sounds/physics:destructibles:wood_shatter",{x=0,y=0,z=0.6},g_Vectors.v010, SOUND_DEFAULT_3D, 0, SOUND_SEMANTIC_PLAYER_FOLEY);
		local hit = {};
		self:Kill(hit);
	end;
end;

function BasicActor:TurnRagdoll()
	self.actor:SetPhysicalizationProfile("ragdoll");
end

function BasicActor:Kill(hit)
	if (hit == nil) then
		Log("Error: BasicActor:Kill - Parameter 'hit' is nil")
		return
	end

	local shooterId = hit.shooterId
	local weaponId = hit.weaponId

	if AI then
		AI.LogEvent("BasicActor:ClientKill( "..tostring(shooterId)..", "..tostring(weaponId));
	end

	if (self.actor:GetHealth() > 0) then
		self.actor:SetHealth(0);
	end

	if (self.SharedSoundSignals) then
		if (hit.explosion) then
			GameAudio.JustPlayPosSignal( hit.target.SharedSoundSignals.KilledByExplosion, hit.target:GetPos() ); -- cant do a normal entityplay because all entity linked sounds are stoped when the entity is "gibbed"
		elseif (hit.type == "kvolt") then
			GameAudio.JustPlayPosSignal( hit.target.SharedSoundSignals.KilledByKvolt, hit.target:GetPos() ); -- cant do a normal entityplay because all entity linked sounds are stoped when the entity is "gibbed"
		elseif(not g_gameRules:IsStealthHealthHit(hit.type)) then
			if (hit.shooterId==g_localActorId) then
				GameAudio.JustPlayEntitySignal( hit.target.SharedSoundSignals.Feedback_KilledByPlayer, hit.targetId );
			end
		end
	end

	if(self.OnCustomKill) then
		self:OnCustomKill(hit);
	end

	-- Check if the weapon is silenced
	local isSilenced = self:HasSilencedWeapon(hit.weaponId);

	self:DoPainSounds(1, hit.type, isSilenced);
	self:KillTimer(PAIN_TIMER);

	self:DropObject();
	local shooter = shooterId and System.GetEntity(shooterId);

	if(shooter and hit.projectileClassId > 0) then
		self.deathProjectileClassId = hit.projectileClassId
		self.deathProjectileClass = hit.projectileClass
	end

	-- Additional signals for player and squad mates
	if (self == g_localActor) then
		if AI then
			AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT, 1, "OnPlayerDied", self.id);
		end
	elseif ((AI.GetReactionOf(self.id, g_localActorId) == Friendly)) then

		-- Make player hostile to squadmates, if he kills one of them
		if(shooter and shooter==g_localActor ) then
			g_SignalData.id = shooter.id;
			AI.Signal(SIGNALFILTER_LEADER,10,"OnUnitBusy",self.id);
			AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT, 1, "OnPlayerTeamKill", self.id,g_SignalData);
		else
			AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT, 1, "OnSquadmateDied", self.id);
		end
	--		elseif (hit.shooter and hit.shooter.Properties and hit.shooter.Properties.species ~= 0) then
	--			AI.Signal(SIGNALFILTER_SENDER, 1, "OnEnemyDied", hit.shooter.id);
	end

	-- When a driver AI is dead, something will happen to his vehicle depending on the situation.
	if ( g_gameRules.game:IsMultiplayer() == false ) then
		if ( self.actor and not self.actor:IsPlayer() ) then
			local vd = self.actor:GetLinkedVehicleId();
			if ( vd ) then
				local ve = System.GetEntity( vd );
				if ( ve ) then
					if ( ve.OnPassengerDead ) then
						ve:OnPassengerDead(self);
					end
				end
			end
		end
	end
	if AI then

		if(self.AI.usingMountedWeapon) then
			local weaponId = self.AI.currentMountedWeaponId;
			if( weaponId ) then
				local weapon = System.GetEntity(weaponId);
				if (weapon and weapon.class == "HMG") then
					AI.Signal(SIGNALFILTER_GROUPONLY, 30, "OnGroupMemberDiedOnHMG",self.id);
				elseif (weapon and weapon.class == "AGL") then
					AI.Signal(SIGNALFILTER_GROUPONLY, 30, "OnGroupMemberDiedOnAGL",self.id);
				end
			end
			AI.Signal(SIGNALFILTER_GROUPONLY, 30, "OnGroupMemberDiedOnMountedWeapon",self.id);
		end

		-- Notify CLeader about this
		AI.Signal(SIGNALFILTER_LEADER, 10, "OnUnitDied", self.id);
		AI.Signal(SIGNALFILTER_LEADERENTITY, 10, "OnUnitDied", self.id);
		-- Notify group about this
		g_SignalData.id = self.id;
		CopyVector(g_SignalData.point,self:GetPos());

		if (AI.GetGroupCount(self.id) > 1) then
			-- Notify the closest looking at me dude
			AI.ChangeParameter(self.id,AIPARAM_COMMRANGE,100);
			-- AI.Signal(SIGNALFILTER_NEARESTINCOMM_LOOKING, 10, "OnBodyFallSound",self.id, g_SignalData);
			-- the closest guy withing 10m will "hear" body falling sound
			AI.ChangeParameter(self.id,AIPARAM_COMMRANGE,10);
			AI.Signal(SIGNALFILTER_NEARESTINCOMM, 10, "OnBodyFallSound",self.id, g_SignalData);

		else
			-- Tell anyone that you have been killed, even outside your group
			AI.Signal(SIGNALFILTER_ANYONEINCOMM, 10, "OnSomebodyDied",self.id);
		end

		if(shooter) then
			AI.LogEvent("Shooter position:"..Vec2Str(shooter:GetWorldPos()));
			AI.SetRefPointPosition(self.id,shooter:GetWorldPos());
			AI.SetBeaconPosition(self.id, shooter:GetWorldPos());
		end

		-- Call the destructor directly, since the following AIEVENT_TARGETDEAD will prevent the AI updating further (and kill all signals too)
		if (self.Behaviour and self.Behaviour.Destructor) then
			AI.LogEvent("Calling Destructor for "..self:GetName().." on Kill.");
			self.Behaviour:Destructor(self);
		end
	end
	-- Notify AI system about this
	self:TriggerEvent(AIEVENT_AGENTDIED, shooterId or NULL_ENTITY)
	if (BasicAI) then
		Script.SetTimerForFunction( 1000 , "BasicAI.OnDeath", self );
	end;

	if ((not self.actor:IsPlayer()) and (not self:IsOnVehicle())) then
		local pos = self:GetPos();
		local level,normal,flow=CryAction.GetWaterInfo(pos);

		if (level and (level-0.8) < pos.z) then
			self:SetTimer(BLOOD_POOL_TIMER,2000);
		end
	end

	NotifyDeathToTerritoryAndWave(self)

	if (self.OnDisabled) then
		self:OnDisabled()
	end

	return true
end


function BasicActor.Client:OnHit(hit)

	local shooter = hit.shooter;

	-- Hit effects
	if (hit.radius == 0) then		-- Do hit feedback for all bullet types, but not for explosions
		if (not self:IsBleeding()) then
			self:SetTimer(BLEED_TIMER, 0);
		end

		if ( (self.DoFeedbackHit2DSounds ~= nil) and self:ShouldGiveLocalPlayerHitFeedback2DSounds(hit) ) then
			self:DoFeedbackHit2DSounds(hit)
		end

		if (g_gameRules.game:IsMultiplayer()) then
			self:WallBloodSplat(hit);
		end
	end

	if (self.actor:GetHealth() <= 0) then
		return;
	end

	if (shooter and (self == g_localActor)) then
		if (AI.GetReactionOf(self.id, shooter.id) ~= Friendly) then
			g_SignalData.id = shooterId;
			g_SignalData.fValue = 0;
			g_SignalData.iValue = LAS_DEFAULT;
			shooter:GetWorldPos(g_SignalData.point);

			if AI then
				AI.Signal(SIGNALFILTER_SUPERGROUP, 1, "OnPlayerHit", self.id, g_SignalData);
			end
		end
	end

end


-- Query if we should give any feedback on a hit (2D sound).
--
-- In:      The hit information.
--
-- Returns: True if feedback should be given; otherwise false.
--
function BasicActor:ShouldGiveLocalPlayerHitFeedback2DSounds(hit)
	if (hit.shooter == nil) then
		return false
	end

	if (hit.shooter ~= g_localActor) then
		return false
	end

	if (hit.target.actor:GetHealth() <= 0.0) then
		return false
	end

	if (g_gameRules:IsStealthHealthHit(hit.type)) then
		return false
	end

	return g_gameRules.game:ShouldGiveLocalPlayerHitFeedback2DSound(hit.damage)
end


function BasicActor:StopSounds()
	self.lastPainSound = nil;
	self.lastPainTime = 0;
end

function BasicActor:DoPainSounds(dead)
	-- function body removed in conjunction with "VoiceType" removal
	-- overridden in Human_x.lua anyway
end

--	NOTE: This function may not be called if 'g_setActorModelFromLua' is set to '0'
--	In this case, an internal function in CActor is used. If you modify anything with this function, be sure to update CActor::SetActorModelInternal as well.
function BasicActor:SetActorModel(isClient)
	local hasChanged = false;
	local PropInstance = self.PropertiesInstance;
	local model = self.Properties.fileModel;

	-- Take care of fp3p
	if (self.Properties.clientFileModel and isClient) then
		model = self.Properties.clientFileModel;
	end

	local nModelVariations = self.Properties.nModelVariations;
	if (nModelVariations and nModelVariations > 0 and PropInstance and PropInstance.nVariation) then
		local nModelIndex = PropInstance.nVariation;
		if (nModelIndex < 1) then
			nModelIndex = 1;
		end
		if (nModelIndex > nModelVariations) then
			nModelIndex = nModelVariations;
		end
		local sVariation = string.format('%.2d',nModelIndex);
		model = string.gsub(model, "_%d%d", "_"..sVariation);
		-- System.Log( "ActorModel = "..model );
	end

	if (self.currModel ~= model) then
		hasChanged = true;
		self.currModel = model;
		self:LoadCharacter(0, model);
		self:InitIKLimbs(); -- Set IK limbs
		self:ForceCharacterUpdate(0, false);
		self:CreateBoneAttachment(0, "weapon_bone", "right_item_attachment");
		self:CreateBoneAttachment(0, "weapon_bone", "weapon");
		self:CreateBoneAttachment(0, "alt_weapon_bone01", "left_item_attachment");
		self:CreateBoneAttachment(0, "weapon_bone", "laser_attachment"); -- Laser bone (need it for updating character correctly when out of camera view)

		if(self.CreateAttachments) then
			self:CreateAttachments();
		end

		-- Need to reselect current item if we have one, as it will no longer be attached
		if (self.inventory) then
			local item = self.inventory:GetCurrentItemId();
			if (item ~= nil) then
				self.actor:SelectItem(item, true);
			end
		end
	end

	-- Take care of shadow character
	local shadowModel = self.Properties.shadowFileModel;
	if (shadowModel and isClient and self.currShadowModel ~= shadowModel) then
		self.currShadowModel = shadowModel;
		self:LoadCharacter(5, self.Properties.shadowFileModel);
	end

	return hasChanged;
end

function BasicActor:OnNextSpawnPoint()
	local entities = System.GetEntitiesByClass( "SpawnPoint" )

	local n = table.getn(entities)
	table.sort( entities, CompareEntitiesByName )

	local nextSpawnPoint = n
	if self.lastSpawnPoint then
		for i = 1, n do
			if self.lastSpawnPoint == entities[i]:GetName() then
				nextSpawnPoint = i
			end
		end
	end
	nextSpawnPoint = nextSpawnPoint + 1
	if nextSpawnPoint > n then
		nextSpawnPoint = 1
	end

	local spawnPoint = entities[nextSpawnPoint]
	self:InternalSpawnAtSpawnPoint(spawnPoint)
end

function BasicActor:InternalSpawnAtSpawnPoint(spawnPoint)
	if (spawnPoint) then
		self.lastSpawnPoint = spawnPoint:GetName()
			if AI then
				AI.LogEvent( "Teleport to "..self.lastSpawnPoint )
			end
		self:SetWorldPos(spawnPoint:GetWorldPos(g_Vectors.temp_v1));
		spawnPoint:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.x = 0;
		g_Vectors.temp_v1.y = 0;
		self.actor:PlayerSetViewAngles(g_Vectors.temp_v1);
		spawnPoint:Spawned(self);
	end
end

function BasicActor:CanGrabObject(object)
	return 0;
end

function BasicActor:GrabObject(object, query)
	if (query and self.actor:IsPlayer()) then
		return 0;
	end

	self.grabParams.entityId = object.id;
	local grabParams = new(self.grabParams);
	grabParams.event = "grabObject";

	if (self.actor:CreateCodeEvent(grabParams)) then
		return 1;
	end

	return 0;
end

function BasicActor:GetGrabbedObject()
    return (self.grabParams.entityId or 0);
end

function BasicActor:DropObject(throw,throwVec,throwDelay)

	local dropTable =
	{
		event = "dropObject",
		throwVec = {x=0,y=0,z=0},
		throwDelay = throwDelay or 0,
	};

	if (throwVec) then
		CopyVector(dropTable.throwVec,throwVec);
	end

	if (self.actor) then
		self.actor:CreateCodeEvent(dropTable);
	end
end

function BasicActor:IsDead()
	local result = true;

	if (self.actor) then
		local health = self.actor:GetHealth();
		if (health) then
			result = (health <= 0);
		end
	end

	return result;
end

function BasicActor:OnSave(save)

end

function BasicActor:OnLoad(saved)

end

function BasicActor:OnPostLoad()
	self.lastHealth = self.actor:GetHealth();
	self.lastDeathImpulse = 0;

	if self.AssignPrimaryWeapon then
		self:AssignPrimaryWeapon();
	end
end

function BasicActor:OnResetLoad()
	self.actor:SetPhysicalizationProfile("alive");
end

function BasicActor:OnSpawn(bIsReload)
	-- System.Log("BasicActor:OnSpawn()"..self:GetName());

	-- Make sure to get a new table
	self.grabParams = new(self.grabParams);
	self.waterStats = new(self.waterStats);

	-- If AI tables were included, propagate call down the chain
	-- Note on InitialSetup:
	-- InitialSetup calls Reset(), which will happen right after for AI, during the OnInit callback
	-- this way we make sure Reset() is only called once during initialization of the entity (reloaded or not)
	-- If not we will end up with two calls, which can re-create the inventory twice and what not (expensive!)
	-- For Players needs to figure out if it is required to call InitialSetup (and Reset) at this point
	if (self.ai) then
		BasicAI.OnSpawn(self,bIsReload);
	else
		self:InitialSetup(bIsReload);
	end

	ApplyCollisionFiltering(self, GetCollisionFiltering(self));
	
	-- Create the Dynamic Response System proxy used by the dialog system
	self:CreateDRSProxy();

end

-- Call some initial code for actor spawn and respawn
function BasicActor:InitialSetup(bIsReload)
	BasicActor.Reset(self, bIsReload);
end

function BasicActor:ScriptEvent(event,value,str)
	if (event == "kill") then
		local hit = {};
		self:Kill(hit);
	elseif ((event == "landed") or (event == "heavylanded")) then
		if (CryAction.IsServer()) then
			local minspeed=8;
			if(value>minspeed) then
				self:DoPainSounds(false);
			end

			-- Generate a small explosion to damage nearby entities
			if (event == "heavylanded" and (not g_gameRules.game:IsMultiplayer())) then
				self:SetTimer(HEAVY_LAND_EXPLOSION, 300.0);
			end
		end
	elseif (event == "jump_splash") then
		local ppos = g_Vectors.temp_v1;
		self:GetWorldPos(ppos);
		ppos.z = ppos.z + value;

		Particle.SpawnEffect(self.particleEffects.waterJumpSplash, ppos, g_Vectors.v001, 1.0);
		if(not str) then
			local volume = 1.0;

			if(self.actor:IsLocalClient()) then
				-- REINSTANTIATE!!!
				-- local sound = self:PlaySoundEventEx("sounds/physics:foleys/npc:bodyfall_water_shallow", SOUND_DEFAULT_3D, 0, volume, g_Vectors.v000, -1, -1, SOUND_SEMANTIC_PLAYER_FOLEY);
			end
		end
	elseif (event == "sleep") then
		-- System.Log(">>>>>>> BasicActor:ScriptEvent:Event_Sleep "..self:GetName());
		-- Don't call event_Sleep - it's not inpue-event, not handled with MakeSpawnable
		if(not self.isFallen) then
			BroadcastEvent(self, "Sleep");
		end
		self.isFallen = 1;
	end
end

function BasicActor.Client:OnAnimationEvent(type, eventData)

	-- Function callback, if any
	local onAnimationKey = self.onAnimationKey;

	if (onAnimationKey) then
		local func = onAnimationKey[strPar];
		if (func) then
			func(self, type, eventData);
		end
	end
end

function BasicActor.Client:OnTimer(timerId,mSec)
	if (timerId == BLEED_TIMER) then
		if (self:IsBleeding()) then
			self:StopBleeding();
		elseif (self.actor:GetHealth()<=0) then
			self:StartBleeding();
		end
	elseif (timerId == PAIN_TIMER) then
		if (self.actor:GetHealth() > 0) then
			self:DoPainSounds();
		end
		self.painSoundTriggered = nil;
	elseif (timerId == BLOOD_POOL_TIMER) then
	  self:DoBloodPool();
	elseif (timerId == HEAVY_LAND_EXPLOSION) then
		g_gameRules:CreateExplosion(self.id, self.id, 100, self:GetPos(), nil, 3.0, nil, 500, nil, nil, 1.0, 3.0, nil, nil, g_gameRules.game:GetHitTypeId("collision"));
	end
end


function BasicActor:DropItem()
	local item = self.inventory:GetCurrentItemId();
	if (item ~= nil) then
		self.actor:DropItem(item);
	end
end

function BasicActor:AIWaveAllowsRemoval()
	return WaveAllowActorRemoval(self);
end

function BasicActor:IsOnVehicle()
	if (self.actor) then
		return self.actor:GetLinkedVehicleId();
	else
		return false;
	end
end


-- TODO: Move to C++
function BasicActor:OnStanceChanged(newStance, oldStance)
	local stanceChangeSound = "";
	local stanceIsCrouchish = (newStance == STANCE_CROUCH or newStance == STANCE_STEALTH);

	if (stanceIsCrouchish) then
		stanceChangeSound = "sounds/physics:foleys/player:crouch_on";
	else
		stanceChangeSound = "sounds/physics:foleys/player:crouch_off";
	end
	
	-- REINSTANTIATE!!!
	-- self:PlaySoundEvent(stanceChangeSound, g_Vectors.v000, g_Vectors.v010, SOUND_DEFAULT_3D, 0, SOUND_SEMANTIC_PLAYER_FOLEY);
end