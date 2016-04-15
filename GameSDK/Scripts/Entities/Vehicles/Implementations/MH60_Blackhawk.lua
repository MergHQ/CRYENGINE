MH60_Blackhawk =
{
	Properties =
	{
		bDisableEngine = 0,
		material = "",
		Modification = "",
		soclasses_SmartObjectClass = "",
		esModularBehaviorTree = "",

		Perception =
		{
			FOVPrimary = -1,
			FOVSecondary = -1,
			sightrange = 200,
			stanceScale = 1,
			persistence = 60,
		},
	},

	isWheelRetracted = true,

	Client = {},
	Server = {},
}

MH60_Blackhawk.AIProperties =
{
	AIType = AIOBJECT_HELICOPTERCRYSIS2,
	AICombatClass = SafeTableGet(AICombatClasses, "Heli"),

	PropertiesInstance =
	{
		aibehavior_behaviour = "HeliIdle",
	},

	Properties =
	{
		aicharacter_character = "Heli",
	},

	AIMovementAbility =
	{
		walkSpeed = 15.0,
		runSpeed = 25.0,
		sprintSpeed = 40.0,
		maneuverSpeed = 5.0,
		b3DMove = 1,
		attackZoneHeight = 6,
		pathType = "AIPATH_DEFAULT",
		minTurnRadius = 1,
		maxTurnRadius = 35,
		optimalFlightHeight = 45.0,
		minFlightHeight = 25.0,
		maxFlightHeight = 300.0,
		pathLookAhead = 40,
		pathSpeedLookAheadPerSpeed = 8.0,
		cornerSlowDown = 1,
		pathFindPrediction = 2.0,
		pathRadius = 10,
		velDecay = 60,
		passRadius = 9.0,
		resolveStickingInTrace = 0,
		pathRegenIntervalDuringTrace = 0,
		avoidanceRadius = 20.0,
	},
}