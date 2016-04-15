ProximityMine = {
	type = "Mine",
	MapVisMask = 0,
	
	Properties = {
		Radius = 4,
		bEnabled = 1,
		Delay = 2,
		bOnlyPlayer = 1,
		fileActivationSound="SOUNDS/items/nvisionactivate.wav",
		fileModel="Objects/Pickups/Health/Medikit.cgf",
		nExplosionDamage=300,
	},
	
	ExplosionParams = {
		pos = nil,
		damage = 100,
		rmin = 2.0,
		rmax = 20.5,
		radius = 3,
		impulsive_pressure = 200,
		shooter = nil,
		weapon = nil,
	},

	mine_triggered=nil,
	activation_sound=nil,
	-- Who triggered me.
	Who=nil,
	Enabled=1,
}

function ProximityMine:OnPropertyChange()
	self:OnReset();
end

function ProximityMine:OnInit()
	self:Activate(0);
	self:TrackColliders(1);
	self:LoadObject(self.Properties.fileModel, 0,1 );
	self:CreateStaticEntity(10,100);
	self:OnReset();
end

function ProximityMine:OnShutDown()
end

function ProximityMine:OnReset()
	local Min = { x=-self.Properties.Radius/2, y=-self.Properties.Radius/2, z=-self.Properties.Radius/2 };
	local Max = { x=self.Properties.Radius/2, y=self.Properties.Radius/2, z=self.Properties.Radius/2 };
	self:SetBBox( Min,Max );
	self:DrawObject( 0,1 );
	self.Enabled = self.Properties.bEnabled;
	self.activation_sound=Sound.Load3DSound(self.Properties.fileActivationSound)
	self.Who = nil;
	self.Died = 0;
	self:GotoState( "Waiting" );
end

function ProximityMine:Event_Enable( sender )
	self.Enabled = 1;
	BroadcastEvent( self,"Enable" );
end

function ProximityMine:Event_Disable( sender )
	self.Enabled = 0;
	BroadcastEvent( self,"Disable" );
end

function ProximityMine:Event_Explode( sender )
	if (self.Enabled)then
		self:Explode();
		self:GotoState( "Dead" );
	end
	BroadcastEvent( self,"Trigger" );
end

function ProximityMine:IsValid( player )
	if (self.Properties.bOnlyPlayer ~= 0 and player.type ~= "Player") 	then
		return 0;
	end
	return 1;
end

------------------------------------------------------------------------------
-- DEAD ----------------------------------------------------------------
------------------------------------------------------------------------------
ProximityMine.Dead = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		-- Hide objects.
		self.Died = 1;
		self:DrawObject( 0,0 );
	end,
}

-------------------------------------------------------------------------------
-- WAITING --------------------------------------------------------------
-------------------------------------------------------------------------------
ProximityMine.Waiting = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.Who = nil;
		self.sound_played=nil;
	end,

	OnEnterArea = function( self, player,areaId )
--System.Log(" entering Area PROX 1111 ");	
		self:SetPos( player:GetPos() );
		self:Explode( self );
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
		if(self.mine_triggered==nil)then
			if(self.activation_sound==nil)then
				System.LogToConsole("Activation sound is nil");
			end
			Sound.SetSoundPosition(self.activation_sound,self:GetPos());
			Sound.PlaySound(self.activation_sound);
			self.mine_triggered=1;
			self:SetTimer( self.Properties.Delay*1000+1 );
		end
	end,
	OnTimer = function(self)
		self:Event_Explode(self);
	end,
	OnDamage = function(self,hit)
		self:Event_Explode(hit.shooter);
	end,
}

function ProximityMine:Explode()
	local normal = { x=0,y=0,z=-0.1 };
	local pos = self:GetPos();
	
	--<<FIXME>> think how to retrieve the correct material
	-- maybe is not really needed
	ExecuteMaterial( pos,normal,Materials.mat_default.projectile_hit, 1 );
	
	self.ExplosionParams.pos = pos;
	self.ExplosionParams.shooter=self;
	self:GotoState("Dead");
	--Server:RemoveEntity(self.id);
	self.ExplosionParams.damage=self.Properties.nExplosionDamage;
	Game:CreateExplosion(self.ExplosionParams);
	--System.DeformTerrain( pos, 8, Grenade.decal_tid );
	--System.AddDynamicLight( pos, 20, 1,1,0.3,1, 1,1,0.3,1, 1 );
end


-----------------------------------------------------------------------------

ProximityMine.FlowEvents =
{
	Inputs =
	{
		Disable = { ProximityMine.Event_Disable, "bool" },
		Enable = { ProximityMine.Event_Enable, "bool" },
		Explode = { ProximityMine.Event_Explode, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Explode = "bool",
	},
}
