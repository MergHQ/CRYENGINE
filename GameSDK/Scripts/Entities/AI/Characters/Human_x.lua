Script.ReloadScript( "SCRIPTS/Entities/AI/Shared/AIBase.lua");

Human_x =
{
	--AnimationGraph = "humanfullbody.xml",
	--UpperBodyGraph = "humanupperbody.xml",

	ActionController = "Animations/Mannequin/ADB/humanControllerDefs.xml",
	AnimDatabase3P = "Animations/Mannequin/ADB/human.adb",
	SoundDatabase = "Animations/Mannequin/ADB/humanSounds.adb",

	UseMannequinAGState = true,
	UseLegacyCoverLocator = false,

	gameParams =
	{
		boneIDs =
		{
			BONE_BIP01 = "Bip01",
			BONE_SPINE = "Bip01 Spine1",
			BONE_SPINE2 = "Bip01 Spine2",
			BONE_SPINE3 = "Bip01 Spine3",
			BONE_HEAD = "Bip01 Head",
			BONE_WEAPON = "weapon_bone",
			BONE_WEAPON2 = "Lweapon_bone",
			BONE_FOOT_R = "Bip01 R Foot",
			BONE_FOOT_L = "Bip01 L Foot",
			BONE_ARM_R = "Bip01 R Forearm",
			BONE_ARM_L = "Bip01 L Forearm",
			BONE_CALF_R = "Bip01 R Calf",
			BONE_CALF_L = "Bip01 L Calf",
		},

		meeleHitRagdollImpulseScale = 1.0, -- Scales melee impulse force (when being hit)
		grenadeLaunchProbability = 0.5,
		grenadeThrowMaxAngle =  45,

		lookFOV = 200, -- Total FOV for looking (degrees)
		lookInVehicleFOV = 110, -- Total FOV for looking in vehicles (degrees)
		aimFOV = 200, -- Total FOV for aiming  (degrees)
		maxLookAimAngle = 120, -- Maximum angle between aim and look direction (degrees)
		aimIKFadeDuration = 0.5, -- time to reach aim pose
		aimIKLayer = 14,

		proceduralLeaningFactor = 0.0, -- disable procedural leaning by default for all humans (infected don't need it, and the others use turn assets)

		canUseComplexLookIK = true,
		lookAtSimpleHeadBone = "Bip01 Head",

		cornerSmoother = 2, -- 1 = C2 method; 2 = C3 method

		stepThresholdTime = 0.5, -- Duration (seconds) the current position deviation needs to be above stepThresholdDistance before the character steps
		stepThresholdDistance = 0.1, -- Distance (meters) that the character has to deviate from the entity before the step timer (stepThresholdTime) steps

		turnThresholdTime = 0.5, -- Duration (seconds) the current angle deviation needs to be above turnThresholdAngle before the character turns
		turnThresholdAngle = 5,  -- Angle (degrees) that the character has to deviate from the entity before the turn timer (turnThresholdTime) turns

		maxDeltaAngleRateNormal = 360, -- Maximum turnspeed (degrees/second)

		nearbyRange = 10, -- Range used to consider a target as nearby
		middleRange = 30, -- Range used to consider a target as in middle distance

		defaultStance = STANCE_RELAXED,

		stance =
		{
			{
				stanceId = STANCE_STAND,
				normalSpeed = 1.0,
				maxSpeed = 50.0,
				heightCollider = 1.2,
				heightPivot = 0.0,
				size = {x=0.5,y=0.5,z=0.2},
				modelOffset = {x=0,y=-0.0,z=0},
				viewOffset = {x=0,y=0.10,z=1.625},
				weaponOffset = {x=0.2,y=0.0,z=1.35},
				leanLeftViewOffset = {x=-0.5,y=0.10,z=1.525},
				leanRightViewOffset = {x=0.5,y=0.10,z=1.525},
				leanLeftWeaponOffset = {x=-0.45,y=0.0,z=1.30},
				leanRightWeaponOffset = {x=0.65,y=0.0,z=1.30},
				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},
				name = "stand",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_ALERTED,
				normalSpeed = 1.0,
				maxSpeed = 50.0,
				heightCollider = 1.2,
				heightPivot = 0.0,
				size = {x=0.5,y=0.5,z=0.2},
				modelOffset = {x=0,y=-0.0,z=0},
				viewOffset = {x=0,y=0.10,z=1.625},
				weaponOffset = {x=0.2,y=0.0,z=1.35},
				leanLeftViewOffset = {x=-0.5,y=0.10,z=1.525},
				leanRightViewOffset = {x=0.5,y=0.10,z=1.525},
				leanLeftWeaponOffset = {x=-0.45,y=0.0,z=1.30},
				leanRightWeaponOffset = {x=0.65,y=0.0,z=1.30},
				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},
				name = "alerted",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_CROUCH,
				normalSpeed = 0.5,
				maxSpeed = 50.0,
				heightCollider = 0.8,
				heightPivot = 0.0,
				size = {x=0.5,y=0.5,z=0.1},
				modelOffset = {x=0.0,y=0.0,z=0},
				viewOffset = {x=0,y=0.0,z=0.9},
				weaponOffset = {x=0.2,y=0.0,z=0.85},
				leanLeftViewOffset = {x=-0.55,y=0.0,z=1.1},
				leanRightViewOffset = {x=0.65,y=0.0,z=0.9},
				leanLeftWeaponOffset = {x=-0.5,y=0.0,z=0.85},
				leanRightWeaponOffset = {x=0.5,y=0.0,z=0.75},
				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},
				name = "crouch",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_LOW_COVER,
				normalSpeed = 0.5,
				maxSpeed = 50.0,
				heightCollider = 0.8,
				heightPivot = 0.0,
				size = {x=0.5,y=0.5,z=0.1},
				modelOffset = {x=0.0,y=0.0,z=0},
				viewOffset = {x=0,y=0.0,z=0.9},
				weaponOffset = {x=0.2,y=0.0,z=0.85},

				leanLeftViewOffset = {x=-0.75,y=0.0,z=0.9},
				leanRightViewOffset = {x=0.75,y=0.0,z=0.95},
				leanLeftWeaponOffset = {x=-0.75,y=0.0,z=0.6},
				leanRightWeaponOffset = {x=0.8,y=0.0,z=0.8},

				whileLeanedLeftViewOffset = {x=0.2,y=0.4,z=0.85},
				whileLeanedRightViewOffset = {x=0.2,y=0.3,z=1.0},
				whileLeanedLeftWeaponOffset = {x=0.25,y=0.4,z=0.8},
				whileLeanedRightWeaponOffset = {x=0.25,y=0.1,z=0.8},

				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},
				name = "coverLow",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_HIGH_COVER,
				normalSpeed = 1.0,
				maxSpeed = 50.0,
				heightCollider = 1.2,
				heightPivot = 0.0,
				size = {x=0.5,y=0.5,z=0.2},
				modelOffset = {x=0,y=-0.0,z=0},
				viewOffset = {x=0,y=0.10,z=1.625},
				weaponOffset = {x=0.2,y=0.0,z=1.35},

				leanLeftViewOffset = {x=-0.7,y=0.10,z=1.525},
				leanRightViewOffset = {x=0.95,y=0.10,z=1.525},
				leanLeftWeaponOffset = {x=-0.6,y=0.10,z=1.30},
				leanRightWeaponOffset = {x=1.0,y=0.10,z=1.30},

				whileLeanedLeftViewOffset = {x=0.1,y=0.1,z=1.5},
				whileLeanedRightViewOffset = {x=0.25,y=0.2,z=1.55},
				whileLeanedLeftWeaponOffset = {x=0.15,y=0.1,z=1.35},
				whileLeanedRightWeaponOffset = {x=0.3,y=0.2,z=1.45},

				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},

				name = "coverHigh",
				useCapsule = 1,
			},
			{
				stanceId = STANCE_SWIM,
				normalSpeed = 1.0, -- this is not even used?
				maxSpeed = 50.0, -- this is ignored, overridden by pl_swim* cvars.
				heightCollider = 0.9,
				heightPivot = 0.5,
				size = {x=0.5,y=0.5,z=0.1},
				modelOffset = {x=0,y=0,z=0.0},
				viewOffset = {x=0,y=0.1,z=0.5},
				weaponOffset = {x=0.2,y=0.0,z=0.3},
				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},
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
				size = {x=0.5,y=0.5,z=0.2},
				modelOffset = {x=0,y=0,z=0},
				viewOffset = {x=0,y=0.10,z=1.625},
				weaponOffset = {x=0.2,y=0.0,z=1.3},
				peekOverViewOffset = {x=0.1,y=0.0,z=1.5},
				peekOverWeaponOffset = {x=0.1,y=0.0,z=1.4},
				name = "relaxed",
				useCapsule = 1,
			},
		},

		characterDBAs =
		{
			"HumanAI",
			"HumanShared",
		},
	},

	Properties =
	{
		TagList = "", -- CryMannequin tags for this entity, separated by '+'

		soclasses_SmartObjectClass = "Human, Actor",

		esModularBehaviorTree = "SDK_Grunt_07",
		esNavigationType = "MediumSizedCharacters",
		fileHitDeathReactionsParamsDataFile = "Libs/HitDeathReactionsData/HitDeathReactions_SDKGrunt.xml",

		esVoice = "AI_01",
		esCommConfig = "Human",
		esFaction = "Grunts",
		commrange = 15.0,

		Damage =
		{
			health = 500,
			fileBodyDamage = "Libs/BodyDamage/BodyDamage_Default.xml",
			fileBodyDamageParts = "Libs/BodyDamage/Bodyparts_HumanMaleShared.xml",
			fileBodyDestructibility = "Libs/BodyDamage/BodyDestructibility_HumanMaleShared.xml",

			CanFall = 1,
			FallSleepTime = 1,
			heatAbsobsion = 0.5, -- how much real damage is absorbed when actor is hit by mike particles. 0 takes all damage, 1 takes no damage at all.
			heatDissipation = 200, -- how much heat damage points are dissipated per second
			minHeatDamage = 100, -- the minumum heat damage needed to burst an enemy
		},

		Perception =
		{
			minAlarmLevel = 0,

			cloakMaxDistCrouchedAndMoving = 1.5,
			cloakMaxDistCrouchedAndStill = 1.3,
			cloakMaxDistMoving = 3.0,
			cloakMaxDistStill = 2.0,
			config = "Human",
		},

		useSpecialMovementTransitions = 1,
		bOverrideAllowLookAimStrafing = 1,

		aibehavior_behaviour = "",
		aicharacter_character = "",
		esBehaviorSelection = "HumanGrunt",

		fileModel = "objects/characters/human/sdk_player/sdk_player_red.cdf",
		bUseFacialFrameRateLimiting = 1,
		equip_EquipmentPack = "AI_Rifle",

		AI =
		{
			bUseRadioChatter = 1,
			bPlayDeathSound = 0,
			sndDeathSound = "",
		},

		PlayerInteractions =
		{
			bStealthKill = 1,
			bCanBeGrabbed = 1,
			esGrabType = "Human"
		},

		CharacterSounds =
		{
			footstepEffect = "footstep_grunt", -- Footstep mfx library to use
			remoteFootstepEffect = "footstep_grunt", -- Footstep mfx library to use for remote players
			footstepIndGearAudioSignal_Walk = "", -- This directly plays the specified audiosignal on every footstep
			footstepIndGearAudioSignal_Run = "", -- This directly plays the specified audiosignal on every footstep
			bFootstepGearEffect = 0, -- This define if we play a gear sound from materialfx or not
			foleyEffect = "foley_player", -- Foley signal effect name
		},

		LipSync =
		{
			esLipSyncType = "LipSync_TransitionQueue",	-- can be either "LipSync_TransitionQueue" or "LipSync_FacialInstance" (the legacy one) at the moment
			bEnabled = true,

			-- these settings will be used by "LipSync_TransitionQueue"
			TransitionQueueSettings =
			{
				nCharacterSlot = 0,
				nAnimLayer = 11,
				sDefaultAnimName = "facial_chewing_01",
			}
		}
	},

	AIMovementAbility =
	{
		allowEntityClampingByAnimation = 1,
		usePredictiveFollowing = 1,
		pathLookAhead = 1,
		walkSpeed = 2.0, -- set up for humans
		runSpeed = 4.0,
		sprintSpeed = 6.4,
		maneuverSpeed = 1.5,
		b3DMove = 0,
		minTurnRadius = 0, -- meters
		maxTurnRadius = 3, -- meters
		pathSpeedLookAheadPerSpeed = -1.5,
		cornerSlowDown = 0.75,
		pathType = AIPATH_HUMAN,
		pathRadius = 0.25,
		passRadius = 0.25,

		distanceToCover = 0.5, -- needs to be at least 20cm more than max(passRadius, pathRadius)
		inCoverRadius = 0.075,
		effectiveCoverHeight = 0.55,
		effectiveHighCoverHeight = 1.75,

		pathFindPrediction = 0.5, -- predict the start of the path finding in the future to prevent turning back when tracing the path.
		maxAccel = 2.0,
		maxDecel = 4.0,
		velDecay = 0.5,
		maneuverTrh = 2.0, -- when cross(dir, desiredDir) > this use manoeuvring
		resolveStickingInTrace = 1,
		pathRegenIntervalDuringTrace = -1,
		lightAffectsSpeed = 1,

		avoidanceAbilities = AVOIDANCE_ALL,
		pushableObstacleWeakAvoidance = true,
		pushableObstacleAvoidanceRadius = 0.4,

		-- These are actually aiparams (as they may be changed during game and need to get serialized),
		-- but defined here so that designers do not try to change them.
		lookIdleTurnSpeed = 30,
		lookCombatTurnSpeed = 50,
		aimTurnSpeed = -1, --120,
		fireTurnSpeed = -1, --120,

		-- Adjust the movement speed based on the angle between body dir and move dir.
		directionalScaleRefSpeedMin = 1.0,
		directionalScaleRefSpeedMax = 8.0,

		AIMovementSpeeds =
		{
			--              { default, min, max }
			Relaxed =
			{
				Slow =      { 0.6,     0.6, 0.6 },
				Walk =      { 1.1,     1.1, 1.1 },
				Run =       { 3.5,     3.5, 3.5 },
				Sprint =    { 5.0,     5.0, 5.0 },
			},
			Alerted =
			{
				Slow =      { 0.8,     0.8, 0.8 },
				Walk =      { 1.4,     1.4, 1.4 },
				Run =       { 3.5,     3.5, 3.5 },
				Sprint =    { 5.0,     5.0, 5.0 },
			},
			Combat =
			{
				Slow =      { 0.8,     0.8, 0.8 },
				Walk =      { 1.7,     1.7, 1.7 },
				Run =       { 4.5,     4.5, 4.5 }, -- Min should ideally be: Max - (<Idle2Move duration> * maxAccel)
				Sprint =    { 6.0,     6.0, 6.0 },
			},
			Crouch =
			{
				Slow =      { 0.8,     0.8, 0.8 },
				Walk =      { 1.3,     1.3, 1.3 },
				Run =       { 2.0,     2.0, 2.0 },
				Sprint =    { 2.0,     2.0, 2.0 },
			},
			LowCover =
			{
				Slow =      { 0.9,     0.9, 0.9 },
				Walk =      { 0.9,     0.9, 0.9 },
				Run =       { 1.8,     1.8, 1.8 },
				Sprint =    { 1.8,     1.8, 1.8 },
			},
			HighCover =
			{
				Slow =      { 1.3,     1.3, 1.3 },
				Walk =      { 1.3,     1.3, 1.3 },
				Run =       { 1.8,     1.8, 1.8 },
				Sprint =    { 1.8,     1.8, 1.8 },
			},
			Swim =
			{
				Slow =      { 1.0,     1.0, 1.0 },
				Walk =      { 1.0,     1.0, 1.0 },
				Run =       { 3.5,     3.5, 3.5 },
				Sprint =    { 5.0,     5.0, 5.0 },
			},
		},
	},

	melee =
	{
		damage = 400,
		damageOffset = {x=0, y=2, z=0};
		damageRadius = 1.8,
		damageRadiusShort = 1.5,
		hitRange = 1.8,
		knockdownChance = 0.1,
		impulse = 600,
		angleThreshold = 180,
	},

	PropertiesInstance =
	{
		AI =
		{
			bGunner = 0,
		},
	},

	SearchModule =
	{
		sightRange = 12.0,
	},

	aiSequenceUser = true,

	OnCustomKill = function(entity)
		if (AIBase.OnCustomKill) then
			AIBase.OnCustomKill(entity)
		end

		if (entity.Properties.AI.bPlayDeathSound and entity.Properties.AI.sndDeathSound~="") then
			entity:PlaySoundEvent(entity.Properties.AI.sndDeathSound, g_Vectors.v000, g_Vectors.v010, SOUND_DEFAULT_3D, 0, SOUND_SEMANTIC_LIVING_ENTITY)
		end
	end,

	OnGrenadeDanger = function(entity, sender, data)
		entity.lastImmediateThreatPos = data.point
	end,

	OnExposedToExplosion = function(entity, sender, data)
		entity.lastImmediateThreatPos = data.point
	end,

	OnScorcherExplosionDanger = function(entity, sender, data)
		entity.lastImmediateThreatPos = data.point
	end,

	OnWaterRippleSeen = function(entity, sender, data)
		entity.AI.lastWaterRipplePosition = data.point

		local currentPosition = g_Vectors.temp_v1
		entity:GetWorldPos(currentPosition)
		if (DistanceVectors(currentPosition, entity.AI.lastWaterRipplePosition) < 15) then
			AI.Signal(SIGNALFILTER_SENDER, 1, "OnNearbyWaterRippleSeen", entity.id)
		end
	end,

	OnEMPGrenadeThrown = function(entity, sender, data)
		AI.Signal(SIGNALFILTER_GROUPONLY, 1, "EMPGrenadeThrownInGroup", entity.id)
		if (entity:HasGroupMembersNearby()) then
			AI.PlayCommunication(entity.id, "ThrowEMP", "Group", 2.0);
		end
	end,

	OnSmokeGrenadeThrown = function(entity, sender, data)
		AI.Signal(SIGNALFILTER_GROUPONLY, 1, "SmokeGrenadeThrownInGroup", entity.id)
		if (entity:HasGroupMembersNearby()) then
			AI.PlayCommunication(entity.id, "ThrowSmoke", "Group", 2.0);
		end
	end,

--	OnFlashbangGrenadeThrown = function(entity, sender, data)
--		AI.Signal(SIGNALFILTER_GROUPONLY, 1, "FlashGrenadeThrownInGroup", entity.id)
--		if (entity:HasGroupMembersNearby()) then
--			AI.PlayCommunication(entity.id, "ThrowFlash", "Group", 2.0);
--		end
--	end,

	OnFragGrenadeThrown = function(entity, sender, data)
		AI.Signal(SIGNALFILTER_GROUPONLY, 1, "FragGrenadeThrownInGroup", entity.id)
		if (entity:HasGroupMembersNearby()) then
			AI.PlayCommunication(entity.id, "ThrowFrag", "Group", 2.0);
		end
	end,

	OccupyAdvantagePoint = function( entity )
		entity:ReleaseAdvantagePoint()

		local advantagePointPosition = AI.GetRefPointPosition( entity.id )
		if ( not advantagePointPosition ) then
			return
		end

		entity.occupiedAdvantagePointPosition = advantagePointPosition
		GameAI.OccupyAdvantagePoint( entity.id, advantagePointPosition )
	end,

	ReleaseAdvantagePoint = function( entity )
		if ( not entity.occupiedAdvantagePointPosition ) then
			return
		end

		GameAI.ReleaseAdvantagePointFor( entity.id, entity.occupiedAdvantagePointPosition )
		entity.occupiedAdvantagePointPosition = nil
	end,

	GroupMemberDied = function(entity)
		if (entity.deadGroupMemberCount == nil) then
			entity.deadGroupMemberCount = 1
		else
			entity.deadGroupMemberCount = entity.deadGroupMemberCount + 1
		end
	end,

	GetDeadGroupMemberCount = function(entity)
		return entity.deadGroupMemberCount or 0
	end,

	IsAlone = function(entity)
		return AI.GetDistanceToClosestGroupMember(entity.id) > 50
	end,

	HasGroupMembersNearby = function(entity)
		return not entity:IsAlone()
	end,

	WatchedMateDie = function(entity)
		-- Comment on the weapon used by the shooter
		local victim = System.GetEntity(entity.deadGroupMemberData.victimID)
		local deathBulletType = ""
		if (victim and victim.deathProjectileClassId) then
			deathBulletType = CryAction.GetClassName(victim.deathProjectileClassId)
		end

		if (deathBulletType == "LTagGrenade") then
			AI.PlayCommunication(entity.id, "NoticePlayerUsingLtag", "EnemyWeaponComment", 3.0)
		elseif (deathBulletType == "SniperBullet" or deathBulletType == "GaussBullet") then
			AI.PlayCommunication(entity.id, "NoticePlayerUsingDSG1", "EnemyWeaponComment", 3.0)
		elseif (deathBulletType == "rocket") then
			AI.PlayCommunication(entity.id, "NoticePlayerUsingJaw", "EnemyWeaponComment", 3.0)
		elseif (AI.GetAlertness(entity.id) >= 2) then
			AI.PlayCommunication(entity.id, "AwareOfTargetAndWitnessedMateDie", "DeadGroupMemberComment", 3.0)
		end
	end,

	OnGroupMemberDiedOnHMG = function(entity)
		AI.PlayCommunication(entity.id, "NoticeMateDiedOnMountedGun", "Group", 2.0)
	end,
}

HumanPostures =
{
	{
		name = "LowCoverPeek",
		templateOnly = true,

		type = POSTURE_PEEK,
		stance = STANCE_LOW_COVER,
		priority = 9.0,

		{
			name = "LowCoverPeekLeft",
			lean = -0.8,
			agInput = "coverLftPeek",
			priority = -0.25,
		},

		{
			name = "LowCoverPeekRight",
			lean = 0.8,
			agInput = "coverRgtPeek",
			priority = -0.25,
		},

		{
			name = "LowCoverPeekCenter",
			peekOver = 0.8,
			agInput = "coverMidPeek",
			priority = -0.5,
		},
	},

	{
		name = "HighCoverPeek",
		templateOnly = true,

		type = POSTURE_PEEK,
		stance = STANCE_HIGH_COVER,
		priority = 9.0,

		{
			name = "HighCoverPeekLeft",
			lean = -0.8,
			agInput = "coverLftPeek",
			priority = -0.15,
		},

		{
			name = "HighCoverPeekRight",
			lean = 0.8,
			agInput = "coverRgtPeek",
			priority = -0.15,
		},
	},

	{
		name = "LowCoverAim",
		templateOnly = true,

		type = POSTURE_AIM,
		stance = STANCE_LOW_COVER,
		priority = 8.0,

		{
			name = "LowCoverAimOverCenter",
			peekOver = 0.8,
			agInput = "coverMidShoot",
		},

		{
			name = "LowCoverAimLeft",
			lean = -0.8,
			agInput = "coverLftShoot",
		},

		{
			name = "LowCoverAimRight",
			lean = 0.8,
			agInput = "coverRgtShoot",
		},
	},

	{
		name = "HighCoverAim",

		type = POSTURE_AIM,
		stance = STANCE_HIGH_COVER,
		priority = 9.0,

		{
			name = "HighCoverAimLeft",
			lean = -0.8,
			agInput = "coverLftShoot",
		},

		{
			name = "HighCoverAimRight",
			lean = 0.8,
			agInput = "coverRgtShoot",
		},
	},

	{
		name = "StandAim",
		type = POSTURE_AIM,
		stance = STANCE_STAND,
		priority = 10.0,

		{
			name = "StandAimCenter",
			lean = 0.0,
			priority = 0.0,
		},
	},

	{
		name = "CrouchAim",
		type = POSTURE_AIM,
		stance = STANCE_CROUCH,
		priority = 8.0,

		{
			name = "CrouchAimCenter",
			lean = 0.0,
			priority = 0.0,
		},
	},

	{
		name = "BlindLowCoverAim",
		templateOnly = true,

		type = POSTURE_AIM,
		stance = STANCE_LOW_COVER,
		priority = 0.0,

		{
			name = "BlindLowCoverAimLeft",
			lean = -0.8,
			agInput = "coverLftBlind",
			priority = -0.25,
		},

		{
			name = "BlindLowCoverAimRight",
			lean = 0.8,
			peekOver = 0.2,
			agInput = "coverRgtBlind",
			priority = -0.25,
		},

		{
			name = "BlindLowCoverAimCenter",
			peekOver = 0.8,
			agInput = "coverMidBlind",
			priority = -0.5,
		},
	},

	{
		name = "BlindHighCoverAim",
		templateOnly = true,

		type = POSTURE_AIM,
		stance = STANCE_HIGH_COVER,
		priority = 0.0,

		{
			name = "BlindHighCoverAimLeft",
			lean = -0.8,
			agInput = "coverLftBlind",
			priority = -0.15,
		},

		{
			name = "BlindHighCoverAimRight",
			lean = 0.8,
			--peekOver = 0.2,
			agInput = "coverRgtBlind",
			priority = -0.15,
		},
	},
}

mergef(Human_x, AIBase, 1)

-----------------------------------------------------------------------------------------------------
function Human_x:OnResetCustom()

	GameAI.UnregisterWithAllModules(self.id)
	AI.SetPostures(self.id, HumanPostures)

	self:ResetSharedSoundIds()

	self.AI.mountedWeaponCheck = true

	if (self.Properties.AI.bUseRadioChatter == 1) then
		GameAI.RegisterWithModule("RadioChatter", self.id)
	end

	GameAI.RegisterWithModule("BattleFront", self.id)
	GameAI.RegisterWithModule("StalkerModule", self.id) -- TODO: Rename StalkerModule to something else since it's used to see if the target can see me
	GameAI.RegisterWithModule("RangeModule", self.id)
	GameAI.AddRange(self.id, 2.5, "OnTargetEnteredMeleeRange", "")
	GameAI.AddRange(self.id, 3, "", "OnTargetLeftMeleeRange")
	GameAI.AddRange(self.id, self.gameParams.nearbyRange, "OnTargetEnteredNearbyRange", "OnTargetLeftNearbyRange")
	GameAI.AddRange(self.id, self.gameParams.middleRange, "OnTargetEnteredMiddleRange", "OnTargetEnteredMiddleRange")
	
	-- Stop advancing if the target is within this range whilst advancing
	GameAI.AddRange(self.id, 8.0, "EnteredTooCloseForComfortRange", "LeftTooCloseForComfortRange")

	self.lastImmediateThreatPos = {x=0, y=0, z=0}
	self.deadGroupMemberCount = 0
	self.suspiciousSoundInvestigationCount = 0

	self.actor:AcquireOrReleaseLipSyncExtension();
	self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
	self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_CAN_COLLIDE_WITH_MERGED_MESHES, 0);
end

--------------------------------------------------
-- Misc
--------------------------------------------------
function Human_x:EnableSearchModule()
	local groupId = AI.GetGroupOf(self.id)
	local targetPos = g_Vectors.temp_v1
	if (AI.GetAttentionTargetPosition(self.id, targetPos)) then
		local targetEntityId = NULL_ENTITY
		local targetEntity = AI.GetAttentionTargetEntity(self.id);
		if (targetEntity) then
			targetEntityId = targetEntity.id
		end
		GameAI.StartSearchModuleFor(groupId, targetPos, targetEntityId, 45.0)
		GameAI.RegisterWithModule("SearchModule", self.id)
		self:Log("Started search module for my group.")
	else
		self:Error("No attention target to search for")
	end
end

function Human_x:SetNextSearchSpotToRefPoint()
	local position = GameAI.GetNextSearchSpot(self.id,
		0.6, -- closenessToAgentWeight
		0.3, -- closenessToTargetWeight
		15.0, -- minDistanceFromAgent
		0.1 -- optional: closenessToTargetCurrentPosWeight
	)
	if (position) then
		AI.SetRefPointPosition(self.id, position);
	else
		AI.Signal(SIGNALFILTER_SENDER, 1, "NoSearchSpotsAvailable", self.id)
	end
end

function Human_x:DisableSearchModule()
	GameAI.UnregisterWithModule("SearchModule", self.id)
end

function Human_x:OnPostLoad()
	self:OnResetCustom();
	BasicActor.OnPostLoad(self);
end

function Human_x:ResetSharedSoundIds()

	self.SharedSoundSignals =
	{
		Feedback_KilledByPlayer = GameAudio.GetSignal("Human_Feedback_KilledByPlayerHit" ),
		KilledByExplosion = GameAudio.GetSignal("Human_KilledByExplosion" ),
		FeedbackHit2D = GameAudio.GetSignal("Human_FeedbackHit2D" ),
		FeedbackHit2D_Head  = GameAudio.GetSignal( "Human_FeedbackHit2D_Head" ),
		FeedbackHit2D_Helmet = GameAudio.GetSignal( "Human_FeedbackHit2D_Helmet" ),	
		FeedbackHit2DMelee = GameAudio.GetSignal("Melee_FeedbackHit2D"),
		FeedbackHit2D_NoDamage = GameAudio.GetSignal("NoDamage_FeedbackHit2D"),
	}

end

-- Set relocation speed depending on distance
function Human_x:SetHoldGroundMoveSpeed()
	local currentPosition = g_Vectors.temp_v1
	self:GetWorldPos(currentPosition)
	if (DistanceVectors(currentPosition, self.AI.HoldGround.pos) > 10 or -- This 10 value matches the strafe set up to be 5+5 in HumanHoldGroundRelocate
			math.abs(currentPosition.z - self.AI.HoldGround.pos.z) > 1.0) then
		AI.SetSpeed(self.id, SPEED_RUN)
	else
		AI.SetSpeed(self.id, SPEED_WALK)
	end
end

function Human_x:DoPainSounds(dead, hitType, isSilenced)

	--If stealthkill, let animevents handle it
	if (g_gameRules:IsStealthHealthHit(hitType)) then
		return
	end

	if (dead) then
		if (self.doHeadShotReadability) then
			AI.PlayCommunication(self.id, "comm_headshot_death", "Injury", 0.5);
			self.doHeadShotReadability = false;
		elseif (hitType == "stickyArrow") then
			AI.PlayCommunication(self.id, "comm_headshot_death", "Injury", 0.5);
		elseif (hitType == "meleeLeft" or hitType == "meleeRight") then
			local isAlerted = AI.GetAlertness(self.id) > 0;
			if ( isAlerted == false ) then
				AI.PlayCommunication(self.id, "comm_headshot_death", "Injury", 0.5);
			else
				AI.PlayCommunication(self.id, "comm_death_melee", "Injury", 0.5);
			end
		elseif (hitType == "silentMelee") then
			AI.PlayCommunication(self.id, "comm_headshot_death", "Injury", 0.5);
		else
			if (isSilenced == true) then
				AI.PlayCommunication(self.id, "comm_headshot_death", "Injury", 0.5);
			else
				AI.PlayCommunication(self.id, "comm_death", "Injury", 0.5);
			end
		end
	else
		if (hitType == "silentMelee") then
			return
		else
			AI.PlayCommunication(self.id, "comm_pain", "InjuryPain", 0.5);
		end
	end

end

function Human_x:DoFeedbackHit2DSounds(hit)
	if (self.SharedSoundSignals) then
		local hostile = AI.Hostile(self.id, g_localActorId);
		if (hostile == true) then
			if (hit.damage <= 0.0) then
				GameAudio.JustPlayEntitySignal(self.SharedSoundSignals.FeedbackHit2D_NoDamage, self.id);
			elseif (hit.meleeHit ~= 0) then
				GameAudio.JustPlayEntitySignal(self.SharedSoundSignals.FeedbackHit2DMelee, self.id);
				--Log("DoFeedbackHit2DSounds - Melee");
			elseif (hit.headShotType == eHeadShotType_Head) then
				self.doHeadShotReadability = true
				GameAudio.JustPlayEntitySignal( self.SharedSoundSignals.FeedbackHit2D_Head, self.id );
				--Log("DoFeedbackHit2DSounds - Head");
			elseif (hit.headShotType == eHeadShotType_Helmet) then
				GameAudio.JustPlayEntitySignal( self.SharedSoundSignals.FeedbackHit2D_Helmet, self.id );
				--Log("DoFeedbackHit2DSounds - Helmet");
			else
				GameAudio.JustPlayEntitySignal(self.SharedSoundSignals.FeedbackHit2D, self.id);
				--Log("DoFeedbackHit2DSounds - Flesh");
			end;
		end;
	end
end

function Human_x:EnableBlindFire(enable)
	if (enable) then
		AI.SetPosturePriority(self.id, "BlindLowCoverAim", 10.0);
		AI.SetPosturePriority(self.id, "BlindHighCoverAim", 10.0);
	else
		AI.SetPosturePriority(self.id, "BlindLowCoverAim", 0.0);
		AI.SetPosturePriority(self.id, "BlindHighCoverAim", 0.0);
	end
end

function Human_x:CheckForMountedWeapons()
	local mountedWeapon = self:FindValidMountedWeapon()
	if (mountedWeapon) then
		--self:Log("checking for mounted weapons : mounted weapon FOUND! - "..mountedWeapon:GetName().." -")
		self.AI.targetMountedWeapon = mountedWeapon
		AI.SetBehaviorVariable(self.id, "GoToMountedWeapon", true)
		return true
	else
		--self:Log("checking for mounted weapons : mounted weapon NOT found.")
		return false
	end
end

function Human_x:FindValidMountedWeapon()
	local weaponName = AI.FindObjectOfType(self:GetPos(), 10, AIOBJECT_MOUNTEDWEAPON)--, AIFAF_VISIBLE_TARGET)
	if (weaponName) then
		local weaponEntity = System.GetEntityByName(weaponName)
		if (weaponEntity and not weaponEntity.inUse and Game.IsMountedWeaponUsableWithTarget(self.id, weaponEntity.id)) then
			return weaponEntity
		end
	end
end

function Human_x:OnUseMountedWeaponRequest(weaponId)
	self:PrepareForMountedWeaponUse(weaponId)
end

function Human_x:PrepareForMountedWeaponUse(weaponId, pipeID)
	local weapon;
	if (weaponId) then
		weapon = System.GetEntity(weaponId);
	else
		-- find nearby mounted available mounted weapon
		-- check if nobody else is trying to use it
	end

	if (not weapon) then
		return false;
	end

	if (weapon.item:IsUsed() and weapon.item:GetOwnerId() ~= self.id) then
		return false;
	end

	local pos = g_Vectors.temp_v1;
	local dir = weapon.item:GetMountedDir(g_Vectors.temp_v2);
	local pivot = weapon:GetWorldPos(g_Vectors.temp_v3);
	FastDifferenceVectors(pos, pivot, dir);

	local toFloor = g_Vectors.temp_v4;
	toFloor.x = 0;
	toFloor.y = 0;
	toFloor.z = -2;

	if (Physics.RayWorldIntersection(pos, toFloor, 2, ent_terrain + ent_static + ent_rigid + ent_sleeping_rigid, self.id, nil, g_HitTable) > 0) then
		pos.z = g_HitTable[1].pos.z;
	end

	AI.SetRefPointPosition(self.id, pos);
	AI.SetRefPointDirection(self.id, dir);

	self.AI.currentMountedWeaponId = weaponId;
	self.AI.currentMountedWeaponInitialDir = dir;
	self.AI.currentMountedWeaponPivot = pivot;
	weapon.inUse = true

	if (not self.AI.theVehicle and (not self.AI.usingMountedWeapon)) then
		AI.SetBehaviorVariable(self.id, "MovingToMountedWeapon", true);
	else
		return false;
	end

	return true;
end

function Human_x:ForceReleaseOfMountedGun()
	self:LeaveMountedWeapon();
end

function Human_x:SetMachineGunInitialDirInRefPoint()
	if (self.AI.currentMountedWeaponInitialDir ~= nil and self.AI.currentMountedWeaponPivot ~= nil) then
		AI.SetRefPointPosition(self.id, self.AI.currentMountedWeaponPivot);
		AI.SetRefPointDirection(self.id, self.AI.currentMountedWeaponInitialDir);
	end
end

function Human_x:UseMountedWeapon()
	if (self.AI.targetMountedWeapon) then
		self.AI.targetMountedWeapon.inUse = true
	end

	local weaponId = self.AI.currentMountedWeaponId;
	if (not weaponId) then
		return;
	end

	self.AI.oldFireTurnSpeed = AI.GetParameter(self.id, AIPARAM_FIRE_TURNSPEED);
	AI.ChangeParameter(self.id, AIPARAM_FIRE_TURNSPEED, 35);

	local perceptionTable = self.Properties.Perception;
	local sightRange = perceptionTable.sightrange;
	AI.ChangeParameter(self.id, AIPARAM_ACCURACY, 1);
	AI.ChangeParameter(self.id, AIPARAM_SIGHTRANGE, sightRange);
	AI.ChangeParameter(self.id, AIPARAM_ATTACKRANGE, sightRange);
	AI.ChangeParameter(self.id, AIPARAM_FOVPRIMARY, 320);
	AI.ChangeParameter(self.id, AIPARAM_FOVSECONDARY, 330);

	local weapon = System.GetEntity(weaponId);
	if (weapon) then
		if (self:GetDistance(weapon.id) < 3) then
			self.actor:HolsterItem(true);
			if (not weapon.item:IsUsed()) then
				weapon.item:StartUse(self.id);

				local pos = g_Vectors.temp_v1;
				local dir = weapon.item:GetMountedDir(g_Vectors.temp_v2);
				local pivot = weapon:GetWorldPos(g_Vectors.temp_v3);

				FastSumVectors(pos, pivot, dir);
				FastSumVectors(pos, pos, dir);

				AI.SetRefPointPosition(self.id, pos);
				AI.SetRefPointDirection(self.id, dir);

				self.AI.usingMountedWeapon = true

				if (weapon.class == "HMG") then
					AI.PlayCommunication(self.id, "AIUsesMountedGun", "Group", 3.0)
				elseif (weapon.class == "AGL") then
					AI.PlayCommunication(self.id, "AIUsesAGL", "Group", 3.0)
				end
			end
		else
			--TODO(márcio): Fail Behavior
			self:DrawWeaponNow();
		end
	end
end

function Human_x:LeaveMountedWeapon()
	if (self.AI.targetMountedWeapon) then
		self.AI.targetMountedWeapon.inUse = false
	end

	local weaponId = self.AI.currentMountedWeaponId;
	if (not weaponId) then
		return;
	end

	self.AI.currentMountedWeaponId = nil;
	self.AI.currentMountedWeaponInitialDir = nil;
	self.AI.currentMountedWeaponPivot = nil

	local weapon = System.GetEntity(weaponId)
	if (weapon) then
		weapon.inUse = false
	else
		self:OnError("(LeaveMountedWeapon) No weapon entity?")
		return
	end

	self.AI.usingMountedWeapon = nil

	AI.ChangeParameter(self.id, AIPARAM_FIRE_TURNSPEED, self.AI.oldFireTurnSpeed);

	local perceptionTable = self.Properties.Perception;
	AI.ChangeParameter(self.id, AIPARAM_ACCURACY, 1);
	AI.ChangeParameter(self.id, AIPARAM_SIGHTRANGE, perceptionTable.sightrange);
	AI.ChangeParameter(self.id, AIPARAM_ATTACKRANGE, perceptionTable.attackrange);
	AI.ChangeParameter(self.id, AIPARAM_FOVPRIMARY, perceptionTable.FOVPrimary);
	AI.ChangeParameter(self.id, AIPARAM_FOVSECONDARY, perceptionTable.FOVSecondary);

	local weapon = System.GetEntity(weaponId);
	if (weapon) then
		weapon.item:StopUse(self.id);
		self.actor:HolsterItem(false);
	end
end

function Human_x:CanShootMountedWeapon()
	local targetType = AI.GetTargetType(self.id);

	if ((targetType ~= AITARGET_ENEMY) and (targetType ~= AITARGET_MEMORY) and (targetType ~= AITARGET_SOUND)) then
		return false;
	end

	if(not self.AI.currentMountedWeaponId) then
		return false;
	end

	local weapon = System.GetEntity(self.AI.currentMountedWeaponId);
	if ((not weapon) or (not Game.IsMountedWeaponUsableWithTarget(self.id, weapon.id, 0))) then
		return false;
	end

	return true;
end

function Human_x:CheckMountedWeaponTarget()
	if (self.AI.mountedWeaponCheck == false) then
		return
	end

	-- Marcio: don't perform these checks for vehicles, designers will send the leave command in this case
	if (self.actor:GetLinkedVehicleId()) then
		return
	end

	local cutoff = 10
	local targetType = AI.GetTargetType(self.id)

	if ((targetType == AITARGET_MEMORY) or (targetType == AITARGET_SOUND)) then
		local dist = AI.GetAttentionTargetDistance(self.id)
		if (dist < cutoff) then
			self:Log("CheckMountedWeaponTarget - dist < cutoff... leaving")
			AI.Signal(SIGNALFILTER_SENDER, 1, "LeaveMountedWeapon", self.id)
		end
	elseif ((targetType == AITARGET_ENEMY) and not self:CanShootMountedWeapon()) then
		self:Log("CheckMountedWeaponTarget - CanShootMountedWeapon is false... leaving")
		AI.Signal(SIGNALFILTER_SENDER, 1, "LeaveMountedWeapon", self.id)
	end
end

function Human_x:IsTargetAnEnemy()
	local targetType = AI.GetTargetType(self.id)
	return (targetType == AITARGET_ENEMY)
end

function Human_x:MountedWeaponTargetChange()
	local targetType = AI.GetTargetType(self.id);
	if (targetType == AITARGET_ENEMY) then
		self:SelectPipe(0, "FireMountedWeapon");
	else
		self:SelectPipe(0, "LookAroundInMountedWeapon");

		-- Vehicles should fire their secondary weapon if possible
		if (self.AI.theVehicle) then
			self:InsertSubpipe(AIGOALPIPE_NOTDUPLICATE, "FireSecondaryMountedWeapon");
		end
	end
end

function Human_x:IsUsable(user)
	if(self:IsDead()) then
		return 0;
	else
		return 1;
	end
end

function Human_x.AnimationEvent(entity, event, value)
	if (event == "StealthMeleeDeath") then
		AI.PlayCommunication(entity.id, "comm_death_melee_stealth", "Injury", 0.5);
	elseif (BasicAI.AnimationEvent) then
		BasicAI.AnimationEvent(entity, event, value)
	end
end

function Human_x:IsFurthestAwayFromTargetInSquad()
	-- Generate a table of the squad members
	local squadId = GameAI.GetSquadId(self.id)
	if (squadId == nil) then
		self:Error("IsFurthestAwayFromTargetInSquad: Squad ID is nil")
		return true
	end

	local members = GameAI.GetSquadMembers(squadId)
	if (members == nil) then
		self:Error("IsFurthestAwayFromTargetInSquad: Squad doesn't have any members (members is nil)")
		return true
	end

	-- See who is furthest away? (Start out with me.)
	local furthestId = self.id
	local furthestDist = 0.0

	for id,member in pairs(members) do
		local dist = AI.GetAttentionTargetDistance(member.id) or 500
		if (dist > furthestDist) then
			furthestId = member.id
			furthestDist = dist
		end
	end

	if (furthestId == self.id) then
		return true
	else
		return false
	end
end

function Human_x:IsFurtherAwayFromTargetThanAveragePositionOfSquadScopeUsers(scopeName)
	local targetPosition = g_Vectors.temp_v1
	if (not AI.GetAttentionTargetPosition(self.id, targetPosition)) then
		--Log("Couldn't get attention target")
		return true
	end

	if (GameAI.GetSquadScopeUserCount(self.id, scopeName) < 2) then
		--Log("Alone in squad scope %s", scopeName)
		return true
	end

	local squadPosition = GameAI.GetAveragePositionOfSquadScopeUsers(self.id, scopeName)
	if (IsNullVector(squadPosition)) then
		--Log("Squad average position is a zero vector")
		return true
	end

	local agentPosition = self:GetWorldPos(g_Vectors.temp_v2)

	local squadToTargetDistanceSq = DistanceSqVectors(squadPosition, targetPosition)
	local agentToTargetDistanceSq = DistanceSqVectors(agentPosition, targetPosition)

	if (agentToTargetDistanceSq > squadToTargetDistanceSq) then
		--Log("Further away than squad in scope %s", scopeName)
		return true
	else
		--Log("Nope, not further away than squad in scope %s", scopeName)
		return false
	end
end

function Human_x:IsClosestToTargetInSquad()
	-- Generate a table of the squad members
	local squadId = GameAI.GetSquadId(self.id)
	if (squadId == nil) then
		self:Error("IsCloserFromTargetInSquad: Squad ID is nil")
		return true
	end

	local members = GameAI.GetSquadMembers(squadId)
	if (members == nil) then
		self:Error("IsCloserFromTargetInSquad: Squad doesn't have any members (members is nil)")
		return true
	end

	-- See who is the closest? (Start out with me.)
	local closestId = self.id
	local closestDist = 1000.0

	for id,member in pairs(members) do
		if (member ~= nil and member:IsActive() and not member:IsDead()) then
			local dist = AI.GetAttentionTargetDistance(member.id) or 500
			if (dist <= closestDist) then
				closestId = member.id
				closestDist = dist
			end
		end
	end

	if (closestId == self.id) then
		return true
	else
		return false
	end
end

function Human_x:IsClosestToTargetInGroup()
	-- See who is the closest? (Start out with me.)
	local closestId = self.id
	local closestDist = 1000.0

	local memberCount = AI.GetGroupCount(self.id)

	for i = 1, memberCount do
		local member = AI.GetGroupMember(self.id, i)
		if (member ~= nil and member:IsActive() and not member:IsDead()) then
			local dist = AI.GetAttentionTargetDistance(member.id) or 500
			if (dist <= closestDist) then
				closestId = member.id
				closestDist = dist
			end
		end
	end

	if (closestId == self.id) then
		return true
	else
		return false
	end
end

function Human_x:IsInRangeFromTarget(range)

	local dist = AI.GetAttentionTargetDistance(self.id) or 500
	if (range and dist <= range) then
		return true
	end
	return false
end

function Human_x:GetTargetDistance()
	return AI.GetAttentionTargetDistance(self.id) or 500
end

function Human_x:SetLastExplosiveTypeAsGrenade()
	self.lastExplosiveType = "grenade"
	return true
end

function Human_x:SetLastExplosiveTypeAsExplosive()
	self.lastExplosiveType = "explosive"
	return true
end

function Human_x:ClearCombatMoveAssignmentIfCloseToTheDestination()
	if (DistanceSqVectors(self:GetWorldPos(), self.AI.combatMove.position) < 25) then
		self:ClearAssignment()
	end
end

function Human_x:ShouldUseGrenade()
	local target = AI.GetAttentionTargetEntity(self.id)

	if (not self:ValidateAttentionTarget(target)) then
		return false
	end

	local entityToAttentionTargetNormal = g_Vectors.temp_v3
	SubVectors(entityToAttentionTargetNormal, target:GetWorldPos(), self:GetWorldPos())
	NormalizeVector(entityToAttentionTargetNormal);

	local dotProduct = dotproduct2d(self:GetDirectionVector(), entityToAttentionTargetNormal);
	local cosMaxAngleAllowed = math.cos(math.rad(self.gameParams.grenadeThrowMaxAngle))
	if dotProduct > cosMaxAngleAllowed then
		if(self.gameParams.grenadeLaunchProbability and random(0.0,1.0) <= self.gameParams.grenadeLaunchProbability) then
			return true
		else
			return false
		end
	end
end

function Human_x:RefreshCurrentDeadGroupMemberBodyPosition()
	if (self.deadGroupMemberData == nil) then
		self:Error("RefreshCurrentDeadGroupMemberBodyPosition expected the 'dead group member data' to exist but it didn't.")
		return
	end

	local deadBodyEntity = System.GetEntity(self.deadGroupMemberData.victimID)
	if (deadBodyEntity) then
		CopyVector(self.deadGroupMemberData.currentBodyPosition, deadBodyEntity:GetWorldPos())
	end
end

function Human_x:SetDeadGroupMemberBodyPositionAsRefPoint()
	if (self.deadGroupMemberData == nil) then
		self:Error("SetDeadGroupMemberPositionAsRefPoint expected the 'dead group member data' to exist but it didn't.")
		return false
	end

	AI.SetRefPointPosition(self.id, self.deadGroupMemberData.currentBodyPosition)
	return true
end

function Human_x:SetDeadGroupMemberDeathPositionAsRefPoint()
	if (self.deadGroupMemberData == nil) then
		self:Error("SetDeadGroupMemberDeathPositionAsRefPoint expected the 'dead group member data' to exist but it didn't.")
		return false
	end

	AI.SetRefPointPosition(self.id, self.deadGroupMemberData.deathPosition)
	return true
end

function Human_x:GrabDeadGroupMemberDataFromGroupScriptTable()
	local groupId = AI.GetGroupOf(self.id)
	local group = AI.GetGroupScriptTable(groupId)

	assert(group ~= nil)
	if (group == nil) then
		self:Error("group == nil in GrabDeadGroupMemberDataFromGroupScriptTable")
		return
	end

	if (group.deadGroupMemberData == nil) then
		self:Error("group.deadGroupMemberData == nil in GrabDeadGroupMemberDataFromGroupScriptTable")
		return
	end

	self.deadGroupMemberData = {}
	mergef(self.deadGroupMemberData, group.deadGroupMemberData, 1)
end

function Human_x:GetDistanceToDeadBody()
	return DistanceVectors(self.deadGroupMemberData.currentBodyPosition, self:GetWorldPos())
end

function Human_x:IsTargetInNearbyRange()
	local dist = AI.GetAttentionTargetDistance(self.id)
	if (dist <= self.gameParams.nearbyRange) then
		return true
	end

	return false
end

function Human_x:IsTargetInMiddleRange()
	local dist = AI.GetAttentionTargetDistance(self.id)
	if (dist <= self.gameParams.middleRange) then
		return true
	end

	return false
end