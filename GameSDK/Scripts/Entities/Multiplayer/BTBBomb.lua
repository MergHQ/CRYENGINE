BTBBomb = {
    	Client = {},
    	Server = {},
    	Properties = {
		--objModel 	= "objects/characters/animals/Birds/Chicken/chicken.chr",
		objModel 	= "objects/weapons/us/av_mine/avmine.cgf",
		--objModel 	= "objects/Weapons/US/c4/c4_tp.cgf",
		
		Health = 200;
		MainExplosion =	{
			Effect				= "explosions.rocket.soil",
			EffectScale			= 0.4,
			Radius				= 25,
			Pressure			= 500,
			Damage				= 250,
			Decal				= "textures/decal/explo_decal.dds",
			DecalScale			= 1,
			Direction			= {x=0, y=0.0, z=-1},

			SoundEffect			= "Sounds/physics:explosions:explo_big_01"
		},

		HugeExplosion =	{
			Effect				= "explosions.TAC.mushroom",
			EffectScale			= 0.4,
			Radius				= 500,
			Pressure			= 800,
			Damage				= 800,
			Decal				= "textures/decal/explo_decal.dds",
			DecalScale			= 1,
			Direction			= {x=0, y=0.0, z=-1},

			SoundEffect			= "Sounds/physics:explosions:explo_big_01"
		},
    	},
    	Editor = {
		Icon		= "Item.bmp",
		IconOnTop	= 1,
    	},
	AttachedEntity = nil,
	ModelSlot = 0,
	SmokeSlot = -1,
	
	States = {"Idle","Carried", "Active","Armed","Destroyed"},

	ArmedBeepSound = "Sounds/crysiswars2:interface:multiplayer/mp_match_timer_beep",	-- TODO: Find a valid sound
	ARMED_BEEP_TIME = 1000,
	ARMED_BEEP = 1,
}

function BTBBomb.Server:OnInit()
	Log("BTBBomb.Server.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
end;

function BTBBomb.Client:OnInit()
	Log("BTBBomb.Client.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
	self:CacheResources();
end;

function BTBBomb:CacheResources()
	self:PreLoadParticleEffect( "smoke_and_fire.Jeep.hull_smoke" );
end

function BTBBomb:OnReset()
	Log("BTBBomb.OnReset");
	self:LoadCorrectGeometry();	
	local radius_2 = 1;
	
 	local Min={x=-radius_2,y=-radius_2,z=-radius_2};
	local Max={x=radius_2,y=radius_2,z=radius_2};
	self:SetTriggerBBox(Min,Max);
	
	self:Physicalize(self.ModelSlot, PE_RIGID, { mass=0 });
	
	--self:EnablePhysics(true);

	self:SetScale(3);

	self.health = self.Properties.Health;
	self.inside = {};
	
	if(self.SmokeSlot~=-1)then
		self:FreeSlot(self.SmokeSlot);
		self.SmokeSlot= -1;
	end;
	
	self:DrawSlot(self.ModelSlot, 1);
	self:GotoState("Idle");
end;

function BTBBomb:Reset()
    	Log ("BTBBomb:Reset()");
	self:OnReset();
end
-------------------------------------------------------------------------
function BTBBomb:OnSpawn()
	CryAction.CreateGameObjectForEntity(self.id);
	CryAction.BindGameObjectToNetwork(self.id);
	CryAction.ForceGameObjectUpdate(self.id, true);

	--self:LoadCorrectGeometry();

	--self:Physicalize(0, PE_RIGID, { mass=1 });
	--self:EnablePhysics(true);
	--self:RedirectAnimationToLayer0(0, true);
	self:Activate(1);
	
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function BTBBomb:LoadGeometry(slot, model)
	Log ("BTBBomb:LoadGeometry() model = \"" .. model .. "\"");
	
	if (string.len(model) > 0) then
		local ext = string.lower(string.sub(model, -4));

		if ((ext == ".chr") or (ext == ".cdf") or (ext == ".cga")) then
			self:LoadCharacter(slot, model);
		else
			self:LoadObject(slot, model);
		end
	end
end

-------------------------------------------------------------------------
function BTBBomb:LoadCorrectGeometry()
	local teamId = g_gameRules.game:GetTeam(self.id);
	Log("BTBBomb.LoadCorrectGeometry()");

	self:LoadGeometry(self.ModelSlot, self.Properties.objModel);
end

----------------------------------------------------------------------------------------------------
function BTBBomb.Server:OnHit(hit)
    self:OnHit(hit, true);
end

function BTBBomb.Client:OnHit(hit)
    self:OnHit(hit, false);
end

function BTBBomb:OnHit(hit, server)
	if (self:GetState()~="Active") then
	    Log("BTBBomb:OnHit Can only be shot when active");
	    --return
	end

    	if (hit.shooter==self) then
	    Log("BTBBomb:OnHit Can't damage self!");
	    return
	end

    	local damage= hit.damage;
	self.health=self.health-damage;
	if (self.health < (self.Properties.Health*0.25)) then
		if (self.SmokeSlot == -1) then
	    		self.SmokeSlot = self:LoadParticleEffect(-1, "smoke_and_fire.Jeep.hull_smoke", {});
		end
	end
	if(self.health<=0 and server==true)then
		if (g_gameRules.Server.BombZeroHealth ~= nil) then
			g_gameRules.Server.BombZeroHealth(g_gameRules);
		end
	end;
	Log("HIT BOMB! health :%d", self.health);
end

----------------------------------------------------------------------------------------------------
function BTBBomb:Armed()
	self:GotoState("Armed");
end

function BTBBomb:Carried()
	self:GotoState("Carried");
end

function BTBBomb:Active()
	self:GotoState("Active");
end

function BTBBomb:Destroyed()
	self:GotoState("Destroyed");
end

----------------------------------------------------------------------------------------------------
function BTBBomb:OnPropertyChange()
	self:OnReset();
end

function BTBBomb:OnEnterArea(entity, areaId)
	Log("BTBBomb.OnEnterArea");
	if (entity and entity.actor and entity.actor:IsPlayer()) then
		InsertIntoTable(self.inside, entity.id);

		if (g_gameRules.Server.EntityEnterBombArea ~= nil) then
			g_gameRules.Server.EntityEnterBombArea(g_gameRules, entity, self);
		end
	end
end

function BTBBomb:OnLeaveArea(entity, areaId)
	Log("BTBBomb.OnLeaveArea");
	if (entity and entity.actor and entity.actor:IsPlayer()) then
		RemoveFromTable(self.inside, entity.id);
	end
end
	
function BTBBomb:AttachTo(entity)
	if (self.AttachedEntity ~= nil) then
		self:DetachThis(1, 0);
		self:SetAngles({x=0, y=0, z=0});
		self:EnablePhysics(true); -- check
	end
	self.AttachedEntity = entity;
	if (entity ~= nil) then
		self:EnablePhysics(false);
		self:SetPos(entity:GetPos());
		entity:AttachChild(self.id, 1);
	end
end

function BTBBomb:ExplodeMain(server)
    	self:Explode(self.Properties.MainExplosion, server);
end

function BTBBomb:ExplodeHuge(server)
    	self:Explode(self.Properties.HugeExplosion, server);
end

function BTBBomb:Explode(explosion, server)
	Log("BTBBomb:Explode %s", tostring(server));

	if (server) then
	    local radius=explosion.Radius;
	    g_gameRules:CreateExplosion(self.id,self.id,explosion.Damage,self:GetPos(),explosion.Direction,radius,nil,explosion.Pressure,explosion.HoleSize,explosion.Effect,explosion.EffectScale);	
	end

	Sound.Play(explosion.SoundEffect, self:GetPos(), SOUND_DEFAULT_3D, SOUND_SEMANTIC_MECHANIC_ENTITY);

	self:GotoState("Destroyed");
end

----------------------------------------------------------------------------------------------------
BTBBomb.Server.Idle=
{
	OnBeginState = function(self)
		self:EnablePhysics(false);
	end,
	OnEnterArea = function(self,entity,areaId)
	    	self:OnEnterArea(entity,areaId);
	end,
	OnLeaveArea = function(self,entity,areaId)
	    	self:OnLeaveArea(entity,areaId);
	end,
};

BTBBomb.Client.Idle=
{
	OnBeginState = function(self)
		self:EnablePhysics(false);
	end,
};

----------------------------------------------------------------------------------------------------
BTBBomb.Server.Carried=
{
	OnBeginState = function(self)
		self:EnablePhysics(false);
	end,
	OnEnterArea = function(self,entity,areaId)
	    	self:OnEnterArea(entity,areaId);
	end,
	OnLeaveArea = function(self,entity,areaId)
	    	self:OnLeaveArea(entity,areaId);
	end,
};

BTBBomb.Client.Carried=
{
	OnBeginState = function(self)
		self:EnablePhysics(false);
	end,
};

----------------------------------------------------------------------------------------------------
BTBBomb.Server.Active=
{
	OnBeginState = function(self)
		self:EnablePhysics(true);
		if (self.health<100) then
			if (self.SmokeSlot == -1) then
			    self.SmokeSlot = self:LoadParticleEffect(-1, "smoke_and_fire.Jeep.hull_smoke", {});
			end
		end
	end,
	OnEnterArea = function(self,entity,areaId)
	    	self:OnEnterArea(entity,areaId);
	end,
	OnLeaveArea = function(self,entity,areaId)
	    	self:OnLeaveArea(entity,areaId);
	end,
	OnEndState = function(self)
		if(self.SmokeSlot~=-1)then
			self:FreeSlot(self.SmokeSlot);
			self.SmokeSlot= -1;
		end;
	end,
};

BTBBomb.Client.Active=
{
	OnBeginState = function(self)
		self:EnablePhysics(true);
		if (self.health<100) then
			if (self.SmokeSlot == -1) then
			    self.SmokeSlot = self:LoadParticleEffect(-1, "smoke_and_fire.Jeep.hull_smoke", {}); -- TODO: use an appropriate effect
			end
		end
	end,
	OnEndState = function(self)
		if(self.SmokeSlot~=-1)then
			self:FreeSlot(self.SmokeSlot);
			self.SmokeSlot= -1;
		end;
	end,
};

----------------------------------------------------------------------------------------------------
BTBBomb.Server.Destroyed=
{
	OnBeginState = function(self)
		self:EnablePhysics(false);
		self:DrawSlot(self.ModelSlot, 0);
	end,
};

BTBBomb.Client.Destroyed=
{
	OnBeginState = function(self)
		self:EnablePhysics(false);
		self:DrawSlot(self.ModelSlot, 0);
	end,
};

----------------------------------------------------------------------------------------------------
BTBBomb.Server.Armed=
{
	OnBeginState = function(self)
	    self:EnablePhysics(false);
	end,
	OnTimer = function(self,timerId)
	end,
	OnEndState = function(self)
	end,
};

BTBBomb.Client.Armed=
{
	OnBeginState = function(self)
	    self:SetTimer(self.ARMED_BEEP, self.ARMED_BEEP_TIME);
	    Sound.Play (self.ArmedBeepSound, self:GetPos(), SOUND_DEFAULT_3D, SOUND_SEMANTIC_MECHANIC_ENTITY);
	    self:EnablePhysics(false);
	end,
	OnTimer = function(self,timerId)
	    if(timerId==self.ARMED_BEEP)then
		Sound.Play (self.ArmedBeepSound, self:GetPos(), SOUND_DEFAULT_3D, SOUND_SEMANTIC_MECHANIC_ENTITY);
		self:SetTimer(self.ARMED_BEEP, self.ARMED_BEEP_TIME);
	    end;
	end,
	OnEndState = function(self)
	    self:KillTimer(self.ARMED_BEEP);
	end,
};
