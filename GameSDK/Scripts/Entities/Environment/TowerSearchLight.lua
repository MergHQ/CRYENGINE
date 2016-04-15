TowerSearchLight =
{
	type = "TowerSearchLight",

	Properties =
	{
		bEnabled = 1,
		bCanDetectStealth = 0,  -- cloaking has no effect in detection
		bAlwaysSeePlayer = 0,  -- the tower will behave as if it always can see the player, dont matter if he is behind cover or whatever
		visionFOV = 5,  -- field of view in degrees
		visionRange = 1000,
		hearingRange = 200, -- centered at the towerSearchLight entity position
		alertAIGroupId = -1, -- the tower targets to places where the AI on this group detect threats
		burstDispersion = 10, -- error distance. full for the first shoot, 0 for the last.
		timeBetweenShootsInABurst = 0.5,
		offsetDistPrediction = 10, -- when the future position of the player is calculated, for shooting, this is added as a distance along the direction the player is looking at
		minPlayerSpeedForOffsetDistPrediction = 0.3, -- skip target prediction when player value is less than this
		maxDistancePrediction = 0, -- limits how far away the future target prediction calculations can go. If 0, there is no limit
		preshootTime = 2, -- defines how earlier than the actual shoot the preshoot actions happens  (preshoot actions include laser beam lock into position and playing preshoot sound)
		weapon = "TowerRocketLauncher",
		objLaserBeamModel = "Objects/weapons/attachments/laser_beam/laser_beam.cgf",
		audioBackground = "TowerSearchLight_Background",  -- always playing at the target spot
		audioShoot = "TowerSearchLight_Shoot",
		audioPreshoot = "TowerSearchLight_Preshoot",
		laserBeamThicknessScale = 20,
		visionPersistenceTime = 0, -- how much time the tower keeps seeing the player after losing it. It was added as a hacky way to workaround some geometry problems in swamp that make vision from the tower unreliable sometimes

		-- attachments are extra graphic entities that are manually moved and hide/unhide from here. IE: beam light graphic, fog volume.
		attachments=
		{
			attachment1=
			{
				linkName="",
				distFromTarget=0,
				rotationZ=0,
			},
			attachment2=
			{
				linkName="",
				distFromTarget=0,
				rotationZ=0,
			},
			attachment3=
			{
				linkName="",
				distFromTarget=0,
				rotationZ=0,
			},
			attachment4=
			{
				linkName="",
				distFromTarget=0,
				rotationZ=0,
			},
		},
		
		weaponSpots=
		{
			spot1=
			{
				bEnabled = 1,
				vOffset	= {x=0, y=0, z=0},
			},
			spot2=
			{
				bEnabled = 0,
				vOffset	= {x=0, y=0, z=0},
			},
			spot3=
			{
				bEnabled = 0,
				vOffset	= {x=0, y=0, z=0},
			},
		},
		
		behaviour= 
		{
			enemyInView=  -- when the tower is seeing an enemy
			{
				timeToFirstBurst = 1.5,
				timeToFirstBurstIfStealth = 1.5,  -- only makes sense if the tower can see cloaked player
				numWarningBursts = 0,
				timeBetweenWarningBursts = 5,
				errorAddedToWarningBursts = 10,
				timeBetweenBursts = 5,
				followDelay = 1,   -- when the player moves from the current target position, the tower does not react for this amount of time
				trackSpeed = 3,    -- speed at what the towers tries to follow player movement
				detectionSoundSequenceCoolDownTime = 20,
				audioDetection1=
				{
					audio = "TowerSearchLight_Detection1",
					delay = 0,
				},
				audioDetection2=
				{
					audio = "TowerSearchLight_Detection2",
					delay = 1,
				},
				audioDetection3=
				{
					audio = "TowerSearchLight_Detection3",
					delay = 1.5,
				},
			},
			enemyLost=   -- when the tower lost the target (went stealh, in cover, or just out of range)
			{
				maxDistSearch = 6,  -- how far away the vision cone/searchlight will move from the last target position while searching around
				timeSearching = 10, -- how much time will be in this state before going back to "idle"
				searchSpeed = 3, -- how fast will move the vision cone/searchlight while searching 
				timeToFirstBurst = 1000, -- if is greater than "timeSearching", the tower will never shoot in this state
				timeBetweenBursts = 10,
				minErrorShoot = 5, -- shots in this state are supposed to be "blind", so they are aimed with an added error distance
				maxErrorShoot = 15, 
			},
		},
	},

	Editor =
	{
		Icon="mine.bmp",
	},

	Client = {},
	Server = {},
}

function TowerSearchLight:OnPropertyChange()
	self.TowerSearchLight:OnPropertyChange()
end

function TowerSearchLight:Event_SetEntityIdleMovement( sender, entity )
	self.TowerSearchLight:SetEntityIdleMovement( entity.id );
end

function TowerSearchLight:Event_SetAlertAIGroupId( sender, AIGroupID )
	self.TowerSearchLight:SetAlertAIGroupID( AIGroupID );
end

function TowerSearchLight:Event_Enable()
	self.TowerSearchLight:Enable();
end

function TowerSearchLight:Event_Disable()
	self.TowerSearchLight:Disable();
end

function TowerSearchLight:Event_Sleep()
	self.TowerSearchLight:Sleep();
end

function TowerSearchLight:Event_Wakeup()
	self.TowerSearchLight:Wakeup();
end

function TowerSearchLight:Event_DisableWeaponSpot1()
	self.TowerSearchLight:DisableWeaponSpot(0);
end
function TowerSearchLight:Event_DisableWeaponSpot2()
	self.TowerSearchLight:DisableWeaponSpot(1);
end
function TowerSearchLight:Event_DisableWeaponSpot3()
	self.TowerSearchLight:DisableWeaponSpot(2);
end


TowerSearchLight.FlowEvents =
{
	Inputs =
	{
		SetAlertAIGroupId = { TowerSearchLight.Event_SetAlertAIGroupId, "int" },
		SetEntityIdleMovement = { TowerSearchLight.Event_SetEntityIdleMovement, "entity" },
		Enable = { TowerSearchLight.Event_Enable, "any" },
		Disable = { TowerSearchLight.Event_Disable, "any" },
		Sleep = { TowerSearchLight.Event_Sleep, "any" }, -- when sleep, the tower will move the lights and stuff along the predefined path, but will not react to any sound, AI notification, player, or anything.
		Wakeup = { TowerSearchLight.Event_Wakeup, "any" },
		DisableWeaponSpot1 = { TowerSearchLight.Event_DisableWeaponSpot1, "any" },
		DisableWeaponSpot2 = { TowerSearchLight.Event_DisableWeaponSpot2, "any" },
		DisableWeaponSpot3 = { TowerSearchLight.Event_DisableWeaponSpot3, "any" },
	},
	Outputs =
	{
		PlayerDetected = "bool",
		SoundHeard = "bool",
		PlayerLost = "bool",
		Preshoot = "bool",
		Shoot = "bool",
		Burst = "bool",
	},
}


