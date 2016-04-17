--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--
--	Description: this is table of shared AI properties for all the actors
--	Actor will have to override property value if it has to be different
--	from defaults. Please don't just duplicate this table for every new
--	actor.
--
--------------------------------------------------------------------------
--  History:
--  - 13/06/2005   15:36 : Created by Kirill Bulatsev
--
--------------------------------------------------------------------------


BasicAITable = {

------------------------------------------------------------------------------------

	PropertiesInstance = {
		soclasses_SmartObjectClass = "",
		groupid = 0,
		aibehavior_behaviour = "",
		bAutoDisable = 0,
		nVariation = 0,
		bAlarmed = 0,
		esVoice = "",			
	},

	Properties =
	{
		commrange = 30.0,

		bFactionHostility = 1,

		esVoice = "",
		esCommConfig = "",
		fFmodCharacterTypeParam = 0,
		esBehaviorSelectionTree = "",
		esModularBehaviorTree = "",
		esNavigationType = "",

		aicharacter_character = "Cover2",
		fileModel = "Objects/Characters/Human/Asian/NK_Soldier/nk_soldier_jungle_cover_light_01.cdf",
		nModelVariations=0,

		bInvulnerable = 0,
	
		eiColliderMode = 0, -- zero as default, meaning 'script does not care and does not override graph, etc'.

		awarenessOfPlayer = 0,

		RateOfDeath =
		{
		  accuracy = 1.0,
		  attackrange = 70,
		  reactionTime = 1.0,
		},

		Perception =
		{
			--fov/angle related
			FOVPrimary = 80,			-- normal fov
			FOVSecondary = 160,		-- periferial vision fov
			--ranges
			sightrange = 70,
			sightrangeVehicle = -1,	-- how far do i see vehicles
			--how heights of the target affects visibility
			--// compare against viewer height
			-- fNewIncrease *= targetHeight/stanceScale
			stanceScale = 1.9,
			-- Sensitivity to sound 0=deaf, 1=normal
			audioScale = 1,
			-- controls how often targets can be switched,
			-- this parameter corresponds to minimum ammount of time the agent will hold aquired target before selectng another one
			-- default = 0
			persistence = 0,
			-- controls how long the attention target have had to be invisible to make the player stunts effective again
			stuntReactionTimeOut = 3.0,
			-- controls how sensitive the agent is to react to collision events (scales the collision event distance).
			collisionReactionScale = 1.0,
			-- flag indicating if the agent perception is affected by light conditions.
			bIsAffectedByLight = 0,
			-- Value between 0..1 indicating the minimum alarm level.
			minAlarmLevel = 0,
			-- Max distance at which the agent is allowed to spot dead bodies
			minDistanceToSpotDeadBodies = 20,

			-- Perception config (used by target tracks)
			-- TODO: Expand config to encompass the values above
			config = "default",

			-- Target track variables
			TargetTracks =
			{
				classThreat = 1,	-- Archetype threat value for target track mod 'ArchetypeThreat'
				targetLimit = 0,	-- How many agents can target me at a time (0 for infinite)
			},
		},

		Interest =
		{
		  bInterested = 1,
		  MinInterestLevel = 0.0,
		  Angle = 270.0,
		},
		Readability =
	 	{
	 		bIgnoreAnimations = 0,
	 		bIgnoreVoice = 0,
		},
	},

	-- the AI movement ability
	AIMovementAbility =
	{
		b3DMove = 0,
		pathLookAhead = 2,
		pathRadius = 0.3,
		walkSpeed = 2.0, -- set up for humans
		runSpeed = 4.0,
		sprintSpeed = 6.4,
		maxAccel = 1000000.0,
		maxDecel = 1000000.0,
		maneuverSpeed = 1.5,
		minTurnRadius = 0,	-- meters
		maxTurnRadius = 3,	-- meters
		maneuverTrh = 2.0,  -- when cross(dir, desiredDir) > this use manouvering
		resolveStickingInTrace = 1,
		pathRegenIntervalDuringTrace = -1,
		avoidanceRadius = 1.5,
		
		-- If this is turned off, then the AI will not avoid collisions, and will not
		-- be avoided by other AI either.
		collisionAvoidanceParticipation = true,

		-- When moving, this increment value will be progressively added
		-- to the agents avoidance radius (the rate is defined with the
		-- CollisionAvoidanceRadiusIncrementIncreaseRate/DecreaseRate
		-- console variables).
		-- This is meant to make the agent keep a bit more distance between 
		-- him and other agents/obstacles. /Mario
		collisionAvoidanceRadiusIncrement = 0.0,
	},

	AIFireProperties = {
	},
	AI = {},

	-- now fast I forget the target (S-O-M speed)
	forgetTimeTarget = 16.0,
	forgetTimeSeek = 20.0,
	forgetTimeMemory = 20.0,
}

-------------------------------------------------------------------------------------------------------
