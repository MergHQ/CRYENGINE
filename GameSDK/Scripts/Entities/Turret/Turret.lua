Turret =
{
	Properties =
	{
		objModel = "objects/weapons/turret/turret.cdf",
		esFaction = "Grunts",
		esTurretState = "Deployed",
		PrimaryWeapon = "TurretGun",
		PrimaryWeaponJointName = "weaponjoint",
		PrimaryWeaponFovDegrees = 15,
		PrimaryWeaponRangeCheckOffset = 2,
		bUsableOnlyOnce = 1,

		Sound =
		{
			GameHintName = "HumanTurret_Gun",
			HorizontalJointName = "arcjoint",
			VerticalJointName = "Def_Gun_JNT",
			soundVerticalMovementStart = "sounds/w_turret:turret_functionality:turn_loop",
			soundVerticalMovementStop = "sounds/w_turret:turret_functionality:vertical_stop",
			soundVerticalMovementLoop = "sounds/w_turret:turret_functionality:vertical_loop",
			soundHorizontalMovementStart = "sounds/w_turret:turret_functionality:turn_start",
			soundHorizontalMovementStop = "sounds/w_turret:turret_functionality:turn_stop",
			soundHorizontalMovementLoop = "sounds/w_turret:turret_functionality:turn_loop",
			soundAlerted = "sounds/w_turret:turret_functionality:alerted",
			soundLockedOnTarget = "",

			fMovementStartSoundThresholdDegreesSecond = 4.0,
			fMovementStopSoundThresholdDegreesSecond = 0.2,
			fHorizontalLoopingSoundDelaySeconds = 0.3,
			fVerticalLoopingSoundDelaySeconds = 0.3,
			fAlertedTimeoutSeconds = 60,
			fLockOnTargetSeconds = 0.2,
		},

		Laser =
		{
			bEnabled = 0,
			objGeometry = "objects/weapons/attachments/laser_beam/laser_beam.cgf",
			DotEffect = "",
			SourceEffect = "",
			JointName = "weaponjoint",
			fOffsetX = 0.45,
			fOffsetY = -0.87,
			fOffsetZ = -0.025,
			fThickness = 0.8,
			fRange = 150,
			fShowDot = 1,
		},

		Vision =
		{
			--bHasRadar = 1,
			fRadarRange = 20,
			fRadarFov = 180,
			fRadarVerticalOffset = 2,

			--bHasEye = 1,
			fEyeRange = 40,
			fEyeFov = 25,
			EyeJointName = "weaponjoint",

			fCloakDetectionDistance = 4,
		},

		Mannequin =
		{
			fileControllerDef = "Animations/Mannequin/ADB/turretControllerDefs.xml",
			fileAnimationDatabase = "Animations/Mannequin/ADB/turret.adb",
		},

		Behavior =
		{
			SearchSweep =
			{
				fYawDegreesHalfLimit = 45,
				fYawDegreesPerSecond = 90,
				fDistance = 100,
				fHeight = 1,

				fRandomOffsetSeconds = 1,
				fRandomOffsetRange = 3,
			},

			SearchSweepPause =
			{
				fLimitPauseSeconds = 1,

				fRandomOffsetSeconds = 0.4,
				fRandomOffsetRange = 5,
			},

			FollowTarget =
			{
				fReevaluateTargetSeconds = 0.5,
				fMinStartFireDelaySeconds = 0.1,
				fMaxStartFireDelaySeconds = 0.15,
				fAimVerticalOffset = -0.3,
				fPredictionDelaySeconds = 0.1,
			},

			FireTarget =
			{
				fMinStopFireSeconds = 0.5,
				fMaxStopFireSeconds = 0.75,
			},

			LostTarget =
			{
				fBackToSearchSeconds = 5.0,
				fDampenVelocityTimeSeconds = 10.0,
				fLastVelocityValueWeight = 0.9,
			},

			Undeployed =
			{
				bIsThreat = 0,
			},
		},

		RateOfDeath =
		{
			-- Time in seconds before each update of the target offset. Different values for when we should be missing or hitting the target.
			missOffsetIntervalSeconds = 0.7,
			hitOffsetIntervalSeconds = 0.7,

			-- Multiplier ranges used when calculating the random target offset. Different values for when we should be missing or hitting the target.
			hitMinRange = 0.1,
			hitMaxRange = 0.5,
			missMinRange = 2,
			missMaxRange = 3,

			-- Distance used to calculate (in conjunction with some CVars from the AI system) the zone a target is in (kill zone, near combat, far combat...) .
			attackRange = 100,
		},

		Damage =
		{
			health = 1000,
			fileBodyDamage = "Libs/BodyDamage/BodyDamage_HumanTurret.xml",
			fileBodyDamageParts = "Libs/BodyDamage/BodyParts_HumanTurret.xml",
			fileBodyDestructibility = "Libs/BodyDamage/BodyDestructibility_HumanTurret.xml",
		},

		AutoAimTargetParams =
		{
			primaryTargetBone = "weaponjoint_ref",
			physicsTargetBone = "weaponjoint_ref",
			secondaryTargetBone = "arcjoint",
			fallbackOffset = 1.2,
			innerRadius = 0.5,
			outerRadius = 0.6,
			snapRadius = 1.5,
			snapRadiusTagged = 3.0,
		},
	},

	PropertiesInstance = 
	{
		groupId = -1,
		fCloakDetectionDistanceMultiplier = 1,
	},

	Editor = 
	{
		Icon = "user.bmp",
		IconOnTop = 1,
	},

	Server = {},
	Client = {},
}

MakeUsable( Turret );


function Turret.Client:OnInit()
	self:CacheResources();
end


function Turret:CacheResources()
	self:PreLoadParticleEffect( self.Properties.Laser.DotEffect );
	self:PreLoadParticleEffect( self.Properties.Laser.SourceEffect );
	
	local requesterName = "Turret.lua";
	Game.CacheResource( requesterName, self.Properties.Laser.objGeometry, eGameCacheResourceType_StaticObject, 0 );
end


function Turret:OnPropertyChange()
	self.turret:OnPropertyChange();
end


function Turret.Server:OnHit( hit )
	self.turret:OnHit( hit );
end


function Turret:OnUsed( user, idx )
	self.turret:SetStateById( eTurretBehaviorState_PartiallyDeployed );
	self.turret:SetFactionToPlayerFaction();
	if ( self.Properties.bUsableOnlyOnce == 1 ) then
		self.bUsable = 0;
	end
end


function Turret:IsUsable( user )
	if ( self.bUsable == 1 ) then
		return 1;
	end
	return 0;
end


function Turret:Event_SetStateDeployed()
	self.turret:SetStateById( eTurretBehaviorState_Deployed );
end

function Turret:Event_SetStatePartiallyDeployed()
	self.turret:SetStateById( eTurretBehaviorState_PartiallyDeployed );
end

function Turret:Event_SetStateUndeployed()
	self.turret:SetStateById( eTurretBehaviorState_Undeployed );
end


function Turret:Event_Enable( params )
	self.turret:Enable();
end

function Turret:Event_Disable( params )
	self.turret:Disable();
end


function Turret:OnReset()
	self.bUsable = self.Properties.bUsable;
end

function Turret:OnSave( tbl )
	tbl.bUsable = self.bUsable;
end

function Turret:OnLoad( tbl )
	self.bUsable = tbl.bUsable;
end


Turret.FlowEvents =
{
	Inputs =
	{
		Enable = { Turret.Event_Enable, "bool" },
		Disable = { Turret.Event_Disable, "bool" },

		SetDeployed = { Turret.Event_SetStateDeployed, "bool" },
		SetPartiallyDeployed = { Turret.Event_SetStatePartiallyDeployed, "bool" },
		SetUndeployed = { Turret.Event_SetStateUndeployed, "bool" },
	},
	
	Outputs =
	{
		Destroyed = "bool",
	},
}
