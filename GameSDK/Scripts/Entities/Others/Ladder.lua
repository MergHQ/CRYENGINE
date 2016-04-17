--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2013.
--------------------------------------------------------------------------
--	Description: Climbable Ladder
--  
--------------------------------------------------------------------------
--  History:
--  - 06:01:2012 : Created by Claire Allan
--  - 29:05:2013 : modified for SDK by Marco Hopp
--------------------------------------------------------------------------

Ladder = 
{
	Properties = 
	{
		fileModel = "objects/props/misc/ladders/mp_ladder1.cgf",
			
		bUsable	= 1,
		bTopBlocked	= 0,
		height = 3.7,
		approachAngle = 60,
		approachAngleTop = 0, -- angle of 0 means no limit

		ViewLimits =
		{
			horizontalViewLimit 		= 30,	-- horizontal view limit while climbing ladders
			verticalUpViewLimit 		= 0,	-- vertical view limit for looking up while climbing ladders
			verticalDownViewLimit 		= 0,	-- vertical view limit for looking down while climbing ladders
		},
		Offsets =
		{
			stopClimbDistanceFromTop 	= 1.92,	-- Distance from the top of a ladder at which the player stops climbing up (and triggers the get-off-at-the-top animation)
			stopClimbDistanceFromBottom	= 0.5,	-- Distance from the bottom of a ladder at which the player stops climbing down (and triggers the get-off-at-the-bottom animation)
			playerHorizontalOffset 		= 0.38, -- Horizontal distance from ladder entity position to player entity position
			getOnDistanceAwayTop 		= 1.40,	-- Player starts this far away from ladder climb position when getting on at top
			getOnDistanceAwayBottom		= 0.54,	-- Player starts this far away from ladder climb position when getting on at bottom
		},
		Movement =
		{
			movementAcceleration 		= 5, 	-- How much speed the player can gain in a second
			movementSpeedUpwards 		= 2.5, 	-- Speed (rungs/second) at which the player moves up ladders
			movementSpeedDownwards 		= 2.25, -- Speed (rungs/second) at which the player moves down ladders
			movementSettleSpeed 		= 0.8, 	-- Speed at which player settles on a rung when stopping part-way up/down a ladder
			movementInertiaDecayRate 	= 5.5, 	-- Speed at which player input inertia returns to 0 when player releases the up/down input
		},
		Camera =
		{
			distanceBetweenRungs		= 0.5,	-- Distance in meters between the rungs of a ladder
			cameraAnimFraction_getOn	= 0.85, -- Fraction of animated camera position/rotation to use when getting onto a ladder
			cameraAnimFraction_getOff	= 0.85,	-- Fraction of animated camera position/rotation to use when getting off a ladder
			cameraAnimFraction_onLadder = 0.3,	-- Fraction of animated camera position/rotation to use when staying on a ladder
			bUseThirdPersonCamera		= 1,	-- Whether to switch to 3rd person camera while on ladder
			bRenderLadderLast			= 0;	-- Render Ladder in front of other geometry
		},
  },
  	
	Editor=
	{
		Icon = "ladder.bmp",
		IconOnTop=1,
	},

	Server = {},
}

function Ladder:OnSpawn()	
	self:LoadObject( 0,self.Properties.fileModel );
	self:Physicalize(0,PE_STATIC,{mass = 0, density = 0});

	if (System.IsEditor()) then
		self:Activate(1);
	end
end

function Ladder:OnPropertyChange()
	self:LoadObject( 0,self.Properties.fileModel );
	self:Physicalize(0,PE_STATIC,{mass = 0, density = 0});
end

function Ladder:IsUsable(user)
	return self.Properties.bUsable;
end

function Ladder:GetUsableMessage(idx)
  if(self.Properties.bUsable==1)then
    return "@use_ladder";
  end;
end;

function Ladder.Server:OnUpdate(frameTime)
	if(System.IsEditing()) then
		local pos = self:GetWorldPos();
		Game.DebugDrawCylinder(pos.x, pos.y, pos.z, 0.3, 0.3, 60, 60, 255, 100);
		Game.DebugDrawCylinder(pos.x, pos.y, pos.z + self.Properties.height, 0.3, 0.3, 60, 60, 255, 100);
		Game.DebugDrawCylinder(pos.x, pos.y, pos.z + self.Properties.height - self.Properties.Offsets.stopClimbDistanceFromTop, 0.3, 0.3, 255, 60, 60, 100);
		Game.DebugDrawCylinder(pos.x, pos.y, pos.z + self.Properties.Offsets.stopClimbDistanceFromBottom, 0.3, 0.3, 255, 60, 60, 100);
	end
end

function Ladder:Event_Enable()
	self.Properties.bUsable = 1;
end

function Ladder:Event_Disable()
	self.Properties.bUsable = 0;
end

function Ladder:Event_TopBlocked()
	self.Properties.bTopBlocked = 1;
end

function Ladder:Event_TopUnblocked()
	self.Properties.bTopBlocked = 0;
end

Ladder.FlowEvents =
{
	Inputs =
	{
		EnableUsable = { Ladder.Event_Enable, "any" },
		DisableUsable = { Ladder.Event_Disable, "any" },
		TopBlocked = { Ladder.Event_TopBlocked, "any" },
		TopUnblocked = { Ladder.Event_TopUnblocked, "any" },
	},

	Outputs =
	{
		PlayerOn = "entity",
		PlayerOff = "entity",
	}
}
