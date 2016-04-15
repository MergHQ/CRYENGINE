Script.ReloadScript( "Scripts/Entities/Boids/Chickens.lua" );

Turtles =
{
	Properties = 
	{
		Boid = 
		{
			nCount = 4, --[0,1000,1,"Specifies how many individual objects will be spawned."]
			object_Model = "Objects/characters/animals/reptiles/turtle/red_eared_slider.cdf",
		},
		Movement =
		{
			SpeedMin = 0.2,
			SpeedMax = 0.5,
			MaxAnimSpeed = 2,
		},
	},
	
	Audio =
	{
		"Play_idle_turtle",		-- idle
		"Play_scared_turtle",	-- scared
		"Play_death_turtle",	-- die
		"Play_scared_turtle",	-- pickup
		"Play_scared_turtle",	-- throw
	},
	
	Animations =
	{
		"walk_loop",	-- walking
		"idle3",		-- idle1
		"scared",		-- scared anim
		"idle3",		-- idle3
		"pickup",		-- pickup
		"throw",		-- throw
	},
	Editor =
	{
		Icon = "bug.bmp"
	},
}

-------------------------------------------------------
MakeDerivedEntityOverride( Turtles,Chickens )

-------------------------------------------------------
function Turtles:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

-------------------------------------------------------
function Turtles:GetFlockType()
	return Boids.FLOCK_TURTLES;
end