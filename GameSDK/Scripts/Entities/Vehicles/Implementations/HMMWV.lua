HMMWV =
{
	Properties = 
	{
		bDisableEngine = 0,
		material = "",
		Modification = "",
		soclasses_SmartObjectClass = "", -- soclasses_SmartObjectClass = "Car", (3.4)
	},
	
	Client = {},
	Server = {},
}

HMMWV.AIProperties = 
{
	AIType = AIOBJECT_CAR,
	PropertiesInstance = 
	{
		aibehavior_behaviour = "CarIdle",
	},

	Properties = 
	{
		aicharacter_character = "Car",
		aiSpeedMult = 1.0,
	},
  
  AIMovementAbility = 
	{
		minTurnRadius = 4,
		maxTurnRadius = 15,    
		walkSpeed = 50,
		runSpeed = 50, -- Maximum speed (without boost)
		sprintSpeed = 50,
		maneuverSpeed = 0, -- Brake at peaks 
		maneuverTrh = 0,
		cornerSlowDown = 0.75, -- cornerSlowDown = 0,
		velDecay = 130, -- velDecay = 0,
		-- maxAccel = 12, (3.4)
		-- maxDecel = 6,
		-- slopeSlowDown = 0.02,

		pathType = AIPATH_CAR, -- pathType = "AIPATH_RACING_CAR",
		pathLookAhead = 30,
		pathRadius = 2,
		pathSpeedLookAheadPerSpeed = 3.0, -- pathSpeedLookAheadPerSpeed = 0,
		pathFindPrediction = 1.0,
		pathRegenIntervalDuringTrace = 3,
		resolveStickingInTrace = 0,
		avoidanceRadius = 3,
	}, 
	hidesUser=0, 
}