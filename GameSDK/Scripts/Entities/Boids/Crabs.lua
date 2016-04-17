Script.ReloadScript( "Scripts/Entities/Boids/Chickens.lua" );

Crabs = 
{
	Properties = 
	{
	},
--	Sounds = 
--	{
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- idle
--		"Sounds/environment:random_oneshots_natural:crab_scared",	-- scared
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- die
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- pickup
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- throw
--	},
	Animations = 
	{
		"walk_loop",   -- walking
		"idle01",      -- idle1
		"idle01",      -- scared anim
		"idle01",      -- idle3
		"idle01",      -- pickup
		"idle01",      -- throw
	},
	Editor = 
	{
		Icon = "Bug.bmp"
	},
}

MakeDerivedEntity( Crabs,Chickens )

function Crabs:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end
