AreaMine = {
	type = "Mine",
	MapVisMask = 0,
	
	Properties = {
		bEnabled = 1,
	},



	Editor={
		Model="Objects/Editor/T.cgf",
	},
		
	
	ExplosionParams = {
		pos = nil,
		damage = 1000,
		rmin = 2.0,
		rmax = 20.5,
		radius = 3,
		impulsive_pressure = 200,
		shooter = nil,
		weapon = nil,
	},

	mine_triggered=nil,
	Enabled=1,
}

function AreaMine:OnPropertyChange()
	self:OnReset();
end

function AreaMine:OnInit()
	self:Activate(0);
	self:OnReset();
end

function AreaMine:OnShutDown()
end

function AreaMine:OnReset()
	self.Enabled = self.Properties.bEnabled;
	
	self:RegisterState("Waiting");
	self:RegisterState("Dead");		
	
	self:GotoState( "Waiting" );
end

function AreaMine:Event_Explode( sender )
	if (self.Enabled)then
		self:Explode();
		self:GotoState( "Dead" );
	end
	BroadcastEvent( self,"Trigger" );
end


------------------------------------------------------------------------------
-- DEAD ----------------------------------------------------------------
------------------------------------------------------------------------------
AreaMine.Dead = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )

	end,
}

-------------------------------------------------------------------------------
-- WAITING --------------------------------------------------------------
-------------------------------------------------------------------------------
AreaMine.Waiting = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )

	end,

	OnEnterArea = function( self, player,areaId )
--System.Log(" entering Area PROX 1111 ");	
		self:SetPos( player:GetPos() );
		self:Explode( self );
	end,
}

function AreaMine:Explode()
	local normal = { x=0,y=0,z=-0.1 };
	local pos = self:GetPos();
	
	--<<FIXME>> think how to retrieve the correct material
	-- maybe is not really needed
	ExecuteMaterial( pos,normal,Materials.mat_default.projectile_hit, 1 );

	self.ExplosionParams.pos = pos;
	self.ExplosionParams.shooter=self;
	self:GotoState("Dead");
	Server:RemoveEntity(self.id);
	Game:CreateExplosion(self.ExplosionParams);
	--System.DeformTerrain( pos, 8, Grenade.decal_tid );
	--System.AddDynamicLight( pos, 20, 1,1,0.3,1, 1,1,0.3,1, 1 );
end


-----------------------------------------------------------------------------

AreaMine.FlowEvents =
{
	Inputs =
	{
		Explode = { AreaMine.Event_Explode, "bool" },
	},
	Outputs =
	{
		Explode = "bool",
	},
}
