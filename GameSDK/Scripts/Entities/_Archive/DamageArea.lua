DamageArea = {
	type = "DamageArea",
	Properties = {	
		damageRate = 5,
		bEnabled = 1,
		},
	Editor={
		Model="Objects/Editor/T.cgf",
	},

	curDamageRate = 0,	
	curDamage = 0,
	isEnabled = 1,
	
	hit = {
		dir = {x=0, y=0, z=1},
		damage = 0,
		target = nil,
		shooter = nil,
		landed = 1,
		impact_force_mul_final=5,
		impact_force_mul=5,
		damage_type = "normal",
		};
	
	
}




function DamageArea:CliSrv_OnInit()

	self:RegisterState("Inactive");
	self:RegisterState("Active");
	self:GotoState("Inactive");

end

-----------------------------------------------------------------------------
function DamageArea:OnReset( player,areaId,fadeCoeff )

	self:GotoState("Inactive");
	self.isEnabled = self.Properties.bEnabled;
	
end


-----------------------------------------------------------------------------
--	fade: 0-out 1-in
function DamageArea:OnProceedFadeAreaClient( player,areaId,fadeCoeff )

	self.curDamageRate = self.Properties.damageRate*fadeCoeff;

--System.LogToConsole("--> bfly FadeIS "..fadeCoeff );
--	System.SetViewDistance(1200);
end

-----------------------------------------------------------------------------
function DamageArea:OnEnterAreaClient( player,areaId )

System.LogToConsole("--> Entering DamageArea Area "..areaId);
	self:GotoState("Active");
end

-----------------------------------------------------------------------------
function DamageArea:OnLeaveAreaClient( player,areaId )

System.LogToConsole("--> Leaving DamageArea Area "..areaId);

--	if(player ~= _localplayer) then
--		return
--	end	
	
	self:GotoState("Inactive");

end

-----------------------------------------------------------------------------
function DamageArea:OnUpdateActiveClient( dt )

--System.Log("\001 dRate ".._localplayer.cnt.health.." "..self.curDamage);

	if(self.isEnabled == 0) then return end

	self.curDamage = self.curDamage + self.curDamageRate*dt;
	
	if( self.curDamage > 1 ) then
--		_localplayer.cnt.health	= _localplayer.cnt.health - 1;
--		_localplayer.cnt.health	= _localplayer.cnt.health - self.curDamage;
--		self.curDamage = 0;
--		if(_localplayer.cnt.health < 0) then
--			_localplayer.cnt.health = 0;
--		end	
		self.hit.damage = self.curDamage;
		self.hit.target = _localplayer;
		self.hit.shooter = _localplayer;
		self.hit.dir = {x=0, y=0, z=1};
		self.hit.landed = 1;
		self.hit.impact_force_mul_final=0;
		self.hit.impact_force_mul=0;
		self.hit.damage_type = "normal";

		_localplayer:Damage( self.hit );
		self.curDamage = 0;
	end	

end


-----------------------------------------------------------------------------
function DamageArea:OnShutDown()
end


DamageArea.Server={
	OnInit=function(self)
		self:CliSrv_OnInit()
	end,
	OnShutDown=function(self)
	end,
	Inactive={
	},
	Active={
		OnBeginState=function(self)
		end,
--		OnUpdate = FlyingFox.OnUpdateActiveServer,
	},
}


DamageArea.Client={
	OnInit=function(self)
		self:CliSrv_OnInit()
	end,
	OnShutDown=function(self)
	end,
	Inactive={
	
		OnEnterArea = DamageArea.OnEnterAreaClient,
	
	},
	Active={
		OnBeginState=function(self)

			self.curDamage = 0,
			self:Activate(1);
		
--System.Log("Entering INACTIVE");		
		end,
		OnLeaveArea = DamageArea.OnLeaveAreaClient,
		OnProceedFadeArea = DamageArea.OnProceedFadeAreaClient,		
--		OnContact = FlyingFox.OnContactClient,
		OnUpdate = DamageArea.OnUpdateActiveClient,
	},
}


----------------------------------------------------------------------------------------------------------------------------
--
function DamageArea:Event_Enable( params )

	self.isEnabled = 1;

end

----------------------------------------------------------------------------------------------------------------------------
--
function DamageArea:Event_Disable( params )

	self.isEnabled = 0;

end

DamageArea.FlowEvents =
{
	Inputs =
	{
		Disable = { DamageArea.Event_Disable, "bool" },
		Enable = { DamageArea.Event_Enable, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
	},
}
