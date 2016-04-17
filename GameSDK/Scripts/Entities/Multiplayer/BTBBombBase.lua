BTBBombBase = {
    Client = {},
    Server = {},
    Properties = {
	fileModel		= "objects/library/props/ctf/mpnots_flagbase01.cgf",
	ModelSubObject		= "main",
	Radius			= 2;
    },
    Editor = {
	Icon		= "Item.bmp",
	IconOnTop	= 1,
    },
}

function BTBBombBase.Server:OnInit()
	Log("BTBBombBase.Server.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
	self.inside = {};
end;

function BTBBombBase.Client:OnInit()
	Log("BTBBombBase.Client.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
	self.inside = {};
end;

function BTBBombBase:OnReset()
    Log("BTBBombBase.OnReset");
    self:LoadObject(0, self.Properties.fileModel);
	
	local radius_2 = self.Properties.Radius / 2;
	
 	local Min={x=-radius_2,y=-radius_2,z=-radius_2};
	local Max={x=radius_2,y=radius_2,z=radius_2};
	self:SetTriggerBBox(Min,Max);
	
	self:Physicalize(0, PE_STATIC, { mass=0 });
	
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 0);
end;

----------------------------------------------------------------------------------------------------

function BTBBombBase.Server:OnHit()
	Log("HIT BASE!");
end
