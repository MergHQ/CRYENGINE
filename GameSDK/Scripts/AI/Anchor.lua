AIAnchorTable = {
	-- combat anchors
	COMBAT_PROTECT_THIS_POINT	= 314,
	COMBAT_HIDESPOT = 320,
	COMBAT_HIDESPOT_SECONDARY = 330,
	COMBAT_ATTACK_DIRECTION = 321,
	COMBAT_TERRITORY = 342,

	--SEARCH_SPOT = 343,
	
	-- alert anchors
	ALERT_STANDBY_IN_RANGE = 340,
	ALERT_STANDBY_SPOT = 341,
	
	--- sniper
	SNIPER_SPOT	= 324,
	--- RPG
	--RPG_SPOT	= 319,

	-- suit advanve point - for boss/super jump
	--SUIT_SPOT	= 325,
	
	--- tank combat
	--TANK_SPOT	= 326,
	--TANK_HIDE_SPOT	= 327,
	--HELI_HIDE_SPOT	= 328,

	ADVANTAGE_POINT = 329, -- used in C++ code too!
	
	-- actions anchors
--	ACTION_LOOK_AROUND = 353,
--	ACTION_RECOG_CORPSE = 354,
		
	-- Shark
	SHARK_ESCAPE_SPOT = 350,
	
	-- civilian
	--CIVILIAN_COWER_POINT = 800,

	-- for anchors used as smart objects with group id
	--SMART_OBJECT = 700,

	-- Light condition related anchors.
	LIGHTSPOT_LIGHT = 401,
	LIGHTSPOT_MEDIUM = 402,
	LIGHTSPOT_DARK = 403,
	LIGHTSPOT_SUPERDARK = 404,

	-- ai will not throw grenades near this anchor (not to damage it)
	-- the anchor radius used for distance check
	NOGRENADE_SPOT = 405,	-- used in C++ code too!
	
	-- do not use as anchor!
	REINFORCEMENT_SPOT = 400,	-- used in C++ code too!

	--FOLLOW_AREA = 801,
	--FOLLOW_PATH = 802,
	
	INTEREST_SYSTEM_SPOT = 803,

	SCORCHER_SCORCH = 900, -- Interresting positions for scorcher to scorch when idling or to guess where an enemy might be hiding behind.
	GRASS_HIDESPOT = 901,
	HEAVY_ADVANTAGE_POINT = 902,
	SCORCHER_CHOKE = 903, -- A choke-point that the scorcher could block with a 'fire-wall' to prevent an enemy from escaping via the flanks.
	GRENADIER_SPOT = 904, -- A good point to use ranged weapons like LTAG, JAW, ...
	-- DEER_SPOT = 905, -- A position where the deer can run to
}
