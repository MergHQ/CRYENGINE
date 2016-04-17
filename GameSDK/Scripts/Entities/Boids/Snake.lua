Script.ReloadScript( "Scripts/Entities/Boids/Chickens.lua" );

Snake =
{
	Properties =
	{
		Boid =
		{
			object_Model = "",
		},
		Movement =
		{
			HeightMin = 1,
			HeightMax = 3,
			SpeedMin = 0.5,
			SpeedMax = 1,
			MaxAnimSpeed = 2,
		},
	},

	Audio =
	{
	},
	
	Animations =
	{
		"walk_loop",   -- walking
		"idle01",      -- idle1
		"idle01",      -- idle2
		"idle01",      -- idle3
		"idle01",      -- pickup
		"idle01",      -- throw
	},
	Editor =
	{
		Icon = "Bird.bmp"
	},
}

MakeDerivedEntityOverride( Snake,Chickens )

function Snake:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end