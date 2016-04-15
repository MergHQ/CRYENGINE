BTBBombSite = {
    Client = {},
    Server = {},
    Properties = {
	fileModel		= "objects/library/props/ctf/mpnots_flagbase01.cgf",
	ModelSubObject		= "main",
	Radius			= 2;
	teamName		= "";
	Explosion =	{
		Effect				= "explosions.rocket.soil",
		EffectScale			= 0.2,
		Radius				= 2,
		Pressure			= 250,
		Damage				= 500,
		Decal				= "textures/decal/explo_decal.dds",
		DecalScale			= 1,
		Direction			= {x=0, y=0.0, z=-1},
	},
    },
    Editor = {
	Icon		= "Item.bmp",
	IconOnTop	= 1,
    },
}

function BTBBombSite.Server:OnInit()
	Log("BTBBombSite.Server.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
	self.inside = {};
end;

function BTBBombSite.Client:OnInit()
	Log("BTBBombSite.Client.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
	self.inside = {};
end;

function BTBBombSite:OnReset()
    Log("BTBBombSite.OnReset");
    self:LoadObject(0, self.Properties.fileModel);
	
	local radius_2 = self.Properties.Radius / 2;
	
 	local Min={x=-radius_2,y=-radius_2,z=-radius_2};
	local Max={x=radius_2,y=radius_2,z=radius_2};
	self:SetTriggerBBox(Min,Max);
	
	self:Physicalize(0, PE_STATIC, { mass=0 });
	
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 0);

	if (CryAction.IsServer() and (self.Properties.teamName ~= "")) then
		local teamId = g_gameRules.game:GetTeamId(self.Properties.teamName);
		if (teamId > 0) then
			g_gameRules.game:SetTeam(teamId, self.id);
		end
	end
end;

----------------------------------------------------------------------------------------------------
function BTBBombSite.Server:OnEnterArea(entity, areaId)
	Log("BTBBombSite.Server.OnEnterArea entity:%s, this areas team:%s", EntityName(entity.id), self.Properties.teamName);
	local inserted = false;
	for i,id in ipairs(self.inside) do
		if (id==entity.id) then
			inserted=true;	
			Log("Entity already inserted");
			break;
		end
	end
	if (not inserted) then
		table.insert(self.inside, entity.id);
		Log("Entity added");
	end

	if (g_gameRules.Server.EntityEnterBombSiteArea ~= nil) then
		g_gameRules.Server.EntityEnterBombSiteArea(g_gameRules, entity, self);
	end
end

----------------------------------------------------------------------------------------------------
function BTBBombSite.Client:OnEnterArea(entity, areaId)
	Log("BTBBombSite.Client.OnEnterArea entity:%s, this areas team:%s", EntityName(entity.id), self.Properties.teamName);

	if (g_gameRules.Client.EntityEnterBombSiteArea ~= nil) then
		g_gameRules.Client.EntityEnterBombSiteArea(g_gameRules, entity, self);
	end
end

function BTBBombSite.Client:OnLeaveArea(entity, areaId)
	Log("BTBBombSite.Client.OnLeaveArea entity:%s, this areas team:%s", EntityName(entity.id), self.Properties.teamName);

	if (g_gameRules.Client.EntityLeaveBombSiteArea ~= nil) then
		g_gameRules.Client.EntityLeaveBombSiteArea(g_gameRules, entity, self);
	end
end

----------------------------------------------------------------------------------------------------
function BTBBombSite.Server:OnLeaveArea(entity, areaId)
	Log("BTBBombSite.Server.OnLeaveArea entity:%s, this areas team:%s", EntityName(entity.id), self.Properties.teamName);
	for i,id in ipairs(self.inside) do
		if (id==entity.id) then
			table.remove(self.inside, i);
			Log("Entity removed");
			break;
		end
	end

	if (g_gameRules.Server.EntityLeftBombSiteArea ~= nil) then
		g_gameRules.Server.EntityLeftBombSiteArea(g_gameRules, entity, self);
	end
end

function BTBBombSite:EntityInsideArea(entityId)
	for i,id in ipairs(self.inside) do
		if (id==entityId) then
			return true;
		end
	end
	return false;
end

function BTBBombSite:Explode()
	local explosion=self.Properties.Explosion;
	local radius=explosion.Radius;
	g_gameRules:CreateExplosion(self.id,self.id,explosion.Damage,self:GetWorldPos(),explosion.Direction,radius,nil,explosion.Pressure,explosion.HoleSize,explosion.Effect,explosion.EffectScale);
end

