WaterRipplesGenerator =
{
	type = "WaterRipplesGenerator",

	Properties =
	{
		bEnabled = 1,
		fScale = 1,
		fStrength = 1,
		Spawning =
		{
			bAutoSpawn = 0,
			bSpawnOnMovement = 1,
			fFrequency = 1,
		},
		Randomization =
		{
			fRandomFreq = 0,
			fRandomScale = 0,
			fRandomStrength = 0,
			fRandomOffsetX = 0,
			fRandomOffsetY = 0,
		},
	},

	Editor =
	{
		Icon="seed.bmp",
	},

	Client = {},
	Server = {},
}

function WaterRipplesGenerator:OnPropertyChange()
  Game.SendEventToGameObject( self.id, "propertyChanged" );
end

function WaterRipplesGenerator:Event_Enable()
	Game.SendEventToGameObject( self.id, "enable" );
end

function WaterRipplesGenerator:Event_Disable()
	Game.SendEventToGameObject( self.id, "disable" );
end

function WaterRipplesGenerator:Event_AutoSpawn( i, val )
	self.Properties.Spawning.bAutoSpawn = val;
end

function WaterRipplesGenerator:Event_SpawnOnMovement( i, val )
	self.Properties.Spawning.bSpawnOnMovement = val;
end

function WaterRipplesGenerator:Event_SetScale( i, val )
	self.Properties.fScale = val;
end

function WaterRipplesGenerator:Event_SetRandomScale( i, val )
	self.Properties.Randomization.fRandomScale = val;
end

function WaterRipplesGenerator:Event_SetStrength( i, val )
	self.Properties.fStrength = val;
end

function WaterRipplesGenerator:Event_SetRandomStrength( i, val )
	self.Properties.Randomization.fRandomStrength = val;
end

function WaterRipplesGenerator:Event_SetFrequency( i, val )
	self.Properties.Spawning.fFrequency = val;
end

function WaterRipplesGenerator:Event_SetRandomFreq( i, val )
	self.Properties.Randomization.fRandomFreq = val;
end

function WaterRipplesGenerator:Event_SetRandomOffset( i, val )
	self.Properties.Randomization.fRandomOffsetX = val.x;
	self.Properties.Randomization.fRandomOffsetY = val.y;
end

WaterRipplesGenerator.FlowEvents =
{
	Inputs =
	{
		Enable = { WaterRipplesGenerator.Event_Enable, "any" },
		Disable = { WaterRipplesGenerator.Event_Disable, "any" },
		AutoSpawn = { WaterRipplesGenerator.Event_AutoSpawn, "bool" },
		SpawnOnMovement = { WaterRipplesGenerator.Event_SpawnOnMovement, "bool" },
		Scale = { WaterRipplesGenerator.Event_SetScale, "float" },
		RandomScale = { WaterRipplesGenerator.Event_SetRandomScale, "float" },
		Strength = { WaterRipplesGenerator.Event_SetStrength, "float" },
		RandomStrength = { WaterRipplesGenerator.Event_SetRandomStrength, "float" },
		Frequency = { WaterRipplesGenerator.Event_SetFrequency, "float" },
		RandomFreq = { WaterRipplesGenerator.Event_SetRandomFreq, "float" },
		RandomOffset = { WaterRipplesGenerator.Event_SetRandomOffset, "vec3" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
	},
}

