--------------------------------------------------------------------------
--	Crytek Source File.
--	Copyright (C), Crytek Studios, 2001-2008.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Implementation of an abrams tank
--
--------------------------------------------------------------------------
--	History:
--	- 17/11/2008   16:26 : Created by Mathieu Pinard
--
--------------------------------------------------------------------------

--------------------------------------------------------------------------
Abrams =
{
	Sounds = {},

	Properties =
	{
		bDisableEngine = 0,
		Modification = "",
		soclasses_SmartObjectClass = "",
	},

	turretAngles = {},
	difPos = {},

	Client = {},
	Server = {}
}



--------------------------------------------------------------------------
function Abrams:GetSpecificFireProperties()
	if (self.AIFireProperties) then
		self.AIFireProperties[1] = {};
		self.AIFireProperties[1].min_distance = 20;
		self.AIFireProperties[1].max_distance = 400;
	end
end

--------------------------------------------------------------------------
Abrams.AIProperties =
{
	AIType = AIOBJECT_CAR,
	AICombatClass = AICombatClasses.Tank,
	AIDefaultSpecies = 1, -- This is enemy vehicle
	PropertiesInstance =
	{
		aibehavior_behaviour = "TankIdle",
		triggerRadius = 90,
	},

	Properties =
	{
		aicharacter_character = "Tank",
		bHidesPlayer=0,
		Perception =
		{
			FOVPrimary = -1, -- Normal fov
			FOVSecondary = -1, -- Peripheral vision fov
			sightrange = 400,
			persistence = 10,
		},
	},

	AIMovementAbility =
	{
		walkSpeed = 7.0,
		runSpeed = 11.0,
		sprintSpeed = 15.0,
		maneuverSpeed = 5.0,
		minTurnRadius = .2,
		maxTurnRadius = 10,
		pathType = "AIPATH_TANK",
		pathLookAhead = 8,
		pathRadius = 3,
		pathSpeedLookAheadPerSpeed = 1.0,
		cornerSlowDown = 0.75,
		pathFindPrediction = 1.0,
		maneuverTrh = 2.0,
		passRadius = 5.0,
		resolveStickingInTrace = 0,
		pathRegenIntervalDuringTrace = 4.0,
		avoidanceRadius = 10.0,
	},

	-- How fast I forget the target (S-O-M speed)
	forgetTimeTarget = 32.0,
	forgetTimeSeek = 40.0,
	forgetTimeMemory = 40.0,
}