FrogMine = {
	type = "Mine",
	MapVisMask = 0,
	
	Properties = {
		Radius = 4,
		bEnabled = 1,
		Delay = 0.2,
		bOnlyPlayer = 1,
	},
	
	-- Physical parameters for jumping.
	JumpParams = {
		mass = 10,
		size = 0.1,
		heading = {x=0,y=0,z=1},
		initial_velocity = 18,
		k_air_resistance = 0,
		acc_thrust = 0,
		acc_lift = 0,
		surface_idx = 0,
		gravity = {x=0, y=0, z=-2*9.81 },
	},
	
	ExplosionParams = {
		pos = {},
		damage = 100,
		rmin = 2.0,
		rmax = 20.5,
		radius = 20,
		impulsive_pressure = 200,
		shooter = nil,
		weapon = nil,
	},

	
	Smoke = {
		focus = 1,
		color = {1,1,1},
		speed = 1,
		count = 2,
		size = 0.2, size_speed=1.0,
		gravity= -9.8,
		lifetime=1,
		frames=1,
		blend_type=0,
		color_based_blending = 0,
		tid = System.LoadTexture("textures/WeaponMuzzleFlash/smoke.dds"),
	},

	
	Editor={
		--Model="Objects/Editor/M.cgf",
	},
		
	-- Who triggered me.
	Who=nil,
	Enabled=1,
}

function FrogMine:OnPropertyChange()
	self:OnReset();
end

function FrogMine:OnInit()
	self:Activate(0);
	self:TrackColliders(1);
	self:LoadObject( "Objects/Pickups/Health/Medikit.cgf", 0,1 );
	--self.CreateParticlePhys( 10,0.1 );
	self:OnReset();
end

function FrogMine:OnShutDown()
end

function FrogMine:OnReset()
	local Min = { x=-self.Properties.Radius/2, y=-self.Properties.Radius/2, z=-self.Properties.Radius/2 };
	local Max = { x=self.Properties.Radius/2, y=self.Properties.Radius/2, z=self.Properties.Radius/2 };
	self:SetBBox( Min,Max );
	
	--System.LogToConsole( "BBox:"..Min.x..","..Min.y..","..Min.z.."  "..Max.x..","..Max.y..","..Max.z );
	self:DrawObject( 0,1 );
	self.Enabled = self.Properties.bEnabled;
	self.Who = nil;
	self.Died = 0;
	self:GotoState( "Waiting" );
end

function FrogMine:Event_Enable( sender )
	self.Enabled = 1;
	BroadcastEvent( self,"Enable" );
end

function FrogMine:Event_Disable( sender )
	self.Enabled = 0;
	BroadcastEvent( self,"Disable" );
end

function FrogMine:Event_Trigger( sender )
	System.LogToConsole( "Trigered" );
	if (self.Enabled ~= 0 and self.Died == 0) then
		self:GotoState( "Triggered" );
	end
	BroadcastEvent( self,"Trigger" );
end

function FrogMine:IsValid( player )
	-- Ignore if not player.
	if (self.Properties.bOnlyPlayer ~= 0 and player.type ~= "Player") 	then
		return 0;
	end
	return 1;
end

-------------------------------------------------------------------------------
-- Dead State ----------------------------------------------------------------
-------------------------------------------------------------------------------
FrogMine.Dead = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		-- Hide objects.
		self.Died = 1;
		self:DrawObject( 0,0 );
	end,
}

-------------------------------------------------------------------------------
-- Triggered State ----------------------------------------------------------------
-------------------------------------------------------------------------------
FrogMine.Waiting = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.Who = nil;
	end,

	OnEnterArea = function( self,player )
		-- Ignore if disabled.
		if (self.Enabled ~= 1) then
			return
		end
		if (self:IsValid(player) == 0) then
			return
		end
			
		self.Who = player;
		self:Event_Trigger( self );
	end,
}

-------------------------------------------------------------------------------
-- Triggered State ----------------------------------------------------------------
-------------------------------------------------------------------------------
FrogMine.Triggered = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		--System.LogToConsole( "--StartTimer" );
		self.part_time = 10;
		self.speed = 20;
		self.curr_time = 0;
		self.startZ = self:GetPos().z;
		if (self.Properties.Delay > 0) then
			self:SetTimer( self.Properties.Delay*1000 );
		else
			self:SetTimer( 1 );
		end
		-- Jump mine into air.
		--self.SetPhysicParams( 1, self.JumpParams );
	end,
	-------------------------------------------------------------------------------
	OnEndState = function( self )
		local pos = self:GetPos();
		pos.z = self.startZ;
		self:SetPos(pos);
	end,

	OnTimer = function( self )
		--System.LogToConsole( "*** BOOM ***" );
		--self:Event_Enter(self.Who);
		--ExecuteMaterial( self.GetPos(),{x=0,y=0,z=1},Materials.mat_rock, 1 );
		self:Explode();
		--
		self:GotoState( "Dead" );
		self:GotoState( "Waiting" );
	end,
	
	OnUpdate = function( self,dt )
		self.curr_time = self.curr_time + dt;
		local t = self.curr_time;
		local pos = self:GetPos();
		pos.z = self.startZ + self.speed*t - 2*9.81*(t*t);
		self:SetPos(pos);
		
		self.part_time = self.part_time + dt*2;
		if (self.part_time > 0.06) then
			Particle.CreateParticle( self:GetPos(),{x=0,y=0,z=0},self.Smoke );
			self.part_time=0;
		end
	end
}

function FrogMine:Explode()
	local normal = { x=0,y=0,z=0.5 };
	local pos = self:GetPos();
	
	ExecuteMaterial(pos,normal,Materials.mat_default.projectile_hit,1);
	
	self.ExplosionParams.pos = pos;
	Game:CreateExplosion( self.ExplosionParams );
	System.DeformTerrain( pos, 8, Grenade.decal_tid );
	
	self:AddDynamicLight( pos, 20, 1,1,0.3,1, 1,1,0.3,1, 1 );
end


FrogMine.FlowEvents =
{
	Inputs =
	{
		Disable = { FrogMine.Event_Disable, "bool" },
		Enable = { FrogMine.Event_Enable, "bool" },
		Trigger = { FrogMine.Event_Trigger, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Trigger = "bool",
	},
}
